/*
    SPDX-FileCopyrightText: 2017 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef LIBZIPPLUGIN_H
#define LIBZIPPLUGIN_H

#include "archiveinterface.h"


#include <zip.h>

using namespace Kerfuffle;

class LibzipPlugin : public ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit LibzipPlugin(QObject *parent, const QVariantList& args);
    ~LibzipPlugin() override;

    bool list() override;
    bool doKill() override;
    bool extractFiles(const QVector<Archive::Entry*> &files, const QString& destinationDirectory, const ExtractionOptions& options) override;

    bool addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions& options, uint numberOfEntriesToAdd = 0) override;
    bool deleteFiles(const QVector<Archive::Entry*> &files) override;
    bool moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool addComment(const QString& comment) override;
    bool testArchive() override;
    bool hasBatchExtractionProgress() const override;

private:
    bool extractEntry(zip_t *archive, const QString &entry, const QString &rootNode, const QString &destDir, bool preservePaths, bool removeRootNode);
    bool writeEntry(zip_t *archive, const QString &entry, const Archive::Entry* destination, const CompressionOptions& options, bool isDir = false);
    bool emitEntryForIndex(zip_t *archive, qlonglong index);
    void emitProgress(double percentage);
    QString permissionsToString(mode_t perm);
    QString fromUnixSeparator(const QString& path);
    QString toUnixSeparator(const QString& path);
    static void progressCallback(zip_t *, double progress, void *that);
    static int cancelCallback(zip_t *, void *that);

    QVector<Archive::Entry*> m_emittedEntries;
    bool m_overwriteAll;
    bool m_skipAll;
    bool m_listAfterAdd;
    bool m_backslashedZip;
};

#endif // LIBZIPPLUGIN_H
