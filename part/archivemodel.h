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
#include "kerfuffle/archiveentry.h"

using Kerfuffle::Archive;

namespace Kerfuffle
{
    class Query;
}

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
    QMimeData *mimeData(const QModelIndexList & indexes) const Q_DECL_OVERRIDE;
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) Q_DECL_OVERRIDE;

    KJob* setArchive(Kerfuffle::Archive *archive);
    Kerfuffle::Archive *archive() const;

    Archive::Entry *entryForIndex(const QModelIndex &index);
    int childCount(const QModelIndex &index, int &dirs, int &files) const;

    Kerfuffle::ExtractJob* extractFile(Archive::Entry *file, const QString& destinationDir, const Kerfuffle::ExtractionOptions& options = Kerfuffle::ExtractionOptions()) const;
    Kerfuffle::ExtractJob* extractFiles(const QList<Archive::Entry*>& files, const QString& destinationDir, const Kerfuffle::ExtractionOptions& options = Kerfuffle::ExtractionOptions()) const;

    Kerfuffle::PreviewJob* preview(Archive::Entry *file) const;
    Kerfuffle::OpenJob* open(Archive::Entry *file) const;
    Kerfuffle::OpenWithJob* openWith(Archive::Entry *file) const;

    Kerfuffle::AddJob* addFiles(QList<Archive::Entry*> &entries, const Archive::Entry *destination, const Kerfuffle::CompressionOptions& options = Kerfuffle::CompressionOptions());
    Kerfuffle::MoveJob* moveFiles(QList<Archive::Entry*> &entries, Archive::Entry *destination, const Kerfuffle::CompressionOptions& options = Kerfuffle::CompressionOptions());
    Kerfuffle::CopyJob* copyFiles(QList<Archive::Entry*> &entries, Archive::Entry *destination, const Kerfuffle::CompressionOptions& options = Kerfuffle::CompressionOptions());
    Kerfuffle::DeleteJob* deleteFiles(QList<Archive::Entry*> entries);

    /**
     * @param password The password to encrypt the archive with.
     * @param encryptHeader Whether to encrypt also the list of files.
     */
    void encryptArchive(const QString &password, bool encryptHeader);

    /**
     * Returns the string list of entry paths, which will be a result of adding/moving/copying entries.
     *
     * @param entries The entries which will be added/moved/copied.
     * @param destination Destination path within the archive to which entries have to be added. For renaming an entry
     * the path has to contain a new filename too.
     * @param entriesWithoutChildren Entries count, excluding their children. For AddJob or CopyJob 0 MUST be passed.
     *
     * @return For entries
     *  some/dir/
     *  some/dir/entry
     *  some/dir/some/entry
     *  some/another/entry
     * and destination
     *  some/destination
     * will return
     *  some/destination/dir/
     *  some/destination/dir/entry
     *  some/destination/dir/some/enty
     *  some/destination/entry
     */
    static QStringList entryPathsFromDestination(QStringList entries, const Archive::Entry *destination, int entriesWithoutChildren);

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

    const QHash<QString, QPixmap> entryIcons() const;

signals:
    void loadingStarted();
    void loadingFinished(KJob *);
    void extractionFinished(bool success);
    void error(const QString& error, const QString& details);
    void droppedFiles(const QStringList& files, const Archive::Entry*, const QString&);

private slots:
    void slotNewEntryFromSetArchive(Archive::Entry *entry);
    void slotNewEntry(Archive::Entry *entry);
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

    Archive::Entry *parentFor(const Kerfuffle::Archive::Entry *entry);
    QModelIndex indexForEntry(Archive::Entry *entry);
    static bool compareAscending(const QModelIndex& a, const QModelIndex& b);
    static bool compareDescending(const QModelIndex& a, const QModelIndex& b);
    /**
     * Insert the node @p node into the model, ensuring all views are notified
     * of the change.
     */
    enum InsertBehaviour { NotifyViews, DoNotNotifyViews };
    void insertEntry(Archive::Entry *entry, InsertBehaviour behaviour = NotifyViews);
    void newEntry(Kerfuffle::Archive::Entry *receivedEntry, InsertBehaviour behaviour);

    QList<Kerfuffle::Archive::Entry*> m_newArchiveEntries; // holds entries from opening a new archive until it's totally open
    QList<int> m_showColumns;
    QScopedPointer<Kerfuffle::Archive> m_archive;
    Archive::Entry m_rootEntry;
    QHash<QString, QPixmap> m_entryIcons;

    QString m_dbusPathName;
};

#endif // ARCHIVEMODEL_H
