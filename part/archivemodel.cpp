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
#include "ark_debug.h"
#include "kerfuffle/archive_kerfuffle.h"
#include "kerfuffle/jobs.h"

#include <KIconLoader>
#include <KLocalizedString>
#include <kio/global.h>

#include <QDateTime>
#include <QDBusConnection>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPersistentModelIndex>
#include <QRegularExpression>
#include <QUrl>

using namespace Kerfuffle;

//used to speed up the loading of large archives
static ArchiveEntry* s_previousMatch = Q_NULLPTR;
Q_GLOBAL_STATIC(QStringList, s_previousPieces)


// TODO: This class hierarchy needs some love.
//       Having a parent take a child class as a parameter in the constructor
//       should trigger one's spider-sense (TM).
class ArchiveEntry
{
public:
    ArchiveEntry(ArchiveEntry *parent, const EntryMetaData & metaData)
        : m_parent(parent)
    {
        setMetaData(metaData);
    }

    ~ArchiveEntry()
    {
        clear();
    }

    QList<ArchiveEntry*> entries()
    {
        Q_ASSERT(m_isDir);
        return m_entries;
    }

    void setEntryAt(int index, ArchiveEntry* value)
    {
        Q_ASSERT(m_isDir);
        m_entries[index] = value;
    }

    void appendEntry(ArchiveEntry* entry)
    {
        Q_ASSERT(m_isDir);
        m_entries.append(entry);
    }

    void removeEntryAt(int index)
    {
        Q_ASSERT(m_isDir);
        delete m_entries.takeAt(index);
    }

    const EntryMetaData &metaData() const
    {
        return m_metaData;
    }

    void setMetaData(const EntryMetaData &metaData)
    {
        m_metaData = metaData;

        const QStringList pieces = metaData[FileName].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts);
        m_name = pieces.isEmpty() ? QString() : pieces.last();

        QMimeDatabase db;
        m_isDir = metaData[IsDirectory].toBool();
        if (m_isDir) {
            m_icon = QIcon::fromTheme(db.mimeTypeForName(QStringLiteral("inode/directory")).iconName()).pixmap(IconSize(KIconLoader::Small),
                                                                                                               IconSize(KIconLoader::Small));
        } else {
            m_icon = QIcon::fromTheme(db.mimeTypeForFile(m_metaData[FileName].toString()).iconName()).pixmap(IconSize(KIconLoader::Small),
                                                                                                          IconSize(KIconLoader::Small));
        }
    }

    ArchiveEntry *parent() const
    {
        return m_parent;
    }

    int row() const;

    bool isDir() const
    {
        return m_isDir;
    }

    QPixmap icon() const
    {
        return m_icon;
    }

    QString name() const
    {
        return m_name;
    }

    ArchiveEntry* find(const QString & name)
    {
        foreach(ArchiveEntry *entry, m_entries) {
            if (entry && (entry->name() == name)) {
                return entry;
            }
        }
        return 0;
    }

    ArchiveEntry* findByPath(const QStringList & pieces, int index = 0)
    {
        if (index == pieces.count()) {
            return 0;
        }

        ArchiveEntry *next = find(pieces.at(index));

        if (index == pieces.count() - 1) {
            return next;
        }
        if (next && next->isDir()) {
            return next->findByPath(pieces, index + 1);
        }
        return 0;
    }

    void returnDirEntries(QList<ArchiveEntry *> *store)
    {
        foreach(ArchiveEntry *entry, m_entries) {
            if (entry->isDir()) {
                store->prepend(entry);
                entry->returnDirEntries(store);
            }
        }
    }

    void clear()
    {
        if (m_isDir) {
            qDeleteAll(m_entries);
            m_entries.clear();
        }
    }

private:
    EntryMetaData           m_metaData;
    bool                    m_isDir;
    QList<ArchiveEntry*>    m_entries;
    QPixmap                 m_icon;
    QString                 m_name;
    ArchiveEntry            *m_parent;
};



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

    inline bool operator()(const QPair<ArchiveEntry*, int> &left, const QPair<ArchiveEntry*, int> &right) const
    {
        if (m_sortOrder == Qt::AscendingOrder) {
            return lessThan(left, right);
        } else {
            return !lessThan(left, right);
        }
    }

protected:
    bool lessThan(const QPair<ArchiveEntry*, int> &left, const QPair<ArchiveEntry*, int> &right) const
    {
        const ArchiveEntry * const leftEntry = left.first;
        const ArchiveEntry * const rightEntry = right.first;

        // #234373: sort folders before files
        if ((leftEntry->isDir()) && (!rightEntry->isDir())) {
            return (m_sortOrder == Qt::AscendingOrder);
        } else if ((!leftEntry->isDir()) && (rightEntry->isDir())) {
            return !(m_sortOrder == Qt::AscendingOrder);
        }

        const QVariant &leftEntryMetaData = leftEntry->metaData()[m_sortColumn];
        const QVariant &rightEntryMetaData = rightEntry->metaData()[m_sortColumn];

        switch (m_sortColumn) {
        case FileName:
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

int ArchiveEntry::row() const
{
    if (parent()) {
        return parent()->entries().indexOf(const_cast<ArchiveEntry*>(this));
    }
    return 0;
}

ArchiveModel::ArchiveModel(const QString &dbusPathName, QObject *parent)
    : QAbstractItemModel(parent)
    , m_dbusPathName(dbusPathName)
{
    EntryMetaData rootMetaData;
    rootMetaData[IsDirectory] = true;
    m_rootEntry = new ArchiveEntry(0, rootMetaData);
}

ArchiveModel::~ArchiveModel()
{
    delete m_rootEntry;
    m_rootEntry = 0;
}

QVariant ArchiveModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        ArchiveEntry *entry = static_cast<ArchiveEntry*>(index.internalPointer());
        switch (role) {
        case Qt::DisplayRole: {
            //TODO: complete the columns
            int columnId = m_showColumns.at(index.column());
            switch (columnId) {
            case FileName:
                return entry->name();
            case Size:
                if (entry->isDir()) {
                    int dirs;
                    int files;
                    const int children = childCount(index, dirs, files);
                    return KIO::itemsSummaryString(children, files, dirs, 0, false);
                } else if (entry->metaData().contains(Link)) {
                    return QVariant();
                } else {
                    return KIO::convertSize(entry->metaData()[ Size ].toULongLong());
                }
            case CompressedSize:
                if (entry->isDir() || entry->metaData().contains(Link)) {
                    return QVariant();
                } else {
                    qulonglong compressedSize = entry->metaData()[ CompressedSize ].toULongLong();
                    if (compressedSize != 0) {
                        return KIO::convertSize(compressedSize);
                    } else {
                        return QVariant();
                    }
                }
            case Ratio: // TODO: Use entry->metaData()[Ratio] when available
                if (entry->isDir() || entry->metaData().contains(Link)) {
                    return QVariant();
                } else {
                    qulonglong compressedSize = entry->metaData()[ CompressedSize ].toULongLong();
                    qulonglong size = entry->metaData()[ Size ].toULongLong();
                    if (compressedSize == 0 || size == 0) {
                        return QVariant();
                    } else {
                        int ratio = int(100 * ((double)size - compressedSize) / size);
                        return QString(QString::number(ratio) + QStringLiteral(" %"));
                    }
                }

            case Timestamp: {
                const QDateTime timeStamp = entry->metaData().value(Timestamp).toDateTime();
                return QLocale().toString(timeStamp, QLocale::ShortFormat);
            }

            default:
                return entry->metaData().value(columnId);
            }
            break;
        }
        case Qt::DecorationRole:
            if (index.column() == 0) {
                return entry->icon();
            }
            return QVariant();
        case Qt::FontRole: {
            QFont f;
            f.setItalic(entry->metaData()[ IsPasswordProtected ].toBool());
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
        case FileName:
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
        ArchiveEntry *parentEntry = parent.isValid() ? static_cast<ArchiveEntry*>(parent.internalPointer()) : m_rootEntry;

        Q_ASSERT(parentEntry->isDir());

        ArchiveEntry *item = parentEntry->entries().value(row, 0);
        if (item) {
            return createIndex(row, column, item);
        }
    }

    return QModelIndex();
}

QModelIndex ArchiveModel::parent(const QModelIndex &index) const
{
    if (index.isValid()) {
        ArchiveEntry *item = static_cast<ArchiveEntry*>(index.internalPointer());
        Q_ASSERT(item);
        if (item->parent() && (item->parent() != m_rootEntry)) {
            return createIndex(item->parent()->row(), 0, item->parent());
        }
    }
    return QModelIndex();
}

EntryMetaData ArchiveModel::metaDataForIndex(const QModelIndex &index)
{
    if (index.isValid()) {
        ArchiveEntry *item = static_cast<ArchiveEntry*>(index.internalPointer());
        Q_ASSERT(item);
        return item->metaData();
    }
    return EntryMetaData();
}

int ArchiveModel::childCount(const QModelIndex &index, int &dirs, int &files) const
{
    if (index.isValid()) {
        dirs = files = 0;
        ArchiveEntry *item = static_cast<ArchiveEntry*>(index.internalPointer());
        Q_ASSERT(item);
        if (item->isDir()) {
            const QList<ArchiveEntry*> entries = static_cast<ArchiveEntry*>(item)->entries();
            foreach(const ArchiveEntry *entry, entries) {
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
        ArchiveEntry *parentEntry = parent.isValid() ? static_cast<ArchiveEntry*>(parent.internalPointer()) : m_rootEntry;

        if (parentEntry && parentEntry->isDir()) {
            return static_cast<ArchiveEntry*>(parentEntry)->entries().count();
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

    QList<ArchiveEntry*> dirEntries;
    m_rootEntry->returnDirEntries(&dirEntries);
    dirEntries.append(m_rootEntry);

    const ArchiveModelSorter modelSorter(m_showColumns.at(column), order);

    foreach(ArchiveEntry* dir, dirEntries) {
        QVector < QPair<ArchiveEntry*,int> > sorting(dir->entries().count());
        for (int i = 0; i < dir->entries().count(); ++i) {
            ArchiveEntry *item = dir->entries().at(i);
            sorting[i].first = item;
            sorting[i].second = i;
        }

        qStableSort(sorting.begin(), sorting.end(), modelSorter);

        QModelIndexList fromIndexes;
        QModelIndexList toIndexes;
        for (int r = 0; r < sorting.count(); ++r) {
            ArchiveEntry *item = sorting.at(r).first;
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
        if (entryForIndex(droppedOnto).value(IsDirectory).toBool()) {
            qCDebug(ARK) << "Using entry";
            path = entryForIndex(droppedOnto).value(FileName).toString();
        } else {
            path = entryForIndex(parent).value(FileName).toString();
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

ArchiveEntry* ArchiveModel::parentFor(const EntryMetaData& metaData)
{
    QStringList pieces = metaData[ FileName ].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts);
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
                return static_cast<ArchiveEntry*>(s_previousMatch);
            }
        }
    }

    ArchiveEntry *parent = m_rootEntry;

    foreach(const QString &piece, pieces) {
        ArchiveEntry *entry = parent->find(piece);
        if (!entry) {
            EntryMetaData entryMetaData;
            entryMetaData[ FileName ] = (parent == m_rootEntry) ?
                            piece : parent->metaData()[ FileName ].toString() + QLatin1Char( '/' ) + piece;
            entryMetaData[ InternalID ] = entryMetaData.value(FileName);
            entryMetaData[ IsDirectory ] = true;
            entry = new ArchiveEntry(parent, entryMetaData);
            insertEntry(entry);
        }
        if (!entry->isDir()) {
            EntryMetaData e(entry->metaData());
            entry = new ArchiveEntry(parent, e);
            //Maybe we have both a file and a directory of the same name
            // We avoid removing previous entries unless necessary
            insertEntry(entry);
        }
        parent = entry;
    }

    s_previousMatch = parent;
    *s_previousPieces = pieces;

    return parent;
}

QModelIndex ArchiveModel::indexForEntry(ArchiveEntry *entry)
{
    Q_ASSERT(entry);
    if (entry != m_rootEntry) {
        Q_ASSERT(entry->parent());
        Q_ASSERT(entry->parent()->isDir());
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

    ArchiveEntry *entry = m_rootEntry->findByPath(entryFileName.split(QLatin1Char( '/' ), QString::SkipEmptyParts));
    if (entry) {
        ArchiveEntry *parent = entry->parent();
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

void ArchiveModel::slotNewEntryFromSetArchive(const EntryMetaData& entry)
{
    // we cache all entries that appear when opening a new archive
    // so we can all them together once it's done, this is a huge
    // performance improvement because we save from doing lots of
    // begin/endInsertRows
    m_newArchiveEntries.push_back(entry);
}

void ArchiveModel::slotNewEntry(const EntryMetaData& entry)
{
    newEntry(entry, NotifyViews);
}

void ArchiveModel::newEntry(const EntryMetaData& receivedMetaData, InsertBehaviour behaviour)
{
    if (receivedMetaData[FileName].toString().isEmpty()) {
        qCDebug(ARK) << "Weird, received empty entry (no filename) - skipping";
        return;
    }

    //if there are no addidional columns registered, then have a look at the
    //entry and populate some
    if (m_showColumns.isEmpty()) {
        //these are the columns we are interested in showing in the display
        static const QList<int> columnsForDisplay =
            QList<int>()
            << FileName
            << Size
            << CompressedSize
            << Permissions
            << Owner
            << Group
            << Ratio
            << CRC
            << Method
            << Version
            << Timestamp
            << Comment;

        QList<int> toInsert;

        foreach(int column, columnsForDisplay) {
            if (receivedMetaData.contains(column)) {
                toInsert << column;
            }
        }
        beginInsertColumns(QModelIndex(), 0, toInsert.size() - 1);
        m_showColumns << toInsert;
        endInsertColumns();

        qCDebug(ARK) << "Showing columns: " << m_showColumns;
    }

    //make a copy
    EntryMetaData metaData = receivedMetaData;

    //#194241: Filenames such as "./file" should be displayed as "file"
    //#241967: Entries called "/" should be ignored
    //#355839: Entries called "//" should be ignored
    QString entryFileName = cleanFileName(metaData[FileName].toString());
    if (entryFileName.isEmpty()) { // The entry contains only "." or "./"
        return;
    }
    metaData[FileName] = entryFileName;

    /// 1. Skip already created entries
    if (m_rootEntry) {
        ArchiveEntry *existing = m_rootEntry->findByPath(metaData[ FileName ].toString().split(QLatin1Char( '/' )));
        if (existing) {
            qCDebug(ARK) << "Refreshing entry for" << metaData[FileName].toString();

            // Multi-volume files are repeated at least in RAR archives.
            // In that case, we need to sum the compressed size for each volume
            qulonglong currentCompressedSize = existing->metaData()[CompressedSize].toULongLong();
            metaData[CompressedSize] = currentCompressedSize + metaData[CompressedSize].toULongLong();

            //TODO: benchmark whether it's a bad idea to reset the metaData here.
            existing->setMetaData(metaData);
            return;
        }
    }

    /// 2. Find Parent Entry, creating missing direcotry ArchiveEntries in the process
    ArchiveEntry *parent = parentFor(metaData);

    /// 3. Create an ArchiveEntry
    const QStringList path = metaData[FileName].toString().split(QLatin1Char('/'), QString::SkipEmptyParts);
    const QString name = path.last();
    ArchiveEntry *entry = parent->find(name);
    if (entry) {
        entry->setMetaData(metaData);
    } else {
        entry = new ArchiveEntry(parent, metaData);
        insertEntry(entry, behaviour);
    }
}

void ArchiveModel::slotLoadingFinished(KJob *job)
{
    int i = 0;
    foreach(const EntryMetaData &entry, m_newArchiveEntries) {
        newEntry(entry, DoNotNotifyViews);
        i++;
    }
    beginResetModel();
    endResetModel();
    m_newArchiveEntries.clear();

    qCDebug(ARK) << "Added" << i << "entries to model";

    emit loadingFinished(job);
}

void ArchiveModel::insertEntry(ArchiveEntry *entry, InsertBehaviour behaviour)
{
    Q_ASSERT(entry);
    ArchiveEntry *parent = entry->parent();
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

    m_rootEntry->clear();
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

ExtractJob* ArchiveModel::extractFile(const QVariant& fileName, const QString& destinationDir, const Kerfuffle::ExtractionOptions& options) const
{
    QList<QVariant> files;
    files << QVariant::fromValue(fileRootNodePair(fileName.toString()));
    return extractFiles(files, destinationDir, options);
}

ExtractJob* ArchiveModel::extractFiles(const QList<QVariant>& files, const QString& destinationDir, const Kerfuffle::ExtractionOptions& options) const
{
    Q_ASSERT(m_archive);
    ExtractJob *newJob = m_archive->copyFiles(files, destinationDir, options);
    connect(newJob, &ExtractJob::userQuery, this, &ArchiveModel::slotUserQuery);
    return newJob;
}

AddJob* ArchiveModel::addFiles(const QStringList & filenames, const CompressionOptions& options)
{
    if (!m_archive) {
        return Q_NULLPTR;
    }

    if (!m_archive->isReadOnly()) {
        AddJob *job = m_archive->addFiles(filenames, options);
        connect(job, &AddJob::newEntry, this, &ArchiveModel::slotNewEntry);
        connect(job, &AddJob::userQuery, this, &ArchiveModel::slotUserQuery);


        return job;
    }
    return Q_NULLPTR;
}

DeleteJob* ArchiveModel::deleteFiles(const QList<QVariant> & files)
{
    Q_ASSERT(m_archive);
    if (!m_archive->isReadOnly()) {
        DeleteJob *job = m_archive->deleteFiles(files);
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
        EntryMetaData metaData = metaDataForIndex(node);

        if (!hasChildren(node)) {
            if (!metaData.contains(InternalID)) {
                nodesToDelete << node;
            }
        } else {
            for (int i = 0; i < rowCount(node); ++i) {
                queue.append(QPersistentModelIndex(index(i, 0, node)));
            }
        }
    }

    foreach(const QPersistentModelIndex& node, nodesToDelete) {
        ArchiveEntry *rawEntry = static_cast<ArchiveEntry*>(node.internalPointer());
        qCDebug(ARK) << "Delete with parent entries " << rawEntry->parent()->entries() << " and row " << rawEntry->row();
        beginRemoveRows(parent(node), rawEntry->row(), rawEntry->row());
        rawEntry->parent()->removeEntryAt(rawEntry->row());
        endRemoveRows();
    }
}
