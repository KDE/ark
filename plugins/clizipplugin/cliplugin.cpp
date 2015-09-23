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
#include "app/logging.h"
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/kerfuffle_export.h"

#include <KPluginFactory>

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(KERFUFFLE_PLUGIN, "ark.kerfuffle.cli7zip", QtWarningMsg)

using namespace Kerfuffle;

K_PLUGIN_FACTORY( CliPluginFactory, registerPlugin< CliPlugin >(); )

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
    : CliInterface(parent, args)
    , m_parseState(ParseStateHeader)
{
    qCDebug(KERFUFFLE_PLUGIN) << "Loaded cli_zip plugin";
}

CliPlugin::~CliPlugin()
{
}

void CliPlugin::resetParsing()
{
    m_parseState = ParseStateHeader;
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
        p[ListProgram] = QStringList() << QStringLiteral("zipinfo");
        p[ExtractProgram] = QStringList() << QStringLiteral("unzip");
        p[DeleteProgram] = p[AddProgram] = QStringList() << QStringLiteral("zip");

        p[ListArgs] = QStringList() << QStringLiteral("-l")
                                    << QStringLiteral("-T")
                                    << QStringLiteral("$Archive");
        p[ExtractArgs] = QStringList() << QStringLiteral("$PreservePathSwitch")
                                       << QStringLiteral("$PasswordSwitch")
                                       << QStringLiteral("$Archive")
                                       << QStringLiteral("$Files");
        p[PreservePathSwitch] = QStringList() << QStringLiteral("")
                                              << QStringLiteral("-j");
        p[PasswordSwitch] = QStringList() << QStringLiteral("-P$Password");

        p[DeleteArgs] = QStringList() << QStringLiteral("-d")
                                      << QStringLiteral("$Archive")
                                      << QStringLiteral("$Files");

        p[FileExistsExpression] = QStringLiteral("^replace (.+)\\? \\[y\\]es, \\[n\\]o, \\[A\\]ll, \\[N\\]one, \\[r\\]ename: $");
        p[FileExistsFileName] = QStringList() << p[FileExistsExpression].toString();
        p[FileExistsInput] = QStringList() << QStringLiteral("y")  //overwrite
                                           << QStringLiteral("n")  //skip
                                           << QStringLiteral("A")  //overwrite all
                                           << QStringLiteral("N"); //autoskip

        p[AddArgs] = QStringList() << QStringLiteral("-r")
                                   << QStringLiteral("$Archive")
                                   << QStringLiteral("$PasswordSwitch")
                                   << QStringLiteral("$Files");

        p[PasswordPromptPattern] = QStringLiteral(" password: ");
        p[WrongPasswordPatterns] = QStringList() << QStringLiteral("incorrect password");
        //p[ExtractionFailedPatterns] = QStringList() << "CRC failed";
    }
    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    static const QRegularExpression entryPattern(QStringLiteral(
        "^(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\d{8}).(\\d{6})\\s+(.+)$") );

    switch (m_parseState) {
    case ParseStateHeader:
        m_parseState = ParseStateEntry;
        break;
    case ParseStateEntry:
        QRegularExpressionMatch rxMatch = entryPattern.match(line);
        if (rxMatch.hasMatch()) {
            ArchiveEntry e;
            e[Permissions] = rxMatch.captured(1);

            // #280354: infozip may not show the right attributes for a given directory, so an entry
            //          ending with '/' is actually more reliable than 'd' bein in the attributes.
            e[IsDirectory] = rxMatch.captured(10).endsWith(QLatin1Char('/'));

            e[Size] = rxMatch.captured(4).toInt();
            QString status = rxMatch.captured(5);
            if (status[0].isUpper()) {
                e[IsPasswordProtected] = true;
            }
            e[CompressedSize] = rxMatch.captured(6).toInt();

            const QDateTime ts(QDate::fromString(rxMatch.captured(8), QStringLiteral("yyyyMMdd")),
                               QTime::fromString(rxMatch.captured(9), QStringLiteral("hhmmss")));
            e[Timestamp] = ts;

            e[FileName] = e[InternalID] = rxMatch.captured(10);
            emit entry(e);
        }
        break;
    }

    return true;
}

#include "cliplugin.moc"
