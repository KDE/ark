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

#include "archive_kerfuffle.h"
#include "app/logging.h"
#include "archiveinterface.h"
#include "jobs.h"

#include <QByteArray>
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>

#include <KPluginLoader>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>

Q_LOGGING_CATEGORY(KERFUFFLE, "ark.kerfuffle", QtWarningMsg)

static bool comparePlugins(const KService::Ptr &p1, const KService::Ptr &p2)
{
    return (p1->property(QLatin1String( "X-KDE-Priority" )).toInt()) > (p2->property(QLatin1String( "X-KDE-Priority" )).toInt());
}

static QString determineMimeType(const QString& filename)
{
    QMimeDatabase db;
    QMimeType mimeFromExtension = db.mimeTypeForFile(filename, QMimeDatabase::MatchExtension);
    QMimeType mimeFromContent = db.mimeTypeForFile(filename, QMimeDatabase::MatchContent);

    // mimeFromContent will be "application/octet-stream" when file is
    // unreadable, so use extension.
    if (!QFileInfo(filename).isReadable()) {
        return mimeFromExtension.name();
    }

    // Compressed tar-archives are detected as single compressed files when
    // detecting by content. The following code prevents tar.gz, tar.bz2 and
    // tar.xz files being opened using the singlefile plugin.
    if ((mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/gzip"))) ||
        (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-bzip-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-bzip"))) ||
        (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-xz-compressed-tar")) &&
         mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-xz")))) {
        return mimeFromExtension.name();
    }

    if (mimeFromExtension != mimeFromContent) {
        qCWarning(KERFUFFLE) << "Mimetype for filename extension (" << mimeFromExtension.name()
                             << ") did not match mimetype for content (" << mimeFromContent.name()
                             << "). Using content-based mimetype.";
    }

    return mimeFromContent.name();
}

static KService::List findPluginOffers(const QString& filename, const QString& fixedMimeType)
{
    KService::List offers;

    qCDebug(KERFUFFLE) << "Find plugin offers for" << filename << "with mime" << fixedMimeType;

    const QString mimeType = fixedMimeType.isEmpty() ? determineMimeType(filename) : fixedMimeType;

    qCDebug(KERFUFFLE) << "Detected mime" << mimeType;

    if (!mimeType.isEmpty()) {
        offers = KMimeTypeTrader::self()->query(mimeType, QLatin1String( "Kerfuffle/Plugin" ), QLatin1String( "(exist Library)" ));
        qSort(offers.begin(), offers.end(), comparePlugins);
    }

    qCDebug(KERFUFFLE) << "Have" << offers.count() << "offers";

    return offers;
}

namespace Kerfuffle
{

QDebug operator<<(QDebug d, const fileRootNodePair &pair)
{
    d.nospace() << "fileRootNodePair(" << pair.file << "," << pair.rootNode << ")";
    return d.space();
}

Archive *Archive::create(const QString &fileName, QObject *parent)
{
    return create(fileName, QString(), parent);
}

Archive *Archive::create(const QString &fileName, const QString &fixedMimeType, QObject *parent)
{
    qCDebug(KERFUFFLE) << "Going to create archive" << fileName;

    qRegisterMetaType<ArchiveEntry>("ArchiveEntry");

    const KService::List offers = findPluginOffers(fileName, fixedMimeType);

    if (offers.isEmpty()) {
        qCWarning(KERFUFFLE) << "Could not find a plugin to handle" << fileName;
        return Q_NULLPTR;
    }

    const QString pluginName = offers.first()->library();
    qCDebug(KERFUFFLE) << "Loading plugin" << pluginName;

    KPluginFactory * const factory = KPluginLoader(pluginName).factory();
    if (!factory) {
        qCWarning(KERFUFFLE) << "Invalid plugin factory for" << pluginName;
        return Q_NULLPTR;
    }

    QVariantList args;
    args.append(QVariant(QFileInfo(fileName).absoluteFilePath()));

    ReadOnlyArchiveInterface * const iface = factory->create<ReadOnlyArchiveInterface>(0, args);
    if (!iface) {
        qCWarning(KERFUFFLE) << "Could not create plugin instance" << pluginName << "for" << fileName;
        return Q_NULLPTR;
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
    qCDebug(KERFUFFLE) << "Created archive instance";

    Q_ASSERT(archiveInterface);
    archiveInterface->setParent(this);

    QMetaType::registerComparators<fileRootNodePair>();
    QMetaType::registerDebugStreamOperator<fileRootNodePair>();
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
    qCDebug(KERFUFFLE) << "Going to list files";

    ListJob *job = new ListJob(m_iface, this);
    job->setAutoDelete(false);

    //if this job has not been listed before, we grab the opportunity to
    //collect some information about the archive
    if (!m_hasBeenListed) {
        connect(job, &ListJob::result, this, &Archive::onListFinished);
    }
    return job;
}

DeleteJob* Archive::deleteFiles(const QList<QVariant> & files)
{
    qCDebug(KERFUFFLE) << "Going to delete files" << files;

    if (m_iface->isReadOnly()) {
        return 0;
    }
    DeleteJob *newJob = new DeleteJob(files, static_cast<ReadWriteArchiveInterface*>(m_iface), this);

    return newJob;
}

AddJob* Archive::addFiles(const QStringList & files, const CompressionOptions& options)
{
    qCDebug(KERFUFFLE) << "Going to add files" << files << "with options" << options;
    Q_ASSERT(!m_iface->isReadOnly());
    AddJob *newJob = new AddJob(files, options, static_cast<ReadWriteArchiveInterface*>(m_iface), this);
    connect(newJob, &AddJob::result, this, &Archive::onAddFinished);
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
        ListJob *job = list();

        connect(job, &ListJob::userQuery, this, &Archive::onUserQuery);

        QEventLoop loop(this);

        connect(job, &KJob::result, &loop, &QEventLoop::quit);
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

void Archive::enableHeaderEncryption(bool enable)
{
    m_iface->setHeaderEncryptionEnabled(enable);
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

    qCDebug(KERFUFFLE) << "Returning supported mimetypes" << supported;

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

    qCDebug(KERFUFFLE) << "Returning supported write mimetypes" << supported;

    return supported;
}

QSet<QString> supportedEncryptEntriesMimeTypes()
{
    const QLatin1String constraint("(exist Library) and ([X-KDE-Kerfuffle-EncryptEntries] == true)");
    const QLatin1String basePartService("Kerfuffle/Plugin");

    const KService::List offers = KServiceTypeTrader::self()->query(basePartService, constraint);
    QSet<QString> supported;

    foreach (const KService::Ptr& service, offers) {
        QStringList mimeTypes = service->serviceTypes();

        foreach (const QString& mimeType, mimeTypes) {
            if (mimeType != basePartService) {
                supported.insert(mimeType);
            }
        }
    }

    qCDebug(KERFUFFLE) << "Returning supported mimetypes" << supported;

    return supported;
}

QSet<QString> supportedEncryptHeaderMimeTypes()
{
    const QLatin1String constraint("(exist Library) and ([X-KDE-Kerfuffle-EncryptHeader] == true)");
    const QLatin1String basePartService("Kerfuffle/Plugin");

    const KService::List offers = KServiceTypeTrader::self()->query(basePartService, constraint);
    QSet<QString> supported;

    foreach (const KService::Ptr& service, offers) {
        QStringList mimeTypes = service->serviceTypes();

        foreach (const QString& mimeType, mimeTypes) {
            if (mimeType != basePartService) {
                supported.insert(mimeType);
            }
        }
    }

    qCDebug(KERFUFFLE) << "Returning supported mimetypes" << supported;

    return supported;
}

} // namespace Kerfuffle
