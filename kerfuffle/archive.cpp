/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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
#include "archiveinterface.h"
#include "jobs.h"

#include <QByteArray>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>

#include <KDebug>
#include <KPluginLoader>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>

static bool comparePlugins(const KService::Ptr &p1, const KService::Ptr &p2)
{
    return (p1->property(QLatin1String( "X-KDE-Priority" )).toInt()) > (p2->property(QLatin1String( "X-KDE-Priority" )).toInt());
}

static QString determineMimeType(const QString& filename)
{
    if (!QFile::exists(filename)) {
        return KMimeType::findByPath(filename)->name();
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

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
        offers = KMimeTypeTrader::self()->query(mimeType, QLatin1String( "Kerfuffle/Plugin" ), QLatin1String( "(exist Library)" ));
        qSort(offers.begin(), offers.end(), comparePlugins);
    }

    return offers;
}

namespace Kerfuffle
{

Archive *Archive::create(const QString &fileName, QObject *parent)
{
    return create(fileName, QString(), parent);
}

Archive *Archive::create(const QString &fileName, const QString &fixedMimeType, QObject *parent)
{
    qRegisterMetaType<ArchiveEntry>("ArchiveEntry");

    const KService::List offers = findPluginOffers(fileName, fixedMimeType);

    if (offers.isEmpty()) {
        kDebug() << "Could not find a plugin to handle" << fileName;
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
    args.append(QVariant(QFileInfo(fileName).absoluteFilePath()));

    ReadOnlyArchiveInterface * const iface = factory->create<ReadOnlyArchiveInterface>(0, args);
    if (!iface) {
        kDebug() << "Could not create plugin instance" << pluginName << "for" << fileName;
        return NULL;
    }

    return new Archive(iface, parent);
}

Archive::Archive(ReadOnlyArchiveInterface *archiveInterface, QObject *parent)
        : QObject(parent),
        m_iface(archiveInterface),
        m_hasBeenListed(false),
        m_isPasswordProtected(false),
        m_isSingleFolderArchive(false)
{
    Q_ASSERT(archiveInterface);
    archiveInterface->setParent(this);
}

Archive::~Archive()
{
}

bool Archive::isReadOnly() const
{
    return m_iface->isReadOnly();
}

KJob* Archive::open()
{
    return 0;
}

KJob* Archive::create()
{
    return 0;
}

ListJob* Archive::list()
{
    ListJob *job = new ListJob(m_iface, this);
    job->setAutoDelete(false);

    //if this job has not been listed before, we grab the opportunity to
    //collect some information about the archive
    if (!m_hasBeenListed) {
        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(onListFinished(KJob*)));
    }
    return job;
}

DeleteJob* Archive::deleteFiles(const QList<QVariant> & files)
{
    if (m_iface->isReadOnly()) {
        return 0;
    }
    DeleteJob *newJob = new DeleteJob(files, static_cast<ReadWriteArchiveInterface*>(m_iface), this);

    return newJob;
}

AddJob* Archive::addFiles(const QStringList & files, const CompressionOptions& options)
{
    Q_ASSERT(!m_iface->isReadOnly());
    AddJob *newJob = new AddJob(files, options, static_cast<ReadWriteArchiveInterface*>(m_iface), this);
    connect(newJob, SIGNAL(result(KJob*)),
            this, SLOT(onAddFinished(KJob*)));
    return newJob;
}

ExtractJob* Archive::copyFiles(const QList<QVariant> & files, const QString & destinationDir, ExtractionOptions options)
{
    ExtractionOptions newOptions = options;
    if (isPasswordProtected()) {
        newOptions[QLatin1String( "PasswordProtectedHint" )] = true;
    }

    ExtractJob *newJob = new ExtractJob(files, destinationDir, newOptions, m_iface, this);
    return newJob;
}

QString Archive::fileName() const
{
    return m_iface->filename();
}

void Archive::onAddFinished(KJob* job)
{
    //if the archive was previously a single folder archive and an add job
    //has successfully finished, then it is no longer a single folder
    //archive (for the current implementation, which does not allow adding
    //folders/files other places than the root.
    //TODO: handle the case of creating a new file and singlefolderarchive
    //then.
    if (m_isSingleFolderArchive && !job->error()) {
        m_isSingleFolderArchive = false;
    }
}

void Archive::onListFinished(KJob* job)
{
    ListJob *ljob = qobject_cast<ListJob*>(job);
    m_extractedFilesSize = ljob->extractedFilesSize();
    m_isSingleFolderArchive = ljob->isSingleFolderArchive();
    m_isPasswordProtected = ljob->isPasswordProtected();
    m_subfolderName = ljob->subfolderName();
    if (m_subfolderName.isEmpty()) {
        QFileInfo fi(fileName());
        QString base = fi.completeBaseName();

        //special case for tar.gz/bzip2 files
        if (base.right(4).toUpper() == QLatin1String(".TAR")) {
            base.chop(4);
        }

        m_subfolderName = base;
    }

    m_hasBeenListed = true;
}

void Archive::listIfNotListed()
{
    if (!m_hasBeenListed) {
        KJob *job = list();

        connect(job, SIGNAL(userQuery(Kerfuffle::Query*)),
                SLOT(onUserQuery(Kerfuffle::Query*)));

        QEventLoop loop(this);

        connect(job, SIGNAL(result(KJob*)),
                &loop, SLOT(quit()));
        job->start();
        loop.exec(); // krazy:exclude=crashy
    }
}

void Archive::onUserQuery(Query* query)
{
    query->execute();
}

bool Archive::isSingleFolderArchive()
{
    listIfNotListed();
    return m_isSingleFolderArchive;
}

bool Archive::isPasswordProtected()
{
    listIfNotListed();
    return m_isPasswordProtected;
}

QString Archive::subfolderName()
{
    listIfNotListed();
    return m_subfolderName;
}

void Archive::setPassword(const QString &password)
{
    m_iface->setPassword(password);
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
