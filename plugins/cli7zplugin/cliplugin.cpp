/*
    SPDX-FileCopyrightText: 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "cliplugin.h"
#include "ark_debug.h"

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>

#include <KLocalizedString>
#include <KPluginFactory>

using namespace Kerfuffle;

K_PLUGIN_CLASS_WITH_JSON(CliPlugin, "kerfuffle_cli7z.json")

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
        : CliInterface(parent, args)
        , m_archiveType(ArchiveType7z)
        , m_parseState(ParseStateTitle)
        , m_linesComment(0)
        , m_isFirstInformationEntry(true)
{
    qCDebug(ARK) << "Loaded cli_7z plugin";

    setupCliProperties();
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

void CliPlugin::setupCliProperties()
{
    qCDebug(ARK) << "Setting up parameters...";

    m_cliProps->setProperty("captureProgress", false);

    m_cliProps->setProperty("addProgram", QStringLiteral("7z"));
    m_cliProps->setProperty("addSwitch", QStringList{QStringLiteral("a"),
                                                 QStringLiteral("-l")});

    m_cliProps->setProperty("deleteProgram", QStringLiteral("7z"));
    m_cliProps->setProperty("deleteSwitch", QStringLiteral("d"));

    m_cliProps->setProperty("extractProgram", QStringLiteral("7z"));
    m_cliProps->setProperty("extractSwitch", QStringList{QStringLiteral("x")});
    m_cliProps->setProperty("extractSwitchNoPreserve", QStringList{QStringLiteral("e")});

    m_cliProps->setProperty("listProgram", QStringLiteral("7z"));
    m_cliProps->setProperty("listSwitch", QStringList{QStringLiteral("l"),
                                                  QStringLiteral("-slt")});

    m_cliProps->setProperty("moveProgram", QStringLiteral("7z"));
    m_cliProps->setProperty("moveSwitch", QStringLiteral("rn"));

    m_cliProps->setProperty("testProgram", QStringLiteral("7z"));
    m_cliProps->setProperty("testSwitch", QStringLiteral("t"));

    m_cliProps->setProperty("passwordSwitch", QStringList{QStringLiteral("-p$Password")});
    m_cliProps->setProperty("passwordSwitchHeaderEnc", QStringList{QStringLiteral("-p$Password"),
                                                               QStringLiteral("-mhe=on")});
    m_cliProps->setProperty("compressionLevelSwitch", QStringLiteral("-mx=$CompressionLevel"));
    m_cliProps->setProperty("compressionMethodSwitch", QHash<QString,QVariant>{{QStringLiteral("application/x-7z-compressed"), QStringLiteral("-m0=$CompressionMethod")},
                                                                               {QStringLiteral("application/zip"), QStringLiteral("-mm=$CompressionMethod")}});
    m_cliProps->setProperty("encryptionMethodSwitch", QHash<QString,QVariant>{{QStringLiteral("application/x-7z-compressed"), QString()},
                                                                              {QStringLiteral("application/zip"), QStringLiteral("-mem=$EncryptionMethod")}});
    m_cliProps->setProperty("multiVolumeSwitch", QStringLiteral("-v$VolumeSizek"));
    m_cliProps->setProperty("testPassedPatterns", QStringList{QStringLiteral("^Everything is Ok$")});
    m_cliProps->setProperty("fileExistsFileNameRegExp", QStringList{QStringLiteral("^file \\./(.*)$"),
                                                                    QStringLiteral("^  Path:     \\./(.*)$")});
    m_cliProps->setProperty("fileExistsInput", QStringList{QStringLiteral("Y"),   //Overwrite
                                                       QStringLiteral("N"),   //Skip
                                                       QStringLiteral("A"),   //Overwrite all
                                                       QStringLiteral("S"),   //Autoskip
                                                       QStringLiteral("Q")}); //Cancel
    m_cliProps->setProperty("multiVolumeSuffix", QStringList{QStringLiteral("$Suffix.001")});
}

void CliPlugin::fixDirectoryFullName()
{
    if (m_currentArchiveEntry->isDir()) {
        const QString directoryName = m_currentArchiveEntry->fullPath();
        if (!directoryName.endsWith(QLatin1Char('/'))) {
            m_currentArchiveEntry->setProperty("fullPath", QString(directoryName + QLatin1Char('/')));
        }
    }
}

bool CliPlugin::readListLine(const QString& line)
{
    static const QLatin1String archiveInfoDelimiter1("--"); // 7z 9.13+
    static const QLatin1String archiveInfoDelimiter2("----"); // 7z 9.04
    static const QLatin1String entryInfoDelimiter("----------");

    if (line.startsWith(QLatin1String("Open ERROR: Can not open the file as [7z] archive"))) {
        Q_EMIT error(i18n("Listing the archive failed."));
        return false;
    }

    const QRegularExpression rxVersionLine(QStringLiteral("^p7zip Version ([\\d\\.]+) .*$"));
    QRegularExpressionMatch matchVersion;

    switch (m_parseState) {
    case ParseStateTitle:
        matchVersion = rxVersionLine.match(line);
        if (matchVersion.hasMatch()) {
            m_parseState = ParseStateHeader;
            const QString p7zipVersion = matchVersion.captured(1);
            qCDebug(ARK) << "p7zip version" << p7zipVersion << "detected";
        }
        break;

    case ParseStateHeader:
        if (line.startsWith(QLatin1String("Listing archive:"))) {
            qCDebug(ARK) << "Archive name: "
                         << line.right(line.size() - 16).trimmed();
        } else if ((line == archiveInfoDelimiter1) ||
                   (line == archiveInfoDelimiter2)) {
            m_parseState = ParseStateArchiveInformation;
        } else if (line.contains(QLatin1String("Error: "))) {
            qCWarning(ARK) << line.mid(7);
        }
        break;

    case ParseStateArchiveInformation:
        if (line == entryInfoDelimiter) {
            m_parseState = ParseStateEntryInformation;

        } else if (line.startsWith(QLatin1String("Type = "))) {
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

        } else if (line.startsWith(QLatin1String("Volumes = "))) {
            m_numberOfVolumes = line.section(QLatin1Char('='), 1).trimmed().toInt();

        } else if (line.startsWith(QLatin1String("Method = "))) {
            QStringList methods = line.section(QLatin1Char('='), 1).trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
            handleMethods(methods);

        } else if (line.startsWith(QLatin1String("Comment = "))) {
            m_parseState = ParseStateComment;
            m_comment.append(line.section(QLatin1Char('='), 1) + QLatin1Char('\n'));
        }
        break;

    case ParseStateComment:
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
        break;

    case ParseStateEntryInformation:
        if (m_isFirstInformationEntry) {
            m_isFirstInformationEntry = false;
            m_currentArchiveEntry = new Archive::Entry(this);
            m_currentArchiveEntry->compressedSizeIsSet = false;
        }
        if (line.startsWith(QLatin1String("Path = "))) {
            const QString entryFilename =
                QDir::fromNativeSeparators(line.mid(7).trimmed());
            m_currentArchiveEntry->setProperty("fullPath", entryFilename);

        } else if (line.startsWith(QLatin1String("Size = "))) {
            m_currentArchiveEntry->setProperty("size", line.mid(7).trimmed());

        } else if (line.startsWith(QLatin1String("Packed Size = "))) {
            // #236696: 7z files only show a single Packed Size value
            //          corresponding to the whole archive.
            if (m_archiveType != ArchiveType7z) {
                m_currentArchiveEntry->compressedSizeIsSet = true;
                m_currentArchiveEntry->setProperty("compressedSize", line.mid(14).trimmed());
            }

        } else if (line.startsWith(QLatin1String("Modified = "))) {
            m_currentArchiveEntry->setProperty("timestamp", QDateTime::fromString(line.mid(11).trimmed(),
                                                                                  QStringLiteral("yyyy-MM-dd hh:mm:ss")));

        } else if (line.startsWith(QLatin1String("Folder = "))) {
            const QString isDirectoryStr = line.mid(9).trimmed();
            Q_ASSERT(isDirectoryStr == QLatin1String("+") || isDirectoryStr == QStringLiteral("-"));
            const bool isDirectory = isDirectoryStr.startsWith(QLatin1Char('+'));
            m_currentArchiveEntry->setProperty("isDirectory", isDirectory);
            fixDirectoryFullName();

        } else if (line.startsWith(QLatin1String("Attributes = "))) {
            const QString attributes = line.mid(13).trimmed();
            if (attributes.contains(QLatin1Char('D'))) {
                m_currentArchiveEntry->setProperty("isDirectory", true);
                fixDirectoryFullName();
            }

            if (attributes.contains(QLatin1Char('_'))) {
                // Unix attributes
                m_currentArchiveEntry->setProperty("permissions",
                                                   attributes.mid(attributes.indexOf(QLatin1Char(' ')) + 1));
            } else {
                // FAT attributes
                m_currentArchiveEntry->setProperty("permissions", attributes);
            }

        } else if (line.startsWith(QLatin1String("CRC = "))) {
            m_currentArchiveEntry->setProperty("CRC", line.mid(6).trimmed());

        } else if (line.startsWith(QLatin1String("Method = "))) {
            m_currentArchiveEntry->setProperty("method", line.mid(9).trimmed());

            // For zip archives we need to check method for each entry.
            if (m_archiveType == ArchiveTypeZip) {
                QStringList methods = line.section(QLatin1Char('='), 1).trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
                handleMethods(methods);
            }

        } else if (line.startsWith(QLatin1String("Encrypted = ")) &&
                   line.size() >= 13) {
            m_currentArchiveEntry->setProperty("isPasswordProtected", line.at(12) == QLatin1Char('+'));

        } else if (line.startsWith(QLatin1String("Block = ")) ||
                   line.startsWith(QLatin1String("Version = "))) {
            m_isFirstInformationEntry = true;
            if (!m_currentArchiveEntry->fullPath().isEmpty()) {
                Q_EMIT entry(m_currentArchiveEntry);
            }
            else {
                delete m_currentArchiveEntry;
            }
            m_currentArchiveEntry = nullptr;
        }
        break;
    }

    return true;
}

bool CliPlugin::readExtractLine(const QString &line)
{
    if (line.startsWith(QLatin1String("ERROR: E_FAIL"))) {
        Q_EMIT error(i18n("Extraction failed due to an unknown error."));
        return false;
    }

    if (line.startsWith(QLatin1String("ERROR: CRC Failed")) ||
        line.startsWith(QLatin1String("ERROR: Headers Error"))) {
        Q_EMIT error(i18n("Extraction failed due to one or more corrupt files. Any extracted files may be damaged."));
        return false;
    }

    return true;
}

bool CliPlugin::readDeleteLine(const QString &line)
{
    if (line.startsWith(QLatin1String("Error: ")) &&
        line.endsWith(QLatin1String(" is not supported archive"))) {
        Q_EMIT error(i18n("Delete operation failed. Try upgrading p7zip or disabling the p7zip plugin in the configuration dialog."));
        return false;
    }

    return true;
}

void CliPlugin::handleMethods(const QStringList &methods)
{
    for (const QString &method : methods) {

        QRegularExpression rxEncMethod(QStringLiteral("^(7zAES|AES-128|AES-192|AES-256|ZipCrypto)$"));
        if (rxEncMethod.match(method).hasMatch()) {
            QRegularExpression rxAESMethods(QStringLiteral("^(AES-128|AES-192|AES-256)$"));
            if (rxAESMethods.match(method).hasMatch()) {
                // Remove dash for AES methods.
                Q_EMIT encryptionMethodFound(QString(method).remove(QLatin1Char('-')));
            } else {
                Q_EMIT encryptionMethodFound(method);
            }
            continue;
        }

        // LZMA methods are output with some trailing numbers by 7z representing dictionary/block sizes.
        // We are not interested in these, so remove them.
        if (method.startsWith(QLatin1String("LZMA2"))) {
            Q_EMIT compressionMethodFound(method.left(5));
        } else if (method.startsWith(QLatin1String("LZMA"))) {
            Q_EMIT compressionMethodFound(method.left(4));
        } else if (method == QLatin1String("xz")) {
            Q_EMIT compressionMethodFound(method.toUpper());
        } else {
            Q_EMIT compressionMethodFound(method);
        }
    }
}

bool CliPlugin::isPasswordPrompt(const QString &line)
{
    return line.startsWith(QLatin1String("Enter password (will not be echoed):"));
}

bool CliPlugin::isWrongPasswordMsg(const QString &line)
{
    return line.contains(QLatin1String("Wrong password"));
}

bool CliPlugin::isCorruptArchiveMsg(const QString &line)
{
    return (line == QLatin1String("Unexpected end of archive") ||
            line == QLatin1String("Headers Error"));
}

bool CliPlugin::isDiskFullMsg(const QString &line)
{
    return line.contains(QLatin1String("No space left on device"));
}

bool CliPlugin::isFileExistsMsg(const QString &line)
{
    return (line == QLatin1String("(Y)es / (N)o / (A)lways / (S)kip all / A(u)to rename all / (Q)uit? ") ||
            line == QLatin1String("? (Y)es / (N)o / (A)lways / (S)kip all / A(u)to rename all / (Q)uit? "));
}

bool CliPlugin::isFileExistsFileName(const QString &line)
{
    return (line.startsWith(QLatin1String("file ./")) ||
            line.startsWith(QLatin1String("  Path:     ./")));
}

#include "cliplugin.moc"
