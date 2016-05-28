//
// Created by mvlabat on 5/27/16.
//

#include "archiveentry.h"

namespace Kerfuffle {
Archive::Entry::Entry(Entry *parent)
    : m_parent(parent)
{
}

Archive::Entry::~Entry()
{
    clear();
}

QList<Archive::Entry*> Archive::Entry::entries()
{
    Q_ASSERT(isDir());
    return m_entries;
}

void Archive::Entry::setEntryAt(int index, Entry* value)
{
    Q_ASSERT(isDir());
    m_entries[index] = value;
}

void Archive::Entry::appendEntry(Entry* entry)
{
    Q_ASSERT(isDir());
    m_entries.append(entry);
}

void Archive::Entry::removeEntryAt(int index)
{
    Q_ASSERT(isDir());
    delete m_entries.takeAt(index);
}

Archive::Entry* Archive::Entry::getParent() const
{
    return m_parent;
}

void Archive::Entry::setParent(Archive::Entry *parent)
{
    m_parent = parent;
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
    return isDirectory.toBool();
}

QPixmap Archive::Entry::icon() const
{
    return m_icon;
}

QString Archive::Entry::name() const
{
    return m_name;
}

Archive::Entry* Archive::Entry::find(const QString & name)
{
        foreach(Entry *entry, m_entries) {
            if (entry && (entry->name() == name)) {
                return entry;
            }
        }
    return 0;
}

Archive::Entry* Archive::Entry::findByPath(const QStringList & pieces, int index)
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

const QVariant &Archive::Entry::getPropertyByColumn(EntryMetaDataType column) const
{
    switch (column) {
        case FileName:
            return fileName;
        case Permissions:
            return permissions;
        case Owner:
            return owner;
        case Group:
            return group;
        case Size:
            return size;
        case CompressedSize:
            return compressedSize;
        case Link:
            return link;
        case Ratio:
            return ratio;
        case EntryMetaDataType::CRC:
            return this->CRC;
        case Method:
            return method;
        case Version:
            return version;
        case Timestamp:
            return timestamp;
        case IsDirectory:
            return isDirectory;
        case Comment:
            return comment;
        case IsPasswordProtected:
            return isPasswordProtected;

        default:
            qCDebug(ARK) << "Weird, trying to get a nonexistent column";
            static QVariant nullProperty = 0;
            return nullProperty;
    }
}

void Archive::Entry::setPropertyByColumn(EntryMetaDataType column, const QVariant &value)
{
    switch (column) {
        case FileName:
            fileName = value;
        case Permissions:
            permissions = value;
        case Owner:
            owner = value;
        case Group:
            group = value;
        case Size:
            size = value;
        case CompressedSize:
            compressedSize = value;
        case Link:
            link = value;
        case Ratio:
            ratio = value;
        case EntryMetaDataType::CRC:
            this->CRC = value;
        case Method:
            method = value;
        case Version:
            version = value;
        case Timestamp:
            timestamp = value;
        case IsDirectory:
            isDirectory = value;
        case Comment:
            comment = value;
        case IsPasswordProtected:
            isPasswordProtected = value;

        default:
            qCDebug(ARK) << "Weird, trying to set a nonexistent column";
    }
}

void Archive::Entry::clearMetaData()
{
    fileName.clear();
    permissions.clear();
    owner.clear();
    group.clear();
    size.clear();
    compressedSize.clear();
    link.clear();
    ratio.clear();
    CRC.clear();
    method.clear();
    version.clear();
    timestamp.clear();
    isDirectory.clear();
    comment.clear();
    isPasswordProtected.clear();
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

}
