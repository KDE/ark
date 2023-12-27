/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "archive_kerfuffle.h"
#include "archiveinterface.h"
#include "ark_debug.h"
#include "jobs.h"
#include "mimetypes.h"
#include "pluginmanager.h"

#include <KPluginFactory>

#include <QMimeDatabase>
#include <QRegularExpression>

namespace Kerfuffle
{
Archive *Archive::create(const QString &fileName, QObject *parent)
{
    return create(fileName, QString(), parent);
}

Archive *Archive::create(const QString &fileName, const QString &fixedMimeType, QObject *parent)
{
    qCDebug(ARK) << "Going to create archive" << fileName;

    PluginManager pluginManager;
    const QMimeType mimeType = fixedMimeType.isEmpty() ? determineMimeType(fileName) : QMimeDatabase().mimeTypeForName(fixedMimeType);

    qCDebug(ARK) << "Looking for plugins that handle the mimetype: " << mimeType.name();
    QVector<Plugin *> offers = pluginManager.preferredPluginsFor(mimeType);
    if (offers.isEmpty()) {
        if (fixedMimeType.isEmpty()) {
            const QMimeType extensionMimeType = determineMimeType(fileName, PreferExtensionMime);
            offers = pluginManager.preferredPluginsFor(extensionMimeType);
        }
        if (offers.isEmpty()) {
            qCCritical(ARK) << "Could not find a plugin to handle" << fileName;
            return new Archive(NoPlugin, parent);
        }
    }

    Archive *archive = nullptr;
    for (Plugin *plugin : std::as_const(offers)) {
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

    const QVariantList args = {QVariant(QFileInfo(fileName).absoluteFilePath()), QVariant().fromValue(plugin->metaData())};
    ReadOnlyArchiveInterface *iface = KPluginFactory::instantiatePlugin<ReadOnlyArchiveInterface>(plugin->metaData(), nullptr, args).plugin;
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

BatchExtractJob *Archive::batchExtract(const QString &fileName, const QString &destination, bool autoSubfolder, bool preservePaths, QObject *parent)
{
    auto loadJob = load(fileName, parent);
    auto batchJob = new BatchExtractJob(loadJob, destination, autoSubfolder, preservePaths);

    return batchJob;
}

CreateJob *
Archive::create(const QString &fileName, const QString &mimeType, const QVector<Archive::Entry *> &entries, const CompressionOptions &options, QObject *parent)
{
    auto archive = create(fileName, mimeType, parent);
    auto createJob = new CreateJob(archive, entries, options);

    return createJob;
}

Archive *Archive::createEmpty(const QString &fileName, const QString &mimeType, QObject *parent)
{
    auto archive = create(fileName, mimeType, parent);
    Q_ASSERT(archive->isEmpty());

    return archive;
}

LoadJob *Archive::load(const QString &fileName, QObject *parent)
{
    return load(fileName, QString(), parent);
}

LoadJob *Archive::load(const QString &fileName, const QString &mimeType, QObject *parent)
{
    auto archive = create(fileName, mimeType, parent);
    auto loadJob = new LoadJob(archive);

    return loadJob;
}

LoadJob *Archive::load(const QString &fileName, Plugin *plugin, QObject *parent)
{
    auto archive = create(fileName, plugin, parent);
    auto loadJob = new LoadJob(archive);

    return loadJob;
}

Archive::Archive(ArchiveError errorCode, QObject *parent)
    : QObject(parent)
    , m_iface(nullptr)
    , m_error(errorCode)
{
    qCDebug(ARK) << "Created archive instance with error";
}

Archive::Archive(ReadOnlyArchiveInterface *archiveInterface, bool isReadOnly, QObject *parent)
    : QObject(parent)
    , m_iface(archiveInterface)
    , m_isReadOnly(isReadOnly)
    , m_isSingleFolder(false)
    , m_isMultiVolume(false)
    , m_extractedFilesSize(0)
    , m_error(NoError)
    , m_encryptionType(Unencrypted)
{
    qCDebug(ARK) << "Created archive instance";

    Q_ASSERT(m_iface);
    m_iface->setParent(this);

    connect(m_iface, &ReadOnlyArchiveInterface::compressionMethodFound, this, &Archive::onCompressionMethodFound);
    connect(m_iface, &ReadOnlyArchiveInterface::encryptionMethodFound, this, &Archive::onEncryptionMethodFound);
}

void Archive::onCompressionMethodFound(const QString &method)
{
    QStringList methods = property("compressionMethods").toStringList();

    if (!methods.contains(method) && method != QLatin1String("Store")) {
        methods.append(method);
    }
    methods.sort();

    setProperty("compressionMethods", methods);
}

void Archive::onEncryptionMethodFound(const QString &method)
{
    QStringList methods = property("encryptionMethods").toStringList();

    if (!methods.contains(method)) {
        methods.append(method);
    }
    methods.sort();

    setProperty("encryptionMethods", methods);
}

Archive::~Archive()
{
}

QString Archive::completeBaseName() const
{
    const QString suffix = QFileInfo(fileName()).suffix();
    QString base = QFileInfo(fileName()).completeBaseName();

    // Special case for compressed tar archives.
    if (base.right(4).toUpper() == QLatin1String(".TAR")) {
        base.chop(4);

        // Multi-volume 7z's are named name.7z.001.
    } else if (base.right(3).toUpper() == QLatin1String(".7Z")) {
        base.chop(3);

        // Multi-volume zip's are named name.zip.001.
    } else if (base.right(4).toUpper() == QLatin1String(".ZIP")) {
        base.chop(4);

        // For multivolume rar's we want to remove the ".partNNN" suffix.
    } else if (suffix.toUpper() == QLatin1String("RAR")) {
        base.remove(QRegularExpression(QStringLiteral("\\.part[0-9]{1,3}$")));
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

CommentJob *Archive::addComment(const QString &comment)
{
    if (!isValid()) {
        return nullptr;
    }

    qCDebug(ARK) << "Going to add comment:" << comment;
    Q_ASSERT(!isReadOnly());
    CommentJob *job = new CommentJob(comment, static_cast<ReadWriteArchiveInterface *>(m_iface));
    return job;
}

TestJob *Archive::testArchive()
{
    if (!isValid()) {
        return nullptr;
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

bool Archive::isEmpty() const
{
    return (numberOfEntries() == 0);
}

bool Archive::isReadOnly() const
{
    return isValid() ? (m_iface->isReadOnly() || m_isReadOnly || (isMultiVolume() && (numberOfEntries() > 0))) : false;
}

bool Archive::isSingleFile() const
{
    // If the only entry is a folder, isSingleFolder() is true.
    return numberOfEntries() == 1 && !isSingleFolder();
}

bool Archive::isSingleFolder() const
{
    if (!isValid()) {
        return false;
    }

    return m_isSingleFolder;
}

bool Archive::hasComment() const
{
    return isValid() ? !comment().isEmpty() : false;
}

bool Archive::isMultiVolume() const
{
    if (!isValid()) {
        return false;
    }

    return m_iface->isMultiVolume();
}

void Archive::setMultiVolume(bool value)
{
    m_iface->setMultiVolume(value);
}

int Archive::numberOfVolumes() const
{
    return m_iface->numberOfVolumes();
}

Archive::EncryptionType Archive::encryptionType() const
{
    if (!isValid()) {
        return Unencrypted;
    }

    return m_encryptionType;
}

QString Archive::password() const
{
    return m_iface->password();
}

uint Archive::numberOfEntries() const
{
    if (!isValid()) {
        return 0;
    }

    return m_iface->numberOfEntries();
}

qulonglong Archive::unpackedSize() const
{
    if (!isValid()) {
        return 0;
    }

    return m_extractedFilesSize;
}

qulonglong Archive::packedSize() const
{
    return isValid() ? static_cast<qulonglong>(QFileInfo(fileName()).size()) : 0;
}

QString Archive::subfolderName() const
{
    if (!isValid()) {
        return QString();
    }

    return m_subfolderName;
}

bool Archive::isValid() const
{
    return m_iface && (m_error == NoError);
}

ArchiveError Archive::error() const
{
    return m_error;
}

DeleteJob *Archive::deleteFiles(QVector<Archive::Entry *> &entries)
{
    if (!isValid()) {
        return nullptr;
    }

    qCDebug(ARK) << "Going to delete" << entries.size() << "entries";

    if (m_iface->isReadOnly()) {
        return nullptr;
    }
    DeleteJob *newJob = new DeleteJob(entries, static_cast<ReadWriteArchiveInterface *>(m_iface));

    return newJob;
}

AddJob *Archive::addFiles(const QVector<Archive::Entry *> &files, const Archive::Entry *destination, const CompressionOptions &options)
{
    if (!isValid()) {
        return nullptr;
    }

    CompressionOptions newOptions = options;
    if (encryptionType() != Unencrypted) {
        newOptions.setEncryptedArchiveHint(true);
    }

    qCDebug(ARK) << "Going to add files" << files << "with options" << newOptions;
    Q_ASSERT(!m_iface->isReadOnly());

    AddJob *newJob = new AddJob(files, destination, newOptions, static_cast<ReadWriteArchiveInterface *>(m_iface));
    connect(newJob, &AddJob::result, this, &Archive::onAddFinished);
    return newJob;
}

MoveJob *Archive::moveFiles(const QVector<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    if (!isValid()) {
        return nullptr;
    }

    CompressionOptions newOptions = options;
    if (encryptionType() != Unencrypted) {
        newOptions.setEncryptedArchiveHint(true);
    }

    qCDebug(ARK) << "Going to move files" << files << "to destination" << destination << "with options" << newOptions;
    Q_ASSERT(!m_iface->isReadOnly());

    MoveJob *newJob = new MoveJob(files, destination, newOptions, static_cast<ReadWriteArchiveInterface *>(m_iface));
    return newJob;
}

CopyJob *Archive::copyFiles(const QVector<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    if (!isValid()) {
        return nullptr;
    }

    CompressionOptions newOptions = options;
    if (encryptionType() != Unencrypted) {
        newOptions.setEncryptedArchiveHint(true);
    }

    qCDebug(ARK) << "Going to copy files" << files << "with options" << newOptions;
    Q_ASSERT(!m_iface->isReadOnly());

    CopyJob *newJob = new CopyJob(files, destination, newOptions, static_cast<ReadWriteArchiveInterface *>(m_iface));
    return newJob;
}

ExtractJob *Archive::extractFiles(const QVector<Archive::Entry *> &files, const QString &destinationDir, ExtractionOptions options)
{
    if (!isValid()) {
        return nullptr;
    }

    ExtractionOptions newOptions = options;
    if (encryptionType() != Unencrypted) {
        newOptions.setEncryptedArchiveHint(true);
    }

    ExtractJob *newJob = new ExtractJob(files, destinationDir, newOptions, m_iface);
    return newJob;
}

PreviewJob *Archive::preview(Archive::Entry *entry)
{
    if (!isValid()) {
        return nullptr;
    }

    PreviewJob *job = new PreviewJob(entry, (encryptionType() != Unencrypted), m_iface);
    return job;
}

OpenJob *Archive::open(Archive::Entry *entry)
{
    if (!isValid()) {
        return nullptr;
    }

    OpenJob *job = new OpenJob(entry, (encryptionType() != Unencrypted), m_iface);
    return job;
}

OpenWithJob *Archive::openWith(Archive::Entry *entry)
{
    if (!isValid()) {
        return nullptr;
    }

    OpenWithJob *job = new OpenWithJob(entry, (encryptionType() != Unencrypted), m_iface);
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

void Archive::onAddFinished(KJob *job)
{
    // if the archive was previously a single folder archive and an add job
    // has successfully finished, then it is no longer a single folder
    // archive (for the current implementation, which does not allow adding
    // folders/files other places than the root.
    // TODO: handle the case of creating a new file and singlefolderarchive
    // then.
    if (m_isSingleFolder && !job->error()) {
        m_isSingleFolder = false;
    }
}

void Archive::onUserQuery(Query *query)
{
    query->execute();
}

QString Archive::multiVolumeName() const
{
    return m_iface->multiVolumeName();
}

ReadOnlyArchiveInterface *Archive::interface()
{
    return m_iface;
}

bool Archive::hasMultipleTopLevelEntries() const
{
    return !isSingleFile() && !isSingleFolder();
}

} // namespace Kerfuffle

#include "moc_archive_kerfuffle.cpp"
