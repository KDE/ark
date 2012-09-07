/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
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
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/kerfuffle_export.h"

#include <QDateTime>
#include <QDir>
#include <QLatin1String>
#include <QString>

#include <KDebug>

using namespace Kerfuffle;

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
    : CliInterface(parent, args)
    , m_archiveType(ArchiveType7z)
    , m_state(ReadStateHeader)
{
}

CliPlugin::~CliPlugin()
{
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        //p[CaptureProgress] = true;
        p[ListProgram] = p[ExtractProgram] = p[DeleteProgram] = p[AddProgram] = QStringList() << QLatin1String("7z");

        p[ListArgs] = QStringList() << QLatin1String("l") << QLatin1String("-slt") << QLatin1String("$Archive");
        p[ExtractArgs] = QStringList() << QLatin1String("$MultiThreadingSwitch") << QLatin1String("$PreservePathSwitch") << QLatin1String("$PasswordSwitch") << QLatin1String("$Archive") << QLatin1String("$Files");
        p[PreservePathSwitch] = QStringList() << QLatin1String("x") << QLatin1String("e");
        p[PasswordSwitch] = QStringList() << QLatin1String("-p$Password");
        p[FileExistsExpression] = QLatin1String("already exists. Overwrite with");
        p[WrongPasswordPatterns] = QStringList() << QLatin1String("Wrong password");

        // TODO: split archives do not support add and delete.
        p[AddArgs] = QStringList() << QLatin1String("-tzip") << QLatin1String("a") << QLatin1String("$TemporaryDirectorySwitch") << QLatin1String("$CompressionLevelSwitch") << QLatin1String("$MultiThreadingSwitch") << QLatin1String("$PasswordSwitch") << QLatin1String("$EncryptHeaderSwitch") << QLatin1String("$EncryptionMethodSwitches") << QLatin1String("$MultiPartSwitch") << QLatin1String("$Archive") << QLatin1String("$Files");
        p[AddFailedPatterns] = QStringList() << QLatin1String("E_NOTIMPL")
                                             << QLatin1String("Cannot modify volume");
        // TODO: split archives do not support add and delete.
        p[DeleteArgs] = QStringList() << QLatin1String("d") << QLatin1String("$Archive") << QLatin1String("$Files");

        p[FileExistsInput] = QStringList()
                             << QLatin1String("Y")   //overwrite
                             << QLatin1String("N")   //skip
                             << QLatin1String("A")   //overwrite all
                             << QLatin1String("S")   //autoskip
                             << QLatin1String("Q")   //cancel
                             << QLatin1String("U")   //Auto rename
                             ;
        p[SupportsRename] = true; // just for GUI feedback

        p[CompressionLevelSwitches] = QStringList() << QLatin1String("-mx=0") << QLatin1String("-mx=2") << QLatin1String("-mx=5") << QLatin1String("-mx=7") << QLatin1String("-mx=9");
        p[MultiThreadingSwitch] = QLatin1String("-mmt");

        p[MultiPartSwitch] = QLatin1String("-v$MultiPartSizek");

        p[PasswordPromptPattern] = QLatin1String("Enter password \\(will not be echoed\\) :");

        // according to 7's man page this only works for 7z archives.
        //p[EncryptHeaderSwitch] = QLatin1String("-mhe");
        p[EncryptionMethodSwitches] = QStringList() << QLatin1String("-mem=AES256") << QLatin1String("-mem=ZipCrypto");

        p[TestProgram] = QStringList() << QLatin1String("7z");
        p[TestArgs] = QStringList() << QLatin1String("t") << QLatin1String("$PasswordSwitch") << QLatin1String("$Archive") << QLatin1String("$Files");
        p[TestFailedPatterns] = QStringList() << QLatin1String("Data Error") << QLatin1String("CRC Failed") << QLatin1String("Can not open file as archive") << QLatin1String("Sub items Errors:");
        p[TemporaryDirectorySwitch] = QLatin1String("-w$DirectoryPath");
    }

    return p;
}

bool CliPlugin::readListLine(const QString& line)
{
    kDebug(1601) << line;

    static const QLatin1String archiveInfoDelimiter1("--"); // 7z 9.13+
    static const QLatin1String archiveInfoDelimiter2("----"); // 7z 9.04
    static const QLatin1String entryInfoDelimiter("----------");

    switch (m_state) {
    case ReadStateHeader:
        if (line.startsWith(QLatin1String("Listing archive:"))) {
            kDebug(1601) << "Archive name: "
                         << line.right(line.size() - 16).trimmed();
            m_numberOfVolumes = -1;
        } else if ((line == archiveInfoDelimiter1) ||
                   (line == archiveInfoDelimiter2)) {
            m_state = ReadStateArchiveInformation;
        } else if (line.contains(QLatin1String("Error:"))) {
            kDebug(1601) << line.mid(6);
        }
        break;

    case ReadStateArchiveInformation:
        if (line == entryInfoDelimiter) {
            m_state = ReadStateEntryInformation;
        } else if (line.startsWith(QLatin1String("Type ="))) {
            const QString type = line.mid(7).trimmed().toLower();
            kDebug(1601) << "Archive type: " << type;

            if (type == QLatin1String("7z")) {
                m_archiveType = ArchiveType7z;
            } else if (type == QLatin1String("bzip2")) {
                m_archiveType = ArchiveTypeBZip2;
            } else if (type == QLatin1String("gzip")) {
                m_archiveType = ArchiveTypeGZip;
            } else if (type == QLatin1String("tar")) {
                m_archiveType = ArchiveTypeTar;
            } else if (type == QLatin1String("zip")) {
                m_archiveType = ArchiveTypeZip;
            } else if (type == QLatin1String("split")) {
                // m_archiveType will be set later with one of the
                // types above. see "7z l -slt" output. Use m_numberOfVolumes
                // to check if the archive is really split (m_numberOfVolumes >= 2)
                // or not.
            } else {
                // Should not happen
                kWarning() << "Unsupported archive type";
                return false;
            }
        } else if (line.startsWith(QLatin1String("Volumes ="))) {
            m_numberOfVolumes = line.mid(9).trimmed().toInt();
            kDebug(1601) << "Number of volumes: " << m_numberOfVolumes;
        }

        break;

    case ReadStateEntryInformation:
        if (line.startsWith(QLatin1String("Path ="))) {
            const QString entryFilename =
                QDir::fromNativeSeparators(line.mid(6).trimmed());
            kDebug(1601) << entryFilename;
            m_currentArchiveEntry.clear();
            m_currentArchiveEntry[FileName] = autoConvertEncoding(entryFilename);
            m_currentArchiveEntry[InternalID] = entryFilename;
        } else if (line.startsWith(QLatin1String("Size = "))) {
            m_currentArchiveEntry[ Size ] = line.mid(7).trimmed();
        } else if (line.startsWith(QLatin1String("Packed Size = "))) {
            // #236696: 7z files only show a single Packed Size value
            //          corresponding to the whole archive.
            if (m_archiveType != ArchiveType7z) {
                m_currentArchiveEntry[CompressedSize] = line.mid(14).trimmed();
            }
        } else if (line.startsWith(QLatin1String("Modified = "))) {
            m_currentArchiveEntry[ Timestamp ] =
                QDateTime::fromString(line.mid(11).trimmed(),
                                      QLatin1String("yyyy-MM-dd hh:mm:ss"));
        } else if (line.startsWith(QLatin1String("Attributes = "))) {
            const QString attributes = line.mid(13).trimmed();

            const bool isDirectory = attributes.startsWith(QLatin1Char('D'));
            m_currentArchiveEntry[ IsDirectory ] = isDirectory;
            if (isDirectory) {
                const QString directoryName =
                    m_currentArchiveEntry[FileName].toString();
                if (!directoryName.endsWith(QLatin1Char('/'))) {
                    const bool isPasswordProtected = (line.at(12) == QLatin1Char('+'));
                    m_currentArchiveEntry[FileName] =
                        m_currentArchiveEntry[InternalID] = QString(directoryName + QLatin1Char('/'));
                    m_currentArchiveEntry[ IsPasswordProtected ] =
                        isPasswordProtected;
                }
            }

            m_currentArchiveEntry[ Permissions ] = attributes.mid(1);
        } else if (line.startsWith(QLatin1String("CRC = "))) {
            m_currentArchiveEntry[ CRC ] = line.mid(6).trimmed();
        } else if (line.startsWith(QLatin1String("Method = "))) {
            m_currentArchiveEntry[ Method ] = line.mid(9).trimmed();
        } else if (line.startsWith(QLatin1String("Encrypted = ")) &&
                   line.size() >= 13) {
            m_currentArchiveEntry[ IsPasswordProtected ] = (line.at(12) == QLatin1Char('+'));
        } else if (line.isEmpty()) {
            if (m_currentArchiveEntry.contains(FileName)) {
                entry(m_currentArchiveEntry);
            }
        }
        break;
    }

    return true;
}
void CliPlugin::resetReadState()
{
    m_state = ReadStateHeader;
}

void CliPlugin::saveLastLine(const QString & line)
{
    m_lastLine = line;
}

// for simplicity checks only the last line, otherwise we would have to parse
// every entry passed to saveLastLine.
QString CliPlugin::fileExistsName()
{
    // sometimes 7z's output lines are concatenated, then we cannot use "^file (.+)" here.
    QRegExp existsPattern(QLatin1String("file (.+)"));

    if (existsPattern.indexIn(m_lastLine) != -1) {
        return existsPattern.cap(1);
    }

    return QString();
}

KERFUFFLE_EXPORT_PLUGIN(CliPlugin)

#include "cliplugin.moc"
