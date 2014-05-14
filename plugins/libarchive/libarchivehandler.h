/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#ifndef LIBARCHIVEHANDLER_H
#define LIBARCHIVEHANDLER_H

#include "kerfuffle/archiveinterface.h"

#include <QDir>
#include <QList>
#include <QScopedPointer>
#include <QStringList>

using namespace Kerfuffle;

class LibArchiveInterface: public ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit LibArchiveInterface(QObject *parent, const QVariantList& args);
    ~LibArchiveInterface();

    bool list();
    bool doKill();
    bool copyFiles(const QVariantList& files, const QString& destinationDirectory, ExtractionOptions options);
    bool addFiles(const QStringList& files, const CompressionOptions& options);
    bool deleteFiles(const QVariantList& files);

private:
    void emitEntryFromArchiveEntry(struct archive_entry *entry);
    int extractionFlags() const;
    void copyData(const QString& filename, struct archive *dest, bool partialprogress = true);
    void copyData(struct archive *source, struct archive *dest, bool partialprogress = true);
    bool writeFile(const QString& fileName, struct archive* arch);

    struct ArchiveReadCustomDeleter;
    struct ArchiveWriteCustomDeleter;
    typedef QScopedPointer<struct archive, ArchiveReadCustomDeleter> ArchiveRead;
    typedef QScopedPointer<struct archive, ArchiveWriteCustomDeleter> ArchiveWrite;

    int m_cachedArchiveEntryCount;
    qlonglong m_currentExtractedFilesSize;
    bool m_emitNoEntries;
    qlonglong m_extractedFilesSize;
    QDir m_workDir;
    QStringList m_writtenFiles;
    ArchiveRead m_archiveReadDisk;
    bool m_abortOperation;
};

#endif // LIBARCHIVEHANDLER_H
