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

#include <QDateTime>
#include <QDir>
#include <QString>
#include <QStringList>

#include <KDebug>

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
        p[ListProgram] = p[ExtractProgram] = "unrar";
        p[DeleteProgram] = p[AddProgram] = "rar";

        p[ListArgs] = QStringList() << "vt" << "-c-" << "-v" << "$Archive";
        p[ExtractArgs] = QStringList() << "-kb" << "-p-"
                                       << "$PreservePathSwitch"
                                       << "$PasswordSwitch"
                                       << "$RootNodeSwitch" << "$Archive"
                                       << "$Files";
        p[PreservePathSwitch] = QStringList() << "x" << "e";
        p[RootNodeSwitch] = QStringList() << "-ap$Path";
        p[PasswordSwitch] = QStringList() << "-p$Password";

        p[DeleteArgs] = QStringList() << "d" << "$Archive" << "$Files";

        p[FileExistsExpression] = "^(.+) already exists. Overwrite it";
        p[FileExistsInput] = QStringList()
                             << "Y" //overwrite
                             << "N" //skip
                             << "A" //overwrite all
                             << "E" //autoskip
                             << "Q" //cancel
                             ;

        p[AddArgs] = QStringList() << "a" << "$Archive" << "$Files";

        p[WrongPasswordPatterns] = QStringList() << "password incorrect";
        p[ExtractionFailedPatterns] = QStringList() << "CRC failed" << "Cannot find volume";
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
        if (subHeaderType == QLatin1String("STM"))
            m_remainingIgnoredSubHeaderLines = 4;
        else
            m_remainingIgnoredSubHeaderLines = 3;

        kDebug() << "Found a subheader of type" << subHeaderType;
        kDebug() << "The next" << m_remainingIgnoredSubHeaderLines
                 << "lines will be ignored";

        return true;
    }

    if (m_parseState == ParseStateEntryFileName) {
        m_isPasswordProtected = (line.at(0) == '*');

        // Start from 1 because the first character is either ' ' or '*'
        m_entryFileName = QDir::fromNativeSeparators(line.mid(1));

        m_parseState = ParseStateEntryDetails;

        return true;
    } else if (m_parseState == ParseStateEntryDetails) {
        const QStringList details = line.split(' ', QString::SkipEmptyParts);

        QDateTime ts(QDate::fromString(details.at(3), "dd-MM-yy"),
                     QTime::fromString(details.at(4), "hh:mm"));

        // unrar outputs dates with a 2-digit year but QDate takes it as 19??
        // let's take 1950 is cut-off; similar to KDateTime
        if (ts.date().year() < 1950)
            ts = ts.addYears(100);

        bool isDirectory = ((details.at(5).at(0) == 'd') ||
                            (details.at(5).at(1) == 'D'));
        if (isDirectory && !m_entryFileName.endsWith('/')) {
            m_entryFileName += '/';
        }

        // If the archive is a multivolume archive, a string indicating
        // whether the archive's position in the volume is displayed
        // instead of the compression ratio.
        QString compressionRatio = details.at(2);
        if ((compressionRatio == QLatin1String("<--")) ||
            (compressionRatio == QLatin1String("<->")) ||
            (compressionRatio == QLatin1String("-->"))) {
            compressionRatio = '0';
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
