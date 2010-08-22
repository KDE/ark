/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2009 Raphael Kubo da Costa <kubito@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "archive.h"
#include "archivebase.h"
#include "archiveinterface.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>

#include <KDebug>
#include <KPluginLoader>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>

static bool comparePlugins(const KService::Ptr &p1, const KService::Ptr &p2)
{
    return (p1->property("X-KDE-Priority").toInt()) > (p2->property("X-KDE-Priority").toInt());
}

static QString determineMimeType(const QString& filename)
{
    if (!QFile::exists(filename))
        return KMimeType::findByPath(filename)->name();

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    const qint64 maxSize = 0x100000; // 1MB
    const qint64 bufferSize = qMin(maxSize, file.size());
    const QByteArray buffer = file.read(bufferSize);

    return KMimeType::findByNameAndContent(filename, buffer)->name();
}

static KService::List findPluginOffers(const QString& filename, const QString& fixedMimeType)
{
    KService::List offers;

    const QString mimeType = fixedMimeType.isEmpty() ? determineMimeType(filename) : fixedMimeType;

    if (!mimeType.isEmpty()) {
        offers = KMimeTypeTrader::self()->query(mimeType, "Kerfuffle/Plugin", "(exist Library)");
        qSort(offers.begin(), offers.end(), comparePlugins);
    }

    return offers;
}

namespace Kerfuffle
{

Archive *factory(const QString& filename, const QString& fixedMimeType)
{
    qRegisterMetaType<ArchiveEntry>("ArchiveEntry");

    const KService::List offers = findPluginOffers(filename, fixedMimeType);

    if (offers.isEmpty()) {
        kDebug() << "Could not find a plugin to handle" << filename;
        return NULL;
    }

    const QString pluginName = offers.first()->library();
    kDebug() << "Loading plugin" << pluginName;

    KPluginFactory * const factory = KPluginLoader(pluginName).factory();
    if (!factory) {
        kDebug() << "Invalid plugin factory for" << pluginName;
        return NULL;
    }

    QVariantList args;
    args.append(QVariant(QFileInfo(filename).absoluteFilePath()));

    ReadOnlyArchiveInterface * const iface = factory->create<ReadOnlyArchiveInterface>(0, args);
    if (!iface) {
        kDebug() << "Could not create plugin instance" << pluginName << "for" << filename;
        return NULL;
    }

    return new ArchiveBase(iface);
}

QStringList supportedMimeTypes()
{
    const QLatin1String constraint("(exist Library)");
    const QLatin1String basePartService("Kerfuffle/Plugin");

    const KService::List offers = KServiceTypeTrader::self()->query(basePartService, constraint);
    KService::List::ConstIterator it = offers.constBegin();
    KService::List::ConstIterator itEnd = offers.constEnd();

    QStringList supported;

    for (; it != itEnd; ++it) {
        KService::Ptr service = *it;
        QStringList mimeTypes = service->serviceTypes();

        foreach (const QString& mimeType, mimeTypes) {
            if (mimeType != basePartService && !supported.contains(mimeType)) {
                supported.append(mimeType);
            }
        }
    }

    kDebug() << "Returning" << supported;

    return supported;
}

QStringList supportedWriteMimeTypes()
{
    const QLatin1String constraint("(exist Library) and ([X-KDE-Kerfuffle-ReadWrite] == true)");
    const QLatin1String basePartService("Kerfuffle/Plugin");

    const KService::List offers = KServiceTypeTrader::self()->query(basePartService, constraint);
    KService::List::ConstIterator it = offers.constBegin();
    KService::List::ConstIterator itEnd = offers.constEnd();

    QStringList supported;

    for (; it != itEnd; ++it) {
        KService::Ptr service = *it;
        QStringList mimeTypes = service->serviceTypes();

        foreach (const QString& mimeType, mimeTypes) {
            if (mimeType != basePartService && !supported.contains(mimeType)) {
                supported.append(mimeType);
            }
        }
    }

    kDebug() << "Returning" << supported;

    return supported;
}

} // namespace Kerfuffle
