/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef LIBARCHIVEPLUGIN_H
#define LIBARCHIVEPLUGIN_H

#include "archiveinterface.h"

#include <archive.h>

#include <QScopedPointer>

using namespace Kerfuffle;

class LibarchivePlugin : public ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit LibarchivePlugin(QObject *parent, const QVariantList &args);
    ~LibarchivePlugin() override;

    bool list() override;
    bool doKill() override;
    bool extractFiles(const QVector<Archive::Entry*> &files, const QString &destinationDirectory, const ExtractionOptions &options) override;

    bool addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions &options, uint numberOfEntriesToAdd = 0) override;
    bool moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool deleteFiles(const QVector<Archive::Entry*> &files) override;
    bool addComment(const QString &comment) override;
    bool testArchive() override;
    bool hasBatchExtractionProgress() const override;

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
    void emitEntryFromArchiveEntry(struct archive_entry *entry, bool isRawFormat = false);
    void copyData(const QString& filename, struct archive *dest, bool partialprogress = true);
    void copyData(const QString& filename, struct archive *source, struct archive *dest, bool partialprogress = true);

    ArchiveRead m_archiveReader;
    ArchiveRead m_archiveReadDisk;

private Q_SLOTS:
    void slotRestoreWorkingDir();

private:
    int extractionFlags() const;
    QString convertCompressionName(const QString &method);
    bool emitCorruptArchive();
    const QString uncompressedFileName() const;
    void copyDataBlock(const QString &filename, struct archive *source, struct archive *dest, bool partialprogress = true);

    int m_cachedArchiveEntryCount;
    qlonglong m_currentExtractedFilesSize;
    bool m_emitNoEntries;
    qlonglong m_extractedFilesSize;
    QVector<Archive::Entry*> m_emittedEntries;
    QString m_oldWorkingDir;
    QStringList m_rawMimetypes;
};

#endif // LIBARCHIVEPLUGIN_H
