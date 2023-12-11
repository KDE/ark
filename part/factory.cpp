/*
    SPDX-FileCopyrightText: 2017 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "factory.h"
#include "part.h"
#include <KPluginMetaData>

QObject *Factory::create(const char *iface, QWidget *parentWidget, QObject *parent, const QVariantList &args)
{
    auto part = new Ark::Part(parentWidget, parent, metaData(), args);
    part->setReadWrite(QByteArray(iface) == QByteArray(KParts::ReadWritePart::staticMetaObject.className()));

    return part;
}

#include "moc_factory.cpp"
