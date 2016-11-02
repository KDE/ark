/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#ifndef LIBARCHIVEPLUGIN_H
#define LIBARCHIVEPLUGIN_H

#include "kerfuffle/archiveinterface.h"
#include "kerfuffle/archiveentry.h"

#include <archive.h>

#include <QScopedPointer>

using namespace Kerfuffle;

class LibarchivePlugin : public ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit LibarchivePlugin(QObject *parent, const QVariantList &args);
    virtual ~LibarchivePlugin();

    virtual bool list() Q_DECL_OVERRIDE;
    virtual bool doKill() Q_DECL_OVERRIDE;
    virtual bool extractFiles(const QVector<Archive::Entry*> &files, const QString &destinationDirectory, const ExtractionOptions &options) Q_DECL_OVERRIDE;

    virtual bool addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions &options, uint numberOfEntriesToAdd = 0) Q_DECL_OVERRIDE;
    virtual bool moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options) Q_DECL_OVERRIDE;
    virtual bool copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options) Q_DECL_OVERRIDE;
    virtual bool deleteFiles(const QVector<Archive::Entry*> &files) Q_DECL_OVERRIDE;
    virtual bool addComment(const QString &comment) Q_DECL_OVERRIDE;
    virtual bool testArchive() Q_DECL_OVERRIDE;

protected:
    struct ArchiveReadCustomDeleter
    {
        static inline void cleanup(struct archive *a)
        {
            if (a) {
                archive_read_free(a);
            }
        }
    };

    struct ArchiveWriteCustomDeleter
    {
        static inline void cleanup(struct archive *a)
        {
            if (a) {
                archive_write_free(a);
            }
        }
    };

    typedef QScopedPointer<struct archive, ArchiveReadCustomDeleter> ArchiveRead;
    typedef QScopedPointer<struct archive, ArchiveWriteCustomDeleter> ArchiveWrite;

    bool initializeReader();
    void emitEntryFromArchiveEntry(struct archive_entry *entry);
    void copyData(const QString& filename, struct archive *dest, bool partialprogress = true);
    void copyData(const QString& filename, struct archive *source, struct archive *dest, bool partialprogress = true);

    ArchiveRead m_archiveReader;
    ArchiveRead m_archiveReadDisk;

private:
    int extractionFlags() const;
    QString convertCompressionName(const QString &method);

    int m_cachedArchiveEntryCount;
    qlonglong m_currentExtractedFilesSize;
    bool m_emitNoEntries;
    qlonglong m_extractedFilesSize;
    QVector<Archive::Entry*> m_emittedEntries;
};

#endif // LIBARCHIVEPLUGIN_H
