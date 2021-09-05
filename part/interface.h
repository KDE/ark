/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>

    SPDX-License-Identifier: GPL-2.0-or-later

*/
#ifndef INTERFACE_H
#define INTERFACE_H

#include <QStringList>
#include <QtPlugin>

namespace Kerfuffle
{
class SettingsPage;
}

class KConfigSkeleton;

class Interface
{
public:
    virtual ~Interface() {}

    virtual bool isBusy() const = 0;
    virtual KConfigSkeleton *config() const = 0;
    virtual QList<Kerfuffle::SettingsPage*> settingsPages(QWidget *parent) const = 0;
};

Q_DECLARE_INTERFACE(Interface, "org.kde.kerfuffle.partinterface/0.43")

#endif // INTERFACE_H
