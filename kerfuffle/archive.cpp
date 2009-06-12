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
#include "archivefactory.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>

#include <KDebug>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>
#include <KLibLoader>

static bool comparePlugins(const KService::Ptr &p1, const KService::Ptr &p2)
{
    return (p1->property("X-KDE-Priority").toInt()) > (p2->property("X-KDE-Priority").toInt());
}

static QString determineMimeType(const QString & filename, const QString & defaultMimeType)
{
    if (!defaultMimeType.isEmpty())
        return defaultMimeType;

    if (!QFile::exists(filename))
        return KMimeType::findByPath(filename)->name();

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    const qint64 maxSize = 0x100000; // 1MB
    qint64 bufferSize = qMin(maxSize, file.size());
    QByteArray buffer = file.read(bufferSize);

    return KMimeType::findByNameAndContent(filename, buffer)->name();
}

namespace Kerfuffle
{
Archive *factory(const QString & filename, const QString & requestedMimeType)
{
    kDebug(1601) ;

    qRegisterMetaType<ArchiveEntry>("ArchiveEntry");

    QString mimeType = determineMimeType(filename, requestedMimeType);
    if (mimeType.isEmpty())
        return 0L;

    KService::List offers = KMimeTypeTrader::self()->query(mimeType, "Kerfuffle/Plugin", "(exist Library)");

    if (offers.isEmpty()) {
        kDebug(1601) << "Trying to find the mimetype by looking at file content";

        int acc;
        QString mimeType = KMimeType::findByFileContent(filename, &acc)->name();
        kDebug(1601) << mimeType << acc;
        offers = KMimeTypeTrader::self()->query(mimeType, "Kerfuffle/Plugin", "(exist Library)");
    }

    qSort(offers.begin(), offers.end(), comparePlugins);

    if (!offers.isEmpty()) {
        QString libraryName = offers[ 0 ]->library();
        KLibrary *lib = KLibLoader::self()->library(QFile::encodeName(libraryName), QLibrary::ExportExternalSymbolsHint);
        //TODO: get rid of the deprecated klibloader::self
#if 0

        KPluginLoader loader(offers.at(0));
        KPluginFactory *factory = loader.factory();
#endif

        kDebug(1601) << "Loading library " << libraryName ;
        if (lib) {
            ArchiveFactory *(*pluginFactory)() = (ArchiveFactory * (*)())lib->resolveFunction("pluginFactory");
            if (pluginFactory) {
                ArchiveFactory *factory = pluginFactory(); // TODO: cache these
                Archive *arch = factory->createArchive(QFileInfo(filename).absoluteFilePath(), 0);
                delete factory;
                return arch;
            }
        }
        kDebug(1601) << "Couldn't load library " << libraryName ;
    }
    kDebug(1601) << "Couldn't find a library capable of handling " << filename ;
    return 0;
}

QStringList supportedMimeTypes()
{
    QString constraint("(exist Library)");
    QLatin1String basePartService("Kerfuffle/Plugin");

    KService::List offers = KServiceTypeTrader::self()->query(basePartService, constraint);
    KService::List::ConstIterator it = offers.constBegin();
    KService::List::ConstIterator itEnd = offers.constEnd();

    QStringList supported;

    for (; it != itEnd; ++it) {
        KService::Ptr service = *it;
        QStringList mimeTypes = service->serviceTypes();
        foreach(const QString& mimeType, mimeTypes)
        if (mimeType != basePartService && !supported.contains(mimeType))
            supported.append(mimeType);
    }

    kDebug(1601) << "Returning" << supported;

    return supported;
}

QStringList supportedWriteMimeTypes()
{
    QString constraint("(exist Library) and ([X-KDE-Kerfuffle-ReadWrite] == true)");
    QLatin1String basePartService("Kerfuffle/Plugin");

    KService::List offers = KServiceTypeTrader::self()->query(basePartService, constraint);
    KService::List::ConstIterator it = offers.constBegin();
    KService::List::ConstIterator itEnd = offers.constEnd();

    QStringList supported;

    for (; it != itEnd; ++it) {
        KService::Ptr service = *it;
        QStringList mimeTypes = service->serviceTypes();
        foreach(const QString& mimeType, mimeTypes)
        if (mimeType != basePartService && !supported.contains(mimeType))
            supported.append(mimeType);
    }

    kDebug(1601) << "Returning" << supported;

    return supported;
}
} // namespace Kerfuffle
