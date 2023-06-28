/*
    SPDX-FileCopyrightText: 2017 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "factory.h"
#include "part.h"
#include <KPluginMetaData>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QObject *Factory::create(const char *iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString &keyword)
{
    Q_UNUSED(keyword)
#else
QObject *Factory::create(const char *iface, QWidget *parentWidget, QObject *parent, const QVariantList &args)
{
#endif
    auto part = new Ark::Part(parentWidget, parent, metaData(), args);
    part->setReadWrite(QByteArray(iface) == QByteArray(KParts::ReadWritePart::staticMetaObject.className()));

    return part;
}

#include "moc_factory.cpp"
