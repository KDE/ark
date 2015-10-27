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
#include "ark_debug.h"
#include "archiveinterface.h"
#include "jobs.h"

#include <QByteArray>
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>

#include <KPluginLoader>
#include <KMimeTypeTrader>

namespace Kerfuffle
{

bool Archive::comparePlugins(const KService::Ptr &p1, const KService::Ptr &p2)
{
    return (p1->property(QStringLiteral( "X-KDE-Priority" )).toInt()) > (p2->property(QStringLiteral( "X-KDE-Priority" )).toInt());
}

QString Archive::determineMimeType(const QString& filename)
{
    QMimeDatabase db;

    QFileInfo fileinfo(filename);
    QString inputFile = filename;

    // #328815: since detection-by-content does not work for compressed tar archives (see below why)
    // we cannot rely on it when the archive extension is wrong; we need to validate by hand.
    if (fileinfo.completeSuffix().toLower().remove(QRegularExpression(QStringLiteral("[^a-z\\.]"))).contains(QStringLiteral("tar."))) {
        inputFile.chop(fileinfo.completeSuffix().length());
        inputFile += fileinfo.completeSuffix().remove(QRegularExpression(QStringLiteral("[^a-zA-Z\\.]")));
        qCDebug(ARK) << "Validated filename of compressed tar" << filename << "into filename" << inputFile;
    }

    QMimeType mimeFromExtension = db.mimeTypeForFile(inputFile, QMimeDatabase::MatchExtension);
    QMimeType mimeFromContent = db.mimeTypeForFile(filename, QMimeDatabase::MatchContent);

    // mimeFromContent will be "application/octet-stream" when file is
    // unreadable, so use extension.
    if (!fileinfo.isReadable()) {
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
        qCWarning(ARK) << "Mimetype for filename extension (" << mimeFromExtension.name()
                             << ") did not match mimetype for content (" << mimeFromContent.name()
                             << "). Using content-based mimetype.";
    }

    return mimeFromContent.name();
}

KService::List Archive::findPluginOffers(const QString& filename, const QString& fixedMimeType)
{
    KService::List offers;

    qCDebug(ARK) << "Find plugin offers for" << filename << "with mime" << fixedMimeType;

    const QString mimeType = fixedMimeType.isEmpty() ? determineMimeType(filename) : fixedMimeType;

    qCDebug(ARK) << "Detected mime" << mimeType;

    if (!mimeType.isEmpty()) {
        offers = KMimeTypeTrader::self()->query(mimeType, QStringLiteral( "Kerfuffle/Plugin" ), QStringLiteral( "(exist Library)" ));
        qSort(offers.begin(), offers.end(), comparePlugins);
    }

    qCDebug(ARK) << "Have" << offers.count() << "offers";

    return offers;
}

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
    qCDebug(ARK) << "Going to create archive" << fileName;

    qRegisterMetaType<ArchiveEntry>("ArchiveEntry");

    QVariantList args;
    args.append(QVariant(QFileInfo(fileName).absoluteFilePath()));

    const KService::List offers = findPluginOffers(fileName, fixedMimeType);
    if (offers.isEmpty()) {
        qCCritical(ARK) << "Could not find a plugin to handle" << fileName;
        return new Archive(NoPlugin, parent);
    }

    KPluginFactory *factory;
    ReadOnlyArchiveInterface *iface;

    foreach (KService::Ptr service, offers) {

        QString pluginName = service->library();
        bool isReadOnly = !service->property(QStringLiteral("X-KDE-Kerfuffle-ReadWrite")).toBool();
        qCDebug(ARK) << "Loading plugin" << pluginName;

        factory = KPluginLoader(pluginName).factory();
        if (!factory) {
            qCWarning(ARK) << "Invalid plugin factory for" << pluginName;
            continue;
        }

        iface = factory->create<ReadOnlyArchiveInterface>(0, args);
        if (!iface) {
            qCWarning(ARK) << "Could not create plugin instance" << pluginName;
            continue;
        }

        if (iface->isCliBased()) {
            qCDebug(ARK) << "Finding executables for plugin" << pluginName;

            if (iface->findExecutables(!isReadOnly)) {
                return new Archive(iface, isReadOnly, parent);
            } else {
                qCWarning(ARK) << "Failed to find needed executables for plugin" << pluginName;
            }
        } else {
            // Not CliBased plugin, don't search for executables.
            return new Archive(iface, isReadOnly, parent);
        }
    }

    qCCritical(ARK) << "Failed to find a usable plugin for" << fileName;
    return new Archive(FailedPlugin, parent);
}

Archive::Archive(ArchiveError errorCode, QObject *parent)
        : QObject(parent)
        , m_error(errorCode)
{
    qCDebug(ARK) << "Created archive instance with error";
}

Archive::Archive(ReadOnlyArchiveInterface *archiveInterface, bool isReadOnly, QObject *parent)
        : QObject(parent)
        , m_iface(archiveInterface)
        , m_hasBeenListed(false)
        , m_isReadOnly(isReadOnly)
        , m_isPasswordProtected(false)
        , m_isSingleFolderArchive(false)
        , m_error(NoError)
{
    qCDebug(ARK) << "Created archive instance";

    Q_ASSERT(archiveInterface);
    archiveInterface->setParent(this);

    QMetaType::registerComparators<fileRootNodePair>();
    QMetaType::registerDebugStreamOperator<fileRootNodePair>();
}

Archive::~Archive()
{
}

bool Archive::isValid() const
{
    return (m_error == NoError);
}

ArchiveError Archive::error() const
{
    return m_error;
}

bool Archive::isReadOnly() const
{
    return (m_iface->isReadOnly() || m_isReadOnly);
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
    qCDebug(ARK) << "Going to list files";

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
    qCDebug(ARK) << "Going to delete files" << files;

    if (m_iface->isReadOnly()) {
        return 0;
    }
    DeleteJob *newJob = new DeleteJob(files, static_cast<ReadWriteArchiveInterface*>(m_iface), this);

    return newJob;
}

AddJob* Archive::addFiles(const QStringList & files, const CompressionOptions& options)
{
    qCDebug(ARK) << "Going to add files" << files << "with options" << options;
    Q_ASSERT(!m_iface->isReadOnly());
    AddJob *newJob = new AddJob(files, options, static_cast<ReadWriteArchiveInterface*>(m_iface), this);
    connect(newJob, &AddJob::result, this, &Archive::onAddFinished);
    return newJob;
}

ExtractJob* Archive::copyFiles(const QList<QVariant> & files, const QString & destinationDir, ExtractionOptions options)
{
    ExtractionOptions newOptions = options;
    if (isPasswordProtected()) {
        newOptions[QStringLiteral( "PasswordProtectedHint" )] = true;
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

QString Archive::comment() const
{
    return m_iface->comment();
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

    qCDebug(ARK) << "Returning supported mimetypes" << supported;

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

    qCDebug(ARK) << "Returning supported write mimetypes" << supported;

    return supported;
}

QSet<QString> supportedEncryptEntriesMimeTypes()
{
    const KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("Kerfuffle/Plugin"),
                                                                    QStringLiteral("(exist Library)"));
    QSet<QString> supported;

    foreach (const KService::Ptr& service, offers) {
        QStringList list(service->property(QStringLiteral("X-KDE-Kerfuffle-EncryptEntries")).toStringList());
        foreach (const QString& mimeType, list) {
            supported.insert(mimeType);
        }
    }

    qCDebug(ARK) << "Entry encryption supported for mimetypes" << supported;

    return supported;
}

QSet<QString> supportedEncryptHeaderMimeTypes()
{
    const KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("Kerfuffle/Plugin"),
                                                                    QStringLiteral("(exist Library)"));
    QSet<QString> supported;

    foreach (const KService::Ptr& service, offers) {
        QStringList list(service->property(QStringLiteral("X-KDE-Kerfuffle-EncryptHeader")).toStringList());
        foreach (const QString& mimeType, list) {
            supported.insert(mimeType);
        }
    }

    qCDebug(ARK) << "Header encryption supported for mimetypes" << supported;

    return supported;
}

} // namespace Kerfuffle
