/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009 Raphael Kubo da Costa <kubito@gmail.com>
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
    explicit CliPlugin(const QString & filename, QObject *parent = 0);
    virtual ~CliPlugin();

protected:
    virtual Kerfuffle::ParameterList parameterList() const;
    virtual bool readListLine(const QString &line);

private:
    Kerfuffle::ArchiveEntry m_currentArchiveEntry;
    int m_state;
};

#endif // CLIPLUGIN_H
