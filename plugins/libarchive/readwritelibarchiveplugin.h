/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef READWRITELIBARCHIVEPLUGIN_H
#define READWRITELIBARCHIVEPLUGIN_H

#include "libarchiveplugin.h"

#include <QStringList>
#include <QSaveFile>

using namespace Kerfuffle;

class ReadWriteLibarchivePlugin : public LibarchivePlugin
{
    Q_OBJECT

public:
    explicit ReadWriteLibarchivePlugin(QObject *parent, const QVariantList &args);
    ~ReadWriteLibarchivePlugin() override;

    bool addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions &options, uint numberOfEntriesToAdd = 0) override;
    bool moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool deleteFiles(const QVector<Archive::Entry*> &files) override;

protected:
    bool initializeWriter(const bool creatingNewFile = false, const CompressionOptions &options = CompressionOptions());
    bool initializeWriterFilters();
    bool initializeNewFileWriterFilters(const CompressionOptions &options);
    void finish(const bool isSuccessful);

private:
    /**
     * Processes all the existing entries and does manipulations to them
     * based on the OperationMode (Add/Move/Copy/Delete).
     *
     * @param entriesCounter Counter of added/moved/copied/deleted entries.
     *
     * @return bool indicating whether the operation was successful.
     */
    bool processOldEntries(uint &entriesCounter, OperationMode mode, uint totalCount);

    /**
     * Writes entry being read into memory.
     *
     * @return bool indicating whether the operation was successful.
     */
    bool writeEntry(struct archive_entry *entry);

    /**
     * Writes entry from physical disk.
     *
     * @return bool indicating whether the operation was successful.
     */
    bool writeFile(const QString &relativeName, const QString &destination);

    QSaveFile m_tempFile;
    ArchiveWrite m_archiveWriter;

    // New added files by addFiles methods. It's assigned to m_filesPaths
    // and then is used by processOldEntries method (in Add mode) for skipping already written entries.
    QStringList m_writtenFiles;

    // Passed argument from job which is used by processOldEntries method.
    QStringList m_filesPaths;
    int m_entriesWithoutChildren = 0;
    const Archive::Entry *m_destination = nullptr;
};

#endif // READWRITELIBARCHIVEPLUGIN_H
