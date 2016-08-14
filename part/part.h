/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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
#ifndef PART_H
#define PART_H

#include "interface.h"
#include "kerfuffle/archiveentry.h"

#include <KParts/Part>
#include <KParts/ReadWritePart>
#include <KParts/StatusBarExtension>
#include <KMessageWidget>

#include <QModelIndex>

class ArchiveModel;
class ArchiveView;
class InfoPanel;

class KAboutData;
class KAbstractWidgetJobTracker;
class KJob;
class KToggleAction;

class QAction;
class QSplitter;
class QTreeView;
class QTemporaryDir;
class QVBoxLayout;
class QSignalMapper;
class QFileSystemWatcher;
class QGroupBox;
class QPlainTextEdit;

namespace Ark
{

class Part: public KParts::ReadWritePart, public Interface
{
    Q_OBJECT
    Q_INTERFACES(Interface)
public:
    enum OpenFileMode {
        Preview,
        OpenFile,
        OpenFileWith
    };

    Part(QWidget *parentWidget, QObject *parent, const QVariantList &);
    ~Part();

    static KAboutData *createAboutData();

    bool openFile() Q_DECL_OVERRIDE;
    bool saveFile() Q_DECL_OVERRIDE;

    bool isBusy() const Q_DECL_OVERRIDE;
    KConfigSkeleton *config() const Q_DECL_OVERRIDE;
    QList<Kerfuffle::SettingsPage*> settingsPages(QWidget *parent) const Q_DECL_OVERRIDE;

    /**
     * Validate the localFilePath() associated to this part.
     * If the file is not valid, an error message is displayed to the user.
     * @return Whether the localFilePath() can be loaded by the part.
     */
    bool isLocalFileValid();

    /**
     * Ask the user whether to overwrite @p targetFile, when creating a new archive with the same path.
     * @return True if the file has been successfully removed upon user's will. False otherwise.
     */
    bool confirmAndDelete(const QString& targetFile);

public slots:
    void extractSelectedFilesTo(const QString& localPath);

private slots:
    void slotLoadingStarted();
    void slotLoadingFinished(KJob *job);
    void slotOpenExtractedEntry(KJob*);
    void slotPreviewExtractedEntry(KJob* job);
    void slotOpenEntry(int mode);
    void slotError(const QString& errorMessage, const QString& details);
    void slotExtractArchive();
    void slotShowExtractionDialog();
    void slotExtractionDone(KJob*);
    void slotQuickExtractFiles(QAction*);

    /**
     * Creates and starts AddJob.
     *
     * @param files Files to add.
     * @param destination Destination path within the archive to which entries have to be added. Is used on addto action
     * or drag'n'drop event, for adding a watched file it has empty.
     * @param relPath Relative path of watched entry inside the archive. Is used only for adding temporarily extracted
     * watched file.
     */
    void slotAddFiles(const QStringList &files, const Kerfuffle::Archive::Entry *destination, const QString &relPath);

    /**
     * Creates and starts MoveJob or CopyJob.
     *
     * @param files Files to paste.
     * @param destination Destination path within the archive to which entries have to be added. For renaming an entry
     * the path has to contain a new filename too.
     * @param entriesWithoutChildren Entries count, excluding their children. For CopyJob 0 MUST be passed.
     */
    void slotPasteFiles(QList<Kerfuffle::Archive::Entry*> &files, Kerfuffle::Archive::Entry *destination, int entriesWithoutChildren);

    void slotAddFiles();
    void slotEditFileName();
    void slotCutFiles();
    void slotCopyFiles();
    void slotRenameFile(QString name);
    void slotPasteFiles();
    void slotAddFilesDone(KJob*);
    void slotPasteFilesDone(KJob*);
    void slotTestingDone(KJob*);
    void slotDeleteFiles();
    void slotDeleteFilesDone(KJob*);
    void slotShowProperties();
    void slotShowContextMenu();
    void slotActivated(QModelIndex);
    void slotToggleInfoPanel(bool);
    void slotSaveAs();
    void updateActions();
    void updateQuickExtractMenu(QAction *extractAction);
    void selectionChanged();
    void adjustColumns();
    void setBusyGui();
    void setReadyGui();
    void setFileNameFromArchive();
    void slotWatchedFileModified(const QString& file);
    void slotShowComment();
    void slotAddComment();
    void slotCommentChanged();
    void slotTestArchive();

signals:
    void busy();
    void ready();
    void quit();

private:
    void resetGui();
    void setupView();
    void setupActions();
    bool isSingleFolderArchive() const;
    QString detectSubfolder() const;
    QList<Kerfuffle::Archive::Entry*> filesForIndexes(const QModelIndexList& list) const;
    QList<Kerfuffle::Archive::Entry*> filesAndRootNodesForIndexes(const QModelIndexList& list) const;
    QModelIndexList addChildren(const QModelIndexList &list) const;
    void registerJob(KJob *job);
    void displayMsgWidget(KMessageWidget::MessageType type, const QString& msg);

    ArchiveModel         *m_model;
    ArchiveView          *m_view;
    QAction *m_previewAction;
    QAction *m_openFileAction;
    QAction *m_openFileWithAction;
    QAction *m_extractArchiveAction;
    QAction *m_extractAction;
    QAction *m_addFilesAction;
    QAction *m_addFilesToAction;
    QAction *m_renameFileAction;
    QAction *m_deleteFilesAction;
    QAction *m_cutFilesAction;
    QAction *m_copyFilesAction;
    QAction *m_pasteFilesAction;
    QAction *m_saveAsAction;
    QAction *m_propertiesAction;
    QAction *m_editCommentAction;
    QAction *m_testArchiveAction;
    KToggleAction *m_showInfoPanelAction;
    InfoPanel            *m_infoPanel;
    QSplitter            *m_splitter;
    QList<QTemporaryDir*>      m_tmpOpenDirList;
    bool                  m_busy;
    OpenFileMode m_openFileMode;
    QUrl m_lastUsedAddPath;
    QList<Kerfuffle::Archive::Entry*> m_jobTempEntries;
    QList<Kerfuffle::Archive::Entry*> m_filesToMove;
    QList<Kerfuffle::Archive::Entry*> m_filesToCopy;
    Kerfuffle::Archive::Entry *m_destination;

    KAbstractWidgetJobTracker  *m_jobTracker;
    KParts::StatusBarExtension *m_statusBarExtension;
    QVBoxLayout *m_vlayout;
    QSignalMapper *m_signalMapper;
    QFileSystemWatcher *m_fileWatcher;
    QSplitter *m_commentSplitter;
    QGroupBox *m_commentBox;
    QPlainTextEdit *m_commentView;
    KMessageWidget *m_commentMsgWidget;
    KMessageWidget *m_messageWidget;
};

} // namespace Ark

#endif // PART_H
