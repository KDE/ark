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

#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "kerfuffle/cliinterface.h"

using namespace Kerfuffle;

class CliPlugin: public CliInterface
{
public:
    explicit CliPlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
    virtual ~CliPlugin();

    virtual ParameterList parameterList() const;
    bool readListLine(const QString &line);

private:
    bool m_isFirstLine, m_incontent, m_isPasswordProtected;
    QString m_entryFilename, m_internalId;

};

#endif // CLIPLUGIN_H
