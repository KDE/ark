/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2010-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "archivemodel.h"
#include "kerfuffle/jobs.h"

#include <KLocalizedString>
#include <kio/global.h>

#include <QDBusConnection>
#include <QMimeData>
#include <QRegularExpression>
#include <QUrl>

using namespace Kerfuffle;

//used to speed up the loading of large archives
static Archive::Entry *s_previousMatch = Q_NULLPTR;
Q_GLOBAL_STATIC(QStringList, s_previousPieces)

/**
 * Meta data related to one entry in a compressed archive.
 *
 * This is used for indexing entry properties as numbers
 * and for determining data displaying order in part's view.
 */
enum EntryMetaDataType {
    FullPath,            /**< The entry's file name */
    Size,                /**< The entry's original size */
    CompressedSize,      /**< The compressed size for the entry */
    Permissions,         /**< The entry's permissions */
    Owner,               /**< The user the entry belongs to */
    Group,               /**< The user group the entry belongs to */
    Ratio,               /**< The compression ratio for the entry */
    CRC,                 /**< The entry's CRC */
    Method,              /**< The compression method used on the entry */
    Version,             /**< The archiver version needed to extract the entry */
    Timestamp,           /**< The timestamp for the current entry */
    Comment,
};

/**
 * Mappings between column indexes and entry properties.
 */
static QMap<int, QString> initializePropertiesList() {
    QMap<int, QString> propertiesList = QMap<int, QString>();
    propertiesList.insert(FullPath, QStringLiteral("fullPath"));
    propertiesList.insert(Size, QStringLiteral("size"));
    propertiesList.insert(CompressedSize, QStringLiteral("compressedSize"));
    propertiesList.insert(Permissions, QStringLiteral("permissions"));
    propertiesList.insert(Owner, QStringLiteral("owner"));
    propertiesList.insert(Group, QStringLiteral("group"));
    propertiesList.insert(Ratio, QStringLiteral("ratio"));
    propertiesList.insert(CRC, QStringLiteral("CRC"));
    propertiesList.insert(Method, QStringLiteral("method"));
    propertiesList.insert(Version, QStringLiteral("version"));
    propertiesList.insert(Timestamp, QStringLiteral("timestamp"));
    propertiesList.insert(Comment, QStringLiteral("comment"));
    return propertiesList;
}
static const QMap<int, QString> propertiesList = initializePropertiesList();

/**
 * Helper functor used by qStableSort.
 *
 * It always sorts folders before files.
 *
 * @internal
 */
class ArchiveModelSorter
{
public:
    ArchiveModelSorter(int column, Qt::SortOrder order)
        : m_sortColumn(column)
        , m_sortOrder(order)
    {
    }

    virtual ~ArchiveModelSorter()
    {
    }

    inline bool operator()(const QPair<Archive::Entry*, int> &left, const QPair<Archive::Entry*, int> &right) const
    {
        if (m_sortOrder == Qt::AscendingOrder) {
            return lessThan(left, right);
        } else {
            return !lessThan(left, right);
        }
    }

protected:
    bool lessThan(const QPair<Archive::Entry*, int> &left, const QPair<Archive::Entry*, int> &right) const
    {
        const Archive::Entry * const leftEntry = left.first;
        const Archive::Entry * const rightEntry = right.first;

        // #234373: sort folders before files
        if ((leftEntry->isDir()) && (!rightEntry->isDir())) {
            return (m_sortOrder == Qt::AscendingOrder);
        } else if ((!leftEntry->isDir()) && (rightEntry->isDir())) {
            return !(m_sortOrder == Qt::AscendingOrder);
        }

        EntryMetaDataType column = static_cast<EntryMetaDataType>(m_sortColumn);
        const QVariant &leftEntryMetaData = leftEntry->property(propertiesList[column].toStdString().c_str());
        const QVariant &rightEntryMetaData = rightEntry->property(propertiesList[column].toStdString().c_str());

        switch (m_sortColumn) {
        case FullPath:
            return leftEntry->name() < rightEntry->name();
        case Size:
        case CompressedSize:
            return leftEntryMetaData.toInt() < rightEntryMetaData.toInt();
        default:
            return leftEntryMetaData.toString() < rightEntryMetaData.toString();
        }

        // We should not get here.
        Q_ASSERT(false);
        return false;
    }

private:
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};

ArchiveModel::ArchiveModel(const QString &dbusPathName, QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootEntry()
    , m_dbusPathName(dbusPathName)
{
    m_rootEntry.setProperty("isDirectory", true);
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
            //TODO: complete the columns
            int column = m_showColumns.at(index.column());
            switch (column) {
            case FullPath:
                return entry->name();
            case Size:
                if (entry->isDir()) {
                    int dirs;
                    int files;
                    const int children = childCount(index, dirs, files);
                    return KIO::itemsSummaryString(children, files, dirs, 0, false);
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
            case Ratio: // TODO: Use entry->metaData()[Ratio] when available
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
                return entry->property(propertiesList[column].toStdString().c_str());
            }
        }
        case Qt::DecorationRole:
            if (index.column() == 0) {
                return entry->icon();
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

    return 0;
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
            return i18nc("CRC hash code", "CRC");
        case Method:
            return i18nc("Compression method", "Method");
        case Version:
            //TODO: what exactly is a file version?
            return i18nc("File version", "Version");
        case Timestamp:
            return i18nc("Timestamp", "Date");
        case Comment:
            return i18nc("File comment", "Comment");
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
                                            : &m_rootEntry;

        Q_ASSERT(parentEntry->isDir());

        const Archive::Entry *item = parentEntry->entries().value(row, Q_NULLPTR);
        if (item != Q_NULLPTR) {
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
        if (item->getParent() && (item->getParent() != &m_rootEntry)) {
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
    return Q_NULLPTR;
}

int ArchiveModel::childCount(const QModelIndex &index, int &dirs, int &files) const
{
    if (index.isValid()) {
        dirs = files = 0;
        Archive::Entry *item = static_cast<Archive::Entry*>(index.internalPointer());
        Q_ASSERT(item);
        if (item->isDir()) {
            const QVector<Archive::Entry*> entries = item->entries();
            foreach(const Archive::Entry *entry, entries) {
                if (entry->isDir()) {
                    dirs++;
                } else {
                    files++;
                }
            }
            return entries.count();
        }
        return 0;
    }
    return -1;
}

int ArchiveModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() <= 0) {
        const Archive::Entry *parentEntry = parent.isValid()
                                            ? static_cast<Archive::Entry*>(parent.internalPointer())
                                            : &m_rootEntry;

        if (parentEntry && parentEntry->isDir()) {
            return parentEntry->entries().count();
        }
    }
    return 0;
}

int ArchiveModel::columnCount(const QModelIndex &parent) const
{
    return m_showColumns.size();
}

void ArchiveModel::sort(int column, Qt::SortOrder order)
{
    if (m_showColumns.size() <= column) {
        return;
    }

    emit layoutAboutToBeChanged();

    QList<Archive::Entry*> dirEntries;
    m_rootEntry.returnDirEntries(&dirEntries);
    dirEntries.append(&m_rootEntry);

    const ArchiveModelSorter modelSorter(m_showColumns.at(column), order);

    foreach(Archive::Entry *dir, dirEntries) {
        QVector < QPair<Archive::Entry*,int> > sorting(dir->entries().count());
        for (int i = 0; i < dir->entries().count(); ++i) {
            Archive::Entry *item = dir->entries().at(i);
            sorting[i].first = item;
            sorting[i].second = i;
        }

        qStableSort(sorting.begin(), sorting.end(), modelSorter);

        QModelIndexList fromIndexes;
        QModelIndexList toIndexes;
        for (int r = 0; r < sorting.count(); ++r) {
            Archive::Entry *item = sorting.at(r).first;
            toIndexes.append(createIndex(r, 0, item));
            fromIndexes.append(createIndex(sorting.at(r).second, 0, sorting.at(r).first));
            dir->setEntryAt(r, sorting.at(r).first);
        }

        changePersistentIndexList(fromIndexes, toIndexes);

        emit dataChanged(
            index(0, 0, indexForEntry(dir)),
            index(dir->entries().size() - 1, 0, indexForEntry(dir)));
    }

    emit layoutChanged();
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

bool ArchiveModel::dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent)
{
    Q_UNUSED(action)
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)

    if (!data->hasUrls()) {
        return false;
    }

    QStringList paths;
    foreach(const QUrl &url, data->urls()) {
        paths << url.toLocalFile();
    }

    //for now, this code is not used because adding files to paths inside the
    //archive is not supported yet. need a solution for this later.
    QString path;
#if 0
    if (parent.isValid()) {
        QModelIndex droppedOnto = index(row, column, parent);
        Archive::Entry *entry = entryForIndex(droppedOnto);
        if (entry->isDir()) {
            qCDebug(ARK) << "Using entry";
            path = entry->fileName.toString();
        } else {
            path = entryForIndex(parent)->fileName.toString();
        }
    }

    qCDebug(ARK) << "Dropped onto " << path;

#endif

    emit droppedFiles(paths, path);

    return true;
}

// For a rationale, see bugs #194241, #241967 and #355839
QString ArchiveModel::cleanFileName(const QString& fileName)
{
    // Skip entries with filename "/" or "//" or "."
    // "." is present in ISO files
    QRegularExpression pattern(QStringLiteral("/+|\\."));
    QRegularExpressionMatch match;
    if (fileName.contains(pattern, &match) && match.captured() == fileName) {
        qCDebug(ARK) << "Skipping entry with filename" << fileName;
        return QString();
    } else if (fileName.startsWith(QLatin1String("./"))) {
        return fileName.mid(2);
    }

    return fileName;
}

Archive::Entry *ArchiveModel::parentFor(const Archive::Entry *entry)
{
    QStringList pieces = entry->property("fullPath").toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts);
    if (pieces.isEmpty()) {
        return Q_NULLPTR;
    }
    pieces.removeLast();

    if (s_previousMatch) {
        //the number of path elements must be the same for the shortcut
        //to work
        if (s_previousPieces->count() == pieces.count()) {
            bool equal = true;

            //make sure all the pieces match up
            for (int i = 0; i < s_previousPieces->count(); ++i) {
                if (s_previousPieces->at(i) != pieces.at(i)) {
                    equal = false;
                    break;
                }
            }

            //if match return it
            if (equal) {
                return s_previousMatch;
            }
        }
    }

    Archive::Entry *parent = &m_rootEntry;

    foreach(const QString &piece, pieces) {
        Archive::Entry *entry = parent->find(piece);
        if (!entry) {
            // Directory entry will be traversed later (that happens for some archive formats, 7z for instance).
            // We have to create one before, in order to construct tree from its children,
            // and then delete the existing one (see ArchiveModel::newEntry).
            entry = new Archive::Entry(parent);

            entry->setProperty("fullPath", (parent == &m_rootEntry)
                                           ? piece
                                           : parent->property("fullPath").toString() + QLatin1Char( '/' ) + piece);
            entry->setProperty("isDirectory", true);
            insertEntry(entry);
        }
        if (!entry->isDir()) {
            Archive::Entry *e = new Archive::Entry(parent);
            copyEntryMetaData(e, entry);
            // Maybe we have both a file and a directory of the same name.
            // We avoid removing previous entries unless necessary.
            insertEntry(e);
        }
        parent = entry;
    }

    s_previousMatch = parent;
    *s_previousPieces = pieces;

    return parent;
}

QModelIndex ArchiveModel::indexForEntry(Archive::Entry *entry)
{
    Q_ASSERT(entry);
    if (entry != &m_rootEntry) {
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

    Archive::Entry *entry = m_rootEntry.findByPath(entryFileName.split(QLatin1Char( '/' ), QString::SkipEmptyParts));
    if (entry) {
        Archive::Entry *parent = entry->getParent();
        QModelIndex index = indexForEntry(entry);
        Q_UNUSED(index);

        beginRemoveRows(indexForEntry(parent), entry->row(), entry->row());

        //delete parent->entries()[ metaData->row() ];
        //parent->entries()[ metaData->row() ] = 0;
        parent->removeEntryAt(entry->row());

        endRemoveRows();
    }
}

void ArchiveModel::slotUserQuery(Kerfuffle::Query *query)
{
    query->execute();
}

void ArchiveModel::slotNewEntryFromSetArchive(Archive::Entry *entry)
{
    // we cache all entries that appear when opening a new archive
    // so we can all them together once it's done, this is a huge
    // performance improvement because we save from doing lots of
    // begin/endInsertRows
    m_newArchiveEntries.push_back(entry);
}

void ArchiveModel::slotNewEntry(Archive::Entry *entry)
{
    newEntry(entry, NotifyViews);
}

void ArchiveModel::newEntry(Archive::Entry *receivedEntry, InsertBehaviour behaviour)
{
    if (receivedEntry->property("fullPath").toString().isEmpty()) {
        qCDebug(ARK) << "Weird, received empty entry (no filename) - skipping";
        return;
    }

    //if there are no addidional columns registered, then have a look at the
    //entry and populate some
    if (m_showColumns.isEmpty()) {
        QList<int> toInsert;

        QMap<int, QString>::const_iterator i = propertiesList.begin();
        while (i != propertiesList.end()) {
            if (!receivedEntry->property(i.value().toStdString().c_str()).toString().isEmpty()) {
                if (i.key() != CompressedSize || receivedEntry->compressedSizeIsSet) {
                    toInsert << i.key();
                }
            }
            ++i;
        }
        beginInsertColumns(QModelIndex(), 0, toInsert.size() - 1);
        m_showColumns << toInsert;
        endInsertColumns();

        qCDebug(ARK) << "Showing columns: " << m_showColumns;
    }

    //#194241: Filenames such as "./file" should be displayed as "file"
    //#241967: Entries called "/" should be ignored
    //#355839: Entries called "//" should be ignored
    QString entryFileName = cleanFileName(receivedEntry->property("fullPath").toString());
    if (entryFileName.isEmpty()) { // The entry contains only "." or "./"
        return;
    }
    receivedEntry->setProperty("fullPath", entryFileName);

    /// 1. Skip already created entries
    Archive::Entry *existing = m_rootEntry.findByPath(entryFileName.split(QLatin1Char( '/' )));
    if (existing) {
        qCDebug(ARK) << "Refreshing entry for" << entryFileName;

        existing->setProperty("fullPath", entryFileName);
        // Multi-volume files are repeated at least in RAR archives.
        // In that case, we need to sum the compressed size for each volume
        qulonglong currentCompressedSize = existing->property("compressedSize").toULongLong();
        existing->setProperty("compressedSize", currentCompressedSize + receivedEntry->property("compressedSize").toULongLong());
        return;
    }

    /// 2. Find Parent Entry, creating missing direcotry ArchiveEntries in the process
    Archive::Entry *parent = parentFor(receivedEntry);

    /// 3. Create an Archive::Entry
    const QStringList path = entryFileName.split(QLatin1Char('/'), QString::SkipEmptyParts);
    const QString name = path.last();
    Archive::Entry *entry = parent->find(name);
    if (entry) {
        copyEntryMetaData(entry, receivedEntry);
        entry->setProperty("fullPath", entryFileName);
        delete receivedEntry;
    } else {
        receivedEntry->setParent(parent);
        insertEntry(receivedEntry, behaviour);
    }
}

void ArchiveModel::slotLoadingFinished(KJob *job)
{
    int i = 0;
    foreach(Archive::Entry *entry, m_newArchiveEntries) {
        newEntry(entry, DoNotNotifyViews);
        i++;
    }
    beginResetModel();
    endResetModel();
    m_newArchiveEntries.clear();

    qCDebug(ARK) << "Added" << i << "entries to model";

    emit loadingFinished(job);
}

void ArchiveModel::copyEntryMetaData(Archive::Entry *destinationEntry, const Archive::Entry *sourceEntry)
{
    destinationEntry->setProperty("fullPath", sourceEntry->property("fullPath"));
    destinationEntry->setProperty("permissions", sourceEntry->property("permissions"));
    destinationEntry->setProperty("owner", sourceEntry->property("owner"));
    destinationEntry->setProperty("group", sourceEntry->property("group"));
    destinationEntry->setProperty("size", sourceEntry->property("size"));
    destinationEntry->setProperty("compressedSize", sourceEntry->property("compressedSize"));
    destinationEntry->setProperty("link", sourceEntry->property("link"));
    destinationEntry->setProperty("ratio", sourceEntry->property("ratio"));
    destinationEntry->setProperty("CRC", sourceEntry->property("CRC"));
    destinationEntry->setProperty("method", sourceEntry->property("method"));
    destinationEntry->setProperty("version", sourceEntry->property("version"));
    destinationEntry->setProperty("timestamp", sourceEntry->property("timestamp").toDateTime());
    destinationEntry->setProperty("isDirectory", sourceEntry->property("isDirectory"));
    destinationEntry->setProperty("comment", sourceEntry->property("comment"));
    destinationEntry->setProperty("isPasswordProtected", sourceEntry->property("isPasswordProtected"));
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

KJob* ArchiveModel::setArchive(Kerfuffle::Archive *archive)
{
    m_archive.reset(archive);

    m_rootEntry.clear();
    s_previousMatch = Q_NULLPTR;
    s_previousPieces->clear();

    Kerfuffle::ListJob *job = Q_NULLPTR;

    m_newArchiveEntries.clear();
    if (m_archive) {
        job = m_archive->list(); // TODO: call "open" or "create"?
        if (job) {
            connect(job, &Kerfuffle::ListJob::newEntry, this, &ArchiveModel::slotNewEntryFromSetArchive);
            connect(job, &Kerfuffle::ListJob::result, this, &ArchiveModel::slotLoadingFinished);
            connect(job, &Kerfuffle::ListJob::userQuery, this, &ArchiveModel::slotUserQuery);

            emit loadingStarted();

            // TODO: make sure if it's ok to not have calls to beginRemoveColumns here
            m_showColumns.clear();
        }
    }
    beginResetModel();
    endResetModel();
    return job;
}

ExtractJob* ArchiveModel::extractFile(Archive::Entry *file, const QString& destinationDir, const Kerfuffle::ExtractionOptions& options) const
{
    QList<Archive::Entry*> files;
    files << file;
    return extractFiles(files, destinationDir, options);
}

ExtractJob* ArchiveModel::extractFiles(const QList<Archive::Entry*>& files, const QString& destinationDir, const Kerfuffle::ExtractionOptions& options) const
{
    Q_ASSERT(m_archive);
    ExtractJob *newJob = m_archive->copyFiles(files, destinationDir, options);
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

AddJob* ArchiveModel::addFiles(QList<Archive::Entry*> &entries, const Archive::Entry *destination, const CompressionOptions& options)
{
    if (!m_archive) {
        return Q_NULLPTR;
    }

    if (destination == Q_NULLPTR) {
        destination = &m_rootEntry;
    }

    if (!m_archive->isReadOnly()) {
        AddJob *job = m_archive->addFiles(entries, destination, options);
        connect(job, &AddJob::newEntry, this, &ArchiveModel::slotNewEntry);
        connect(job, &AddJob::userQuery, this, &ArchiveModel::slotUserQuery);


        return job;
    }
    return Q_NULLPTR;
}

DeleteJob* ArchiveModel::deleteFiles(QList<Archive::Entry*> entries)
{
    Q_ASSERT(m_archive);
    if (!m_archive->isReadOnly()) {
        DeleteJob *job = m_archive->deleteFiles(entries);
        connect(job, &DeleteJob::entryRemoved, this, &ArchiveModel::slotEntryRemoved);

        connect(job, &DeleteJob::finished, this, &ArchiveModel::slotCleanupEmptyDirs);

        connect(job, &DeleteJob::userQuery, this, &ArchiveModel::slotUserQuery);
        return job;
    }
    return Q_NULLPTR;
}

void ArchiveModel::encryptArchive(const QString &password, bool encryptHeader)
{
    if (!m_archive) {
        return;
    }

    m_archive->encrypt(password, encryptHeader);
}

void ArchiveModel::slotCleanupEmptyDirs()
{
    QList<QPersistentModelIndex> queue;
    QList<QPersistentModelIndex> nodesToDelete;

    //add root nodes
    for (int i = 0; i < rowCount(); ++i) {
        queue.append(QPersistentModelIndex(index(i, 0)));
    }

    //breadth-first traverse
    while (!queue.isEmpty()) {
        QPersistentModelIndex node = queue.takeFirst();
        Archive::Entry *entry = entryForIndex(node);

        if (!hasChildren(node)) {
            if (entry->property("fullPath").toString().isEmpty()) {
                nodesToDelete << node;
            }
        } else {
            for (int i = 0; i < rowCount(node); ++i) {
                queue.append(QPersistentModelIndex(index(i, 0, node)));
            }
        }
    }

    foreach(const QPersistentModelIndex& node, nodesToDelete) {
        Archive::Entry *rawEntry = static_cast<Archive::Entry*>(node.internalPointer());
        qCDebug(ARK) << "Delete with parent entries " << rawEntry->getParent()->entries() << " and row " << rawEntry->row();
        beginRemoveRows(parent(node), rawEntry->row(), rawEntry->row());
            rawEntry->getParent()->removeEntryAt(rawEntry->row());
        endRemoveRows();
    }
}
