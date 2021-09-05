/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef READONLYLIBARCHIVEPLUGIN_H
#define READONLYLIBARCHIVEPLUGIN_H

#include "libarchiveplugin.h"

class ReadOnlyLibarchivePlugin : public LibarchivePlugin
{
    Q_OBJECT

public:
    explicit ReadOnlyLibarchivePlugin(QObject *parent, const QVariantList& args);
    ~ReadOnlyLibarchivePlugin() override;


};

#endif // READONLYLIBARCHIVEPLUGIN_H
