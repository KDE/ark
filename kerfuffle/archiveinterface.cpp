/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2009-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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

#include "archiveinterface.h"
#include "ark_debug.h"
#include "mimetypes.h"

#include <QDir>
#include <QFileInfo>

namespace Kerfuffle
{
ReadOnlyArchiveInterface::ReadOnlyArchiveInterface(QObject *parent, const QVariantList & args)
        : QObject(parent)
        , m_numberOfVolumes(0)
        , m_numberOfEntries(0)
        , m_waitForFinishedSignal(false)
        , m_isHeaderEncryptionEnabled(false)
        , m_isCorrupt(false)
        , m_isMultiVolume(false)
{
    Q_ASSERT(args.size() >= 2);

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
    Q_UNUSED(archiveEntry)
    m_numberOfEntries++;
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
    //default implementation
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

int ReadOnlyArchiveInterface::moveRequiredSignals() const {
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

QStringList ReadOnlyArchiveInterface::entryFullPaths(const QVector<Archive::Entry*> &entries, PathFormat format)
{
    QStringList filesList;
    for (const Archive::Entry *file : entries) {
        filesList << file->fullPath(format);
    }
    return filesList;
}

QVector<Archive::Entry*> ReadOnlyArchiveInterface::entriesWithoutChildren(const QVector<Archive::Entry*> &entries)
{
    // QMap is easy way to get entries sorted by their fullPath.
    QMap<QString, Archive::Entry*> sortedEntries;
    for (Archive::Entry *entry : entries) {
        sortedEntries.insert(entry->fullPath(), entry);
    }

    QVector<Archive::Entry*> filteredEntries;
    QString lastFolder;
    for (Archive::Entry *entry : qAsConst(sortedEntries)) {
        if (lastFolder.count() > 0 && entry->fullPath().startsWith(lastFolder)) {
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
    for (const QString &entryPath : qAsConst(entries)) {
        if (lastFolder.count() > 0 && entryPath.startsWith(lastFolder)) {
            // Replace last moved or copied folder path with destination path.
            int charsCount = entryPath.count() - lastFolder.count();
            if (entriesWithoutChildren != 1) {
                charsCount += nameLength;
            }
            newPath = destinationPath + entryPath.right(charsCount);
        } else {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            const QString name = entryPath.split(QLatin1Char('/'), QString::SkipEmptyParts).last();
#else
            const QString name = entryPath.split(QLatin1Char('/'), Qt::SkipEmptyParts).last();
#endif
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
                nameLength = name.count() + 1; // plus slash
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
