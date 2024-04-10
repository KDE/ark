/*
    SPDX-FileCopyrightText: 2017 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef LIBZIPPLUGIN_H
#define LIBZIPPLUGIN_H

#include "archiveinterface.h"

#include <zip.h>

using namespace Kerfuffle;

class ZipSource;

class LibzipPlugin : public ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit LibzipPlugin(QObject *parent, const QVariantList &args);
    ~LibzipPlugin() override;

    bool list() override;
    bool doKill() override;
    bool extractFiles(const QList<Archive::Entry *> &files, const QString &destinationDirectory, const ExtractionOptions &options) override;

    bool addFiles(const QList<Archive::Entry *> &files,
                  const Archive::Entry *destination,
                  const CompressionOptions &options,
                  uint numberOfEntriesToAdd = 0) override;
    bool deleteFiles(const QList<Archive::Entry *> &files) override;
    bool moveFiles(const QList<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool copyFiles(const QList<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool addComment(const QString &comment) override;
    bool testArchive() override;
    bool hasBatchExtractionProgress() const override;

    bool isReadOnly() const override;
    QString multiVolumeName() const override;

private:
    bool extractEntry(zip_t *archive, const QString &entry, const QString &rootNode, const QString &destDir, bool preservePaths, bool removeRootNode);
    bool writeEntry(zip_t *archive, const QString &entry, const Archive::Entry *destination, const CompressionOptions &options, bool isDir = false);
    bool emitEntryForIndex(zip_t *archive, qlonglong index);
    void emitProgress(double percentage);
    QString fromUnixSeparator(const QString &path);
    QString toUnixSeparator(const QString &path);
    static void progressCallback(zip_t *, double progress, void *that);
    static int cancelCallback(zip_t *, void *that);

    QList<Archive::Entry *> m_emittedEntries;
    bool m_overwriteAll;
    bool m_skipAll;
    bool m_listAfterAdd;
    bool m_backslashedZip;
    QString m_multiVolumeName;
    std::unique_ptr<ZipSource> m_zipSource;
};

#endif // LIBZIPPLUGIN_H
