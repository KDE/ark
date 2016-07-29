/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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
 */

#include "cliplugin.h"
#include "ark_debug.h"
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/kerfuffle_export.h"
#include "kerfuffle/archiveentry.h"

#include <KPluginFactory>

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>

using namespace Kerfuffle;

K_PLUGIN_FACTORY_WITH_JSON(CliPluginFactory, "kerfuffle_clizip.json", registerPlugin<CliPlugin>();)

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
    : CliInterface(parent, args)
    , m_parseState(ParseStateHeader)
    , m_linesComment(0)
{
    qCDebug(ARK) << "Loaded cli_zip plugin";
}

CliPlugin::~CliPlugin()
{
}

void CliPlugin::resetParsing()
{
    m_parseState = ParseStateHeader;
    m_tempComment.clear();
    m_comment.clear();
}

// #208091: infozip applies special meanings to some characters, so we
//          need to escape them with backslashes.see match.c in
//          infozip's source code
QString CliPlugin::escapeFileName(const QString &fileName) const
{
    const QString escapedCharacters(QStringLiteral("[]*?^-\\!"));

    QString quoted;
    const int len = fileName.length();
    const QLatin1Char backslash('\\');
    quoted.reserve(len * 2);

    for (int i = 0; i < len; ++i) {
        if (escapedCharacters.contains(fileName.at(i))) {
            quoted.append(backslash);
        }

        quoted.append(fileName.at(i));
    }

    return quoted;
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[CaptureProgress] = false;
        p[ListProgram] = QStringList() << QStringLiteral("zipinfo");
        p[ExtractProgram] = p[TestProgram] = QStringList() << QStringLiteral("unzip");
        p[DeleteProgram] = p[AddProgram] = QStringList() << QStringLiteral("zip");

        p[ListArgs] = QStringList() << QStringLiteral("-l")
                                    << QStringLiteral("-T")
                                    << QStringLiteral("-z")
                                    << QStringLiteral("$Archive");
        p[ExtractArgs] = QStringList() << QStringLiteral("$PreservePathSwitch")
                                       << QStringLiteral("$PasswordSwitch")
                                       << QStringLiteral("$Archive")
                                       << QStringLiteral("$Files");
        p[PreservePathSwitch] = QStringList() << QStringLiteral("")
                                              << QStringLiteral("-j");
        p[PasswordSwitch] = QStringList() << QStringLiteral("-P$Password");
        p[CompressionLevelSwitch] = QStringLiteral("-$CompressionLevel");
        p[DeleteArgs] = QStringList() << QStringLiteral("-d")
                                      << QStringLiteral("$Archive")
                                      << QStringLiteral("$Files");

        p[FileExistsExpression] = QStringList()
            << QStringLiteral("^replace (.+)\\? \\[y\\]es, \\[n\\]o, \\[A\\]ll, \\[N\\]one, \\[r\\]ename: $");
        p[FileExistsFileName] = QStringList() << p[FileExistsExpression].toString();
        p[FileExistsInput] = QStringList() << QStringLiteral("y")  //overwrite
                                           << QStringLiteral("n")  //skip
                                           << QStringLiteral("A")  //overwrite all
                                           << QStringLiteral("N"); //autoskip

        p[AddArgs] = QStringList() << QStringLiteral("-r")
                                   << QStringLiteral("$Archive")
                                   << QStringLiteral("$PasswordSwitch")
                                   << QStringLiteral("$CompressionLevelSwitch")
                                   << QStringLiteral("$Files");

        p[PasswordPromptPattern] = QStringLiteral(" password: ");
        p[WrongPasswordPatterns] = QStringList() << QStringLiteral("incorrect password");
        //p[ExtractionFailedPatterns] = QStringList() << "CRC failed";
        p[CorruptArchivePatterns] = QStringList() << QStringLiteral("End-of-central-directory signature not found");
        p[DiskFullPatterns] = QStringList() << QStringLiteral("write error \\(disk full\\?\\)")
                                            << QStringLiteral("No space left on device");
        p[TestArgs] = QStringList() << QStringLiteral("-t")
                                    << QStringLiteral("$Archive");
        p[TestPassedPattern] = QStringLiteral("^No errors detected in compressed data of ");
    }
    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    static const QRegularExpression entryPattern(QStringLiteral(
        "^(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\d{8}).(\\d{6})\\s+(.+)$") );

    // RegExp to identify the line preceding comments.
    const QRegularExpression commentPattern(QStringLiteral("^Archive:  .*$"));
    // RegExp to identify the line following comments.
    const QRegularExpression commentEndPattern(QStringLiteral("^Zip file size: .*$"));

    switch (m_parseState) {
    case ParseStateHeader:
        if (commentPattern.match(line).hasMatch()) {
            m_parseState = ParseStateComment;
        } else if (commentEndPattern.match(line).hasMatch()){
            m_parseState = ParseStateEntry;
        }
        break;
    case ParseStateComment:
        if (commentEndPattern.match(line).hasMatch()) {
            m_parseState = ParseStateEntry;
            if (!m_tempComment.trimmed().isEmpty()) {
                m_comment = m_tempComment.trimmed();
                m_linesComment = m_comment.count(QLatin1Char('\n')) + 1;
                qCDebug(ARK) << "Found a comment with" << m_linesComment << "lines";
            }
        } else {
            m_tempComment.append(line + QLatin1Char('\n'));
        }
    case ParseStateEntry:
        QRegularExpressionMatch rxMatch = entryPattern.match(line);
        if (rxMatch.hasMatch()) {
            Archive::Entry *e = new Archive::Entry();
            e->setProperty("permissions", rxMatch.captured(1));

            // #280354: infozip may not show the right attributes for a given directory, so an entry
            //          ending with '/' is actually more reliable than 'd' bein in the attributes.
            e->setProperty("isDirectory", rxMatch.captured(10).endsWith(QLatin1Char('/')));

            e->setProperty("size", rxMatch.captured(4));
            QString status = rxMatch.captured(5);
            if (status[0].isUpper()) {
                e->setProperty("isPasswordProtected", true);
            }
            e->setProperty("compressedSize", rxMatch.captured(6).toInt());

            const QDateTime ts(QDate::fromString(rxMatch.captured(8), QStringLiteral("yyyyMMdd")),
                               QTime::fromString(rxMatch.captured(9), QStringLiteral("hhmmss")));
            e->setProperty("timestamp", ts);

            e->setProperty("fullPath", rxMatch.captured(10));
            emit entry(e);
        }
        break;
    }

    return true;
}

bool CliPlugin::moveFiles(const QList<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    m_oldWorkingDir = QDir::currentPath();
    m_moveExtractedDir = new QTemporaryDir();
    m_moveAddedDir = new QTemporaryDir();
    QDir::setCurrent(m_moveExtractedDir->path());
    m_movedFiles = files;
    m_moveDestination = destination;
    m_moveCompressionOptions = options;

    m_moveSubOperation = Copy;
    connect(this, &CliPlugin::finished, this, &CliPlugin::continueMoving);

    return copyFiles(files, QDir::currentPath(), options);
}

int CliPlugin::moveRequiredSignals() const {
    return 4;
}

void CliPlugin::continueMoving(bool result)
{
    if (!result) {
        cleanUpMoving();
        emit cancelled();
        emit finished(false);
    }

    switch (m_moveSubOperation) {
        case Copy:
            m_moveSubOperation = Delete;
            deleteFiles(m_movedFiles);
            break;

        case Delete:
            m_moveSubOperation = Add;
            setAddedFiles();
            addFiles(m_moveAddedFiles, m_moveDestination, m_moveCompressionOptions);
            break;

        case Add:
            cleanUpMoving();
            emit progress(1.0);
            emit finished(true);
            break;

        default:
            Q_ASSERT(false);
    }
}

void CliPlugin::setAddedFiles()
{
    QDir::setCurrent(m_moveAddedDir->path());
    // If there are more files being moved than 1, we have destination as a destination folder,
    // otherwise it's new entry full path.
    if (m_movedFiles.count() > 1) {
        foreach (const Archive::Entry *file, m_movedFiles) {
            QString oldPath = m_moveExtractedDir->path() + QLatin1Char('/') + file->property("fullPath").toString();
            // Move function can't accept the second argument as a path with trailing slash.
            if (oldPath[oldPath.count() - 1] == QLatin1Char('/')) {
                oldPath.remove(oldPath.count() - 1, 1);
            }
            QString newPath = m_moveAddedDir->path() + QLatin1Char('/') + file->name();
            QFile::rename(oldPath, newPath);
            m_moveAddedFiles << new Archive::Entry(Q_NULLPTR, file->name());
        }
    }
    else {
        const Archive::Entry *file = m_movedFiles.at(0);
        QString oldPath = m_moveExtractedDir->path() + QLatin1Char('/') + file->property("fullPath").toString();
        // Move function can't accept the second argument as a path with trailing slash.
        if (oldPath[oldPath.count() - 1] == QLatin1Char('/')) {
            oldPath.remove(oldPath.count() - 1, 1);
        }
        QString newPath = m_moveAddedDir->path() + QLatin1Char('/') + m_moveDestination->name();
        QFile::rename(oldPath, newPath);
        m_moveAddedFiles << new Archive::Entry(Q_NULLPTR, m_moveDestination->name());

        // We have to exclude file name from destination path in order to pass it to addFiles method.
        const QString destinationPath = m_moveDestination->property("fullPath").toString();
        int destinationLength = destinationPath.count();
        bool iteratedChar = false;
        do {
            destinationLength--;
            if (destinationPath.at(destinationLength) != QLatin1Char('/')) {
                iteratedChar = true;
            }
        } while (destinationLength > 0 && !(iteratedChar && destinationPath.at(destinationLength) == QLatin1Char('/')));
        m_moveDestination->setProperty("fullPath", destinationPath.left(destinationLength + 1));
    }
}

void CliPlugin::cleanUpMoving()
{
    disconnect(this, &CliPlugin::finished, this, &CliPlugin::continueMoving);
    qDeleteAll(m_moveAddedFiles);
    m_moveAddedFiles.clear();
    QDir::setCurrent(m_oldWorkingDir);
    delete m_moveExtractedDir;
    m_moveExtractedDir = Q_NULLPTR;
    delete m_moveAddedDir;
    m_moveAddedDir = Q_NULLPTR;
}

#include "cliplugin.moc"
