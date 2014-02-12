/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2010-2011,2014 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include <KDebug>

#include <QDateTime>
#include <QDir>
#include <QString>
#include <QStringList>

using namespace Kerfuffle;

CliPlugin::CliPlugin(QObject *parent, const QVariantList& args)
        : CliInterface(parent, args)
        , m_parseState(ParseStateColumnDescription1)
        , m_isPasswordProtected(false)
        , m_remainingIgnoredSubHeaderLines(0)
        , m_remainingIgnoredDetailsLines(0)
        , m_isUnrarFree(false)
        , m_isUnrarVersion5(false)
{
}

CliPlugin::~CliPlugin()
{
}

// #272281: the proprietary unrar program does not like trailing '/'s
//          in directories passed to it when extracting only part of
//          the files in an archive.
QString CliPlugin::escapeFileName(const QString &fileName) const
{
    if (fileName.endsWith(QLatin1Char('/'))) {
        return fileName.left(fileName.length() - 1);
    }

    return fileName;
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[CaptureProgress] = true;
        p[ListProgram] = p[ExtractProgram] = QStringList() << QLatin1String( "unrar" );
        p[DeleteProgram] = p[AddProgram] = QStringList() << QLatin1String( "rar" );

        p[ListArgs] = QStringList() << QLatin1String( "vt" ) << QLatin1String( "-c-" ) << QLatin1String( "-v" ) << QLatin1String( "$Archive" );
        p[ExtractArgs] = QStringList() << QLatin1String( "-kb" ) << QLatin1String( "-p-" )
                                       << QLatin1String( "$PreservePathSwitch" )
                                       << QLatin1String( "$PasswordSwitch" )
                                       << QLatin1String( "$RootNodeSwitch" )
                                       << QLatin1String( "$Archive" )
                                       << QLatin1String( "$Files" );
        p[PreservePathSwitch] = QStringList() << QLatin1String( "x" ) << QLatin1String( "e" );
        p[RootNodeSwitch] = QStringList() << QLatin1String( "-ap$Path" );
        p[PasswordSwitch] = QStringList() << QLatin1String( "-p$Password" );

        p[DeleteArgs] = QStringList() << QLatin1String( "d" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        p[FileExistsExpression] = QLatin1String( "^(.+) already exists. Overwrite it" );
        p[FileExistsInput] = QStringList()
                             << QLatin1String( "Y" ) //overwrite
                             << QLatin1String( "N" ) //skip
                             << QLatin1String( "A" ) //overwrite all
                             << QLatin1String( "E" ) //autoskip
                             << QLatin1String( "Q" ) //cancel
                             ;

        p[AddArgs] = QStringList() << QLatin1String( "a" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        p[PasswordPromptPattern] = QLatin1String("Enter password \\(will not be echoed\\) for");

        p[WrongPasswordPatterns] = QStringList() << QLatin1String("password incorrect") << QLatin1String("wrong password");
        p[ExtractionFailedPatterns] = QStringList() << QLatin1String( "CRC failed" ) << QLatin1String( "Cannot find volume" );
    }

    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    static const QLatin1String headerString("----------------------");
    static const QLatin1String subHeaderString("Data header type: ");
    static const QLatin1String columnDescription1String("                  Size   Packed Ratio  Date   Time     Attr      CRC   Meth Ver");
    static const QLatin1String columnDescription2String("               Host OS    Solid   Old"); // Only present in unrar-nonfree

    if (m_isUnrarVersion5) {
        int colonPos = line.indexOf(QLatin1Char(':'));
        if (colonPos == -1) {
            if (m_entryFileName.isEmpty()) {
                return true;
            }
            ArchiveEntry e;

            QString compressionRatio = m_entryDetails.value(QLatin1String("ratio"));
            compressionRatio.chop(1); // Remove the '%'

            QString time = m_entryDetails.value(QLatin1String("mtime"));
            // FIXME unrar 5 beta 8 seems to lack the seconds, or the trailing ,000 is not the milliseconds
            QDateTime ts = QDateTime::fromString(time, QLatin1String("yyyy-MM-dd HH:mm,zzz"));

            bool isDirectory = m_entryDetails.value(QLatin1String("type")) == QLatin1String("Directory");
            if (isDirectory && !m_entryFileName.endsWith(QLatin1Char( '/' ))) {
                m_entryFileName += QLatin1Char( '/' );
            }

            QString compression = m_entryDetails.value(QLatin1String("compression"));
            int optionPos = compression.indexOf(QLatin1Char('-'));
            if (optionPos != -1) {
                e[Method] = compression.mid(optionPos);
                e[Version] = compression.left(optionPos).trimmed();
            } else {
                // no method specified
                e[Method].clear();
                e[Version] = compression;
            }

            m_isPasswordProtected = m_entryDetails.value(QLatin1String("flags")).contains(QLatin1String("encrypted"));

            e[FileName] = m_entryFileName;
            e[InternalID] = m_entryFileName;
            e[Size] = m_entryDetails.value(QLatin1String("size"));
            e[CompressedSize] = m_entryDetails.value(QLatin1String("packed size"));
            e[Ratio] = compressionRatio;
            e[Timestamp] = ts;
            e[IsDirectory] = isDirectory;
            e[Permissions] = m_entryDetails.value(QLatin1String("attributes"));
            e[CRC] = m_entryDetails.value(QLatin1String("crc32"));
            e[IsPasswordProtected] = m_isPasswordProtected;
            kDebug() << "Added entry: " << e;

            emit entry(e);

            m_entryFileName.clear();

            return true;
        }

        QString key = line.left(colonPos).trimmed().toLower();
        QString value = line.mid(colonPos + 2);

        if (key == QLatin1String("name")) {
            m_entryFileName = value;
            m_entryDetails.clear();
            return true;
        }

        // in multivolume archives, the split CRC32 is denoted specially
        if (key == QLatin1String("pack-crc32")) {
            key = key.mid(5);
        }

        m_entryDetails.insert(key, value);

        return true;
    }

    switch (m_parseState)
    {
    case ParseStateColumnDescription1:
        if (line.startsWith(QLatin1String("Details:"))) {
            m_isUnrarVersion5 = true;
            setListEmptyLines(true);
            // no previously detected entry
            m_entryFileName.clear();
        }
        if (line.startsWith(columnDescription1String)) {
            m_parseState = ParseStateColumnDescription2;
        }

        break;

    case ParseStateColumnDescription2:
        // #243273: We need a way to differentiate unrar and unrar-free,
        //          as their output for the "vt" option is different.
        //          Currently, we differ them by checking if "vt" produces
        //          two lines of column names before the header string, as
        //          only unrar does that (unrar-free always outputs one line
        //          for column names regardless of how verbose we tell it to
        //          be).
        if (line.startsWith(columnDescription2String)) {
            m_parseState = ParseStateHeader;
        } else if (line.startsWith(headerString)) {
            m_parseState = ParseStateEntryFileName;
            m_isUnrarFree = true;
        }

        break;

    case ParseStateHeader:
        if (line.startsWith(headerString)) {
            m_parseState = ParseStateEntryFileName;
        }

        break;

    case ParseStateEntryFileName:
        if (m_remainingIgnoredSubHeaderLines > 0) {
            --m_remainingIgnoredSubHeaderLines;
            return true;
        }

        // #242071: The RAR file format has the concept of service headers,
        //          such as CMT (comments), STM (NTFS alternate data streams)
        //          and RR (recovery record). These service headers do no
        //          interest us, and ignoring them seems harmless (at least
        //          7zip and WinRAR do not show them either).
        if (line.startsWith(subHeaderString)) {
            // subHeaderString's length is 18
            const QString subHeaderType(line.mid(18));

            // XXX: If we ever support archive comments, this code must
            //      be changed, because the comments will be shown after
            //      a CMT subheader and will have an arbitrary number of lines
            if (subHeaderType == QLatin1String("STM")) {
                m_remainingIgnoredSubHeaderLines = 4;
            } else {
                m_remainingIgnoredSubHeaderLines = 3;
            }

            kDebug() << "Found a subheader of type" << subHeaderType;
            kDebug() << "The next" << m_remainingIgnoredSubHeaderLines
                     << "lines will be ignored";

            return true;
        } else if (line.startsWith(headerString)) {
            m_parseState = ParseStateHeader;

            return true;
        }

        m_isPasswordProtected = (line.at(0) == QLatin1Char( '*' ));

        // Start from 1 because the first character is either ' ' or '*'
        m_entryFileName = QDir::fromNativeSeparators(line.mid(1));

        m_parseState = ParseStateEntryDetails;

        break;

    case ParseStateEntryIgnoredDetails:
        if (m_remainingIgnoredDetailsLines > 0) {
            --m_remainingIgnoredDetailsLines;
            return true;
        }
        m_parseState = ParseStateEntryFileName;

        break;

    case ParseStateEntryDetails:
        if (line.startsWith(headerString)) {
            m_parseState = ParseStateHeader;
            return true;
        }

        const QStringList details = line.split(QLatin1Char( ' ' ),
                                               QString::SkipEmptyParts);

        QDateTime ts(QDate::fromString(details.at(3),
                                       QLatin1String("dd-MM-yy")),
                     QTime::fromString(details.at(4),
                                       QLatin1String("hh:mm")));

        // unrar outputs dates with a 2-digit year but QDate takes it as 19??
        // let's take 1950 is cut-off; similar to KDateTime
        if (ts.date().year() < 1950) {
            ts = ts.addYears(100);
        }

        bool isDirectory = ((details.at(5).at(0) == QLatin1Char( 'd' )) ||
                            (details.at(5).at(1) == QLatin1Char( 'D' )));
        if (isDirectory && !m_entryFileName.endsWith(QLatin1Char( '/' ))) {
            m_entryFileName += QLatin1Char( '/' );
        }

        // If the archive is a multivolume archive, a string indicating
        // whether the archive's position in the volume is displayed
        // instead of the compression ratio.
        QString compressionRatio = details.at(2);
        if ((compressionRatio == QLatin1String("<--")) ||
            (compressionRatio == QLatin1String("<->")) ||
            (compressionRatio == QLatin1String("-->"))) {
            compressionRatio = QLatin1Char( '0' );
        } else {
            compressionRatio.chop(1); // Remove the '%'
        }

        // TODO:
        // - Permissions differ depending on the system the entry was added
        //   to the archive.
        // - unrar reports the ratio as ((compressed size * 100) / size);
        //   we consider ratio as (100 * ((size - compressed size) / size)).
        ArchiveEntry e;
        e[FileName] = m_entryFileName;
        e[InternalID] = m_entryFileName;
        e[Size] = details.at(0);
        e[CompressedSize] = details.at(1);
        e[Ratio] = compressionRatio;
        e[Timestamp] = ts;
        e[IsDirectory] = isDirectory;
        e[Permissions] = details.at(5);
        e[CRC] = details.at(6);
        e[Method] = details.at(7);
        e[Version] = details.at(8);
        e[IsPasswordProtected] = m_isPasswordProtected;
        kDebug() << "Added entry: " << e;

        // #314297: When RAR 3.x and RAR 4.x list a symlink, they output an
        //          extra line after the "Host OS/Solid/Old" one mentioning the
        //          target of the symlink in question. We are not interested in
        //          this line at the moment, so we just tell the parser to skip
        //          it.
        if (e[Permissions].toString().startsWith(QLatin1Char('l'))) {
            m_remainingIgnoredDetailsLines = 1;
        } else {
            m_remainingIgnoredDetailsLines = 0;
        }

        emit entry(e);

        // #243273: unrar-free does not output the third file entry line,
        //          skip directly to parsing a new entry.
        if (m_isUnrarFree) {
            m_parseState = ParseStateEntryFileName;
        } else {
            m_parseState = ParseStateEntryIgnoredDetails;
        }

        break;
    }

    return true;
}

KERFUFFLE_EXPORT_PLUGIN(CliPlugin)

#include "cliplugin.moc"
