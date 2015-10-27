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
#include "ark_debug.h"
#include "kerfuffle/kerfuffle_export.h"

#include <QDate>
#include <QTime>

#include <KPluginFactory>

using namespace Kerfuffle;

K_PLUGIN_FACTORY( CliPluginFactory, registerPlugin< CliPlugin >(); )

CliPlugin::CliPlugin(QObject *parent, const QVariantList &args)
            : CliInterface(parent, args),
            m_parseState(ParseStateHeader),
            m_firstLine(true)
{
    qCDebug(ARK) << "Loaded cli_7z plugin";
}

CliPlugin::~CliPlugin()
{
}

void CliPlugin::resetParsing()
{
    m_parseState = ParseStateHeader;
    m_comment.clear();
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;
    if (p.isEmpty()) {
        p[CaptureProgress] = true;
        p[ListProgram] = p[ExtractProgram] = p[DeleteProgram] = p[AddProgram] = QStringList() << QStringLiteral("lha");

        p[ListArgs] = QStringList() << QStringLiteral("v") << QStringLiteral("-v") << QStringLiteral("$Archive");
        p[ExtractArgs] = QStringList() << QStringLiteral("e") << QStringLiteral("-v") << QStringLiteral("$PreservePathSwitch") << QStringLiteral("$Archive") << QStringLiteral("$Files");

        p[DeleteArgs] = QStringList() << QStringLiteral("d") << QStringLiteral("-v") << QStringLiteral("$Archive") << QStringLiteral("$Files");

        p[FileExistsExpression] = QStringList() << QStringLiteral("^(.+) OverWrite \\?");
        p[FileExistsFileName] = QStringList() << p[FileExistsExpression].toString();
        p[FileExistsMode] = 1; // Watch for messages in stdout
        p[FileExistsInput] = QStringList()
                                << QStringLiteral("Y") //overwrite
                                << QStringLiteral("N") //skip
                                << QStringLiteral("A") //overwrite all
                                << QStringLiteral("S") //autoskip
                                ;

        p[AddArgs] = QStringList() << QStringLiteral("a") << QStringLiteral("-v") << QStringLiteral("$Archive") << QStringLiteral("$Files");

        p[ExtractionFailedPatterns] = QStringList() << QStringLiteral("Error");
        p[PreservePathSwitch] = QStringList() << QLatin1String( "" ) << QStringLiteral( "-i" );
    }
    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    const QString m_headerString = QStringLiteral("----------");

    switch(m_parseState) {
        case ParseStateHeader:
            if (line.startsWith(m_headerString)) {
                m_parseState = ParseStateEntry;
                m_firstLine = true;
            }
            break;
        case ParseStateEntry:
            const QStringList entryList = line.split(QLatin1Char(' '), QString::SkipEmptyParts);

            if (m_firstLine) { // This line will contain the filename
                if (entryList.count() == 8) { // End of entries
                    m_parseState = ParseStateHeader;
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
                e[FileName] = m_entryFilename;
                e[InternalID] = m_internalId;

                if (entryList.count() == 9) { // UID/GID is missing
                    e[CompressedSize] = entryList[1];
                    e[Size] = entryList[2];
                    e[Ratio] = entryList[3];
                    e[Method] = entryList[4];
                    e[CRC] = entryList[5];

                    QDateTime timestamp(
                        QDate::fromString(entryList[6], QStringLiteral("yyyy-MM-dd")),
                        QTime::fromString(entryList[7], QStringLiteral("HH:mm:ss")));
                    e[Timestamp] = timestamp;
                    emit entry(e);
                }
                else if (entryList.count() == 10) { // All info is available
                    const QStringList ownerList = entryList[1].split(QLatin1Char('/')); // Separate uid from gui
                    e[Owner] = ownerList.at(0);
                    e[Group] = ownerList.at(1);
                    e[CompressedSize] = entryList[2];
                    e[Size] = entryList[3];
                    e[Ratio] = entryList[4];
                    e[Method] = entryList[5];
                    e[CRC] = entryList[6];

                    QDateTime timestamp(
                        QDate::fromString(entryList[7], QStringLiteral("yyyy-MM-dd")),
                        QTime::fromString(entryList[8], QStringLiteral("HH:mm:ss")));
                    e[Timestamp] = timestamp;
                    emit entry(e);
                }

                m_firstLine = true;
            }
        break;
    }
    return true;
}

#include "cliplugin.moc"
