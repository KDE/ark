/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "kerfuffle/cliinterface.h"

class CliPlugin : public Kerfuffle::CliInterface
{
    Q_OBJECT

public:
    explicit CliPlugin(QObject *parent, const QVariantList & args);

    virtual ~CliPlugin();

    virtual QString escapeFileName(const QString &fileName) const;

    virtual Kerfuffle::ParameterList parameterList() const;

    virtual bool readListLine(const QString &line);

private:
    enum {
        ParseStateColumnDescription1 = 0,
        ParseStateColumnDescription2,
        ParseStateHeader,
        ParseStateEntryFileName,
        ParseStateEntryDetails,
        ParseStateEntryIgnoredDetails
    } m_parseState;

    QString m_entryFileName;
    QHash<QString, QString> m_entryDetails;

    bool m_isPasswordProtected;

    int m_remainingIgnoredSubHeaderLines;
    int m_remainingIgnoredDetailsLines;

    bool m_isUnrarFree;
    bool m_isUnrarVersion5;
};

#endif // CLIPLUGIN_H
