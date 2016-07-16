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
#include "mimetypes.h"
#include "pluginmanager.h"

#include <QByteArray>
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>

#include <KPluginFactory>
#include <KPluginLoader>

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
    qCDebug(ARK) << "Going to create archive" << fileName;

    qRegisterMetaType<ArchiveEntry>("ArchiveEntry");

    PluginManager pluginManager;
    const QMimeType mimeType = fixedMimeType.isEmpty() ? determineMimeType(fileName) : QMimeDatabase().mimeTypeForName(fixedMimeType);

    const QVector<Plugin*> offers = pluginManager.preferredPluginsFor(mimeType);
    if (offers.isEmpty()) {
        qCCritical(ARK) << "Could not find a plugin to handle" << fileName;
        return new Archive(NoPlugin, parent);
    }

    Archive *archive;
    foreach (Plugin *plugin, offers) {
        archive = create(fileName, plugin, parent);
        // Use the first valid plugin, according to the priority sorting.
        if (archive->isValid()) {
            return archive;
        }
    }

    qCCritical(ARK) << "Failed to find a usable plugin for" << fileName;
    return archive;
}

Archive *Archive::create(const QString &fileName, Plugin *plugin, QObject *parent)
{
    Q_ASSERT(plugin);

    qCDebug(ARK) << "Checking plugin" << plugin->metaData().pluginId();

    KPluginFactory *factory = KPluginLoader(plugin->metaData().fileName()).factory();
    if (!factory) {
        qCWarning(ARK) << "Invalid plugin factory for" << plugin->metaData().pluginId();
        return new Archive(FailedPlugin, parent);
    }

    const QVariantList args = {QVariant(QFileInfo(fileName).absoluteFilePath())};
    ReadOnlyArchiveInterface *iface = factory->create<ReadOnlyArchiveInterface>(Q_NULLPTR, args);
    if (!iface) {
        qCWarning(ARK) << "Could not create plugin instance" << plugin->metaData().pluginId();
        return new Archive(FailedPlugin, parent);
    }

    if (!plugin->isValid()) {
        qCDebug(ARK) << "Cannot use plugin" << plugin->metaData().pluginId() << "- check whether" << plugin->readOnlyExecutables() << "are installed.";
        return new Archive(FailedPlugin, parent);
    }

    qCDebug(ARK) << "Successfully loaded plugin" << plugin->metaData().pluginId();
    return new Archive(iface, !plugin->isReadWrite(), parent);
}

Archive::Archive(ArchiveError errorCode, QObject *parent)
        : QObject(parent)
        , m_iface(Q_NULLPTR)
        , m_error(errorCode)
{
    qCDebug(ARK) << "Created archive instance with error";
}

Archive::Archive(ReadOnlyArchiveInterface *archiveInterface, bool isReadOnly, QObject *parent)
        : QObject(parent)
        , m_iface(archiveInterface)
        , m_hasBeenListed(false)
        , m_isReadOnly(isReadOnly)
        , m_isSingleFolderArchive(false)
        , m_extractedFilesSize(0)
        , m_error(NoError)
        , m_encryptionType(Unencrypted)
        , m_numberOfFiles(0)
        , m_numberOfFolders(0)
{
    qCDebug(ARK) << "Created archive instance";

    Q_ASSERT(archiveInterface);
    archiveInterface->setParent(this);

    QMetaType::registerComparators<fileRootNodePair>();
    QMetaType::registerDebugStreamOperator<fileRootNodePair>();

    connect(m_iface, &ReadOnlyArchiveInterface::entry, this, &Archive::onNewEntry);
}


Archive::~Archive()
{
}

QString Archive::completeBaseName() const
{
    QString base = QFileInfo(fileName()).completeBaseName();

    // Special case for compressed tar archives.
    if (base.right(4).toUpper() == QLatin1String(".TAR")) {
        base.chop(4);
    }

    return base;
}

QString Archive::fileName() const
{
    return isValid() ? m_iface->filename() : QString();
}

QString Archive::comment() const
{
    return isValid() ? m_iface->comment() : QString();
}

CommentJob* Archive::addComment(const QString &comment)
{
    if (!isValid()) {
        return Q_NULLPTR;
    }

    qCDebug(ARK) << "Going to add comment:" << comment;
    Q_ASSERT(!isReadOnly());
    CommentJob *job = new CommentJob(comment, static_cast<ReadWriteArchiveInterface*>(m_iface));
    return job;
}

TestJob* Archive::testArchive()
{
    if (!isValid()) {
        return Q_NULLPTR;
    }

    qCDebug(ARK) << "Going to test archive";

    TestJob *job = new TestJob(m_iface);
    return job;
}

QMimeType Archive::mimeType()
{
    if (!isValid()) {
        return QMimeType();
    }

    if (!m_mimeType.isValid()) {
        m_mimeType = determineMimeType(fileName());
    }

    return m_mimeType;
}

bool Archive::isReadOnly() const
{
    return isValid() ? (m_iface->isReadOnly() || m_isReadOnly) : false;
}

bool Archive::isSingleFolderArchive()
{
    if (!isValid()) {
        return false;
    }

    listIfNotListed();
    return m_isSingleFolderArchive;
}

bool Archive::hasComment() const
{
    return isValid() ? !comment().isEmpty() : false;
}

Archive::EncryptionType Archive::encryptionType()
{
    if (!isValid()) {
        return Unencrypted;
    }

    listIfNotListed();
    return m_encryptionType;
}

qulonglong Archive::numberOfFiles()
{
    if (!isValid()) {
        return 0;
    }

    listIfNotListed();
    return m_numberOfFiles;
}

qulonglong Archive::numberOfFolders()
{
    if (!isValid()) {
        return 0;
    }

    listIfNotListed();
    return m_numberOfFolders;
}

qulonglong Archive::unpackedSize()
{
    if (!isValid()) {
        return 0;
    }

    listIfNotListed();
    return m_extractedFilesSize;
}

qulonglong Archive::packedSize() const
{
    return isValid() ? QFileInfo(fileName()).size() : 0;
}

QString Archive::subfolderName()
{
    if (!isValid()) {
        return QString();
    }

    listIfNotListed();
    return m_subfolderName;
}

void Archive::onNewEntry(const ArchiveEntry &entry)
{
    entry[IsDirectory].toBool() ? m_numberOfFolders++ : m_numberOfFiles++;
}

bool Archive::isValid() const
{
    return m_iface && (m_error == NoError);
}

ArchiveError Archive::error() const
{
    return m_error;
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
    if (!isValid() || !QFileInfo::exists(fileName())) {
        return Q_NULLPTR;
    }

    qCDebug(ARK) << "Going to list files";

    ListJob *job = new ListJob(m_iface);

    //if this job has not been listed before, we grab the opportunity to
    //collect some information about the archive
    if (!m_hasBeenListed) {
        connect(job, &ListJob::result, this, &Archive::onListFinished);
    }
    return job;
}

DeleteJob* Archive::deleteFiles(const QList<QVariant> & files)
{
    if (!isValid()) {
        return Q_NULLPTR;
    }

    qCDebug(ARK) << "Going to delete files" << files;

    if (m_iface->isReadOnly()) {
        return 0;
    }
    DeleteJob *newJob = new DeleteJob(files, static_cast<ReadWriteArchiveInterface*>(m_iface));

    return newJob;
}

AddJob* Archive::addFiles(const QStringList & files, const CompressionOptions& options)
{
    if (!isValid()) {
        return Q_NULLPTR;
    }

    CompressionOptions newOptions = options;
    if (encryptionType() != Unencrypted) {
        newOptions[QStringLiteral("PasswordProtectedHint")] = true;
    }

    qCDebug(ARK) << "Going to add files" << files << "with options" << newOptions;
    Q_ASSERT(!m_iface->isReadOnly());

    AddJob *newJob = new AddJob(files, newOptions, static_cast<ReadWriteArchiveInterface*>(m_iface));
    connect(newJob, &AddJob::result, this, &Archive::onAddFinished);
    return newJob;
}

ExtractJob* Archive::copyFiles(const QList<QVariant>& files, const QString& destinationDir, const ExtractionOptions& options)
{
    if (!isValid()) {
        return Q_NULLPTR;
    }

    ExtractionOptions newOptions = options;
    if (encryptionType() != Unencrypted) {
        newOptions[QStringLiteral( "PasswordProtectedHint" )] = true;
    }

    ExtractJob *newJob = new ExtractJob(files, destinationDir, newOptions, m_iface);
    return newJob;
}

PreviewJob *Archive::preview(const QString &file)
{
    if (!isValid()) {
        return Q_NULLPTR;
    }

    PreviewJob *job = new PreviewJob(file, (encryptionType() != Unencrypted), m_iface);
    return job;
}

OpenJob *Archive::open(const QString &file)
{
    if (!isValid()) {
        return Q_NULLPTR;
    }

    OpenJob *job = new OpenJob(file, (encryptionType() != Unencrypted), m_iface);
    return job;
}

OpenWithJob *Archive::openWith(const QString &file)
{
    if (!isValid()) {
        return Q_NULLPTR;
    }

    OpenWithJob *job = new OpenWithJob(file, (encryptionType() != Unencrypted), m_iface);
    return job;
}

void Archive::encrypt(const QString &password, bool encryptHeader)
{
    if (!isValid()) {
        return;
    }

    m_iface->setPassword(password);
    m_iface->setHeaderEncryptionEnabled(encryptHeader);
    m_encryptionType = encryptHeader ? HeaderEncrypted : Encrypted;
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
    m_subfolderName = ljob->subfolderName();
    if (m_subfolderName.isEmpty()) {
        m_subfolderName = completeBaseName();
    }

    if (ljob->isPasswordProtected()) {
        // If we already know the password, it means that the archive is header-encrypted.
        m_encryptionType = m_iface->password().isEmpty() ? Encrypted : HeaderEncrypted;
    }

    m_hasBeenListed = true;
}

void Archive::listIfNotListed()
{
    if (!m_hasBeenListed) {
        ListJob *job = list();
        if (!job) {
            return;
        }

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

void Archive::setCompressionOptions(const CompressionOptions &opts)
{
    m_compOptions = opts;
}

CompressionOptions Archive::compressionOptions() const
{
    return m_compOptions;
}

} // namespace Kerfuffle
