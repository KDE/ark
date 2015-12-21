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
#include "kerfuffle/kerfuffle_export.h"

#include <QJsonArray>
#include <QJsonParseError>

#include <KPluginFactory>

using namespace Kerfuffle;

K_PLUGIN_FACTORY(CliPluginFactory, registerPlugin<CliPlugin>();)

CliPlugin::CliPlugin(QObject *parent, const QVariantList &args)
        : CliInterface(parent, args)
        , m_indentLevel(0)

{
    qCDebug(ARK) << "Loaded cli_unarchiver plugin";
}

CliPlugin::~CliPlugin()
{
}

void CliPlugin::resetParsing()
{
    // TODO
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;
    if (p.isEmpty()) {
        /* Limitations:
            *  01 - creates an empty file upon entering wrong password
            *  02 - unar detects if output is being redirected and then does not accept any input
            */

        ///////////////[ COMMON ]/////////////

        p[CaptureProgress] = false;
        p[PasswordPromptPattern] = QLatin1String("Password (will not be shown): ");

        ///////////////[ LIST ]/////////////

        p[ListProgram] = QLatin1String("lsar");
        p[ListArgs] = QStringList() << QLatin1String("-json") << QLatin1String("$Archive");

        ///////////////[ EXTRACT ]/////////////

        p[ExtractProgram] = QLatin1String("unar");
        p[ExtractArgs] = QStringList() << QLatin1String("$Archive") << QLatin1String("$Files") << QLatin1String("$PasswordSwitch") << QLatin1String("$RootNodeSwitch");
        p[NoTrailingSlashes]  = true;
        p[PasswordSwitch] = QStringList() << QLatin1String("-password $Password");
        p[RootNodeSwitch] = QStringList() << QLatin1String("-output-directory") << QLatin1String("$Path");
        p[FileExistsExpression] = QLatin1String("^\\\"(.+)\\\" already exists.");
        p[FileExistsInput] = QStringList()
                    << QLatin1String("o") //overwrite
                    << QLatin1String("s") //skip
                    << QLatin1String("O") //overwrite all
                    << QLatin1String("S") //autoskip
                    << QLatin1String("q") //cancel
                    ;

        ///////////////[ DELETE ]/////////////

        p[DeleteProgram] = QLatin1String("x-fakeprogram");
        //p[DeleteArgs]    =

        ///////////////[ ADD ]/////////////

        p[AddProgram] = QLatin1String("x-fakeprogram");
        //p[AddArgs]    =

        ///////////////[ ERRORS ]/////////////

        p[ExtractionFailedPatterns] = QStringList()
            << QLatin1String("Failed! \\((.+)\\)$")
            << QLatin1String("Segmentation fault$");

        p[WrongPasswordPatterns] = QStringList()
            << QLatin1String("Failed! \\((.+)\\)$");
    }
    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    Q_UNUSED(line)

    return true;
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

void CliPlugin::handleLine(const QString& line)
{
    // Collect the json output line by line.
    if (m_operationMode == List) {
        m_jsonOutput += line + QLatin1Char('\n');
    }

    CliInterface::handleLine(line);
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
