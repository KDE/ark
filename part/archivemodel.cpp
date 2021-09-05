/*
    ark -- archiver for the KDE project

    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2010-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#include "archivemodel.h"
#include "ark_debug.h"
#include "jobs.h"
#include "util.h"
#include "qstringtokenizer.h"

#include <KIO/Global>
#include <KLocalizedString>

#include <QApplication>
#include <QDBusConnection>
#include <QMimeData>
#include <QRegularExpression>
#include <QStyle>
#include <QUrl>

using namespace Kerfuffle;

// Used to speed up the loading of large archives.
static Archive::Entry *s_previousMatch = nullptr;
Q_GLOBAL_STATIC(QString, s_previousPath)

ArchiveModel::ArchiveModel(const QString &dbusPathName, QObject *parent)
    : QAbstractItemModel(parent)
    , m_dbusPathName(dbusPathName)
    , m_numberOfFiles(0)
    , m_numberOfFolders(0)
    , m_fileEntryListed(false)
{
    initRootEntry();

    // Mappings between column indexes and entry properties.
    m_propertiesMap = {
        { FullPath, "fullPath" },
        { Size, "size" },
        { CompressedSize, "compressedSize" },
        { Permissions, "permissions" },
        { Owner, "owner" },
        { Group, "group" },
        { Ratio, "ratio" },
        { CRC, "CRC" },
        { BLAKE2, "BLAKE2" },
        { Method, "method" },
        { Version, "version" },
        { Timestamp, "timestamp" },
    };
}

ArchiveModel::~ArchiveModel()
{
}

QVariant ArchiveModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        Archive::Entry *entry = static_cast<Archive::Entry*>(index.internalPointer());
        switch (role) {
        case Qt::DisplayRole: {
            // TODO: complete the columns.
            int column = m_showColumns.at(index.column());
            switch (column) {
            case FullPath:
                return entry->name();
            case Size:
                if (entry->isDir()) {
                    uint dirs;
                    uint files;
                    entry->countChildren(dirs, files);
                    return KIO::itemsSummaryString(dirs + files, files, dirs, 0, false);
                } else if (!entry->property("link").toString().isEmpty()) {
                    return QVariant();
                } else {
                    return KIO::convertSize(entry->property("size").toULongLong());
                }
            case CompressedSize:
                if (entry->isDir() || !entry->property("link").toString().isEmpty()) {
                    return QVariant();
                } else {
                    qulonglong compressedSize = entry->property("compressedSize").toULongLong();
                    if (compressedSize != 0) {
                        return KIO::convertSize(compressedSize);
                    } else {
                        return QVariant();
                    }
                }
            case Ratio: // TODO: Use entry->metaData()[Ratio] when available.
                if (entry->isDir() || !entry->property("link").toString().isEmpty()) {
                    return QVariant();
                } else {
                    qulonglong compressedSize = entry->property("compressedSize").toULongLong();
                    qulonglong size = entry->property("size").toULongLong();
                    if (compressedSize == 0 || size == 0) {
                        return QVariant();
                    } else {
                        int ratio = int(100 * ((double)size - compressedSize) / size);
                        return QString(QString::number(ratio) + QStringLiteral(" %"));
                    }
                }

            case Timestamp: {
                const QDateTime timeStamp = entry->property("timestamp").toDateTime();
                return QLocale().toString(timeStamp, QLocale::ShortFormat);
            }

            default:
                return entry->property(m_propertiesMap[column].constData());
            }
        }
        case Qt::DecorationRole:
            if (index.column() == 0) {
                Archive::Entry *e = static_cast<Archive::Entry*>(index.internalPointer());
                QIcon::Mode mode = (filesToMove.contains(e->fullPath())) ? QIcon::Disabled : QIcon::Normal;
                return e->icon().pixmap(QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize), mode);
            }
            return QVariant();
        case Qt::FontRole: {
            QFont f;
            f.setItalic(entry->property("isPasswordProtected").toBool());
            return f;
        }
        default:
            return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags ArchiveModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (index.isValid()) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | defaultFlags;
    }

    return Qt::NoItemFlags;
}

QVariant ArchiveModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (section >= m_showColumns.size()) {
            qCDebug(ARK) << "WEIRD: showColumns.size = " << m_showColumns.size()
            << " and section = " << section;
            return QVariant();
        }

        int columnId = m_showColumns.at(section);

        switch (columnId) {
        case FullPath:
            return i18nc("Name of a file inside an archive", "Name");
        case Size:
            return i18nc("Uncompressed size of a file inside an archive", "Size");
        case CompressedSize:
            return i18nc("Compressed size of a file inside an archive", "Compressed");
        case Ratio:
            return i18nc("Compression rate of file", "Rate");
        case Owner:
            return i18nc("File's owner username", "Owner");
        case Group:
            return i18nc("File's group", "Group");
        case Permissions:
            return i18nc("File permissions", "Mode");
        case CRC:
            return i18nc("CRC hash code", "CRC checksum");
        case BLAKE2:
            return i18nc("BLAKE2 hash code", "BLAKE2 checksum");
        case Method:
            return i18nc("Compression method", "Method");
        case Version:
            // TODO: what exactly is a file version?
            return i18nc("File version", "Version");
        case Timestamp:
            return i18nc("Timestamp", "Date");
        default:
            return i18nc("Unnamed column", "??");
        }
    }
    return QVariant();
}

QModelIndex ArchiveModel::index(int row, int column, const QModelIndex &parent) const
{
    if (hasIndex(row, column, parent)) {
        const Archive::Entry *parentEntry = parent.isValid()
                                            ? static_cast<Archive::Entry*>(parent.internalPointer())
                                            : m_rootEntry.data();

        Q_ASSERT(parentEntry->isDir());

        const Archive::Entry *item = parentEntry->entries().value(row, nullptr);
        if (item != nullptr) {
            return createIndex(row, column, const_cast<Archive::Entry*>(item));
        }
    }

    return QModelIndex();
}

QModelIndex ArchiveModel::parent(const QModelIndex &index) const
{
    if (index.isValid()) {
        Archive::Entry *item = static_cast<Archive::Entry*>(index.internalPointer());
        Q_ASSERT(item);
        if (item->getParent() && (item->getParent() != m_rootEntry.data())) {
            return createIndex(item->getParent()->row(), 0, item->getParent());
        }
    }
    return QModelIndex();
}

Archive::Entry *ArchiveModel::entryForIndex(const QModelIndex &index)
{
    if (index.isValid()) {
        Archive::Entry *item = static_cast<Archive::Entry*>(index.internalPointer());
        Q_ASSERT(item);
        return item;
    }
    return nullptr;
}

int ArchiveModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() <= 0) {
        const Archive::Entry *parentEntry = parent.isValid()
                                            ? static_cast<Archive::Entry*>(parent.internalPointer())
                                            : m_rootEntry.data();

        if (parentEntry && parentEntry->isDir()) {
            return parentEntry->entries().count();
        }
    }
    return 0;
}

int ArchiveModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_showColumns.size();
}

Qt::DropActions ArchiveModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList ArchiveModel::mimeTypes() const
{
    QStringList types;

    // MIME types we accept for dragging (eg. Dolphin -> Ark).
    types << QStringLiteral("text/uri-list")
          << QStringLiteral("text/plain")
          << QStringLiteral("text/x-moz-url");

    // MIME types we accept for dropping (eg. Ark -> Dolphin).
    types << QStringLiteral("application/x-kde-ark-dndextract-service")
          << QStringLiteral("application/x-kde-ark-dndextract-path");

    return types;
}

QMimeData *ArchiveModel::mimeData(const QModelIndexList &indexes) const
{
    Q_UNUSED(indexes)

    QMimeData *mimeData = new QMimeData;
    mimeData->setData(QStringLiteral("application/x-kde-ark-dndextract-service"),
                      QDBusConnection::sessionBus().baseService().toUtf8());
    mimeData->setData(QStringLiteral("application/x-kde-ark-dndextract-path"),
                      m_dbusPathName.toUtf8());

    return mimeData;
}

bool ArchiveModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action)

    if (!data->hasUrls()) {
        return false;
    }

    if (archive()->isReadOnly() ||
        (archive()->encryptionType() != Archive::Unencrypted &&
         archive()->password().isEmpty())) {
        Q_EMIT messageWidget(KMessageWidget::Error, i18n("Adding files is not supported for this archive."));
        return false;
    }

    QStringList paths;
    const auto urls = data->urls();
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            Q_EMIT messageWidget(KMessageWidget::Error, i18n("You can only add local files to an archive."));
            return false;
        }
        paths << url.toLocalFile();
    }

    const Archive::Entry *entry = nullptr;
    QModelIndex droppedOnto = index(row, column, parent);
    if (droppedOnto.isValid()) {
        entry = entryForIndex(droppedOnto);
        if (!entry->isDir()) {
            entry = entry->getParent();
        }
    }

    Q_EMIT droppedFiles(paths, entry);

    return true;
}

// For a rationale, see bugs #194241, #241967 and #355839
QString ArchiveModel::cleanFileName(const QString& fileName)
{
    // Skip entries with filename "/" or "//" or "."
    // "." is present in ISO files.
    static QRegularExpression pattern(QStringLiteral("/+|\\."));
    QRegularExpressionMatch match;
    if (fileName.contains(pattern, &match) && match.captured() == fileName) {
        qCDebug(ARK) << "Skipping entry with filename" << fileName;
        return QString();
    } else if (fileName.startsWith(QLatin1String("./"))) {
        return fileName.mid(2);
    }

    return fileName;
}

void ArchiveModel::initRootEntry()
{
    m_rootEntry.reset(new Archive::Entry());
    m_rootEntry->setProperty("isDirectory", true);
}

Archive::Entry *ArchiveModel::parentFor(const Archive::Entry *entry, InsertBehaviour behaviour)
{
    QString fullPath = entry->fullPath();

    if (fullPath.endsWith(QLatin1Char('/'))) {
        fullPath = fullPath.chopped(1);
    }

    // Used to speed up loading of large archives.
    const int index = fullPath.lastIndexOf(QLatin1Char('/'));
    const QString folderPath = index != -1 ? fullPath.left(index) : QString();

    if (s_previousMatch && *s_previousPath == folderPath) {
        return s_previousMatch;
    }

    Archive::Entry *parent = m_rootEntry.data();

    const auto pieces = QStringTokenizer{folderPath, QLatin1Char('/'), Qt::SkipEmptyParts};

    for (const auto piece : pieces) {
        Archive::Entry *entry = parent->find(piece);
        if (!entry) {
            // Directory entry will be traversed later (that happens for some archive formats, 7z for instance).
            // We have to create one before, in order to construct tree from its children,
            // and then delete the existing one (see ArchiveModel::newEntry).
            entry = new Archive::Entry(parent);

            entry->setProperty("fullPath", (parent == m_rootEntry.data())
                                           ? QString(piece + QLatin1Char('/'))
                                           : QString(parent->fullPath(WithTrailingSlash) + piece + QLatin1Char('/')));
            entry->setProperty("isDirectory", true);
            insertEntry(entry, behaviour);
        }
        if (!entry->isDir()) {
            Archive::Entry *e = new Archive::Entry(parent);
            e->copyMetaData(entry);
            // Maybe we have both a file and a directory of the same name.
            // We avoid removing previous entries unless necessary.
            insertEntry(e, behaviour);
        }
        parent = entry;
    }

    s_previousMatch = parent;
    *s_previousPath = folderPath;

    return parent;
}

QModelIndex ArchiveModel::indexForEntry(Archive::Entry *entry)
{
    Q_ASSERT(entry);
    if (entry != m_rootEntry.data()) {
        Q_ASSERT(entry->getParent());
        Q_ASSERT(entry->getParent()->isDir());
        return createIndex(entry->row(), 0, entry);
    }
    return QModelIndex();
}

void ArchiveModel::slotEntryRemoved(const QString & path)
{
    const QString entryFileName(cleanFileName(path));
    if (entryFileName.isEmpty()) {
        return;
    }

    Archive::Entry *entry = m_rootEntry->findByPath(entryFileName.split(QLatin1Char('/'), Qt::SkipEmptyParts));
    if (entry) {
        Archive::Entry *parent = entry->getParent();
        QModelIndex index = indexForEntry(entry);
        Q_UNUSED(index);

        beginRemoveRows(indexForEntry(parent), entry->row(), entry->row());
        parent->removeEntryAt(entry->row());
        endRemoveRows();
    }
}

void ArchiveModel::slotUserQuery(Kerfuffle::Query *query)
{
    query->execute();
}

void ArchiveModel::slotNewEntry(Archive::Entry *entry)
{
    newEntry(entry, NotifyViews);
}

void ArchiveModel::slotListEntry(Archive::Entry *entry)
{
    newEntry(entry, DoNotNotifyViews);
}

void ArchiveModel::newEntry(Archive::Entry *receivedEntry, InsertBehaviour behaviour)
{
    if (receivedEntry->fullPath().isEmpty()) {
        qCDebug(ARK) << "Weird, received empty entry (no filename) - skipping";
        return;
    }

    // If there are no columns registered, then populate columns from entry. If the first entry
    // is a directory we check again for the first file entry to ensure all relevant columms are shown.
    if (m_showColumns.isEmpty() || !m_fileEntryListed) {
        QList<int> toInsert;

        const auto size = receivedEntry->property("size").toULongLong();
        const auto compressedSize = receivedEntry->property("compressedSize").toULongLong();
        for (auto i = m_propertiesMap.begin(); i != m_propertiesMap.end(); ++i) {
            // Singlefile plugin doesn't report the uncompressed size.
            if (i.key() == Size && size == 0 && compressedSize > 0) {
                continue;
            }
            if (!receivedEntry->property(i.value().constData()).toString().isEmpty()) {
                if (i.key() != CompressedSize || receivedEntry->compressedSizeIsSet) {
                    if (!m_showColumns.contains(i.key())) {
                        toInsert << i.key();
                    }
                }
            }
        }
        if (behaviour == NotifyViews) {
            beginInsertColumns(QModelIndex(), 0, toInsert.size() - 1);
        }
        m_showColumns << toInsert;
        if (behaviour == NotifyViews) {
            endInsertColumns();
        }

        m_fileEntryListed = !receivedEntry->isDir();
    }

    // #194241: Filenames such as "./file" should be displayed as "file"
    // #241967: Entries called "/" should be ignored
    // #355839: Entries called "//" should be ignored
    QString entryFileName = cleanFileName(receivedEntry->fullPath());
    if (entryFileName.isEmpty()) { // The entry contains only "." or "./"
        return;
    }
    receivedEntry->setProperty("fullPath", entryFileName);

    // For some archive formats (e.g. AppImage and RPM) paths of folders do not
    // contain a trailing slash, so we append it.
    if (receivedEntry->property("isDirectory").toBool() &&
        !receivedEntry->property("fullPath").toString().endsWith(QLatin1Char('/'))) {
        receivedEntry->setProperty("fullPath", QString(receivedEntry->property("fullPath").toString() + QLatin1Char('/')));
        qCDebug(ARK) << "Trailing slash appended to entry:" << receivedEntry->property("fullPath");
    }

    // Skip already created entries.
    Archive::Entry *existing = m_rootEntry->findByPath(entryFileName.split(QLatin1Char('/')));
    if (existing) {
        existing->setProperty("fullPath", entryFileName);
        // Multi-volume files are repeated at least in RAR archives.
        // In that case, we need to sum the compressed size for each volume
        qulonglong currentCompressedSize = existing->property("compressedSize").toULongLong();
        existing->setProperty("compressedSize", currentCompressedSize + receivedEntry->property("compressedSize").toULongLong());
        return;
    }

    // Find parent entry, creating missing directory Archive::Entry's in the process.
    Archive::Entry *parent = parentFor(receivedEntry, behaviour);

    // Create an Archive::Entry.
    Archive::Entry *entry = parent->find(Kerfuffle::Util::lastPathSegment(entryFileName));
    if (entry) {
        entry->copyMetaData(receivedEntry);
        entry->setProperty("fullPath", entryFileName);
    } else {
        receivedEntry->setParent(parent);
        insertEntry(receivedEntry, behaviour);
    }
}

void ArchiveModel::slotLoadingFinished(KJob *job)
{
    std::sort(m_showColumns.begin(), m_showColumns.end());

    if (!job->error()) {

        qCDebug(ARK) << "Showing columns: " << m_showColumns;

        m_archive.reset(qobject_cast<LoadJob*>(job)->archive());

        beginResetModel();
        endResetModel();
    }

    Q_EMIT loadingFinished(job);
}

void ArchiveModel::insertEntry(Archive::Entry *entry, InsertBehaviour behaviour)
{
    Q_ASSERT(entry);
    Archive::Entry *parent = entry->getParent();
    Q_ASSERT(parent);
    if (behaviour == NotifyViews) {
        beginInsertRows(indexForEntry(parent), parent->entries().count(), parent->entries().count());
    }
    parent->appendEntry(entry);
    if (behaviour == NotifyViews) {
        endInsertRows();
    }
}

Kerfuffle::Archive* ArchiveModel::archive() const
{
    return m_archive.data();
}

void ArchiveModel::reset()
{
    m_archive.reset(nullptr);
    s_previousMatch = nullptr;
    s_previousPath->clear();
    initRootEntry();

    // TODO: make sure if it's ok to not have calls to beginRemoveColumns here
    m_showColumns.clear();
    beginResetModel();
    endResetModel();
}

void ArchiveModel::createEmptyArchive(const QString &path, const QString &mimeType, QObject *parent)
{
    reset();
    m_archive.reset(Archive::createEmpty(path, mimeType, parent));
}

KJob *ArchiveModel::loadArchive(const QString &path, const QString &mimeType, QObject *parent)
{
    reset();

    auto loadJob = Archive::load(path, mimeType, parent);
    connect(loadJob, &KJob::result, this, &ArchiveModel::slotLoadingFinished);
    connect(loadJob, &Job::newEntry, this, &ArchiveModel::slotListEntry);
    connect(loadJob, &Job::userQuery, this, &ArchiveModel::slotUserQuery);

    Q_EMIT loadingStarted();

    return loadJob;
}

ExtractJob* ArchiveModel::extractFile(Archive::Entry *file, const QString& destinationDir, Kerfuffle::ExtractionOptions options) const
{
    QVector<Archive::Entry*> files({file});
    return extractFiles(files, destinationDir, options);
}

ExtractJob* ArchiveModel::extractFiles(const QVector<Archive::Entry*>& files, const QString& destinationDir, Kerfuffle::ExtractionOptions options) const
{
    Q_ASSERT(m_archive);
    ExtractJob *newJob = m_archive->extractFiles(files, destinationDir, options);
    connect(newJob, &ExtractJob::userQuery, this, &ArchiveModel::slotUserQuery);
    return newJob;
}

Kerfuffle::PreviewJob *ArchiveModel::preview(Archive::Entry *file) const
{
    Q_ASSERT(m_archive);
    PreviewJob *job = m_archive->preview(file);
    connect(job, &Job::userQuery, this, &ArchiveModel::slotUserQuery);
    return job;
}

OpenJob *ArchiveModel::open(Archive::Entry *file) const
{
    Q_ASSERT(m_archive);
    OpenJob *job = m_archive->open(file);
    connect(job, &Job::userQuery, this, &ArchiveModel::slotUserQuery);
    return job;
}

OpenWithJob *ArchiveModel::openWith(Archive::Entry *file) const
{
    Q_ASSERT(m_archive);
    OpenWithJob *job = m_archive->openWith(file);
    connect(job, &Job::userQuery, this, &ArchiveModel::slotUserQuery);
    return job;
}

AddJob* ArchiveModel::addFiles(QVector<Archive::Entry*> &entries, const Archive::Entry *destination, const CompressionOptions& options)
{
    if (!m_archive) {
        return nullptr;
    }

    if (!m_archive->isReadOnly()) {
        AddJob *job = m_archive->addFiles(entries, destination, options);
        connect(job, &AddJob::newEntry, this, &ArchiveModel::slotNewEntry);
        connect(job, &AddJob::userQuery, this, &ArchiveModel::slotUserQuery);


        return job;
    }
    return nullptr;
}

Kerfuffle::MoveJob *ArchiveModel::moveFiles(QVector<Archive::Entry*> &entries, Archive::Entry *destination, const CompressionOptions &options)
{
    if (!m_archive) {
        return nullptr;
    }

    if (!m_archive->isReadOnly()) {
        MoveJob *job = m_archive->moveFiles(entries, destination, options);
        connect(job, &MoveJob::newEntry, this, &ArchiveModel::slotNewEntry);
        connect(job, &MoveJob::userQuery, this, &ArchiveModel::slotUserQuery);
        connect(job, &MoveJob::entryRemoved, this, &ArchiveModel::slotEntryRemoved);
        connect(job, &MoveJob::finished, this, &ArchiveModel::slotCleanupEmptyDirs);


        return job;
    }
    return nullptr;
}
Kerfuffle::CopyJob *ArchiveModel::copyFiles(QVector<Archive::Entry*> &entries, Archive::Entry *destination, const CompressionOptions &options)
{
    if (!m_archive) {
        return nullptr;
    }

    if (!m_archive->isReadOnly()) {
        CopyJob *job = m_archive->copyFiles(entries, destination, options);
        connect(job, &CopyJob::newEntry, this, &ArchiveModel::slotNewEntry);
        connect(job, &CopyJob::userQuery, this, &ArchiveModel::slotUserQuery);


        return job;
    }
    return nullptr;
}

DeleteJob* ArchiveModel::deleteFiles(QVector<Archive::Entry*> entries)
{
    Q_ASSERT(m_archive);
    if (!m_archive->isReadOnly()) {
        DeleteJob *job = m_archive->deleteFiles(entries);
        connect(job, &DeleteJob::entryRemoved, this, &ArchiveModel::slotEntryRemoved);

        connect(job, &DeleteJob::finished, this, &ArchiveModel::slotCleanupEmptyDirs);

        connect(job, &DeleteJob::userQuery, this, &ArchiveModel::slotUserQuery);
        return job;
    }
    return nullptr;
}

void ArchiveModel::encryptArchive(const QString &password, bool encryptHeader)
{
    if (!m_archive) {
        return;
    }

    m_archive->encrypt(password, encryptHeader);
}

bool ArchiveModel::conflictingEntries(QList<const Archive::Entry*> &conflictingEntries, const QStringList &entries, bool allowMerging) const
{
    bool error = false;

    // We can't accept destination as an argument, because it can be a new entry path for renaming.
    const Archive::Entry *destination;
    {
        QStringList destinationParts = entries.first().split(QLatin1Char('/'), Qt::SkipEmptyParts);
        destinationParts.removeLast();
        if (destinationParts.count() > 0) {
            destination = m_rootEntry->findByPath(destinationParts);
        } else {
            destination = m_rootEntry.data();
        }
    }
    const Archive::Entry *lastDirEntry = destination;
    QString skippedDirPath;

    for (const QString &entry : entries) {
        if (skippedDirPath.count() > 0 && entry.startsWith(skippedDirPath)) {
            continue;
        } else {
            skippedDirPath.clear();
        }

        while (!entry.startsWith(lastDirEntry->fullPath())) {
            lastDirEntry = lastDirEntry->getParent();
        }

        bool isDir = entry.right(1) == QLatin1String("/");
        const Archive::Entry *archiveEntry = lastDirEntry->find(entry.split(QLatin1Char('/'), Qt::SkipEmptyParts).last());

        if (archiveEntry != nullptr) {
            if (archiveEntry->isDir() != isDir || !allowMerging) {
                if (isDir) {
                    skippedDirPath = lastDirEntry->fullPath();
                }

                if (!error) {
                    conflictingEntries.clear();
                    error = true;
                }
                conflictingEntries << archiveEntry;
            } else {
                if (isDir) {
                    lastDirEntry = archiveEntry;
                }
                else if (!error) {
                    conflictingEntries << archiveEntry;
                }
            }
        } else if (isDir) {
            skippedDirPath = entry;
        }
    }

    return error;
}

bool ArchiveModel::hasDuplicatedEntries(const QStringList &entries)
{
    QStringList tempList;
    for (const QString &entry : entries) {
        if (tempList.contains(entry)) {
            return true;
        }
        tempList << entry;
    }
    return false;
}

QMap<QString, Archive::Entry*> ArchiveModel::entryMap(const QVector<Archive::Entry*> &entries)
{
    QMap<QString, Archive::Entry*> map;
    for (Archive::Entry *entry : entries) {
        map.insert(entry->fullPath(), entry);
    }
    return map;
}

void ArchiveModel::slotCleanupEmptyDirs()
{
    QList<QPersistentModelIndex> queue;
    QList<QPersistentModelIndex> nodesToDelete;

    // Add root nodes.
    for (int i = 0; i < rowCount(); ++i) {
        queue.append(QPersistentModelIndex(index(i, 0)));
    }

    // Breadth-first traverse.
    while (!queue.isEmpty()) {
        QPersistentModelIndex node = queue.takeFirst();
        Archive::Entry *entry = entryForIndex(node);

        if (!hasChildren(node)) {
            if (entry->fullPath().isEmpty()) {
                nodesToDelete << node;
            }
        } else {
            for (int i = 0; i < rowCount(node); ++i) {
                queue.append(QPersistentModelIndex(index(i, 0, node)));
            }
        }
    }

    for (const QPersistentModelIndex& node : std::as_const(nodesToDelete)) {
        Archive::Entry *rawEntry = static_cast<Archive::Entry*>(node.internalPointer());
        qCDebug(ARK) << "Delete with parent entries " << rawEntry->getParent()->entries() << " and row " << rawEntry->row();
        beginRemoveRows(parent(node), rawEntry->row(), rawEntry->row());
        rawEntry->getParent()->removeEntryAt(rawEntry->row());
        endRemoveRows();
    }
}

void ArchiveModel::countEntriesAndSize()
{
    // This function is used to count the number of folders/files and
    // the total compressed size. This is needed for PropertiesDialog
    // to update the corresponding values after adding/deleting files.

    // When ArchiveModel has been properly fixed, this code can likely
    // be removed.

    m_numberOfFiles = 0;
    m_numberOfFolders = 0;
    m_uncompressedSize = 0;

    QElapsedTimer timer;
    timer.start();

    traverseAndCountDirNode(m_rootEntry.data());

    qCDebug(ARK) << "Time to count entries and size:" << timer.elapsed() << "ms";
}

void ArchiveModel::traverseAndCountDirNode(Archive::Entry *dir)
{
    const auto entries = dir->entries();
    for (Archive::Entry *entry : entries) {
        if (entry->isDir()) {
            traverseAndCountDirNode(entry);
            m_numberOfFolders++;
        } else {
            m_numberOfFiles++;
            m_uncompressedSize += entry->property("size").toULongLong();
        }
    }
}

qulonglong ArchiveModel::numberOfFiles() const
{
    return m_numberOfFiles;
}

qulonglong ArchiveModel::numberOfFolders() const
{
    return m_numberOfFolders;
}

qulonglong ArchiveModel::uncompressedSize() const
{
    return m_uncompressedSize;
}

QList<int> ArchiveModel::shownColumns() const
{
    return m_showColumns;
}

QMap<int, QByteArray> ArchiveModel::propertiesMap() const
{
    return m_propertiesMap;
}
