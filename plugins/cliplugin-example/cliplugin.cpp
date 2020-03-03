/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Claudio Bantaloukas <rockdreamer@gmail.com>
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
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
#include "kerfuffle/archiveentry.h"
#include "kerfuffle/kerfuffle_export.h"

#include <QDir>
#include <QDate>
#include <QTime>

K_PLUGIN_CLASS_WITH_JSON(CliPlugin, "kerfuffle_cli.json")

CliPlugin::CliPlugin(QObject *parent, const QVariantList &args)
    : CliInterface(parent, args),
      m_isFirstLine(true),
      m_incontent(false),
      m_isPasswordProtected(false)
{
    qCDebug(ARK) << "Loaded cli-example plugin";
}

CliPlugin::~CliPlugin()
{
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[CaptureProgress] = true;
        p[ListProgram] = p[ExtractProgram] = p[DeleteProgram] = p[AddProgram] = QLatin1String("rar");

        p[ListArgs] = QStringList() << QLatin1String("v") << QLatin1String("-c-") << QLatin1String("$Archive");
        p[ExtractArgs] = QStringList() << QLatin1String("-p-") << QLatin1String("$PreservePathSwitch") << QLatin1String("$PasswordSwitch") << QLatin1String("$Archive") << QLatin1String("$Files");
        p[PreservePathSwitch] = QStringList() << QLatin1String("x") << QLatin1String("e");
        p[PasswordSwitch] = QStringList() << QLatin1String("-p$Password");

        p[DeleteArgs] = QStringList() << QLatin1String("d") << QLatin1String("$Archive") << QLatin1String("$Files");

        p[FileExistsExpression] = QLatin1String("^(.+) already exists. Overwrite it");
        p[FileExistsInput] = QStringList()
                                << QLatin1String("Y") //overwrite
                                << QLatin1String("N") //skip
                                << QLatin1String("A") //overwrite all
                                << QLatin1String("E") //autoskip
                                << QLatin1String("Q") //cancel
                                ;

        p[AddArgs] = QStringList() << QLatin1String("a") << QLatin1String("$Archive") << QLatin1String("$Files");

        p[WrongPasswordPatterns] = QStringList() << QLatin1String("password incorrect");
        p[ExtractionFailedPatterns] = QStringList() << QLatin1String("CRC failed");
    }

    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    const QString m_headerString = QLatin1String("-----------------------------------------");

    // skip the heading
    if (!m_incontent) {
        if (line.startsWith(m_headerString)) {
            m_incontent = true;
        }
        return true;
    }

    // catch final line
    if (line.startsWith(m_headerString)) {
        m_incontent = false;
        return true;
    }

    // rar gives one line for the filename and a line after it with some file properties
    if (m_isFirstLine) {
        m_entryFilename = line.trimmed();
        //m_entryFilename.chop(1); // handle newline
        if (!m_entryFilename.isEmpty() && m_entryFilename.at(0) == QLatin1Char('*')) {
            m_isPasswordProtected = true;
            m_entryFilename.remove(0, 1);   // and the spaces in front
        } else
            m_isPasswordProtected = false;

        m_isFirstLine = false;
        return true;
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList fileprops = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
#else
    QStringList fileprops = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
#endif
    m_entryFilename = QDir::fromNativeSeparators(m_entryFilename);
    bool isDirectory = (bool)(fileprops[ 5 ].contains(QLatin1Char('d'), Qt::CaseInsensitive));

    QDateTime ts(QDate::fromString(fileprops[ 3 ], QLatin1String("dd-MM-yy")),
                 QTime::fromString(fileprops[ 4 ], QLatin1String("hh:mm")));
    // rar output date with 2 digit year but QDate takes is as 19??
    // let's take 1950 is cut-off; similar to KDateTime
    if (ts.date().year() < 1950) {
        ts = ts.addYears(100);
    }

    if (isDirectory && !m_entryFilename.endsWith(QLatin1Char('/'))) {
        m_entryFilename += QLatin1Char('/');
    }

    qCDebug(ARK) << m_entryFilename << " : " << fileprops;
    Archive::Entry *e = new Archive::Entry();
    e->setProperty("fullPath", m_entryFilename);
    e->setProperty("size", fileprops[ 0 ]);
    e->setProperty("compressedSize", fileprops[ 1 ]);
    e->setProperty("ratio", fileprops[ 2 ]);
    e->setProperty("timestamp", ts);
    e->setProperty("isDirectory", isDirectory);
    e->setProperty("permissions", fileprops[ 5 ].remove(0, 1));
    e->setProperty("CRC", fileprops[ 6 ]);
    e->setProperty("method", fileprops[ 7 ]);
    e->setProperty("version", fileprops[ 8 ]);
    e->setProperty("ssPasswordProtected", m_isPasswordProtected);
    qCDebug(ARK) << "Added entry: " << e;

    emit entry(e);
    m_isFirstLine = true;
    return true;
}

#include "cliplugin.moc"
