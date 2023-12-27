/*
    SPDX-FileCopyrightText: 2008 Claudio Bantaloukas <rockdreamer@gmail.com>
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "kerfuffle/cliinterface.h"

using namespace Kerfuffle;

class CliPlugin : public CliInterface
{
public:
    explicit CliPlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
    virtual ~CliPlugin();

    virtual ParameterList parameterList() const;
    bool readListLine(const QString &line);

private:
    bool m_isFirstLine, m_incontent, m_isPasswordProtected;
    QString m_entryFilename;
};

#endif // CLIPLUGIN_H
