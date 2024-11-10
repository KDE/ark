/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef PART_H
#define PART_H

#include "archiveentry.h"
#include "interface.h"

#include <KMessageWidget>
#include <KParts/Part>
#include <KParts/ReadWritePart>
#include <KParts/StatusBarExtension>

#include <QModelIndex>
#include <QTimer>

class ArchiveModel;
class ArchiveSortFilterModel;
class ArchiveView;
class InfoPanel;

class KAbstractWidgetJobTracker;
class KJob;
class KToggleAction;

class QAction;
class QLineEdit;
class QSplitter;
class QTemporaryDir;
class QVBoxLayout;
class QFileSystemWatcher;
class QGroupBox;
class QPlainTextEdit;
class QPushButton;

namespace Ark
{
class Part : public KParts::ReadWritePart, public Interface
{
    Q_OBJECT
    Q_INTERFACES(Interface)
public:
    enum OpenFileMode {
        Preview,
        OpenFile,
        OpenFileWith,
    };

    enum OverwriteBehaviour {
        ShowOverwriteDialog,
        DoNotShowOverwriteDialog,
    };

    Part(QWidget *parentWidget, QObject *parent, const KPluginMetaData &metaData, const QVariantList &);
    ~Part() override;

    bool openFile() override;
    bool saveFile() override;

    bool isBusy() const override;
    KConfigSkeleton *config() const override;
    QList<Kerfuffle::SettingsPage *> settingsPages(QWidget *parent) const override;
    QWidget *infoPanel() const override;

    bool eventFilter(QObject *target, QEvent *event) override;

    /**
     * Return custom compnentName for KXMLGUIClient, as by history not the plugin id is used
     */
    QString componentName() const override;

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
    bool confirmAndDelete(const QString &targetFile);

public Q_SLOTS:
    // Extracts selected files to @p localPath when drag and drop'ing from Ark to e.g. Dolphin
    void extractSelectedFilesTo(const QString &localPath);

protected:
    void guiActivateEvent(KParts::GUIActivateEvent *event) override;

private Q_SLOTS:
    void slotCompleted();
    void slotLoadingStarted();
    void slotLoadingFinished(KJob *job);
    void slotOpenExtractedEntry(KJob *);
    void slotPreviewExtractedEntry(KJob *job);
    void slotOpenEntry(int mode);
    void slotError(const QString &errorMessage, const QString &details);
    void slotExtractArchive();
    void slotShowExtractionDialog();
    void slotExtractionDone(KJob *);
    void slotQuickExtractFiles(QAction *);

    /**
     * Creates and starts AddJob.
     *
     * @param files Files to add.
     * @param destination Destination path within the archive to which entries have to be added. Is used on addto action
     * or drag'n'drop event, for adding a watched file it has empty.
     * @param relPath Relative path of watched entry inside the archive. Is used only for adding temporarily extracted
     * watched file.
     * @param onOverwrite Whether to show a confirmation dialog when files will be overwritten
     */
    void slotAddFiles(const QStringList &files,
                      const Kerfuffle::Archive::Entry *destination,
                      const QString &relPath,
                      Ark::Part::OverwriteBehaviour onOverwrite = ShowOverwriteDialog);
    void slotDroppedFiles(const QStringList &files, const Kerfuffle::Archive::Entry *destination);

    /**
     * Creates and starts MoveJob or CopyJob.
     *
     * @param files Files to paste.
     * @param destination Destination path within the archive to which entries have to be added. For renaming an entry
     * the path has to contain a new filename too.
     * @param entriesWithoutChildren Entries count, excluding their children. For CopyJob 0 MUST be passed.
     */
    void slotPasteFiles(QList<Kerfuffle::Archive::Entry *> &files, Kerfuffle::Archive::Entry *destination, int entriesWithoutChildren);

    void slotAddFiles();
    void slotCutFiles();
    void slotCopyFiles();
    void slotRenameFile(const QString &name);
    void slotPasteFiles();
    void slotAddFilesDone(KJob *);
    void slotPasteFilesDone(KJob *);
    void slotTestingDone(KJob *);
    void slotDeleteFiles();
    void slotDeleteFilesDone(KJob *);
    void slotShowProperties();
    void slotShowContextMenu();
    void slotActivated(const QModelIndex &index);
    void slotToggleInfoPanel(bool);
    void slotSaveAs();
    void updateActions();
    void updateQuickExtractMenu(QAction *extractAction);
    void selectionChanged();
    void setBusyGui();
    void setReadyGui();
    void setFileNameFromArchive();
    void slotResetFileChangeTimer(const QString &file);
    void slotWatchedFileModified(const QString &file);
    void slotShowComment();
    void slotAddComment();
    void slotCommentChanged();
    void slotTestArchive();
    void slotShowFind();
    void displayMsgWidget(KMessageWidget::MessageType type, const QString &msg);
    void searchEdited(const QString &text);

Q_SIGNALS:
    void busy();
    void ready();
    void quit();

private:
    /**
     * @return true if both the current archive and the part are read-write, false otherwise.
     */
    bool isArchiveWritable() const;

    /**
     * @return Whether the part has been told to create a new archive.
     */
    bool isCreatingNewArchive() const;

    void createArchive();
    void loadArchive();
    void resetArchive();
    void resetGui();
    void setupView();
    void setupActions();
    QString detectSubfolder() const;
    QList<Kerfuffle::Archive::Entry *> filesForIndexes(const QModelIndexList &list) const;
    QList<Kerfuffle::Archive::Entry *> filesAndRootNodesForIndexes(const QModelIndexList &list) const;
    QModelIndexList addChildren(const QModelIndexList &list) const;
    void registerJob(KJob *job);
    QModelIndexList getSelectedIndexes();
    void readCompressionOptions();

    ArchiveModel *m_model;
    ArchiveView *m_view;
    QAction *m_previewAction;
    QAction *m_openFileAction;
    QAction *m_openFileWithAction;
    QAction *m_extractArchiveAction;
    QAction *m_extractAction;
    QAction *m_addFilesAction;
    QAction *m_renameFileAction;
    QAction *m_deleteFilesAction;
    QAction *m_cutFilesAction;
    QAction *m_copyFilesAction;
    QAction *m_pasteFilesAction;
    QAction *m_saveAsAction;
    QAction *m_propertiesAction;
    QAction *m_editCommentAction;
    QAction *m_testArchiveAction;
    QAction *m_searchAction;
    KToggleAction *m_showInfoPanelAction;
    InfoPanel *m_infoPanel;
    QSplitter *m_splitter;
    QList<QTemporaryDir *> m_tmpExtractDirList;
    bool m_busy;

    OpenFileMode m_openFileMode;
    QUrl m_lastUsedAddPath;
    QList<Kerfuffle::Archive::Entry *> m_jobTempEntries;
    Kerfuffle::Archive::Entry *m_destination;
    QModelIndexList m_cutIndexes;

    QTimer m_watchedFileChangeTimer;
    QString m_lastChangedFilename;

    KAbstractWidgetJobTracker *m_jobTracker;
    KParts::StatusBarExtension *m_statusBarExtension;
    QVBoxLayout *m_vlayout;
    std::unique_ptr<QFileSystemWatcher> m_fileWatcher;
    QSplitter *m_commentSplitter;
    QGroupBox *m_commentBox;
    QPlainTextEdit *m_commentView;
    KMessageWidget *m_commentMsgWidget;
    KMessageWidget *m_messageWidget;
    Kerfuffle::CompressionOptions m_compressionOptions;
    ArchiveSortFilterModel *m_filterModel;
    QWidget *m_searchWidget;
    QLineEdit *m_searchLineEdit;
    QPushButton *m_searchCloseButton;
};

} // namespace Ark

#endif // PART_H
