/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/kerfuffle_export.h"

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>

#include <KPluginFactory>

using namespace Kerfuffle;

K_PLUGIN_FACTORY_WITH_JSON(CliPluginFactory, "kerfuffle_cli7z.json", registerPlugin<CliPlugin>();)

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
        : CliInterface(parent, args)
        , m_archiveType(ArchiveType7z)
        , m_parseState(ParseStateTitle)
        , m_linesComment(0)
        , m_isFirstInformationEntry(true)
{
    qCDebug(ARK) << "Loaded cli_7z plugin";
}

CliPlugin::~CliPlugin()
{
}

void CliPlugin::resetParsing()
{
    m_parseState = ParseStateTitle;
    m_comment.clear();
    m_numberOfVolumes = 0;
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        //p[CaptureProgress] = true;
        p[ListProgram] = p[ExtractProgram] = p[DeleteProgram] = p[MoveProgram] = p[AddProgram] = p[TestProgram] = QStringList() << QStringLiteral("7z");
        p[ListArgs] = QStringList() << QStringLiteral("l")
                                    << QStringLiteral("-slt")
                                    << QStringLiteral("$PasswordSwitch")
                                    << QStringLiteral("$Archive");
        p[ExtractArgs] = QStringList() << QStringLiteral("$PreservePathSwitch")
                                       << QStringLiteral("$PasswordSwitch")
                                       << QStringLiteral("$Archive")
                                       << QStringLiteral("$Files");
        p[PreservePathSwitch] = QStringList() << QStringLiteral("x")
                                              << QStringLiteral("e");
        p[PasswordSwitch] = QStringList() << QStringLiteral("-p$Password");
        p[PasswordHeaderSwitch] = QStringList { QStringLiteral("-p$Password"), QStringLiteral("-mhe=on") };
        p[WrongPasswordPatterns] = QStringList() << QStringLiteral("Wrong password");
        p[CompressionLevelSwitch] = QStringLiteral("-mx=$CompressionLevel");
        p[AddArgs] = QStringList() << QStringLiteral("a")
                                   << QStringLiteral("-l")
                                   << QStringLiteral("$Archive")
                                   << QStringLiteral("$PasswordSwitch")
                                   << QStringLiteral("$CompressionLevelSwitch")
                                   << QStringLiteral("$CompressionMethodSwitch")
                                   << QStringLiteral("$MultiVolumeSwitch")
                                   << QStringLiteral("$Files");
        p[MoveArgs] = QStringList() << QStringLiteral("rn")
                                    << QStringLiteral("$PasswordSwitch")
                                    << QStringLiteral("$Archive")
                                    << QStringLiteral("$PathPairs");
        p[DeleteArgs] = QStringList() << QStringLiteral("d")
                                      << QStringLiteral("$PasswordSwitch")
                                      << QStringLiteral("$Archive")
                                      << QStringLiteral("$Files");
        p[TestArgs] = QStringList() << QStringLiteral("t")
                                    << QStringLiteral("$Archive")
                                    << QStringLiteral("$PasswordSwitch");
        p[TestPassedPattern] = QStringLiteral("^Everything is Ok$");

        p[FileExistsExpression] = QStringList()
            << QStringLiteral("^\\(Y\\)es / \\(N\\)o / \\(A\\)lways / \\(S\\)kip all / A\\(u\\)to rename all / \\(Q\\)uit\\? $")
            << QStringLiteral("^\\? \\(Y\\)es / \\(N\\)o / \\(A\\)lways / \\(S\\)kip all / A\\(u\\)to rename all / \\(Q\\)uit\\? $");
        p[FileExistsFileName] = QStringList() << QStringLiteral("^file \\./(.*)$")
                                              << QStringLiteral("^  Path:     \\./(.*)$");
        p[FileExistsInput] = QStringList() << QStringLiteral("Y")  //overwrite
                                           << QStringLiteral("N")  //skip
                                           << QStringLiteral("A")  //overwrite all
                                           << QStringLiteral("S")  //autoskip
                                           << QStringLiteral("Q"); //cancel
        p[PasswordPromptPattern] = QStringLiteral("Enter password \\(will not be echoed\\)");
        p[ExtractionFailedPatterns] = QStringList() << QStringLiteral("ERROR: E_FAIL") << QStringLiteral("Open ERROR: Can not open the file as \\[7z\\] archive");
        p[CorruptArchivePatterns] = QStringList() << QStringLiteral("Unexpected end of archive")
                                                  << QStringLiteral("Headers Error");
        p[DiskFullPatterns] = QStringList() << QStringLiteral("No space left on device");
        p[MultiVolumeSwitch] = QStringLiteral("-v$VolumeSizek");
        p[MultiVolumeSuffix] = QStringList() << QStringLiteral("$Suffix.001");
        QMap<QString,QVariant> compMethodMap;
        compMethodMap[QStringLiteral("zip")] = QStringLiteral("-mm=$CompressionMethod");
        compMethodMap[QStringLiteral("7z")] = QStringLiteral("-m0=$CompressionMethod");
        p[CompressionMethodSwitch] = compMethodMap;
    }

    return p;
}

bool CliPlugin::readListLine(const QString& line)
{
    static const QLatin1String archiveInfoDelimiter1("--"); // 7z 9.13+
    static const QLatin1String archiveInfoDelimiter2("----"); // 7z 9.04
    static const QLatin1String entryInfoDelimiter("----------");
    const QRegularExpression rxComment(QStringLiteral("Comment = .+$"));

    if (m_parseState == ParseStateTitle) {

        const QRegularExpression rxVersionLine(QStringLiteral("^p7zip Version ([\\d\\.]+) .*$"));
        QRegularExpressionMatch matchVersion = rxVersionLine.match(line);
        if (matchVersion.hasMatch()) {
            m_parseState = ParseStateHeader;
            const QString p7zipVersion = matchVersion.captured(1);
            qCDebug(ARK) << "p7zip version" << p7zipVersion << "detected";
        }

    } else if (m_parseState == ParseStateHeader) {

        if (line.startsWith(QStringLiteral("Listing archive:"))) {
            qCDebug(ARK) << "Archive name: "
                     << line.right(line.size() - 16).trimmed();
        } else if ((line == archiveInfoDelimiter1) ||
                   (line == archiveInfoDelimiter2)) {
            m_parseState = ParseStateArchiveInformation;
        } else if (line.contains(QStringLiteral("Error: "))) {
            qCWarning(ARK) << line.mid(7);
        }

    } else if (m_parseState == ParseStateArchiveInformation) {

        if (line == entryInfoDelimiter) {
            m_parseState = ParseStateEntryInformation;
        } else if (line.startsWith(QStringLiteral("Type = "))) {
            const QString type = line.mid(7).trimmed();
            qCDebug(ARK) << "Archive type: " << type;

            if (type == QLatin1String("7z")) {
                m_archiveType = ArchiveType7z;
            } else if (type == QLatin1String("bzip2")) {
                m_archiveType = ArchiveTypeBZip2;
            } else if (type == QLatin1String("gzip")) {
                m_archiveType = ArchiveTypeGZip;
            } else if (type == QLatin1String("xz")) {
                m_archiveType = ArchiveTypeXz;
            } else if (type == QLatin1String("tar")) {
                m_archiveType = ArchiveTypeTar;
            } else if (type == QLatin1String("zip")) {
                m_archiveType = ArchiveTypeZip;
            } else if (type == QLatin1String("Rar")) {
                m_archiveType = ArchiveTypeRar;
            } else if (type == QLatin1String("Split")) {
                setMultiVolume(true);
            } else {
                // Should not happen
                qCWarning(ARK) << "Unsupported archive type";
                return false;
            }
        } else if (line.startsWith(QStringLiteral("Volumes = "))) {
            m_numberOfVolumes = line.section(QLatin1Char('='), 1).trimmed().toInt();

        } else if (line.startsWith(QStringLiteral("Method = "))) {
            QStringList methods = line.section(QLatin1Char('='), 1).trimmed().split(QLatin1Char(' '), QString::SkipEmptyParts);
            // LZMA methods are output with some trailing numbers by 7z representing dictionary/block sizes.
            // We are not interested in these, so remove them.
            QMutableListIterator<QString> i(methods);
            while (i.hasNext()) {
                QString m = i.next();
                if (m.startsWith(QLatin1String("LZMA2"))) {
                    m = m.left(5);
                } else if (m.startsWith(QLatin1String("LZMA"))) {
                    m = m.left(4);
                }
                i.setValue(m);
            }
            emit compressionMethodFound(methods);

        } else if (rxComment.match(line).hasMatch()) {
            m_parseState = ParseStateComment;
            m_comment.append(line.section(QLatin1Char('='), 1) + QLatin1Char('\n'));
        }

    } else if (m_parseState == ParseStateComment) {

        if (line == entryInfoDelimiter) {
            m_parseState = ParseStateEntryInformation;
            if (!m_comment.trimmed().isEmpty()) {
                m_comment = m_comment.trimmed();
                m_linesComment = m_comment.count(QLatin1Char('\n')) + 1;
                qCDebug(ARK) << "Found a comment with" << m_linesComment << "lines";
            }
        } else {
            m_comment.append(line + QLatin1Char('\n'));
        }

    } else if (m_parseState == ParseStateEntryInformation) {

        if (m_isFirstInformationEntry) {
            m_isFirstInformationEntry = false;
            m_currentArchiveEntry = new Archive::Entry(this);
            m_currentArchiveEntry->compressedSizeIsSet = false;
        }
        if (line.startsWith(QStringLiteral("Path = "))) {
            const QString entryFilename =
                QDir::fromNativeSeparators(line.mid(7).trimmed());
            m_currentArchiveEntry->setProperty("fullPath", entryFilename);
        } else if (line.startsWith(QStringLiteral("Size = "))) {
            m_currentArchiveEntry->setProperty("size", line.mid(7).trimmed());
        } else if (line.startsWith(QStringLiteral("Packed Size = "))) {
            // #236696: 7z files only show a single Packed Size value
            //          corresponding to the whole archive.
            if (m_archiveType != ArchiveType7z) {
                m_currentArchiveEntry->compressedSizeIsSet = true;
                m_currentArchiveEntry->setProperty("compressedSize", line.mid(14).trimmed());
            }
        } else if (line.startsWith(QStringLiteral("Modified = "))) {
            m_currentArchiveEntry->setProperty("timestamp", QDateTime::fromString(line.mid(11).trimmed(),
                                                                                  QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        } else if (line.startsWith(QStringLiteral("Attributes = "))) {
            const QString attributes = line.mid(13).trimmed();

            const bool isDirectory = attributes.startsWith(QLatin1Char('D'));
            m_currentArchiveEntry->setProperty("isDirectory", isDirectory);
            if (isDirectory) {
                const QString directoryName =
                    m_currentArchiveEntry->fullPath();
                if (!directoryName.endsWith(QLatin1Char('/'))) {
                    const bool isPasswordProtected = (line.at(12) == QLatin1Char('+'));
                    m_currentArchiveEntry->setProperty("fullPath", QString(directoryName + QLatin1Char('/')));
                    m_currentArchiveEntry->setProperty("isPasswordProtected", isPasswordProtected);
                }
            }

            m_currentArchiveEntry->setProperty("permissions", attributes.mid(1));
        } else if (line.startsWith(QStringLiteral("CRC = "))) {
            m_currentArchiveEntry->setProperty("CRC", line.mid(6).trimmed());
        } else if (line.startsWith(QStringLiteral("Method = "))) {
            m_currentArchiveEntry->setProperty("method", line.mid(9).trimmed());

            // For zip archives we need to check method for each entry.
            if (m_archiveType == ArchiveTypeZip) {
                QString method = line.mid(9).trimmed();
                if (method == QLatin1String("xz")) {
                    method = QStringLiteral("XZ");
                }
                if (!m_compressionMethods.contains(method)) {
                    m_compressionMethods.append(method);
                    emit compressionMethodFound(m_compressionMethods);
                }
            }

        } else if (line.startsWith(QStringLiteral("Encrypted = ")) &&
                   line.size() >= 13) {
            m_currentArchiveEntry->setProperty("isPasswordProtected", line.at(12) == QLatin1Char('+'));
        } else if (line.startsWith(QStringLiteral("Block = ")) ||
                   line.startsWith(QStringLiteral("Version = "))) {
            m_isFirstInformationEntry = true;
            if (!m_currentArchiveEntry->fullPath().isEmpty()) {
                emit entry(m_currentArchiveEntry);
            }
            else {
                delete m_currentArchiveEntry;
            }
            m_currentArchiveEntry = Q_NULLPTR;
        }
    }

    return true;
}

QStringList CliPlugin::passwordHeaderSwitch(const QString& password) const
{
    if (password.isEmpty()) {
        return QStringList();
    }

    Q_ASSERT(m_param.contains(PasswordHeaderSwitch));

    QStringList passwordHeaderSwitch = m_param.value(PasswordHeaderSwitch).toStringList();
    Q_ASSERT(!passwordHeaderSwitch.isEmpty() && passwordHeaderSwitch.size() == 2);

    passwordHeaderSwitch[0].replace(QLatin1String("$Password"), password);

    return passwordHeaderSwitch;
}

QString CliPlugin::compressionMethodSwitch(const QString &method) const
{
    if (method.isEmpty()) {
        return QString();
    }

    Q_ASSERT(!filename().isEmpty());
    Q_ASSERT(m_param.contains(CompressionMethodSwitch));

    QMap<QString,QVariant> switches = m_param.value(CompressionMethodSwitch).toMap();
    Q_ASSERT(!switches.isEmpty());

    QString compMethodSwitch;

    compMethodSwitch = switches[QFileInfo(filename()).suffix().toLower()].toString();
    compMethodSwitch.replace(QLatin1String("$CompressionMethod"), method);

    return compMethodSwitch;
}

#include "cliplugin.moc"
