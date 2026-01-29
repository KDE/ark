/*
    SPDX-FileCopyrightText: 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "jsonarchiveinterface.h"
#include "archiveentry.h"

#include <QFile>

JSONArchiveInterface::JSONArchiveInterface(QObject *parent, const QVariantList &args)
    : Kerfuffle::ReadWriteArchiveInterface(parent, args)
{
}

JSONArchiveInterface::~JSONArchiveInterface()
{
}

bool JSONArchiveInterface::list()
{
    JSONParser::JSONArchive::const_iterator it = m_archive.cbegin();
    for (; it != m_archive.cend(); ++it) {
        Q_EMIT entry(it->second.get());
    }

    return true;
}

bool JSONArchiveInterface::open()
{
    QFile file(filename());

    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    m_archive = JSONParser::parse(&file);

    return !m_archive.empty();
}

bool JSONArchiveInterface::addFiles(const QList<Kerfuffle::Archive::Entry *> &files,
                                    const Kerfuffle::Archive::Entry *destination,
                                    const Kerfuffle::CompressionOptions &options,
                                    uint numberOfEntriesToAdd)
{
    Q_UNUSED(options)
    Q_UNUSED(numberOfEntriesToAdd)

    for (const Kerfuffle::Archive::Entry *entry : files) {
        const QString &path = destination->fullPath() + entry->fullPath();
        if (m_archive.contains(path)) {
            return false;
        }

        auto e = std::make_unique<Kerfuffle::Archive::Entry>(nullptr);
        e->setProperty("fullPath", path);

        m_archive[path] = std::move(e);
    }

    return true;
}

bool JSONArchiveInterface::moveFiles(const QList<Kerfuffle::Archive::Entry *> &files,
                                     Kerfuffle::Archive::Entry *destination,
                                     const Kerfuffle::CompressionOptions &options)
{
    Q_UNUSED(files)
    Q_UNUSED(destination)
    Q_UNUSED(options)

    return true;
}

bool JSONArchiveInterface::copyFiles(const QList<Kerfuffle::Archive::Entry *> &files,
                                     Kerfuffle::Archive::Entry *destination,
                                     const Kerfuffle::CompressionOptions &options)
{
    Q_UNUSED(files)
    Q_UNUSED(destination)
    Q_UNUSED(options)

    return false;
}

bool JSONArchiveInterface::extractFiles(const QList<Kerfuffle::Archive::Entry *> &files,
                                        const QString &destinationDirectory,
                                        const Kerfuffle::ExtractionOptions &options)
{
    Q_UNUSED(files)
    Q_UNUSED(destinationDirectory)
    Q_UNUSED(options)

    return true;
}

bool JSONArchiveInterface::deleteFiles(const QList<Kerfuffle::Archive::Entry *> &files)
{
    for (const Kerfuffle::Archive::Entry *file : files) {
        const QString &fileName = file->fullPath();
        if (m_archive.contains(fileName)) {
            m_archive.erase(fileName);
            Q_EMIT entryRemoved(fileName);
        }
    }

    return true;
}

bool JSONArchiveInterface::addComment(const QString &comment)
{
    Q_UNUSED(comment)
    return true;
}

bool JSONArchiveInterface::testArchive()
{
    return true;
}

#include "moc_jsonarchiveinterface.cpp"
