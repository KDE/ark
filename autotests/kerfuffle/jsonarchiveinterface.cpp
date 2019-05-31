/*
 * Copyright (c) 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include "jsonarchiveinterface.h"
#include "archiveentry.h"

#include <QFile>

JSONArchiveInterface::JSONArchiveInterface(QObject *parent, const QVariantList& args)
    : Kerfuffle::ReadWriteArchiveInterface(parent, args)
{
}

JSONArchiveInterface::~JSONArchiveInterface()
{
}

bool JSONArchiveInterface::list()
{
    JSONParser::JSONArchive::const_iterator it = m_archive.constBegin();
    for (; it != m_archive.constEnd(); ++it) {
        emit entry(*it);
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

    return !m_archive.isEmpty();
}

bool JSONArchiveInterface::addFiles(const QVector<Kerfuffle::Archive::Entry*>& files, const Kerfuffle::Archive::Entry *destination, const Kerfuffle::CompressionOptions& options, uint numberOfEntriesToAdd)
{
    Q_UNUSED(options)
    Q_UNUSED(numberOfEntriesToAdd)

    for (const Kerfuffle::Archive::Entry *entry : files) {
        const QString &path = destination->fullPath() + entry->fullPath();
        if (m_archive.contains(path)) {
            return false;
        }

        Kerfuffle::Archive::Entry *e = new Kerfuffle::Archive::Entry(nullptr);
        e->setProperty("fullPath", path);

        m_archive[path] = e;
    }

    return true;
}

bool JSONArchiveInterface::moveFiles(const QVector<Kerfuffle::Archive::Entry*>& files, Kerfuffle::Archive::Entry *destination, const Kerfuffle::CompressionOptions& options)
{
    Q_UNUSED(files)
    Q_UNUSED(destination)
    Q_UNUSED(options)

    return true;
}

bool JSONArchiveInterface::copyFiles(const QVector<Kerfuffle::Archive::Entry*>& files, Kerfuffle::Archive::Entry *destination, const Kerfuffle::CompressionOptions& options)
{
    Q_UNUSED(files)
    Q_UNUSED(destination)
    Q_UNUSED(options)

    return false;
}

bool JSONArchiveInterface::extractFiles(const QVector<Kerfuffle::Archive::Entry*>& files, const QString &destinationDirectory, const Kerfuffle::ExtractionOptions& options)
{
    Q_UNUSED(files)
    Q_UNUSED(destinationDirectory)
    Q_UNUSED(options)

    return true;
}

bool JSONArchiveInterface::deleteFiles(const QVector<Kerfuffle::Archive::Entry*>& files)
{
    for (const Kerfuffle::Archive::Entry *file : files) {
        const QString &fileName = file->fullPath();
        if (m_archive.contains(fileName)) {
            m_archive.remove(fileName);
            emit entryRemoved(fileName);
        }
    }

    return true;
}

bool JSONArchiveInterface::addComment(const QString& comment)
{
    Q_UNUSED(comment)
    return true;
}

bool JSONArchiveInterface::testArchive()
{
    return true;
}
