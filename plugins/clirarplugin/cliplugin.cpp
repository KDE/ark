    /*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2010-2011,2014 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (C) 2015-2016 Ragnar Thomsen <rthomsen6@gmail.com>
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
#include "archiveentry.h"

#include <QDateTime>

#include <KLocalizedString>
#include <KPluginFactory>

    using namespace Kerfuffle;

K_PLUGIN_CLASS_WITH_JSON(CliPlugin, "kerfuffle_clirar.json")

CliPlugin::CliPlugin(QObject *parent, const QVariantList& args)
        : CliInterface(parent, args)
        , m_parseState(ParseStateTitle)
        , m_isUnrar5(false)
        , m_isPasswordProtected(false)
        , m_isSolid(false)
        , m_isRAR5(false)
        , m_isLocked(false)
        , m_remainingIgnoreLines(1) //The first line of UNRAR output is empty.
        , m_linesComment(0)
{
    qCDebug(ARK) << "Loaded cli_rar plugin";

    // Empty lines are needed for parsing output of unrar.
    setListEmptyLines(true);

    setupCliProperties();
}

CliPlugin::~CliPlugin()
{
}

void CliPlugin::resetParsing()
{
    m_parseState = ParseStateTitle;
    m_remainingIgnoreLines = 1;
    m_unrarVersion.clear();
    m_comment.clear();
    m_numberOfVolumes = 0;
}

void CliPlugin::setupCliProperties()
{
    qCDebug(ARK) << "Setting up parameters...";

    m_cliProps->setProperty("captureProgress", true);

    m_cliProps->setProperty("addProgram", QStringLiteral("rar"));
    m_cliProps->setProperty("addSwitch", QStringList({QStringLiteral("a")}));

    m_cliProps->setProperty("deleteProgram", QStringLiteral("rar"));
    m_cliProps->setProperty("deleteSwitch", QStringLiteral("d"));

    m_cliProps->setProperty("extractProgram", QStringLiteral("unrar"));
    m_cliProps->setProperty("extractSwitch", QStringList{QStringLiteral("x"),
                                                     QStringLiteral("-kb"),
                                                     QStringLiteral("-p-")});
    m_cliProps->setProperty("extractSwitchNoPreserve", QStringList{QStringLiteral("e"),
                                                               QStringLiteral("-kb"),
                                                               QStringLiteral("-p-")});

    m_cliProps->setProperty("listProgram", QStringLiteral("unrar"));
    m_cliProps->setProperty("listSwitch", QStringList{QStringLiteral("vt"),
                                                  QStringLiteral("-v")});

    m_cliProps->setProperty("moveProgram", QStringLiteral("rar"));
    m_cliProps->setProperty("moveSwitch", QStringLiteral("rn"));

    m_cliProps->setProperty("testProgram", QStringLiteral("unrar"));
    m_cliProps->setProperty("testSwitch", QStringLiteral("t"));

    m_cliProps->setProperty("commentSwitch", QStringList{QStringLiteral("c"),
                                                     QStringLiteral("-z$CommentFile")});

    m_cliProps->setProperty("passwordSwitch", QStringList{QStringLiteral("-p$Password")});
    m_cliProps->setProperty("passwordSwitchHeaderEnc", QStringList{QStringLiteral("-hp$Password")});

    m_cliProps->setProperty("compressionLevelSwitch", QStringLiteral("-m$CompressionLevel"));
    m_cliProps->setProperty("compressionMethodSwitch", QHash<QString,QVariant>{{QStringLiteral("application/vnd.rar"), QStringLiteral("-ma$CompressionMethod")},
                                                                           {QStringLiteral("application/x-rar"), QStringLiteral("-ma$CompressionMethod")}});
    m_cliProps->setProperty("multiVolumeSwitch", QStringLiteral("-v$VolumeSizek"));


    m_cliProps->setProperty("testPassedPatterns", QStringList{QStringLiteral("^All OK$")});
    m_cliProps->setProperty("fileExistsFileNameRegExp", QStringList{QStringLiteral("^(.+) already exists. Overwrite it"),  // unrar 3 & 4
                                                                    QStringLiteral("^Would you like to replace the existing file (.+)$")}); // unrar 5
    m_cliProps->setProperty("fileExistsInput", QStringList{QStringLiteral("Y"),   //Overwrite
                                                       QStringLiteral("N"),   //Skip
                                                       QStringLiteral("A"),   //Overwrite all
                                                       QStringLiteral("E"),   //Autoskip
                                                       QStringLiteral("Q")}); //Cancel

    // rar will sometimes create multi-volume archives where first volume is
    // called name.part1.rar and other times name.part01.rar.
    m_cliProps->setProperty("multiVolumeSuffix", QStringList{QStringLiteral("part01.$Suffix"),
                                                         QStringLiteral("part1.$Suffix")});
}

bool CliPlugin::readListLine(const QString &line)
{
    // Ignore number of lines corresponding to m_remainingIgnoreLines.
    if (m_remainingIgnoreLines > 0) {
        --m_remainingIgnoreLines;
        return true;
    }

    // Parse the title line, which contains the version of unrar.
    if (m_parseState == ParseStateTitle) {

        QRegularExpression rxVersionLine(QStringLiteral("^UNRAR (\\d+\\.\\d+)( beta \\d)? .*$"));
        QRegularExpressionMatch matchVersion = rxVersionLine.match(line);

        if (matchVersion.hasMatch()) {
            m_parseState = ParseStateComment;
            m_unrarVersion = matchVersion.captured(1);
            qCDebug(ARK) << "UNRAR version" << m_unrarVersion << "detected";
            if (m_unrarVersion.toFloat() >= 5) {
                m_isUnrar5 = true;
                qCDebug(ARK) << "Using UNRAR 5 parser";
            } else {
                qCDebug(ARK) << "Using UNRAR 4 parser";
            }
        }  else {
            // If the second line doesn't contain an UNRAR title, something
            // is wrong.
            qCCritical(ARK) << "Failed to detect UNRAR output.";
            return false;
        }

    // Or see what version of unrar we are dealing with and call specific
    // handler functions.
    } else if (m_isUnrar5) {
        return handleUnrar5Line(line);
    } else {
        return handleUnrar4Line(line);
    }
    return true;
}

bool CliPlugin::handleUnrar5Line(const QString &line)
{
    if (line.startsWith(QLatin1String("Cannot find volume "))) {
        emit error(i18n("Failed to find all archive volumes."));
        return false;
    }

    // Parses the comment field.
    if (m_parseState == ParseStateComment) {

        // "Archive: " is printed after the comment.
        // FIXME: Comment itself could also contain the searched string.

        if (line.startsWith(QLatin1String("Archive: "))) {
            m_parseState = ParseStateHeader;
            m_comment = m_comment.trimmed();
            m_linesComment = m_comment.count(QLatin1Char('\n')) + 1;
            if (!m_comment.isEmpty()) {
                qCDebug(ARK) << "Found a comment with" << m_linesComment << "lines";
            }

        } else {
            m_comment.append(line + QLatin1Char('\n'));
        }

        return true;
    }

    // Parses the header, which is whatever is between the comment field
    // and the entries.
    else if (m_parseState == ParseStateHeader) {

        // "Details: " indicates end of header.
        if (line.startsWith(QLatin1String("Details: "))) {
            ignoreLines(1, ParseStateEntryDetails);
            if (line.contains(QLatin1String("volume"))) {
                m_numberOfVolumes++;
                if (!isMultiVolume()) {
                    setMultiVolume(true);
                    qCDebug(ARK) << "Multi-volume archive detected";
                }
            }
            if (line.contains(QLatin1String("solid")) && !m_isSolid) {
                m_isSolid = true;
                qCDebug(ARK) << "Solid archive detected";
            }
            if (line.contains(QLatin1String("RAR 4"))) {
                emit compressionMethodFound(QStringLiteral("RAR4"));
            } else if (line.contains(QLatin1String("RAR 5"))) {
                emit compressionMethodFound(QStringLiteral("RAR5"));
                m_isRAR5 = true;
            }
            if (line.contains(QLatin1String("lock"))) {
                m_isLocked = true;
            }
        }
        return true;
    }

    // Parses the entry details for each entry.
    else if (m_parseState == ParseStateEntryDetails) {

        // For multi-volume archives there is a header between the entries in
        // each volume.
        if (line.startsWith(QLatin1String("Archive: "))) {
            m_parseState = ParseStateHeader;
            return true;

        // Empty line indicates end of entry.
        } else if (line.trimmed().isEmpty() && !m_unrar5Details.isEmpty()) {
            handleUnrar5Entry();

        } else {

            // All detail lines should contain a colon.
            if (!line.contains(QLatin1Char(':'))) {
                qCWarning(ARK) << "Unrecognized line:" << line;
                return true;
            }

            // The details are on separate lines, so we store them in the QHash
            // m_unrar5Details.
            m_unrar5Details.insert(line.section(QLatin1Char(':'), 0, 0).trimmed().toLower(),
                                   line.section(QLatin1Char(':'), 1).trimmed());
        }

        return true;
    }
    return true;
}

void CliPlugin::handleUnrar5Entry()
{
    Archive::Entry *e = new Archive::Entry(this);

    QString compressionRatio = m_unrar5Details.value(QStringLiteral("ratio"));
    compressionRatio.chop(1); // Remove the '%'
    e->setProperty("ratio", compressionRatio);

    e->setProperty("timestamp", QDateTime::fromString(m_unrar5Details.value(QStringLiteral("mtime")), QStringLiteral("yyyy-MM-dd HH:mm:ss,zzz")));

    bool isDirectory = (m_unrar5Details.value(QStringLiteral("type")) == QLatin1String("Directory"));
    e->setProperty("isDirectory", isDirectory);

    if (isDirectory && !m_unrar5Details.value(QStringLiteral("name")).endsWith(QLatin1Char('/'))) {
        m_unrar5Details[QStringLiteral("name")] += QLatin1Char('/');
    }

    QString compression = m_unrar5Details.value(QStringLiteral("compression"));
    int optionPos = compression.indexOf(QLatin1Char('-'));
    if (optionPos != -1) {
        e->setProperty("method", compression.mid(optionPos));
        e->setProperty("version", compression.left(optionPos).trimmed());
    } else {
        // No method specified.
        e->setProperty("method", QString());
        e->setProperty("version", compression);
    }

    m_isPasswordProtected = m_unrar5Details.value(QStringLiteral("flags")).contains(QStringLiteral("encrypted"));
    e->setProperty("isPasswordProtected", m_isPasswordProtected);
    if (m_isPasswordProtected) {
        m_isRAR5 ? emit encryptionMethodFound(QStringLiteral("AES256")) : emit encryptionMethodFound(QStringLiteral("AES128"));
    }

    e->setProperty("fullPath", m_unrar5Details.value(QStringLiteral("name")));
    e->setProperty("size", m_unrar5Details.value(QStringLiteral("size")));
    e->setProperty("compressedSize", m_unrar5Details.value(QStringLiteral("packed size")));
    e->setProperty("permissions", m_unrar5Details.value(QStringLiteral("attributes")));
    e->setProperty("CRC", m_unrar5Details.value(QStringLiteral("crc32")));

    if (e->property("permissions").toString().startsWith(QLatin1Char('l'))) {
        e->setProperty("link", m_unrar5Details.value(QStringLiteral("target")));
    }

    m_unrar5Details.clear();
    emit entry(e);
}

bool CliPlugin::handleUnrar4Line(const QString &line)
{
    if (line.startsWith(QLatin1String("Cannot find volume "))) {
        emit error(i18n("Failed to find all archive volumes."));
        return false;
    }

    // Parses the comment field.
    if (m_parseState == ParseStateComment) {

        // RegExp matching end of comment field.
        // FIXME: Comment itself could also contain the Archive path string here.
        QRegularExpression rxCommentEnd(QStringLiteral("^(Solid archive|Archive|Volume) .+$"));

        // unrar 4 outputs the following string when opening v5 RAR archives.
        if (line == QLatin1String("Unsupported archive format. Please update RAR to a newer version.")) {
            emit error(i18n("Your unrar executable is version %1, which is too old to handle this archive. Please update to a more recent version.",
                            m_unrarVersion));
            return false;
        }

        // unrar 3 reports a non-RAR archive when opening v5 RAR archives.
        if (line.endsWith(QLatin1String(" is not RAR archive"))) {
            emit error(i18n("Unrar reported a non-RAR archive. The installed unrar version (%1) is old. Try updating your unrar.",
                            m_unrarVersion));
            return false;
        }

        // If we reach this point, then we can be sure that it's not a RAR5
        // archive, so assume RAR4.
        emit compressionMethodFound(QStringLiteral("RAR4"));

        if (rxCommentEnd.match(line).hasMatch()) {

            if (line.startsWith(QLatin1String("Volume "))) {
                m_numberOfVolumes++;
                if (!isMultiVolume()) {
                    setMultiVolume(true);
                    qCDebug(ARK) << "Multi-volume archive detected";
                }
            }
            if (line.startsWith(QLatin1String("Solid archive")) && !m_isSolid) {
                m_isSolid = true;
                qCDebug(ARK) << "Solid archive detected";
            }

            m_parseState = ParseStateHeader;
            m_comment = m_comment.trimmed();
            m_linesComment = m_comment.count(QLatin1Char('\n')) + 1;
            if (!m_comment.isEmpty()) {
                qCDebug(ARK) << "Found a comment with" << m_linesComment << "lines";
            }

        } else {
            m_comment.append(line + QLatin1Char('\n'));
        }

        return true;
    }

    // Parses the header, which is whatever is between the comment field
    // and the entries.
    else if (m_parseState == ParseStateHeader) {

        // Horizontal line indicates end of header.
        if (line.startsWith(QLatin1String("--------------------"))) {
            m_parseState = ParseStateEntryFileName;
        } else if (line.startsWith(QLatin1String("Volume "))) {
            m_numberOfVolumes++;
        } else if (line == QLatin1String("Lock is present")) {
            m_isLocked = true;
        }
        return true;
    }

    // Parses the entry name, which is on the first line of each entry.
    else if (m_parseState == ParseStateEntryFileName) {

        // Ignore empty lines.
        if (line.trimmed().isEmpty()) {
            return true;
        }

        // Three types of subHeaders can be displayed for unrar 3 and 4.
        // STM has 4 lines, RR has 3, and CMT has lines corresponding to
        // length of comment field +3. We ignore the subheaders.
        QRegularExpression rxSubHeader(QStringLiteral("^Data header type: (CMT|STM|RR)$"));
        QRegularExpressionMatch matchSubHeader = rxSubHeader.match(line);
        if (matchSubHeader.hasMatch()) {
            qCDebug(ARK) << "SubHeader of type" << matchSubHeader.captured(1) << "found";
            if (matchSubHeader.captured(1) == QLatin1String("STM")) {
                ignoreLines(4, ParseStateEntryFileName);
            } else if (matchSubHeader.captured(1) == QLatin1String("CMT")) {
                ignoreLines(m_linesComment + 3, ParseStateEntryFileName);
            } else if (matchSubHeader.captured(1) == QLatin1String("RR")) {
                ignoreLines(3, ParseStateEntryFileName);
            }
            return true;
        }

        // The entries list ends with a horizontal line, followed by a
        // single summary line or, for multi-volume archives, another header.
        if (line.startsWith(QLatin1String("-----------------"))) {
            m_parseState = ParseStateHeader;
            return true;

        // Encrypted files are marked with an asterisk.
        } else if (line.startsWith(QLatin1Char('*'))) {
            m_isPasswordProtected = true;
            m_unrar4Details.append(QString(line.trimmed()).remove(0, 1)); //Remove the asterisk
            emit encryptionMethodFound(QStringLiteral("AES128"));

        // Entry names always start at the second position, so a line not
        // starting with a space is not an entry name.
        } else if (!line.startsWith(QLatin1Char(' '))) {
            qCWarning(ARK) << "Unrecognized line:" << line;
            return true;

        // If we reach this, then we can assume the line is an entry name, so
        // save it, and move on to the rest of the entry details.
        } else {
            m_unrar4Details.append(line.trimmed());
        }

        m_parseState = ParseStateEntryDetails;

        return true;
    }

    // Parses the remainder of the entry details for each entry.
    else if (m_parseState == ParseStateEntryDetails) {

        // If the line following an entry name is empty, we did something
        // wrong.
        Q_ASSERT(!line.trimmed().isEmpty());

        // If we reach a horizontal line, then the previous line was not an
        // entry name, so go back to header.
        if (line.startsWith(QLatin1String("-----------------"))) {
            m_parseState = ParseStateHeader;
            return true;
        }

        // In unrar 3 and 4 the details are on a single line, so we
        // pass a QStringList containing the details. We need to store
        // it due to symlinks (see below).
        m_unrar4Details.append(line.split(QLatin1Char(' '),
                                          QString::SkipEmptyParts));

        // The details line contains 9 fields, so m_unrar4Details
        // should now contain 9 + the filename = 10 strings. If not, this is
        // not an archive entry.
        if (m_unrar4Details.size() != 10) {
            m_parseState = ParseStateHeader;
            return true;
        }

        // When unrar 3 and 4 list a symlink, they output an extra line
        // containing the link target. The extra line is output after
        // the line we ignore, so we first need to ignore one line.
        if (m_unrar4Details.at(6).startsWith(QLatin1Char('l'))) {
            ignoreLines(1, ParseStateLinkTarget);
            return true;
        } else {
            handleUnrar4Entry();
        }

        // Unrar 3 & 4 show a third line for each entry, which contains
        // three details: Host OS, Solid, and Old. We can ignore this
        // line.
        ignoreLines(1, ParseStateEntryFileName);

        return true;
    }

    // Parses a symlink target.
    else if (m_parseState == ParseStateLinkTarget) {

        m_unrar4Details.append(QString(line).remove(QStringLiteral("-->")).trimmed());
        handleUnrar4Entry();

        m_parseState = ParseStateEntryFileName;
        return true;
    }
    return true;
}

void CliPlugin::handleUnrar4Entry()
{
    Archive::Entry *e = new Archive::Entry(this);

    QDateTime ts = QDateTime::fromString(QString(m_unrar4Details.at(4) + QLatin1Char(' ') + m_unrar4Details.at(5)),
                                         QStringLiteral("dd-MM-yy hh:mm"));
    // Unrar 3 & 4 output dates with a 2-digit year but QDateTime takes it as
    // 19??. Let's take 1950 as cut-off; similar to KDateTime.
    if (ts.date().year() < 1950) {
        ts = ts.addYears(100);
    }
    e->setProperty("timestamp", ts);

    bool isDirectory = ((m_unrar4Details.at(6).at(0) == QLatin1Char('d')) ||
                        (m_unrar4Details.at(6).at(1) == QLatin1Char('D')));
    e->setProperty("isDirectory", isDirectory);

    if (isDirectory && !m_unrar4Details.at(0).endsWith(QLatin1Char('/'))) {
        m_unrar4Details[0] += QLatin1Char('/');
    }

    // Unrar reports the ratio as ((compressed size * 100) / size);
    // we consider ratio as (100 * ((size - compressed size) / size)).
    // If the archive is a multivolume archive, a string indicating
    // whether the archive's position in the volume is displayed
    // instead of the compression ratio.
    QString compressionRatio = m_unrar4Details.at(3);
    if ((compressionRatio == QStringLiteral("<--")) ||
        (compressionRatio == QStringLiteral("<->")) ||
        (compressionRatio == QStringLiteral("-->"))) {
        compressionRatio = QLatin1Char('0');
    } else {
        compressionRatio.chop(1); // Remove the '%'
    }
    e->setProperty("ratio", compressionRatio);

    // TODO:
    // - Permissions differ depending on the system the entry was added
    //   to the archive.
    e->setProperty("fullPath", m_unrar4Details.at(0));
    e->setProperty("size", m_unrar4Details.at(1));
    e->setProperty("compressedSize", m_unrar4Details.at(2));
    e->setProperty("permissions", m_unrar4Details.at(6));
    e->setProperty("CRC", m_unrar4Details.at(7));
    e->setProperty("method", m_unrar4Details.at(8));
    e->setProperty("version", m_unrar4Details.at(9));
    e->setProperty("isPasswordProtected", m_isPasswordProtected);

    if (e->property("permissions").toString().startsWith(QLatin1Char('l'))) {
        e->setProperty("link", m_unrar4Details.at(10));
    }

    m_unrar4Details.clear();
    emit entry(e);
}

bool CliPlugin::readExtractLine(const QString &line)
{
    const QRegularExpression rxCRC(QStringLiteral("CRC failed"));
    if (rxCRC.match(line).hasMatch()) {
        emit error(i18n("One or more wrong checksums"));
        return false;
    }

    if (line.startsWith(QLatin1String("Cannot find volume "))) {
        emit error(i18n("Failed to find all archive volumes."));
        return false;
    }

    return true;
}

bool CliPlugin::hasBatchExtractionProgress() const
{
    return true;
}

void CliPlugin::ignoreLines(int lines, ParseState nextState)
{
    m_remainingIgnoreLines = lines;
    m_parseState = nextState;
}

bool CliPlugin::isPasswordPrompt(const QString &line)
{
    return line.startsWith(QLatin1String("Enter password (will not be echoed) for"));
}

bool CliPlugin::isWrongPasswordMsg(const QString &line)
{
    return (line.contains(QLatin1String("password incorrect")) || line.contains(QLatin1String("wrong password")));
}

bool CliPlugin::isCorruptArchiveMsg(const QString &line)
{
    return (line == QLatin1String("Unexpected end of archive") ||
            line.contains(QLatin1String("the file header is corrupt")) ||
            line.endsWith(QLatin1String("checksum error")));
}

bool CliPlugin::isDiskFullMsg(const QString &line)
{
    return line.contains(QLatin1String("No space left on device"));
}

bool CliPlugin::isFileExistsMsg(const QString &line)
{
    return (line == QLatin1String("[Y]es, [N]o, [A]ll, n[E]ver, [R]ename, [Q]uit "));
}

bool CliPlugin::isFileExistsFileName(const QString &line)
{
    return (line.startsWith(QLatin1String("Would you like to replace the existing file ")) || // unrar 5
            line.contains(QLatin1String(" already exists. Overwrite it"))); // unrar 3 & 4
}

bool CliPlugin::isLocked() const
{
    return m_isLocked;
}

#include "cliplugin.moc"
