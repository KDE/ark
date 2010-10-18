/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2010 Raphael Kubo da Costa <kubito@gmail.com>
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
        , m_parseState(ParseStateHeader)
        , m_remainingIgnoredSubHeaderLines(0)
{
}

CliPlugin::~CliPlugin()
{
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[CaptureProgress] = true;
        p[ListProgram] = p[ExtractProgram] = QLatin1String( "unrar" );
        p[DeleteProgram] = p[AddProgram] = QLatin1String( "rar" );

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

        p[WrongPasswordPatterns] = QStringList() << QLatin1String( "password incorrect" );
        p[ExtractionFailedPatterns] = QStringList() << QLatin1String( "CRC failed" ) << QLatin1String( "Cannot find volume" );
    }

    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    static const QLatin1String headerString("----------------------");
    static const QLatin1String subHeaderString("Data header type: ");

    if (line.startsWith(headerString)) {
        if (m_parseState == ParseStateHeader) { // Skip the heading
            m_parseState = ParseStateEntryFileName;
        } else { // Catch the final line
            m_parseState = ParseStateHeader;
        }

        return true;
    }

    if (m_parseState == ParseStateHeader) {
        return true;
    } else if (m_parseState == ParseStateEntryIgnoredDetails) {
        m_parseState = ParseStateEntryFileName;
        return true;
    }

    if (m_remainingIgnoredSubHeaderLines > 0) {
        --m_remainingIgnoredSubHeaderLines;
        return true;
    }

    // #242071: The RAR file format has the concept of subheaders, such as
    //          CMT for comments and STM for NTFS streams (?).
    //          Since the format is undocumented, we cannot do much, and
    //          ignoring them seems harmless (at least 7zip and WinRAR do not
    //          show them either).
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
    }

    if (m_parseState == ParseStateEntryFileName) {
        m_isPasswordProtected = (line.at(0) == QLatin1Char( '*' ));

        // Start from 1 because the first character is either ' ' or '*'
        m_entryFileName = QDir::fromNativeSeparators(line.mid(1));

        m_parseState = ParseStateEntryDetails;

        return true;
    } else if (m_parseState == ParseStateEntryDetails) {
        const QStringList details = line.split(QLatin1Char( ' ' ), QString::SkipEmptyParts);

        QDateTime ts(QDate::fromString(details.at(3), QLatin1String( "dd-MM-yy" )),
                     QTime::fromString(details.at(4), QLatin1String( "hh:mm" )));

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

        entry(e);

        m_parseState = ParseStateEntryIgnoredDetails;
    }

    return true;
}

KERFUFFLE_EXPORT_PLUGIN(CliPlugin)

#include "cliplugin.moc"
