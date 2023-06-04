/*
    SPDX-FileCopyrightText: 2017 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FACTORY_H
#define FACTORY_H

#include <KPluginFactory>

class Factory: public KPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "ark_part.json")
    Q_INTERFACES(KPluginFactory)

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QObject *create(const char *iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString &keyword) override;
#else
    QObject *create(const char *iface, QWidget *parentWidget, QObject *parent, const QVariantList &args) override;
#endif
};

#endif
