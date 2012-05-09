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
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/kerfuffle_export.h"

#include <KDebug>

#include <QDateTime>
#include <QDir>
#include <QLatin1String>
#include <QRegExp>
#include <QString>
#include <QStringList>

using namespace Kerfuffle;

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
    : CliInterface(parent, args)
    , m_status(Header)
{
}

CliPlugin::~CliPlugin()
{
}

// #208091: infozip applies special meanings to some characters, so we
//          need to escape them with backslashes.see match.c in
//          infozip's source code
QString CliPlugin::escapeFileName(const QString &fileName) const
{
    const QString escapedCharacters(QLatin1String("[]*?^-\\!"));

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
        p[ListProgram] = QStringList() << QLatin1String( "zipinfo" );
        p[ExtractProgram] = QStringList() << QLatin1String( "unzip" );
        p[DeleteProgram] = p[AddProgram] = QStringList() << QLatin1String( "zip" );

        p[ListArgs] = QStringList() << QLatin1String( "-l" ) << QLatin1String( "-T" ) << QLatin1String( "$Archive" );
        p[ExtractArgs] = QStringList() << QLatin1String( "$PreservePathSwitch" ) << QLatin1String( "$PasswordSwitch" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );
        p[PreservePathSwitch] = QStringList() << QLatin1String( "" ) << QLatin1String( "-j" );
        p[PasswordSwitch] = QStringList() << QLatin1String( "-P$Password" );

        p[DeleteArgs] = QStringList() << QLatin1String( "-d" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        p[FileExistsExpression] = QLatin1String( "^replace (.+)\\?" );
        p[FileExistsInput] = QStringList()
                             << QLatin1String( "y" ) //overwrite
                             << QLatin1String( "n" ) //skip
                             << QLatin1String( "A" ) //overwrite all
                             << QLatin1String( "N" ) //autoskip
                             ;

        p[AddArgs] = QStringList() << QLatin1String( "-r" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        p[PasswordPromptPattern] = QLatin1String(" password: ");
        p[WrongPasswordPatterns] = QStringList() << QLatin1String( "incorrect password" );
        //p[ExtractionFailedPatterns] = QStringList() << "CRC failed";
    }
    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    static const QRegExp entryPattern(QLatin1String(
        "^(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\d{8}).(\\d{6})\\s+(.+)$") );

    switch (m_status) {
    case Header:
        m_status = Entry;
        break;
    case Entry:
        if (entryPattern.indexIn(line) != -1) {
            ArchiveEntry e;
            e[Permissions] = entryPattern.cap(1);

            // #280354: infozip may not show the right attributes for a given directory, so an entry
            //          ending with '/' is actually more reliable than 'd' bein in the attributes.
            e[IsDirectory] = entryPattern.cap(10).endsWith(QLatin1Char('/'));

            e[Size] = entryPattern.cap(4).toInt();
            QString status = entryPattern.cap(5);
            if (status[0].isUpper()) {
                e[IsPasswordProtected] = true;
            }
            e[CompressedSize] = entryPattern.cap(6).toInt();

            const QDateTime ts(QDate::fromString(entryPattern.cap(8), QLatin1String( "yyyyMMdd" )),
                               QTime::fromString(entryPattern.cap(9), QLatin1String( "hhmmss" )));
            e[Timestamp] = ts;

            e[FileName] = e[InternalID] = entryPattern.cap(10);
            emit entry(e);
        }
        break;
    }

    return true;
}

KERFUFFLE_EXPORT_PLUGIN(CliPlugin)

