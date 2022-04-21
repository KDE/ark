/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef ARCHIVEMODEL_H
#define ARCHIVEMODEL_H

#include "archiveentry.h"

#include <KMessageWidget>

#include <QAbstractItemModel>
#include <QScopedPointer>

using Kerfuffle::Archive;

namespace Kerfuffle
{
    class Query;
}

/**
 * Meta data related to one entry in a compressed archive.
 *
 * This is used for indexing entry properties as numbers
 * and for determining data displaying order in part's view.
 */
enum EntryMetaDataType {
    FullPath,            /**< The entry's file name */
    Size,                /**< The entry's original size */
    CompressedSize,      /**< The compressed size for the entry */
    Permissions,         /**< The entry's permissions */
    Owner,               /**< The user the entry belongs to */
    Group,               /**< The user group the entry belongs to */
    Ratio,               /**< The compression ratio for the entry */
    CRC,                 /**< The entry's CRC */
    BLAKE2,              /**< The entry's BLAKE2 */
    Method,              /**< The compression method used on the entry */
    Version,             /**< The archiver version needed to extract the entry */
    Timestamp            /**< The timestamp for the current entry */
};

class ArchiveModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ArchiveModel(const QString &dbusPathName, QObject *parent = nullptr);
    ~ArchiveModel() override;

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    //drag and drop related
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList & indexes) const override;
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) override;

    void reset();
    void createEmptyArchive(const QString &path, const QString &mimeType, QObject *parent);
    KJob* loadArchive(const QString &path, const QString &mimeType, QObject *parent);
    Kerfuffle::Archive *archive() const;

    QList<int> shownColumns() const;
    QMap<int, QByteArray> propertiesMap() const;

    Archive::Entry *entryForIndex(const QModelIndex &index);

    Kerfuffle::ExtractJob* extractFile(Archive::Entry *file, const QString& destinationDir, Kerfuffle::ExtractionOptions options = Kerfuffle::ExtractionOptions()) const;
    Kerfuffle::ExtractJob* extractFiles(const QVector<Archive::Entry*>& files, const QString& destinationDir, Kerfuffle::ExtractionOptions options = Kerfuffle::ExtractionOptions()) const;

    Kerfuffle::PreviewJob* preview(Archive::Entry *file) const;
    Kerfuffle::OpenJob* open(Archive::Entry *file) const;
    Kerfuffle::OpenWithJob* openWith(Archive::Entry *file) const;

    Kerfuffle::AddJob* addFiles(QVector<Archive::Entry*> &entries, const Archive::Entry *destination, const Kerfuffle::CompressionOptions& options = Kerfuffle::CompressionOptions());
    Kerfuffle::MoveJob* moveFiles(QVector<Archive::Entry*> &entries, Archive::Entry *destination, const Kerfuffle::CompressionOptions& options = Kerfuffle::CompressionOptions());
    Kerfuffle::CopyJob* copyFiles(QVector<Archive::Entry*> &entries, Archive::Entry *destination, const Kerfuffle::CompressionOptions& options = Kerfuffle::CompressionOptions());
    Kerfuffle::DeleteJob* deleteFiles(QVector<Archive::Entry*> entries);

    /**
     * @param password The password to encrypt the archive with.
     * @param encryptHeader Whether to encrypt also the list of files.
     */
    void encryptArchive(const QString &password, bool encryptHeader);

    void countEntriesAndSize();
    qulonglong numberOfFiles() const;
    qulonglong numberOfFolders() const;
    qulonglong uncompressedSize() const;

    /**
     * Constructs a list of conflicting entries.
     *
     * @param conflictingEntries Reference to the empty mutable entries list, which will be constructed.
     * If the method returns false, this list will contain only entries which produce a critical conflict.
     * @param entries New entries paths list.
     * @param allowMerging Boolean variable indicating whether merging is permitted.
     * If true, existing entries won't generate an error.
     *
     * @return Boolean variable indicating whether conflicts are not critical (true for not critical,
     * false for critical). For example, if there are both "some/file" (not a directory) and "some/file/" (a directory)
     * entries for both new and existing paths, the method will return false. Also, if merging is not allowed,
     * this method will return false for entries with the same path and types.
     */
    bool conflictingEntries(QList<const Archive::Entry*> &conflictingEntries, const QStringList &entries, bool allowMerging) const;

    static bool hasDuplicatedEntries(const QStringList &entries);

    static QMap<QString, Archive::Entry*> entryMap(const QVector<Archive::Entry*> &entries);

    QMap<QString, Kerfuffle::Archive::Entry*> filesToMove;
    QMap<QString, Kerfuffle::Archive::Entry*> filesToCopy;

Q_SIGNALS:
    void loadingStarted();
    void loadingFinished(KJob *);
    void error(const QString& error, const QString& details);
    void droppedFiles(const QStringList& files, const Archive::Entry*);
    void messageWidget(KMessageWidget::MessageType type, const QString& msg);

private Q_SLOTS:
    void slotNewEntry(Archive::Entry *entry);
    void slotListEntry(Archive::Entry *entry);
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

    void initRootEntry();

    enum InsertBehaviour { NotifyViews, DoNotNotifyViews };
    Archive::Entry *parentFor(const Kerfuffle::Archive::Entry *entry, InsertBehaviour behaviour = NotifyViews);
    QModelIndex indexForEntry(Archive::Entry *entry);
    /**
     * Insert the node @p node into the model, ensuring all views are notified
     * of the change.
     */

    void insertEntry(Archive::Entry *entry, InsertBehaviour behaviour = NotifyViews);
    void newEntry(Kerfuffle::Archive::Entry *receivedEntry, InsertBehaviour behaviour);

    qulonglong traverseAndComputeDirSizes(Archive::Entry *dir);

    QList<int> m_showColumns;
    QScopedPointer<Kerfuffle::Archive> m_archive;
    QScopedPointer<Archive::Entry> m_rootEntry;
    QHash<QString, QIcon> m_entryIcons;
    QMap<int, QByteArray> m_propertiesMap;

    QString m_dbusPathName;

    qulonglong m_numberOfFiles;
    qulonglong m_numberOfFolders;

    // Whether a file entry has been listed. Used to ensure all relevant columns are shown,
    // since directories might have fewer columns than files.
    bool m_fileEntryListed;
};

#endif // ARCHIVEMODEL_H
