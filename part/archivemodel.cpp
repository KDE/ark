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
#include "kerfuffle/archive.h"
#include "kerfuffle/jobs.h"

#include <KDebug>
#include <KIconLoader>
#include <KLocale>
#include <KMimeType>
#include <KIO/NetAccess>

#include <QDir>
#include <QFont>
#include <QLatin1String>
#include <QList>
#include <QMimeData>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QtDBus/QtDBus>

using namespace Kerfuffle;

class ArchiveDirNode;

//used to speed up the loading of large archives
static ArchiveNode* s_previousMatch = NULL;
K_GLOBAL_STATIC(QStringList, s_previousPieces)


// TODO: This class hierarchy needs some love.
//       Having a parent take a child class as a parameter in the constructor
//       should trigger one's spider-sense (TM).
class ArchiveNode
{
public:
    ArchiveNode(ArchiveDirNode *parent, const ArchiveEntry & entry)
        : m_parent(parent)
    {
        setEntry(entry);
    }

    virtual ~ArchiveNode()
    {
    }

    const ArchiveEntry &entry() const
    {
        return m_entry;
    }

    void setEntry(const ArchiveEntry& entry)
    {
        m_entry = entry;

        const QStringList pieces = entry[FileName].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts);
        m_name = pieces.isEmpty() ? QString() : pieces.last();

        if (entry[IsDirectory].toBool()) {
            m_icon = KIconLoader::global()->loadMimeTypeIcon(KMimeType::mimeType(QLatin1String("inode/directory"))->iconName(), KIconLoader::Small);
        } else {
            const KMimeType::Ptr mimeType = KMimeType::findByPath(m_entry[FileName].toString(), 0, true);
            m_icon = KIconLoader::global()->loadMimeTypeIcon(mimeType->iconName(), KIconLoader::Small);
        }
    }

    ArchiveDirNode *parent() const
    {
        return m_parent;
    }

    int row() const;

    virtual bool isDir() const
    {
        return false;
    }

    QPixmap icon() const
    {
        return m_icon;
    }

    QString name() const
    {
        return m_name;
    }

protected:
    void setIcon(const QPixmap &icon)
    {
        m_icon = icon;
    }

private:
    ArchiveEntry    m_entry;
    QPixmap         m_icon;
    QString         m_name;
    ArchiveDirNode *m_parent;
};


class ArchiveDirNode: public ArchiveNode
{
public:
    ArchiveDirNode(ArchiveDirNode *parent, const ArchiveEntry & entry)
        : ArchiveNode(parent, entry)
    {
    }

    ~ArchiveDirNode()
    {
        clear();
    }

    QList<ArchiveNode*> entries()
    {
        return m_entries;
    }

    void setEntryAt(int index, ArchiveNode* value)
    {
        m_entries[index] = value;
    }

    void appendEntry(ArchiveNode* entry)
    {
        m_entries.append(entry);
    }

    void removeEntryAt(int index)
    {
        delete m_entries.takeAt(index);
    }

    virtual bool isDir() const
    {
        return true;
    }

    ArchiveNode* find(const QString & name)
    {
        foreach(ArchiveNode *node, m_entries) {
            if (node && (node->name() == name)) {
                return node;
            }
        }
        return 0;
    }

    ArchiveNode* findByPath(const QStringList & pieces, int index = 0)
    {
        if (index == pieces.count()) {
            return 0;
        }

        ArchiveNode *next = find(pieces.at(index));

        if (index == pieces.count() - 1) {
            return next;
        }
        if (next && next->isDir()) {
            return static_cast<ArchiveDirNode*>(next)->findByPath(pieces, index + 1);
        }
        return 0;
    }

    void returnDirNodes(QList<ArchiveDirNode*> *store)
    {
        foreach(ArchiveNode *node, m_entries) {
            if (node->isDir()) {
                store->prepend(static_cast<ArchiveDirNode*>(node));
                static_cast<ArchiveDirNode*>(node)->returnDirNodes(store);
            }
        }
    }

    void clear()
    {
        qDeleteAll(m_entries);
        m_entries.clear();
    }

private:
    QList<ArchiveNode*> m_entries;
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

    inline bool operator()(const QPair<ArchiveNode*, int> &left, const QPair<ArchiveNode*, int> &right) const
    {
        if (m_sortOrder == Qt::AscendingOrder) {
            return lessThan(left, right);
        } else {
            return !lessThan(left, right);
        }
    }

protected:
    bool lessThan(const QPair<ArchiveNode*, int> &left, const QPair<ArchiveNode*, int> &right) const
    {
        const ArchiveNode * const leftNode = left.first;
        const ArchiveNode * const rightNode = right.first;

        // #234373: sort folders before files
        if ((leftNode->isDir()) && (!rightNode->isDir())) {
            return (m_sortOrder == Qt::AscendingOrder);
        } else if ((!leftNode->isDir()) && (rightNode->isDir())) {
            return !(m_sortOrder == Qt::AscendingOrder);
        }

        const QVariant &leftEntry = leftNode->entry()[m_sortColumn];
        const QVariant &rightEntry = rightNode->entry()[m_sortColumn];

        switch (m_sortColumn) {
        case FileName:
            return leftNode->name() < rightNode->name();
        case Size:
        case CompressedSize:
            return leftEntry.toInt() < rightEntry.toInt();
        default:
            return leftEntry.toString() < rightEntry.toString();
        }

        // We should not get here.
        Q_ASSERT(false);
        return false;
    }

private:
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};

int ArchiveNode::row() const
{
    if (parent()) {
        return parent()->entries().indexOf(const_cast<ArchiveNode*>(this));
    }
    return 0;
}

ArchiveModel::ArchiveModel(const QString &dbusPathName, QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(new ArchiveDirNode(0, ArchiveEntry()))
    , m_dbusPathName(dbusPathName)
{
}

ArchiveModel::~ArchiveModel()
{
    delete m_rootNode;
    m_rootNode = 0;
}

QVariant ArchiveModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        ArchiveNode *node = static_cast<ArchiveNode*>(index.internalPointer());
        switch (role) {
        case Qt::DisplayRole: {
            //TODO: complete the columns
            int columnId = m_showColumns.at(index.column());
            switch (columnId) {
            case FileName:
                return node->name();
            case Size:
                if (node->isDir()) {
                    int dirs;
                    int files;
                    const int children = childCount(index, dirs, files);
                    return KIO::itemsSummaryString(children, files, dirs, 0, false);
                } else if (node->entry().contains(Link)) {
                    return QVariant();
                } else {
                    return KIO::convertSize(node->entry()[ Size ].toULongLong());
                }
            case CompressedSize:
                if (node->isDir() || node->entry().contains(Link)) {
                    return QVariant();
                } else {
                    qulonglong compressedSize = node->entry()[ CompressedSize ].toULongLong();
                    if (compressedSize != 0) {
                        return KIO::convertSize(compressedSize);
                    } else {
                        return QVariant();
                    }
                }
            case Ratio: // TODO: Use node->entry()[Ratio] when available
                if (node->isDir() || node->entry().contains(Link)) {
                    return QVariant();
                } else {
                    qulonglong compressedSize = node->entry()[ CompressedSize ].toULongLong();
                    qulonglong size = node->entry()[ Size ].toULongLong();
                    if (compressedSize == 0 || size == 0) {
                        return QVariant();
                    } else {
                        int ratio = int(100 * ((double)size - compressedSize) / size);
                        return QString(QString::number(ratio) + QLatin1String( " %" ));
                    }
                }

            case Timestamp: {
                const QDateTime timeStamp = node->entry().value(Timestamp).toDateTime();
                return  KGlobal::locale()->formatDateTime(timeStamp);
            }

            default:
                return node->entry().value(columnId);
            }
            break;
        }
        case Qt::DecorationRole:
            if (index.column() == 0) {
                return node->icon();
            }
            return QVariant();
        case Qt::FontRole: {
            QFont f;
            f.setItalic(node->entry()[ IsPasswordProtected ].toBool());
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
            kDebug() << "WEIRD: showColumns.size = " << m_showColumns.size()
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
        ArchiveDirNode *parentNode = parent.isValid() ? static_cast<ArchiveDirNode*>(parent.internalPointer()) : m_rootNode;

        Q_ASSERT(parentNode->isDir());

        ArchiveNode *item = parentNode->entries().value(row, 0);
        if (item) {
            return createIndex(row, column, item);
        }
    }

    return QModelIndex();
}

QModelIndex ArchiveModel::parent(const QModelIndex &index) const
{
    if (index.isValid()) {
        ArchiveNode *item = static_cast<ArchiveNode*>(index.internalPointer());
        Q_ASSERT(item);
        if (item->parent() && (item->parent() != m_rootNode)) {
            return createIndex(item->parent()->row(), 0, item->parent());
        }
    }
    return QModelIndex();
}

ArchiveEntry ArchiveModel::entryForIndex(const QModelIndex &index)
{
    if (index.isValid()) {
        ArchiveNode *item = static_cast<ArchiveNode*>(index.internalPointer());
        Q_ASSERT(item);
        return item->entry();
    }
    return ArchiveEntry();
}

int ArchiveModel::childCount(const QModelIndex &index, int &dirs, int &files) const
{
    if (index.isValid()) {
        dirs = files = 0;
        ArchiveNode *item = static_cast<ArchiveNode*>(index.internalPointer());
        Q_ASSERT(item);
        if (item->isDir()) {
            const QList<ArchiveNode*> entries = static_cast<ArchiveDirNode*>(item)->entries();
            foreach(const ArchiveNode *node, entries) {
                if (node->isDir()) {
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
        ArchiveNode *parentNode = parent.isValid() ? static_cast<ArchiveNode*>(parent.internalPointer()) : m_rootNode;

        if (parentNode && parentNode->isDir()) {
            return static_cast<ArchiveDirNode*>(parentNode)->entries().count();
        }
    }
    return 0;
}

int ArchiveModel::columnCount(const QModelIndex &parent) const
{
    return m_showColumns.size();
    if (parent.isValid()) {
        return static_cast<ArchiveNode*>(parent.internalPointer())->entry().size();
    }
}

void ArchiveModel::sort(int column, Qt::SortOrder order)
{
    if (m_showColumns.size() <= column) {
        return;
    }

    emit layoutAboutToBeChanged();

    QList<ArchiveDirNode*> dirNodes;
    m_rootNode->returnDirNodes(&dirNodes);
    dirNodes.append(m_rootNode);

    const ArchiveModelSorter modelSorter(m_showColumns.at(column), order);

    foreach(ArchiveDirNode* dir, dirNodes) {
        QVector < QPair<ArchiveNode*,int> > sorting(dir->entries().count());
        for (int i = 0; i < dir->entries().count(); ++i) {
            ArchiveNode *item = dir->entries().at(i);
            sorting[i].first = item;
            sorting[i].second = i;
        }

        qStableSort(sorting.begin(), sorting.end(), modelSorter);

        QModelIndexList fromIndexes;
        QModelIndexList toIndexes;
        for (int r = 0; r < sorting.count(); ++r) {
            ArchiveNode *item = sorting.at(r).first;
            toIndexes.append(createIndex(r, 0, item));
            fromIndexes.append(createIndex(sorting.at(r).second, 0, sorting.at(r).first));
            dir->setEntryAt(r, sorting.at(r).first);
        }

        changePersistentIndexList(fromIndexes, toIndexes);

        emit dataChanged(
            index(0, 0, indexForNode(dir)),
            index(dir->entries().size() - 1, 0, indexForNode(dir)));
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
    types << QLatin1String("text/uri-list")
          << QLatin1String("text/plain")
          << QLatin1String("text/x-moz-url");

    // MIME types we accept for dropping (eg. Ark -> Dolphin).
    types << QLatin1String("application/x-kde-ark-dndextract-service")
          << QLatin1String("application/x-kde-ark-dndextract-path");

    return types;
}

QMimeData *ArchiveModel::mimeData(const QModelIndexList &indexes) const
{
    Q_UNUSED(indexes)

    QMimeData *mimeData = new QMimeData;
    mimeData->setData(QLatin1String("application/x-kde-ark-dndextract-service"),
                      QDBusConnection::sessionBus().baseService().toUtf8());
    mimeData->setData(QLatin1String("application/x-kde-ark-dndextract-path"),
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
            kDebug() << "Using entry";
            path = entryForIndex(droppedOnto).value(FileName).toString();
        } else {
            path = entryForIndex(parent).value(FileName).toString();
        }
    }

    kDebug() << "Dropped onto " << path;

#endif

    emit droppedFiles(paths, path);

    return true;
}

// For a rationale, see bugs #194241 and #241967
QString ArchiveModel::cleanFileName(const QString& fileName)
{
    if ((fileName == QLatin1String("/")) ||
        (fileName == QLatin1String("."))) { // "." is present in ISO files
        return QString();
    } else if (fileName.startsWith(QLatin1String("./"))) {
        return fileName.mid(2);
    }

    return fileName;
}

ArchiveDirNode* ArchiveModel::parentFor(const ArchiveEntry& entry)
{
    QStringList pieces = entry[ FileName ].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts);
    if (pieces.isEmpty()) {
        return NULL;
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
                return static_cast<ArchiveDirNode*>(s_previousMatch);
            }
        }
    }

    ArchiveDirNode *parent = m_rootNode;

    foreach(const QString &piece, pieces) {
        ArchiveNode *node = parent->find(piece);
        if (!node) {
            ArchiveEntry e;
            e[ FileName ] = (parent == m_rootNode) ?
                            piece : parent->entry()[ FileName ].toString() + QLatin1Char( '/' ) + piece;
            e[ IsDirectory ] = true;
            node = new ArchiveDirNode(parent, e);
            insertNode(node);
        }
        if (!node->isDir()) {
            ArchiveEntry e(node->entry());
            node = new ArchiveDirNode(parent, e);
            //Maybe we have both a file and a directory of the same name
            // We avoid removing previous entries unless necessary
            insertNode(node);
        }
        parent = static_cast<ArchiveDirNode*>(node);
    }

    s_previousMatch = parent;
    *s_previousPieces = pieces;

    return parent;
}
QModelIndex ArchiveModel::indexForNode(ArchiveNode *node)
{
    Q_ASSERT(node);
    if (node != m_rootNode) {
        Q_ASSERT(node->parent());
        Q_ASSERT(node->parent()->isDir());
        return createIndex(node->row(), 0, node);
    }
    return QModelIndex();
}

void ArchiveModel::slotEntryRemoved(const QString & path)
{
    kDebug() << "Removed node at path " << path;

    const QString entryFileName(cleanFileName(path));
    if (entryFileName.isEmpty()) {
        return;
    }

    ArchiveNode *entry = m_rootNode->findByPath(entryFileName.split(QLatin1Char( '/' ), QString::SkipEmptyParts));
    if (entry) {
        ArchiveDirNode *parent = entry->parent();
        QModelIndex index = indexForNode(entry);

        beginRemoveRows(indexForNode(parent), entry->row(), entry->row());

        //delete parent->entries()[ entry->row() ];
        //parent->entries()[ entry->row() ] = 0;
        parent->removeEntryAt(entry->row());

        endRemoveRows();
    } else {
        kDebug() << "Did not find the removed node";
    }
}

void ArchiveModel::slotUserQuery(Kerfuffle::Query *query)
{
    query->execute();
}

void ArchiveModel::slotNewEntryFromSetArchive(const ArchiveEntry& entry)
{
    // we cache all entries that appear when opening a new archive
    // so we can all them together once it's done, this is a huge
    // performance improvement because we save from doing lots of
    // begin/endInsertRows
    m_newArchiveEntries.push_back(entry);
}

void ArchiveModel::slotNewEntry(const ArchiveEntry& entry)
{
    newEntry(entry, NotifyViews);
}

void ArchiveModel::newEntry(const ArchiveEntry& receivedEntry, InsertBehaviour behaviour)
{
    if (receivedEntry[FileName].toString().isEmpty()) {
        kDebug() << "Weird, received empty entry (no filename) - skipping";
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
            if (receivedEntry.contains(column)) {
                toInsert << column;
            }
        }
        beginInsertColumns(QModelIndex(), 0, toInsert.size() - 1);
        m_showColumns << toInsert;
        endInsertColumns();

        kDebug() << "Show columns detected: " << m_showColumns;
    }

    //make a copy
    ArchiveEntry entry = receivedEntry;

    //#194241: Filenames such as "./file" should be displayed as "file"
    //#241967: Entries called "/" should be ignored
    QString entryFileName = cleanFileName(entry[FileName].toString());
    if (entryFileName.isEmpty()) { // The entry contains only "." or "./"
        return;
    }
    entry[FileName] = entryFileName;

    /// 1. Skip already created nodes
    if (m_rootNode) {
        ArchiveNode *existing = m_rootNode->findByPath(entry[ FileName ].toString().split(QLatin1Char( '/' )));
        if (existing) {
            kDebug() << "Refreshing entry for" << entry[FileName].toString();

            // Multi-volume files are repeated at least in RAR archives.
            // In that case, we need to sum the compressed size for each volume
            qulonglong currentCompressedSize = existing->entry()[CompressedSize].toULongLong();
            entry[CompressedSize] = currentCompressedSize + entry[CompressedSize].toULongLong();

            //TODO: benchmark whether it's a bad idea to reset the entry here.
            existing->setEntry(entry);
            return;
        }
    }

    /// 2. Find Parent Node, creating missing ArchiveDirNodes in the process
    ArchiveDirNode *parent = parentFor(entry);

    /// 3. Create an ArchiveNode
    QString name = entry[ FileName ].toString().split(QLatin1Char( '/' ), QString::SkipEmptyParts).last();
    ArchiveNode *node = parent->find(name);
    if (node) {
        node->setEntry(entry);
    } else {
        if (entry[ FileName ].toString().endsWith(QLatin1Char( '/' )) || (entry.contains(IsDirectory) && entry[ IsDirectory ].toBool())) {
            node = new ArchiveDirNode(parent, entry);
        } else {
            node = new ArchiveNode(parent, entry);
        }
        insertNode(node, behaviour);
    }
}

void ArchiveModel::slotLoadingFinished(KJob *job)
{
    //kDebug() << entry;
    foreach(const ArchiveEntry &entry, m_newArchiveEntries) {
        newEntry(entry, DoNotNotifyViews);
    }
    reset();
    m_newArchiveEntries.clear();

    emit loadingFinished(job);
}

void ArchiveModel::insertNode(ArchiveNode *node, InsertBehaviour behaviour)
{
    Q_ASSERT(node);
    ArchiveDirNode *parent = node->parent();
    Q_ASSERT(parent);
    if (behaviour == NotifyViews) {
        beginInsertRows(indexForNode(parent), parent->entries().count(), parent->entries().count());
    }
    parent->appendEntry(node);
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

    m_rootNode->clear();
    s_previousMatch = 0;
    s_previousPieces->clear();

    Kerfuffle::ListJob *job = NULL;

    m_newArchiveEntries.clear();
    if (m_archive) {
        job = m_archive->list(); // TODO: call "open" or "create"?

        connect(job, SIGNAL(newEntry(ArchiveEntry)),
                this, SLOT(slotNewEntryFromSetArchive(ArchiveEntry)));

        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotLoadingFinished(KJob*)));

        connect(job, SIGNAL(userQuery(Kerfuffle::Query*)),
                this, SLOT(slotUserQuery(Kerfuffle::Query*)));

        emit loadingStarted();

        // TODO: make sure if it's ok to not have calls to beginRemoveColumns here
        m_showColumns.clear();
    }
    reset();
    return job;
}

ExtractJob* ArchiveModel::extractFile(const QVariant& fileName, const QString & destinationDir, const Kerfuffle::ExtractionOptions options) const
{
    QList<QVariant> files;
    files << fileName;
    return extractFiles(files, destinationDir, options);
}

ExtractJob* ArchiveModel::extractFiles(const QList<QVariant>& files, const QString & destinationDir, const Kerfuffle::ExtractionOptions options) const
{
    Q_ASSERT(m_archive);
    ExtractJob *newJob = m_archive->copyFiles(files, destinationDir, options);
    connect(newJob, SIGNAL(userQuery(Kerfuffle::Query*)),
            this, SLOT(slotUserQuery(Kerfuffle::Query*)));
    return newJob;
}

AddJob* ArchiveModel::addFiles(const QStringList & filenames, const CompressionOptions& options)
{
    if (!m_archive) {
        return NULL;
    }

    if (!m_archive->isReadOnly()) {
        AddJob *job = m_archive->addFiles(filenames, options);
        connect(job, SIGNAL(newEntry(ArchiveEntry)),
                this, SLOT(slotNewEntry(ArchiveEntry)));
        connect(job, SIGNAL(userQuery(Kerfuffle::Query*)),
                this, SLOT(slotUserQuery(Kerfuffle::Query*)));


        return job;
    }
    return 0;
}

DeleteJob* ArchiveModel::deleteFiles(const QList<QVariant> & files)
{
    Q_ASSERT(m_archive);
    if (!m_archive->isReadOnly()) {
        DeleteJob *job = m_archive->deleteFiles(files);
        connect(job, SIGNAL(entryRemoved(QString)),
                this, SLOT(slotEntryRemoved(QString)));

        connect(job, SIGNAL(finished(KJob*)),
                this, SLOT(slotCleanupEmptyDirs()));

        connect(job, SIGNAL(userQuery(Kerfuffle::Query*)),
                this, SLOT(slotUserQuery(Kerfuffle::Query*)));
        return job;
    }
    return 0;
}

void ArchiveModel::slotCleanupEmptyDirs()
{
    kDebug();
    QList<QPersistentModelIndex> queue;
    QList<QPersistentModelIndex> nodesToDelete;

    //add root nodes
    for (int i = 0; i < rowCount(); ++i) {
        queue.append(QPersistentModelIndex(index(i, 0)));
    }

    //breadth-first traverse
    while (!queue.isEmpty()) {
        QPersistentModelIndex node = queue.takeFirst();
        ArchiveEntry entry = entryForIndex(node);
        //kDebug() << "Trying " << entry[FileName].toString();

        if (!hasChildren(node)) {
            if (!entry.contains(InternalID)) {
                nodesToDelete << node;
            }
        } else {
            for (int i = 0; i < rowCount(node); ++i) {
                queue.append(QPersistentModelIndex(index(i, 0, node)));
            }
        }
    }

    foreach(const QPersistentModelIndex& node, nodesToDelete) {
        ArchiveNode *rawNode = static_cast<ArchiveNode*>(node.internalPointer());
        kDebug() << "Delete with parent entries " << rawNode->parent()->entries() << " and row " << rawNode->row();
        beginRemoveRows(parent(node), rawNode->row(), rawNode->row());
        rawNode->parent()->removeEntryAt(rawNode->row());
        endRemoveRows();
        //kDebug() << "Removed entry " << entry[FileName].toString();
    }
}

#include "archivemodel.moc"
