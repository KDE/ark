/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2011 Intzoglou Theofilos <int.teo@gmail.com>
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

#include "kerfuffle/kerfuffle_export.h"
#include <QDate>
#include <QTime>
#include <kdebug.h>

using namespace Kerfuffle;

CliPlugin::CliPlugin(QObject *parent, const QVariantList &args)
            : CliInterface(parent, args),
            m_status(Header),
            m_firstLine(true)
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
        p[ListProgram] = p[ExtractProgram] = p[DeleteProgram] = p[AddProgram] = QStringList() << QLatin1String("lha");

        p[ListArgs] = QStringList() << QLatin1String("vv") << QLatin1String("$Archive");
        p[ExtractArgs] = QStringList() << QLatin1String("ev") << QLatin1String("$PreservePathSwitch") << QLatin1String("$Archive") << QLatin1String("$Files");

        p[DeleteArgs] = QStringList() << QLatin1String("dv") << QLatin1String("$Archive") << QLatin1String("$Files");

        p[FileExistsExpression] = QLatin1String("^(.+) OverWrite \\?");
        p[FileExistsMode] = 1; // Watch for messages in stdout
        p[FileExistsInput] = QStringList()
                                << QLatin1String("Y") //overwrite
                                << QLatin1String("N") //skip
                                << QLatin1String("A") //overwrite all
                                << QLatin1String("S") //autoskip
                                ;

        p[AddArgs] = QStringList() << QLatin1String("av") << QLatin1String("$Archive") << QLatin1String("$Files");

        p[ExtractionFailedPatterns] = QStringList() << QLatin1String("Error");
        p[PreservePathSwitch] = QStringList() << QLatin1String( "" ) << QLatin1String( "-i" );
    }
    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    const QString m_headerString = QLatin1String("----------");

    switch(m_status) {
        case Header:
            if (line.startsWith(m_headerString)) {
                m_status = Entry;
                m_firstLine = true;
            }
            break;
        case Entry:
            const QStringList entryList = line.split(QLatin1Char(' '), QString::SkipEmptyParts);

            if (m_firstLine) { // This line will contain the filename
                if (entryList.count() == 8) { // End of entries
                    m_status = Header;
                }
                else {
                    m_internalId = line;
                    m_firstLine = false;
                }
            }
            else { // This line contains the rest of the information
                ArchiveEntry e;

                if (!entryList[0].startsWith(QLatin1Char('['))) {
                    e[Permissions] = entryList[0];
                }

                e[IsDirectory] = m_internalId.endsWith(QLatin1Char('/'));
                m_entryFilename = m_internalId;
                e[FileName] = autoConvertEncoding(m_entryFilename);
                e[InternalID] = m_internalId;

                if (entryList.count() == 10) { // UID/GID is missing
                    e[CompressedSize] = entryList[1];
                    e[Size] = entryList[2];
                    e[Ratio] = entryList[3];
                    e[Method] = entryList[4];
                    e[CRC] = entryList[5];

                    if (entryList[8].indexOf(QLatin1Char(':')) != -1) {
                        e[Timestamp] = QDateTime(QDate::fromString(QString().setNum(QDate::currentDate().year())+QLatin1Char('-')+entryList[6]+QLatin1Char('-')+entryList[7], QLatin1String("yyyy-MMM-d")),
                                                 QTime::fromString(entryList[8], QLatin1String("hh:mm")));
                    } else {
                        e[Timestamp] = QDateTime(QDate::fromString(entryList[8]+QLatin1Char('-')+entryList[6]+QLatin1Char('-')+entryList[7], QLatin1String("yyyy-MMM-d")),
                                                 QTime());
                    }
                    entry(e);
                }
                else if (entryList.count() == 11) { // All info is available
                    const QStringList ownerList = entryList[1].split(QLatin1Char('/')); // Split uid from gid
                    e[Owner] = ownerList.at(0);
                    e[Group] = ownerList.at(1);
                    e[CompressedSize] = entryList[2];
                    e[Size] = entryList[3];
                    e[Ratio] = entryList[4];
                    e[Method] = entryList[5];
                    e[CRC] = entryList[6];

                    if (entryList[9].indexOf(QLatin1Char(':')) != -1) {
                        e[Timestamp] = QDateTime(QDate::fromString(QString().setNum(QDate::currentDate().year())+QLatin1Char('-')+entryList[7]+QLatin1Char('-')+entryList[8], QLatin1String("yyyy-MMM-d")),
                                                 QTime::fromString(entryList[9], QLatin1String("hh:mm")));
                    } else {
                        e[Timestamp] = QDateTime(QDate::fromString(entryList[9]+QLatin1Char('-')+entryList[7]+QLatin1Char('-')+entryList[8], QLatin1String("yyyy-MMM-d")),
                                                 QTime());
                    }
                    entry(e);
                }

                m_firstLine = true;
            }
        break;
    }
    return true;
}

void CliPlugin::resetReadState()
{
    m_status = Header;
}

KERFUFFLE_EXPORT_PLUGIN(CliPlugin)

