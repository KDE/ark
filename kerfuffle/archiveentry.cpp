/*
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "archiveentry.h"

#include <QMimeDatabase>

#include "util.h"

namespace Kerfuffle {
Archive::Entry::Entry(QObject *parent, const QString &fullPath, const QString &rootNode)
    : QObject(parent)
    , rootNode(rootNode)
    , compressedSizeIsSet(true)
    , m_parent(qobject_cast<Entry*>(parent))
    , m_size(0)
    , m_compressedSize(0)
    , m_sparseSize(0)
    , m_isDirectory(false)
    , m_isExecutable(false)
    , m_isPasswordProtected(false)
    , m_isSparse(false)
{
    if (!fullPath.isEmpty())
        setFullPath(fullPath);
}

Archive::Entry::~Entry()
{
}

qulonglong Archive::Entry::size() const
{
    return m_size;
}

qulonglong Archive::Entry::sparseSize() const
{
    return m_sparseSize;
}

bool Archive::Entry::isSparse() const
{
    return m_isSparse;
}

void Archive::Entry::copyMetaData(const Archive::Entry *sourceEntry)
{
    setProperty("fullPath", sourceEntry->property("fullPath"));
    setProperty("permissions", sourceEntry->property("permissions"));
    setProperty("owner", sourceEntry->property("owner"));
    setProperty("group", sourceEntry->property("group"));
    setProperty("size", sourceEntry->property("size"));
    setProperty("compressedSize", sourceEntry->property("compressedSize"));
    setProperty("sparseSize", sourceEntry->property("sparseSize"));
    setProperty("link", sourceEntry->property("link"));
    setProperty("ratio", sourceEntry->property("ratio"));
    setProperty("CRC", sourceEntry->property("CRC"));
    setProperty("BLAKE2", sourceEntry->property("BLAKE2"));
    setProperty("method", sourceEntry->property("method"));
    setProperty("version", sourceEntry->property("version"));
    setProperty("timestamp", sourceEntry->property("timestamp").toDateTime());
    setProperty("isDirectory", sourceEntry->property("isDirectory"));
    setProperty("isPasswordProtected", sourceEntry->property("isPasswordProtected"));
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
    Q_ASSERT(index < m_entries.count());
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
    Q_ASSERT(index < m_entries.count());
    m_entries.remove(index);
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

    m_name = Kerfuffle::Util::lastPathSegment(m_fullPath);

}

QString Archive::Entry::fullPath(PathFormat format) const
{
    if (format == NoTrailingSlash && m_fullPath.endsWith(QLatin1Char('/'))) {
        return m_fullPath.left(m_fullPath.size() - 1);
    } else {
        return m_fullPath;
    }
}

QString Archive::Entry::displayName() const
{
    if (m_displayName.isEmpty()) {
        return m_name;
    }

    return m_displayName;
}

QString Archive::Entry::name() const
{
    return m_name;
}

QStringView Archive::Entry::nameView() const
{
    return m_name;
}

void Archive::Entry::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

void Archive::Entry::setIsDirectory(const bool isDirectory)
{
    m_isDirectory = isDirectory;
}

bool Archive::Entry::isDir() const
{
    return m_isDirectory;
}

void Archive::Entry::setIsExecutable(const bool isExecutable)
{
    m_isExecutable = isExecutable;
}

bool Archive::Entry::isExecutable() const
{
    return m_isExecutable;
}

int Archive::Entry::row() const
{
    if (getParent()) {
        return getParent()->entries().indexOf(const_cast<Archive::Entry*>(this));
    }
    return 0;
}

Archive::Entry *Archive::Entry::find(QStringView name) const
{
    for (Entry *entry : std::as_const(m_entries)) {
        if (entry && (entry->nameView() == name)) {
            return entry;
        }
    }
    return nullptr;
}

Archive::Entry *Archive::Entry::findByPath(const QStringList &pieces, int index) const
{
    if (index == pieces.count()) {
        return nullptr;
    }

    Entry *next = find(pieces.at(index));
    if (index == pieces.count() - 1) {
        return next;
    }
    if (next && next->isDir()) {
        return next->findByPath(pieces, index + 1);
    }
    return nullptr;
}

void Archive::Entry::countChildren(uint &dirs, uint &files) const
{
    dirs = files = 0;
    if (!isDir()) {
        return;
    }

    const auto archiveEntries = entries();
    for (auto entry : archiveEntries) {
        if (entry->isDir()) {
            dirs++;
        } else {
            files++;
        }
    }
}

QIcon Archive::Entry::icon() const
{
    if (m_icon.isNull()) {
        QMimeDatabase db;

        if (m_isDirectory) {
            static QIcon directoryIcon = QIcon::fromTheme(db.mimeTypeForName(QStringLiteral("inode/directory")).iconName());
            m_icon = directoryIcon;
        } else {
            m_icon = QIcon::fromTheme(db.mimeTypeForFile(displayName(), QMimeDatabase::MatchMode::MatchExtension).iconName());
        }
    }

    return m_icon;
}

bool Archive::Entry::operator==(const Archive::Entry &right) const
{
    return m_fullPath == right.m_fullPath;
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

#include "moc_archiveentry.cpp"
