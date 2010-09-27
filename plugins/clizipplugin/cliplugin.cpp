/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

class CliPlugin: public CliInterface
{
public:
    explicit CliPlugin(QObject *parent, const QVariantList & args)
            : CliInterface(parent, args),
            m_status(Header) {
        // #208091: infozip applies special meanings to some characters
        //          see match.c in infozip's source code
        setEscapedCharacters(QLatin1String("[]*?^-\\!"));
    }

    virtual ~CliPlugin() {

    }

    virtual ParameterList parameterList() const {
        static ParameterList p;
        if (p.isEmpty()) {

            p[CaptureProgress] = false;
            p[ListProgram] = QLatin1String( "zipinfo" );
            p[ExtractProgram] = QLatin1String( "unzip" );
            p[DeleteProgram] = p[AddProgram] = QLatin1String( "zip" );

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
                                 << QLatin1String( "N" ) //cancel
                                 ;

            p[AddArgs] = QStringList() << QLatin1String( "-r" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

            p[WrongPasswordPatterns] = QStringList() << QLatin1String( "incorrect password" );
            //p[ExtractionFailedPatterns] = QStringList() << "CRC failed";
        }
        return p;
    }

    QString m_entryFilename, m_internalId;
    ArchiveEntry m_currentEntry;

    enum ReadStatus {
        Header = 0,
        Entry
    };

    ReadStatus m_status;

    bool readListLine(const QString &line) {
        static QRegExp entryPattern(QLatin1String(
            "^(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\d{8}).(\\d{6})\\s+(.+)$") );

        int i;
        switch (m_status) {
        case Header:
            m_status = Entry;
            break;
        case Entry:
            i = entryPattern.indexIn(line);
            if (i != -1) {
                ArchiveEntry e;
                e[Permissions] = entryPattern.cap(1);
                e[IsDirectory] = (entryPattern.cap(1).at(0) == QLatin1Char( 'd' ));
                e[Size] = entryPattern.cap(4).toInt();
                QString status = entryPattern.cap(5);
                if (status[0].isUpper())
                    e[IsPasswordProtected] = true;
                e[CompressedSize] = entryPattern.cap(6).toInt();

                const QDateTime ts(QDate::fromString(entryPattern.cap(8), QLatin1String( "yyyyMMdd" )),
                                   QTime::fromString(entryPattern.cap(9), QLatin1String( "hhmmss" )));
                e[Timestamp] = ts;

                e[FileName] = e[InternalID] = entryPattern.cap(10);
                entry(e);
            }
            break;
        }
        return true;
    }
};

KERFUFFLE_EXPORT_PLUGIN(CliPlugin)

