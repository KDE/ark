/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2011 Luke Shumaker <lukeshu@sbcglobal.net>
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

#include <QDateTime>
#include <QDir>
#include <QRegExp>
#include <QString>
#include <QStringList>

using namespace Kerfuffle;

K_PLUGIN_FACTORY(CliPluginFactory, registerPlugin<CliPlugin>();)

CliPlugin::CliPlugin(QObject *parent, const QVariantList &args)
        : CliInterface(parent, args)
        , m_indentLevel(0)

{
    qCDebug(ARK) << "Loaded cli_unarchiver plugin";
}

CliPlugin::~CliPlugin()
{
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;
    if (p.isEmpty()) {
        /* Limitations:
            *  01 - creates an empty file upon entering wrong password
            *  02 - unar detects if output is being redirected and then does not accept any input
            */

        ///////////////[ COMMON ]/////////////

        p[CaptureProgress] = false;
        p[PasswordPromptPattern] = QLatin1String("Password (will not be shown): ");

        ///////////////[ LIST ]/////////////

        p[ListProgram] = QLatin1String("lsar");
        p[ListArgs] = QStringList() << QLatin1String("-json") << QLatin1String("$Archive");

        ///////////////[ EXTRACT ]/////////////

        p[ExtractProgram] = QLatin1String("unar");
        p[ExtractArgs] = QStringList() << QLatin1String("$Archive") << QLatin1String("$Files") << QLatin1String("$PasswordSwitch") << QLatin1String("$RootNodeSwitch");
        p[NoTrailingSlashes]  = true;
        p[PasswordSwitch] = QStringList() << QLatin1String("-password") << QLatin1String("$Password");
    p[RootNodeSwitch] = QStringList() << QLatin1String("-output-directory") << QLatin1String("$Path");
        p[FileExistsExpression] = QLatin1String("^\\\"(.+)\\\" already exists.");
        p[FileExistsInput] = QStringList()
                    << QLatin1String("o") //overwrite
                    << QLatin1String("s") //skip
                    << QLatin1String("O") //overwrite all
                    << QLatin1String("S") //autoskip
                    << QLatin1String("q") //cancel
                    ;

        ///////////////[ DELETE ]/////////////

        p[DeleteProgram] = QLatin1String("x-fakeprogram");
        //p[DeleteArgs]    =

        ///////////////[ ADD ]/////////////

        p[AddProgram] = QLatin1String("x-fakeprogram");
        //p[AddArgs]    =

        ///////////////[ ERRORS ]/////////////

        p[ExtractionFailedPatterns] = QStringList()
            << QLatin1String("Failed! \\((.+)\\)$")
            << QLatin1String("Segmentation fault$");

        p[WrongPasswordPatterns] = QStringList()
            << QLatin1String("Failed! \\((.+)\\)$");
    }
    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    /* lsar will give us JSON output.  However, we actually parse based on
        * the indentation.  Ugly, I know, but
        *  1. It's easier
        *  2. lsar's JSON is invalid JSON, so actual parsers bork.
        */

    int spaces;
    for(spaces=0;(spaces<line.size())&&(line[spaces]==QLatin1Char(' '));spaces++){}
    // Since this is so ugly anyway, I'm not even going to check to
    // make sure that spaces is even.  I mean, what would I do about it?
    int m_newIndentLevel = spaces/2;

    if (m_newIndentLevel>m_indentLevel) {
        if (m_newIndentLevel==3) {
            m_currentEntry.clear();
            m_currentEntry[IsDirectory] = false;
        }
    } else if (m_newIndentLevel<m_indentLevel) {
        if ( (m_newIndentLevel<3) && (m_indentLevel>=3) ) {
            EntryMetaDataType index = IsDirectory;
            if (m_currentEntry[index].toBool()) {
                m_currentEntry[FileName].toString().append(QLatin1String("/"));
            }
            kDebug() << "Added entry:" << m_currentEntry;
            entry(m_currentEntry);
        }
    }
    m_indentLevel = m_newIndentLevel;

    QRegExp rx(QLatin1String("^\\s*\"([^\"]*)\": (.*),$"));
    if (rx.indexIn(line) >= 0) {
        QRegExp rx_unquote(QLatin1String("^\"(.*)\"$"));
        QString key = rx.cap(1);
        QString value = rx.cap(2);

        if (false) {
        } else if (key==QLatin1String("XADFileName")) {
            rx_unquote.indexIn(value);
            m_currentEntry[FileName] = m_currentEntry[InternalID] = rx_unquote.cap(1);
        } else if (key==QLatin1String("XADFileSize")) {
            m_currentEntry[Size] = value.toInt();
        } else if (key==QLatin1String("XADCompressedSize")) {
            m_currentEntry[CompressedSize] = value.toInt();
        } else if (key==QLatin1String("XADLastModificationDate")) {
            QDateTime ts(QDate::fromString(value, QLatin1String("\"YYYY-MM-DD hh:mm:ss")));
            m_currentEntry[Timestamp] = ts;
        } else if (key==QLatin1String("XADIsDirectory")) {
            m_currentEntry[IsDirectory] = (value==QLatin1String("1"));
        } else if (key==QLatin1String("RARCRC32")) {
            m_currentEntry[CRC] = value.toInt();
        } else if (key==QLatin1String("RARCompressionMethod")) {
            m_currentEntry[Method] = value.toInt();
        } else if (key==QLatin1String("Encrypted")) {
            m_currentEntry[IsPasswordProtected] = (value.toInt() != 0);
        }
        // TODO: add RAR version. ([Version])
    }

    return true;
}

#include "cliplugin.moc"
