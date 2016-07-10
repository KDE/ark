/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef ARCHIVEMODEL_H
#define ARCHIVEMODEL_H

#include <QAbstractItemModel>
#include <QScopedPointer>

#include <kjobtrackerinterface.h>
#include "kerfuffle/archive_kerfuffle.h"

using Kerfuffle::ArchiveEntry;

namespace Kerfuffle
{
    class Query;
}

class ArchiveNode;
class ArchiveDirNode;

class ArchiveModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ArchiveModel(const QString &dbusPathName, QObject *parent = 0);
    ~ArchiveModel();

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) Q_DECL_OVERRIDE;

    //drag and drop related
    Qt::DropActions supportedDropActions() const Q_DECL_OVERRIDE;
    QStringList mimeTypes() const Q_DECL_OVERRIDE;
    QMimeData * mimeData(const QModelIndexList & indexes) const Q_DECL_OVERRIDE;
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) Q_DECL_OVERRIDE;

    KJob* setArchive(Kerfuffle::Archive *archive);
    Kerfuffle::Archive *archive() const;

    Kerfuffle::ArchiveEntry entryForIndex(const QModelIndex &index);
    int childCount(const QModelIndex &index, int &dirs, int &files) const;

    Kerfuffle::ExtractJob* extractFile(const QVariant& fileName, const QString& destinationDir, const Kerfuffle::ExtractionOptions& options = Kerfuffle::ExtractionOptions()) const;
    Kerfuffle::ExtractJob* extractFiles(const QList<QVariant>& files, const QString& destinationDir, const Kerfuffle::ExtractionOptions& options = Kerfuffle::ExtractionOptions()) const;

    Kerfuffle::PreviewJob* preview(const QString& file) const;
    Kerfuffle::OpenJob* open(const QString& file) const;
    Kerfuffle::OpenWithJob* openWith(const QString& file) const;

    Kerfuffle::AddJob* addFiles(const QStringList & paths, const Kerfuffle::CompressionOptions& options = Kerfuffle::CompressionOptions());
    Kerfuffle::DeleteJob* deleteFiles(const QList<QVariant> & files);

    /**
     * @param password The password to encrypt the archive with.
     * @param encryptHeader Whether to encrypt also the list of files.
     */
    void encryptArchive(const QString &password, bool encryptHeader);

    void countEntriesAndSize();
    qulonglong numberOfFiles() const;
    qulonglong numberOfFolders() const;
    qulonglong uncompressedSize() const;

signals:
    void loadingStarted();
    void loadingFinished(KJob *);
    void extractionFinished(bool success);
    void error(const QString& error, const QString& details);
    void droppedFiles(const QStringList& files, const QString& path = QString());

private slots:
    void slotNewEntryFromSetArchive(const ArchiveEntry& entry);
    void slotNewEntry(const ArchiveEntry& entry);
    void slotLoadingFinished(KJob *job);
    void slotEntryRemoved(const QString & path);
    void slotUserQuery(Kerfuffle::Query *query);
    void slotCleanupEmptyDirs();

private:
    /**
     * Strips file names that start with './'.
     *
     * For more information, see bug 194241.
     *
     * @param fileName The file name that will be stripped.
     *
     * @return @p fileName without the leading './'
     */
    QString cleanFileName(const QString& fileName);

    ArchiveDirNode* parentFor(const Kerfuffle::ArchiveEntry& entry);
    QModelIndex indexForNode(ArchiveNode *node);
    static bool compareAscending(const QModelIndex& a, const QModelIndex& b);
    static bool compareDescending(const QModelIndex& a, const QModelIndex& b);
    /**
     * Insert the node @p node into the model, ensuring all views are notified
     * of the change.
     */
    enum InsertBehaviour { NotifyViews, DoNotNotifyViews };
    void insertNode(ArchiveNode *node, InsertBehaviour behaviour = NotifyViews);
    void newEntry(const Kerfuffle::ArchiveEntry& entry, InsertBehaviour behaviour);

    void traverseAndCountDirNode(ArchiveDirNode *dir);

    QList<Kerfuffle::ArchiveEntry> m_newArchiveEntries; // holds entries from opening a new archive until it's totally open
    QList<int> m_showColumns;
    QScopedPointer<Kerfuffle::Archive> m_archive;
    ArchiveDirNode *m_rootNode;

    QString m_dbusPathName;

    qulonglong m_numberOfFiles;
    qulonglong m_numberOfFolders;
    qulonglong m_uncompressedSize;
};

#endif // ARCHIVEMODEL_H
