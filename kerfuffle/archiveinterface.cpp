/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "archiveinterface.h"
#include "ark_debug.h"
#include "jobs.h"
#include "mimetypes.h"
#include "windows_stat.h"

#include <QDir>
#include <QFileInfo>

namespace Kerfuffle
{
ReadOnlyArchiveInterface::ReadOnlyArchiveInterface(QObject *parent, const QVariantList &args)
    : QObject(parent)
    , m_numberOfVolumes(0)
    , m_numberOfEntries(0)
    , m_waitForFinishedSignal(false)
    , m_isHeaderEncryptionEnabled(false)
    , m_isCorrupt(false)
    , m_isMultiVolume(false)
    , m_unpackedSize(0)
{
    Q_ASSERT(args.size() >= 2);
    qRegisterMetaType<Kerfuffle::Query *>();

    qCDebug(ARK) << "Created read-only interface for" << args.first().toString();
    m_filename = args.first().toString();
    m_mimetype = determineMimeType(m_filename);
    connect(this, &ReadOnlyArchiveInterface::entry, this, &ReadOnlyArchiveInterface::onEntry);
    m_metaData = args.at(1).value<KPluginMetaData>();
}

ReadOnlyArchiveInterface::~ReadOnlyArchiveInterface()
{
}

void ReadOnlyArchiveInterface::onEntry(Archive::Entry *archiveEntry)
{
    Q_ASSERT(archiveEntry);
    m_numberOfEntries += 1;
    m_unpackedSize += archiveEntry->isSparse() ? archiveEntry->sparseSize() : archiveEntry->size();
}

QString ReadOnlyArchiveInterface::filename() const
{
    return m_filename;
}

QString ReadOnlyArchiveInterface::comment() const
{
    return m_comment;
}

bool ReadOnlyArchiveInterface::isReadOnly() const
{
    return true;
}

bool ReadOnlyArchiveInterface::open()
{
    return true;
}

void ReadOnlyArchiveInterface::setPassword(const QString &password)
{
    m_password = password;
}

void ReadOnlyArchiveInterface::setHeaderEncryptionEnabled(bool enabled)
{
    m_isHeaderEncryptionEnabled = enabled;
}

QString ReadOnlyArchiveInterface::password() const
{
    return m_password;
}

bool ReadOnlyArchiveInterface::doKill()
{
    // default implementation
    return false;
}

void ReadOnlyArchiveInterface::setCorrupt(bool isCorrupt)
{
    m_isCorrupt = isCorrupt;
}

bool ReadOnlyArchiveInterface::isCorrupt() const
{
    return m_isCorrupt;
}

bool ReadOnlyArchiveInterface::isMultiVolume() const
{
    return m_isMultiVolume;
}

void ReadOnlyArchiveInterface::setMultiVolume(bool value)
{
    m_isMultiVolume = value;
}

int ReadOnlyArchiveInterface::numberOfVolumes() const
{
    return m_numberOfVolumes;
}

QString ReadOnlyArchiveInterface::multiVolumeName() const
{
    return filename();
}

ReadWriteArchiveInterface::ReadWriteArchiveInterface(QObject *parent, const QVariantList &args)
    : ReadOnlyArchiveInterface(parent, args)
{
    qCDebug(ARK) << "Created read-write interface for" << args.first().toString();

    connect(this, &ReadWriteArchiveInterface::entryRemoved, this, &ReadWriteArchiveInterface::onEntryRemoved);
}

ReadWriteArchiveInterface::~ReadWriteArchiveInterface()
{
}

bool ReadOnlyArchiveInterface::waitForFinishedSignal()
{
    return m_waitForFinishedSignal;
}

int ReadOnlyArchiveInterface::moveRequiredSignals() const
{
    return 1;
}

int ReadOnlyArchiveInterface::copyRequiredSignals() const
{
    return 1;
}

void ReadOnlyArchiveInterface::setWaitForFinishedSignal(bool value)
{
    m_waitForFinishedSignal = value;
}

qulonglong ReadOnlyArchiveInterface::unpackedSize() const
{
    return m_unpackedSize;
}

QString ReadOnlyArchiveInterface::permissionsToString(mode_t perm)
{
    QString modeval;
    if ((perm & S_IFMT) == QT_STAT_DIR) {
        modeval.append(QLatin1Char('d'));
    } else if ((perm & S_IFMT) == QT_STAT_LNK) {
        modeval.append(QLatin1Char('l'));
    } else {
        modeval.append(QLatin1Char('-'));
    }
    modeval.append((perm & S_IRUSR) ? QLatin1Char('r') : QLatin1Char('-'));
    modeval.append((perm & S_IWUSR) ? QLatin1Char('w') : QLatin1Char('-'));
    if ((perm & S_ISUID) && (perm & S_IXUSR)) {
        modeval.append(QLatin1Char('s'));
    } else if ((perm & S_ISUID)) {
        modeval.append(QLatin1Char('S'));
    } else if ((perm & S_IXUSR)) {
        modeval.append(QLatin1Char('x'));
    } else {
        modeval.append(QLatin1Char('-'));
    }
    modeval.append((perm & S_IRGRP) ? QLatin1Char('r') : QLatin1Char('-'));
    modeval.append((perm & S_IWGRP) ? QLatin1Char('w') : QLatin1Char('-'));
    if ((perm & S_ISGID) && (perm & S_IXGRP)) {
        modeval.append(QLatin1Char('s'));
    } else if ((perm & S_ISGID)) {
        modeval.append(QLatin1Char('S'));
    } else if ((perm & S_IXGRP)) {
        modeval.append(QLatin1Char('x'));
    } else {
        modeval.append(QLatin1Char('-'));
    }
    modeval.append((perm & S_IROTH) ? QLatin1Char('r') : QLatin1Char('-'));
    modeval.append((perm & S_IWOTH) ? QLatin1Char('w') : QLatin1Char('-'));
    if ((perm & S_ISVTX) && (perm & S_IXOTH)) {
        modeval.append(QLatin1Char('t'));
    } else if ((perm & S_ISVTX)) {
        modeval.append(QLatin1Char('T'));
    } else if ((perm & S_IXOTH)) {
        modeval.append(QLatin1Char('x'));
    } else {
        modeval.append(QLatin1Char('-'));
    }
    return modeval;
}

QStringList ReadOnlyArchiveInterface::entryFullPaths(const QVector<Archive::Entry *> &entries, PathFormat format)
{
    QStringList filesList;
    for (const Archive::Entry *file : entries) {
        filesList << file->fullPath(format);
    }
    return filesList;
}

QVector<Archive::Entry *> ReadOnlyArchiveInterface::entriesWithoutChildren(const QVector<Archive::Entry *> &entries)
{
    // QMap is easy way to get entries sorted by their fullPath.
    QMap<QString, Archive::Entry *> sortedEntries;
    for (Archive::Entry *entry : entries) {
        sortedEntries.insert(entry->fullPath(), entry);
    }

    QVector<Archive::Entry *> filteredEntries;
    QString lastFolder;
    for (Archive::Entry *entry : std::as_const(sortedEntries)) {
        if (!lastFolder.isEmpty() && entry->fullPath().startsWith(lastFolder)) {
            continue;
        }

        lastFolder = (entry->fullPath().right(1) == QLatin1String("/")) ? entry->fullPath() : QString();
        filteredEntries << entry;
    }

    return filteredEntries;
}

QStringList ReadOnlyArchiveInterface::entryPathsFromDestination(QStringList entries, const Archive::Entry *destination, int entriesWithoutChildren)
{
    QStringList paths = QStringList();
    entries.sort();
    QString lastFolder;
    const QString destinationPath = (destination == nullptr) ? QString() : destination->fullPath();

    QString newPath;
    int nameLength = 0;
    for (const QString &entryPath : std::as_const(entries)) {
        if (!lastFolder.isEmpty() && entryPath.startsWith(lastFolder)) {
            // Replace last moved or copied folder path with destination path.
            int charsCount = entryPath.length() - lastFolder.length();
            if (entriesWithoutChildren != 1) {
                charsCount += nameLength;
            }
            newPath = destinationPath + entryPath.right(charsCount);
        } else {
            const QString name = entryPath.split(QLatin1Char('/'), Qt::SkipEmptyParts).last();
            if (entriesWithoutChildren != 1) {
                newPath = destinationPath + name;
                if (entryPath.right(1) == QLatin1String("/")) {
                    newPath += QLatin1Char('/');
                }
            } else {
                // If the mode is set to Move and there is only one passed file in the list,
                // we have to use destination as newPath.
                newPath = destinationPath;
            }
            if (entryPath.right(1) == QLatin1String("/")) {
                nameLength = name.length() + 1; // plus slash
                lastFolder = entryPath;
            } else {
                nameLength = 0;
                lastFolder = QString();
            }
        }
        paths << newPath;
    }

    return paths;
}

bool ReadOnlyArchiveInterface::isHeaderEncryptionEnabled() const
{
    return m_isHeaderEncryptionEnabled;
}

QMimeType ReadOnlyArchiveInterface::mimetype() const
{
    return m_mimetype;
}

bool ReadOnlyArchiveInterface::hasBatchExtractionProgress() const
{
    return false;
}

bool ReadOnlyArchiveInterface::isLocked() const
{
    return false;
}

bool ReadWriteArchiveInterface::isReadOnly() const
{
    if (isLocked()) {
        return true;
    }

    // We set corrupt archives to read-only to avoid add/delete actions, that
    // are likely to fail anyway.
    if (isCorrupt()) {
        return true;
    }

    QFileInfo fileInfo(filename());
    if (fileInfo.exists()) {
        return !fileInfo.isWritable();
    } else {
        return !fileInfo.dir().exists(); // TODO: Should also check if we can create a file in that directory
    }
}

uint ReadOnlyArchiveInterface::numberOfEntries() const
{
    return m_numberOfEntries;
}

void ReadWriteArchiveInterface::onEntryRemoved(const QString &path)
{
    Q_UNUSED(path)
    m_numberOfEntries--;
}

} // namespace Kerfuffle

#include "moc_archiveinterface.cpp"
