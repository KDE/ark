/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2011 Luke Shumaker <lukeshu@sbcglobal.net>
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "cliplugin.h"
#include "ark_debug.h"
#include "kerfuffle_export.h"
#include "queries.h"

#include <QJsonArray>
#include <QJsonParseError>

#include <KLocalizedString>
#include <KPluginFactory>
#include <KPtyProcess>

using namespace Kerfuffle;

K_PLUGIN_FACTORY_WITH_JSON(CliPluginFactory, "kerfuffle_cliunarchiver.json", registerPlugin<CliPlugin>();)

CliPlugin::CliPlugin(QObject *parent, const QVariantList &args)
        : CliInterface(parent, args)
{
    qCDebug(ARK) << "Loaded cli_unarchiver plugin";
}

CliPlugin::~CliPlugin()
{
}

bool CliPlugin::list()
{
    resetParsing();
    cacheParameterList();
    m_operationMode = List;

    const auto args = substituteListVariables(m_param.value(ListArgs).toStringList(), password());
    return runProcess(m_param.value(ListProgram).toStringList(), args);
}

bool CliPlugin::copyFiles(const QList<QVariant> &files, const QString &destinationDirectory, const ExtractionOptions &options)
{
    ExtractionOptions newOptions = options;

    // unar has the following limitations:
    // 1. creates an empty file upon entering a wrong password.
    // 2. detects that the stdout has been redirected and blocks the stdin.
    //    This prevents Ark from executing unar's overwrite queries.
    // To prevent both, we always extract to a temporary directory
    // and then we move the files to the intended destination.

    qCDebug(ARK) << "Enabling extraction to temporary directory.";
    newOptions[QStringLiteral("AlwaysUseTmpDir")] = true;

    return CliInterface::copyFiles(files, destinationDirectory, newOptions);
}

void CliPlugin::resetParsing()
{
    m_jsonOutput.clear();
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;
    if (p.isEmpty()) {

        ///////////////[ COMMON ]/////////////

        p[CaptureProgress] = false;
        // Displayed when running lsar -json with header-encrypted archives.
        p[PasswordPromptPattern] = QStringLiteral("This archive requires a password to unpack. Use the -p option to provide one.");

        ///////////////[ LIST ]/////////////

        p[ListProgram] = QStringLiteral("lsar");
        p[ListArgs] = QStringList() << QStringLiteral("-json") << QStringLiteral("$Archive") << QStringLiteral("$PasswordSwitch");

        ///////////////[ EXTRACT ]/////////////

        p[ExtractProgram] = QStringLiteral("unar");
        p[ExtractArgs] = QStringList() << QStringLiteral("-D") << QStringLiteral("$Archive") << QStringLiteral("$Files") << QStringLiteral("$PasswordSwitch");
        p[NoTrailingSlashes]  = true;
        p[PasswordSwitch] = QStringList() << QStringLiteral("-password") << QStringLiteral("$Password");

        ///////////////[ ERRORS ]/////////////

        p[ExtractionFailedPatterns] = QStringList()
            << QStringLiteral("Failed! \\((.+)\\)$")
            << QStringLiteral("Segmentation fault$");
    }
    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    Q_UNUSED(line)

    return true;
}

void CliPlugin::setJsonOutput(const QString &jsonOutput)
{
    m_jsonOutput = jsonOutput;
    readJsonOutput();
}

void CliPlugin::readStdout(bool handleAll)
{
    if (!handleAll) {
        CliInterface::readStdout(false);
        return;
    }

    // We are ready to read the json output.
    readJsonOutput();
}

void CliPlugin::cacheParameterList()
{
    m_param = parameterList();
    Q_ASSERT(m_param.contains(ExtractProgram));
    Q_ASSERT(m_param.contains(ListProgram));
}

void CliPlugin::handleLine(const QString& line)
{
    // Collect the json output line by line.
    if (m_operationMode == List) {
        m_jsonOutput += line + QLatin1Char('\n');
    }

    // TODO: is this check really needed?
    if (m_operationMode == Copy) {
        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            qCWarning(ARK) << "Error in extraction:" << line;
            emit error(i18n("Extraction failed because of an unexpected error."));
            killProcess();
            return;
        }
    }

    if (m_operationMode == List) {
        // This can only be an header-encrypted archive.
        if (checkForPasswordPromptMessage(line)) {
            qCDebug(ARK) << "Detected header-encrypted RAR archive";

            Kerfuffle::PasswordNeededQuery query(filename());
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                emit cancelled();
                // Process is gone, so we emit finished() manually.
                emit finished(false);
            } else {
                setPassword(query.password());
                CliPlugin::list();
            }
        }
    }
}

void CliPlugin::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(ARK) << "Process finished, exitcode:" << exitCode << "exitstatus:" << exitStatus;

    if (m_process) {
        //handle all the remaining data in the process
        readStdout(true);

        delete m_process;
        m_process = Q_NULLPTR;
    }

    // #193908 - #222392
    // Don't emit finished() if the job was killed quietly.
    if (m_abortingOperation) {
        return;
    }

    if (!password().isEmpty()) {

        // lsar -json exits with error code 1 if the archive is header-encrypted and the password is wrong.
        if (exitCode == 1) {
            qCWarning(ARK) << "Wrong password, list() aborted";
            emit error(i18n("Wrong password."));
            emit finished(false);
            setPassword(QString());
            return;
        }
    }

    // lsar -json exits with error code 2 if the archive is header-encrypted and no password is given as argument.
    // At this point we are asking a password to the user and we are going to list() again after we get one.
    // This means that we cannot emit finished here.
    if (exitCode == 2) {
        return;
    }

    emit finished(true);
}

void CliPlugin::readJsonOutput()
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(m_jsonOutput.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        qCDebug(ARK) << "Could not parse json output:" << error.errorString();
        return;
    }

    const QJsonObject json = jsonDoc.object();

    const QJsonObject properties = json.value(QStringLiteral("lsarProperties")).toObject();
    const QJsonArray volumes = properties.value(QStringLiteral("XADVolumes")).toArray();
    if (volumes.count() > 1) {
        qCDebug(ARK) << "Detected multivolume archive";
        m_numberOfVolumes = volumes.count();
        setMultiVolume(true);
    }

    const QJsonArray entries = json.value(QStringLiteral("lsarContents")).toArray();

    foreach (const QJsonValue& value, entries) {
        const QJsonObject currentEntry = value.toObject();

        m_currentEntry.clear();

        QString filename = currentEntry.value(QStringLiteral("XADFileName")).toString();

        m_currentEntry[IsDirectory] = !currentEntry.value(QStringLiteral("XADIsDirectory")).isUndefined();
        if (m_currentEntry[IsDirectory].toBool()) {
            filename += QLatin1Char('/');
        }

        m_currentEntry[FileName] = filename;
        m_currentEntry[InternalID] = filename;

        // FIXME: archives created from OSX (i.e. with the __MACOSX folder) list each entry twice, the 2nd time with size 0
        m_currentEntry[Size] = currentEntry.value(QStringLiteral("XADFileSize"));
        m_currentEntry[CompressedSize] = currentEntry.value(QStringLiteral("XADCompressedSize"));
        m_currentEntry[Timestamp] = currentEntry.value(QStringLiteral("XADLastModificationDate")).toVariant();
        m_currentEntry[Size] = currentEntry.value(QStringLiteral("XADFileSize"));
        m_currentEntry[IsPasswordProtected] = (currentEntry.value(QStringLiteral("XADIsEncrypted")).toInt() == 1);
        // TODO: missing fields

        emit entry(m_currentEntry);
    }
}

#include "cliplugin.moc"
