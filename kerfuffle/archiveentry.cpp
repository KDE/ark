//
// Created by mvlabat on 5/27/16.
//

#include "archiveentry.h"

namespace Kerfuffle {
Archive::Entry::Entry(QObject *parent, QString fullPath, QString rootNode)
    : QObject(parent)
    , rootNode(rootNode)
    , compressedSizeIsSet(true)
    , m_parent(qobject_cast<Entry*>(parent))
{
    clearMetaData();
    if (!fullPath.isEmpty())
        setFullPath(fullPath);
}

Archive::Entry::~Entry()
{
    clear();
}

QVector<Archive::Entry*> Archive::Entry::entries()
{
    Q_ASSERT(isDir());
    return m_entries;
}

const QVector<Archive::Entry*> Archive::Entry::entries() const {
    Q_ASSERT(isDir());
    return m_entries;
}

void Archive::Entry::setEntryAt(int index, Entry *value)
{
    Q_ASSERT(isDir());
    m_entries[index] = value;
}

void Archive::Entry::appendEntry(Entry *entry)
{
    Q_ASSERT(isDir());
    m_entries.append(entry);
}

void Archive::Entry::removeEntryAt(int index)
{
    Q_ASSERT(isDir());
    delete m_entries.takeAt(index);
}

Archive::Entry *Archive::Entry::getParent() const
{
    return m_parent;
}

void Archive::Entry::setParent(Archive::Entry *parent)
{
    m_parent = parent;
}

void Archive::Entry::setFullPath(const QString &fullPath)
{
    m_fullPath = fullPath;
    processNameAndIcon();
}

void Archive::Entry::setIsDirectory(const bool isDirectory)
{
    m_isDirectory = isDirectory;
    processNameAndIcon();
}

int Archive::Entry::row() const
{
    if (getParent()) {
        return getParent()->entries().indexOf(const_cast<Archive::Entry*>(this));
    }
    return 0;
}

bool Archive::Entry::isDir() const
{
    return m_isDirectory;
}

void Archive::Entry::processNameAndIcon()
{
    const QStringList pieces = m_fullPath.split(QLatin1Char( '/' ), QString::SkipEmptyParts);
    m_name = pieces.isEmpty() ? QString() : pieces.last();

    QMimeDatabase db;
    if (isDir()) {
        m_icon = QIcon::fromTheme(db.mimeTypeForName(QStringLiteral("inode/directory")).iconName()).pixmap(IconSize(KIconLoader::Small),
                                                                                                           IconSize(KIconLoader::Small));
    } else {
        m_icon = QIcon::fromTheme(db.mimeTypeForFile(m_fullPath).iconName()).pixmap(IconSize(KIconLoader::Small),
                                                                                                      IconSize(KIconLoader::Small));
    }
}

QPixmap Archive::Entry::icon() const
{
    return m_icon;
}

QString Archive::Entry::name() const
{
    return m_name;
}

Archive::Entry *Archive::Entry::find(const QString & name)
{
    foreach(Entry *entry, m_entries) {
        if (entry && (entry->name() == name)) {
            return entry;
        }
    }
    return 0;
}

Archive::Entry *Archive::Entry::findByPath(const QStringList & pieces, int index)
{
    if (index == pieces.count()) {
        return 0;
    }

    Entry *next = find(pieces.at(index));

    if (index == pieces.count() - 1) {
        return next;
    }
    if (next && next->isDir()) {
        return next->findByPath(pieces, index + 1);
    }
    return 0;
}

void Archive::Entry::clearMetaData()
{
    m_fullPath.clear();
    m_permissions.clear();
    m_owner.clear();
    m_group.clear();
    m_size = 0;
    m_compressedSize = 0;
    m_link.clear();
    m_ratio.clear();
    m_CRC.clear();
    m_method.clear();
    m_version.clear();
    m_timestamp = QDateTime();
    m_isDirectory = false;
    m_comment.clear();
    m_isPasswordProtected = false;
}

void Archive::Entry::returnDirEntries(QList<Entry *> *store)
{
        foreach(Entry *entry, m_entries) {
            if (entry->isDir()) {
                store->prepend(entry);
                entry->returnDirEntries(store);
            }
        }
}

void Archive::Entry::clear()
{
    if (isDir()) {
        qDeleteAll(m_entries);
        m_entries.clear();
    }
}

bool Archive::Entry::operator==(const Archive::Entry *right) const
{
    return m_fullPath == right->m_fullPath;
}

QDebug operator<<(QDebug d, const Kerfuffle::Archive::Entry &entry)
{
    d.nospace() << "Entry(" << entry.property("fullPath");
    if (!entry.rootNode.isEmpty()) {
        d.nospace() << "," << entry.rootNode;
    }
    d.nospace() << ")";
    return d.space();
}

QDebug operator<<(QDebug d, const Kerfuffle::Archive::Entry *entry)
{
    d.nospace() << "Entry(" << entry->property("fullPath");
    if (!entry->rootNode.isEmpty()) {
        d.nospace() << "," << entry->rootNode;
    }
    d.nospace() << ")";
    return d.space();
}

}
