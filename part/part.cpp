/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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

#include "part.h"
#include "ark_debug.h"
#include "adddialog.h"
#include "overwritedialog.h"
#include "archiveformat.h"
#include "archivemodel.h"
#include "archivesortfiltermodel.h"
#include "archiveview.h"
#include "arkviewer.h"
#include "dnddbusinterfaceadaptor.h"
#include "infopanel.h"
#include "jobtracker.h"
#include "generalsettingspage.h"
#include "extractiondialog.h"
#include "extractionsettingspage.h"
#include "jobs.h"
#include "settings.h"
#include "previewsettingspage.h"
#include "propertiesdialog.h"
#include "pluginsettingspage.h"
#include "pluginmanager.h"

#include <KAboutData>
#include <KActionCollection>
#include <KConfigGroup>
#include <KGuiItem>
#include <KIO/Job>
#include <KJobWidgets>
#include <KIO/StatJob>
#include <KMessageBox>
#include <KParts/OpenUrlArguments>
#include <KPluginFactory>
#include <KRun>
#include <KSelectAction>
#include <KStandardGuiItem>
#include <KToggleAction>
#include <KLocalizedString>
#include <KXMLGUIFactory>

#include <QAction>
#include <QCursor>
#include <QHeaderView>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QScopedPointer>
#include <QStatusBar>
#include <QPointer>
#include <QSplitter>
#include <QTimer>
#include <QFileDialog>
#include <QIcon>
#include <QInputDialog>
#include <QFileSystemWatcher>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QPushButton>

using namespace Kerfuffle;

namespace Ark
{

static quint32 s_instanceCounter = 1;

Part::Part(QWidget *parentWidget, QObject *parent, const QVariantList& args)
        : KParts::ReadWritePart(parent),
          m_splitter(nullptr),
          m_busy(false),
          m_jobTracker(nullptr)
{
    Q_UNUSED(args)
    KAboutData aboutData(QStringLiteral("ark"),
                         i18n("ArkPart"),
                         QStringLiteral("3.0"));
    setComponentData(aboutData, false);

    new DndExtractAdaptor(this);

    const QString pathName = QStringLiteral("/DndExtract/%1").arg(s_instanceCounter++);
    if (!QDBusConnection::sessionBus().registerObject(pathName, this)) {
        qCCritical(ARK) << "Could not register a D-Bus object for drag'n'drop";
    }

    // m_vlayout is needed for later insertion of QMessageWidget
    QWidget *mainWidget = new QWidget;
    m_vlayout = new QVBoxLayout;
    m_model = new ArchiveModel(pathName, this);
    m_filterModel = new ArchiveSortFilterModel(this);
    m_splitter = new QSplitter(Qt::Horizontal, parentWidget);
    m_view = new ArchiveView;
    m_infoPanel = new InfoPanel(m_model);

    // Add widgets for the comment field.
    m_commentView = new QPlainTextEdit();
    m_commentView->setReadOnly(true);
    m_commentView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_commentBox = new QGroupBox(i18n("Comment"));
    m_commentBox->hide();
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(m_commentView);
    m_commentBox->setLayout(vbox);

    m_messageWidget = new KMessageWidget(parentWidget);
    m_messageWidget->setWordWrap(true);
    m_messageWidget->hide();

    m_commentMsgWidget = new KMessageWidget();
    m_commentMsgWidget->setText(i18n("Comment has been modified."));
    m_commentMsgWidget->setMessageType(KMessageWidget::Information);
    m_commentMsgWidget->setCloseButtonVisible(false);
    m_commentMsgWidget->hide();

    QAction *saveAction = new QAction(i18n("Save"), m_commentMsgWidget);
    m_commentMsgWidget->addAction(saveAction);
    connect(saveAction, &QAction::triggered, this, &Part::slotAddComment);

    m_commentBox->layout()->addWidget(m_commentMsgWidget);

    connect(m_commentView, &QPlainTextEdit::textChanged, this, &Part::slotCommentChanged);

    setWidget(mainWidget);
    mainWidget->setLayout(m_vlayout);

    // Setup search widget.
    m_searchWidget = new QWidget(parentWidget);
    m_searchWidget->setVisible(false);
    m_searchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->setContentsMargins(2, 2, 2, 2);
    m_vlayout->addWidget(m_searchWidget);
    m_searchWidget->setLayout(searchLayout);
    m_searchCloseButton = new QPushButton(QIcon::fromTheme(QStringLiteral("dialog-close")), QString(), m_searchWidget);
    m_searchCloseButton->setFlat(true);
    m_searchLineEdit = new QLineEdit(m_searchWidget);
    m_searchLineEdit->setClearButtonEnabled(true);
    m_searchLineEdit->setPlaceholderText(i18n("Type to search..."));
    mainWidget->installEventFilter(this);
    searchLayout->addWidget(m_searchCloseButton);
    searchLayout->addWidget(m_searchLineEdit);
    connect(m_searchCloseButton, &QPushButton::clicked, this, [=]() {
        m_searchWidget->hide();
        m_searchLineEdit->clear();
    });
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, &Part::searchEdited);

    // Configure the QVBoxLayout and add widgets
    m_vlayout->setContentsMargins(0,0,0,0);
    m_vlayout->addWidget(m_messageWidget);
    m_vlayout->addWidget(m_splitter);

    // Vertical QSplitter for the file view and comment field.
    m_commentSplitter = new QSplitter(Qt::Vertical, parentWidget);
    m_commentSplitter->setOpaqueResize(false);
    m_commentSplitter->addWidget(m_view);
    m_commentSplitter->addWidget(m_commentBox);
    m_commentSplitter->setCollapsible(0, false);

    // Horizontal QSplitter for the file view and infopanel.
    m_splitter->addWidget(m_commentSplitter);
    m_splitter->addWidget(m_infoPanel);

    // Read settings from config file and show/hide infoPanel.
    if (!ArkSettings::showInfoPanel()) {
        m_infoPanel->hide();
    } else {
        m_splitter->setSizes(ArkSettings::splitterSizes());
    }

    setupView();
    setupActions();

    connect(m_view, &ArchiveView::entryChanged,
            this, &Part::slotRenameFile);

    connect(m_model, &ArchiveModel::loadingStarted,
            this, &Part::slotLoadingStarted);
    connect(m_model, &ArchiveModel::loadingFinished,
            this, &Part::slotLoadingFinished);
    connect(m_model, &ArchiveModel::droppedFiles,
            this, QOverload<const QStringList&, const Archive::Entry*, const QString&>::of(&Part::slotAddFiles));
    connect(m_model, &ArchiveModel::error,
            this, &Part::slotError);
    connect(m_model, &ArchiveModel::messageWidget,
            this, &Part::displayMsgWidget);

    connect(this, &Part::busy,
            this, &Part::setBusyGui);
    connect(this, &Part::ready,
            this, &Part::setReadyGui);
    connect(this, &KParts::ReadOnlyPart::urlChanged,
            this, &Part::setFileNameFromArchive);
    connect(this, QOverload<>::of(&KParts::ReadOnlyPart::completed),
            this, &Part::setFileNameFromArchive);
    connect(this, QOverload<>::of(&KParts::ReadOnlyPart::completed),
            this, &Part::slotCompleted);
    connect(ArkSettings::self(), &KCoreConfigSkeleton::configChanged, this, &Part::updateActions);

    m_statusBarExtension = new KParts::StatusBarExtension(this);

    setXMLFile(QStringLiteral("ark_part.rc"));
}

Part::~Part()
{
    qDeleteAll(m_tmpExtractDirList);

    // Only save splitterSizes if infopanel is visible,
    // because we don't want to store zero size for infopanel.
    if (m_showInfoPanelAction->isChecked()) {
        ArkSettings::setSplitterSizes(m_splitter->sizes());
    }
    ArkSettings::setShowInfoPanel(m_showInfoPanelAction->isChecked());
    ArkSettings::self()->save();

    m_extractArchiveAction->menu()->deleteLater();
    m_extractAction->menu()->deleteLater();
}

void Part::slotCommentChanged()
{
    if (!m_model->archive()) {
        return;
    }

    if (m_commentMsgWidget->isHidden() && m_commentView->toPlainText() != m_model->archive()->comment()) {
        m_commentMsgWidget->animatedShow();
    } else if (m_commentMsgWidget->isVisible() && m_commentView->toPlainText() == m_model->archive()->comment()) {
        m_commentMsgWidget->hide();
    }
}

void Part::registerJob(KJob* job)
{
    if (!m_jobTracker) {
        m_jobTracker = new JobTracker(widget());
        m_statusBarExtension->addStatusBarItem(m_jobTracker->widget(nullptr), 0, true);
        m_jobTracker->widget(job)->show();
    }
    m_jobTracker->registerJob(job);

    emit busy();
    connect(job, &KJob::result, this, &Part::ready);
}

// TODO: KIO::mostLocalHere is used here to resolve some KIO URLs to local
// paths (e.g. desktop:/), but more work is needed to support extraction
// to non-local destinations. See bugs #189322 and #204323.
void Part::extractSelectedFilesTo(const QString& localPath)
{
    if (!m_model) {
        return;
    }

    const QUrl url = QUrl::fromUserInput(localPath, QString());
    KIO::StatJob* statJob = nullptr;

    // Try to resolve the URL to a local path.
    if (!url.isLocalFile() && !url.scheme().isEmpty()) {
        statJob = KIO::mostLocalUrl(url);

        if (!statJob->exec() || statJob->error() != 0) {
            return;
        }
    }

    const QString destination = statJob ? statJob->statResult().stringValue(KIO::UDSEntry::UDS_LOCAL_PATH) : localPath;
    delete statJob;

    // The URL could not be resolved to a local path.
    if (!url.isLocalFile() && destination.isEmpty()) {
        qCWarning(ARK) << "Ark cannot extract to non-local destination:" << localPath;
        KMessageBox::sorry(widget(), xi18nc("@info", "Ark can only extract to local destinations."));
        return;
    }

    qCDebug(ARK) << "Extract to" << destination;

    Kerfuffle::ExtractionOptions options;
    options.setDragAndDropEnabled(true);

    // Create and start the ExtractJob.
    ExtractJob *job = m_model->extractFiles(filesAndRootNodesForIndexes(addChildren(getSelectedIndexes())), destination, options);
    registerJob(job);
    connect(job, &KJob::result,
            this, &Part::slotExtractionDone);
    job->start();
}

void Part::guiActivateEvent(KParts::GUIActivateEvent *event)
{
    // #357660: prevent parent's implementation from changing the window title.
    Q_UNUSED(event)
}

void Part::setupView()
{
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);

    m_filterModel->setSourceModel(m_model);
    m_view->setModel(m_filterModel);
    m_filterModel->setFilterKeyColumn(0);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &Part::updateActions);
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &Part::selectionChanged);

    connect(m_view, &QTreeView::activated, this, &Part::slotActivated);

    connect(m_view, &QWidget::customContextMenuRequested, this, &Part::slotShowContextMenu);
}

void Part::slotActivated(const QModelIndex &index)
{
    Q_UNUSED(index)

    // The activated signal is emitted when items are selected with the mouse,
    // so do nothing if CTRL or SHIFT key is pressed.
    if (QGuiApplication::keyboardModifiers() != Qt::ShiftModifier &&
        QGuiApplication::keyboardModifiers() != Qt::ControlModifier) {
        ArkSettings::defaultOpenAction() == ArkSettings::EnumDefaultOpenAction::Preview ? slotOpenEntry(Preview) : slotOpenEntry(OpenFile);
    }
}

void Part::setupActions()
{
    // We use a QSignalMapper for the preview, open and openwith actions. This
    // way we can connect all three actions to the same slot slotOpenEntry and
    // pass the OpenFileMode as argument to the slot.
    m_signalMapper = new QSignalMapper(this);

    m_showInfoPanelAction = new KToggleAction(i18nc("@action:inmenu", "Show Information Panel"), this);
    actionCollection()->addAction(QStringLiteral( "show-infopanel" ), m_showInfoPanelAction);
    m_showInfoPanelAction->setChecked(ArkSettings::showInfoPanel());
    connect(m_showInfoPanelAction, &QAction::triggered,
            this, &Part::slotToggleInfoPanel);

    m_saveAsAction = actionCollection()->addAction(KStandardAction::SaveAs, QStringLiteral("ark_file_save_as"), this, SLOT(slotSaveAs()));

    m_openFileAction = actionCollection()->addAction(QStringLiteral("openfile"));
    m_openFileAction->setText(i18nc("open a file with external program", "&Open"));
    m_openFileAction->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    m_openFileAction->setToolTip(i18nc("@info:tooltip", "Click to open the selected file with the associated application"));
    connect(m_openFileAction, &QAction::triggered, m_signalMapper, QOverload<>::of(&QSignalMapper::map));
    m_signalMapper->setMapping(m_openFileAction, OpenFile);

    m_openFileWithAction = actionCollection()->addAction(QStringLiteral("openfilewith"));
    m_openFileWithAction->setText(i18nc("open a file with external program", "Open &With..."));
    m_openFileWithAction->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    m_openFileWithAction->setToolTip(i18nc("@info:tooltip", "Click to open the selected file with an external program"));
    connect(m_openFileWithAction, &QAction::triggered, m_signalMapper, QOverload<>::of(&QSignalMapper::map));
    m_signalMapper->setMapping(m_openFileWithAction, OpenFileWith);

    m_previewAction = actionCollection()->addAction(QStringLiteral("preview"));
    m_previewAction->setText(i18nc("to preview a file inside an archive", "Pre&view"));
    m_previewAction->setIcon(QIcon::fromTheme(QStringLiteral("document-preview-archive")));
    m_previewAction->setToolTip(i18nc("@info:tooltip", "Click to preview the selected file"));
    actionCollection()->setDefaultShortcut(m_previewAction, Qt::CTRL + Qt::Key_P);
    connect(m_previewAction, &QAction::triggered, m_signalMapper, QOverload<>::of(&QSignalMapper::map));
    m_signalMapper->setMapping(m_previewAction, Preview);

    m_extractArchiveAction = actionCollection()->addAction(QStringLiteral("extract_all"));
    m_extractArchiveAction->setText(i18nc("@action:inmenu", "E&xtract All"));
    m_extractArchiveAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-extract")));
    m_extractArchiveAction->setToolTip(i18n("Click to open an extraction dialog, where you can choose how to extract all the files in the archive"));
    actionCollection()->setDefaultShortcut(m_extractArchiveAction, Qt::CTRL + Qt::SHIFT + Qt::Key_E);
    connect(m_extractArchiveAction, &QAction::triggered, this, &Part::slotExtractArchive);

    m_extractAction = actionCollection()->addAction(QStringLiteral("extract"));
    m_extractAction->setText(i18nc("@action:inmenu", "&Extract"));
    m_extractAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-extract")));
    actionCollection()->setDefaultShortcut(m_extractAction, Qt::CTRL + Qt::Key_E);
    m_extractAction->setToolTip(i18n("Click to open an extraction dialog, where you can choose to extract either all files or just the selected ones"));
    connect(m_extractAction, &QAction::triggered, this, &Part::slotShowExtractionDialog);

    m_addFilesAction = actionCollection()->addAction(QStringLiteral("add"));
    m_addFilesAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-insert")));
    m_addFilesAction->setText(i18n("Add &Files..."));
    m_addFilesAction->setToolTip(i18nc("@info:tooltip", "Click to add files to the archive"));
    actionCollection()->setDefaultShortcut(m_addFilesAction, Qt::ALT + Qt::Key_A);
    connect(m_addFilesAction, &QAction::triggered, this, QOverload<>::of(&Part::slotAddFiles));

    m_renameFileAction = KStandardAction::renameFile(m_view, &ArchiveView::renameSelectedEntry, actionCollection());

    m_deleteFilesAction = KStandardAction::deleteFile(this, &Part::slotDeleteFiles, actionCollection());
    m_deleteFilesAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-remove")));
    actionCollection()->setDefaultShortcut(m_deleteFilesAction, Qt::Key_Delete);

    m_cutFilesAction = actionCollection()->addAction(QStringLiteral("cut"));
    m_cutFilesAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-cut")));
    m_cutFilesAction->setText(i18nc("@action:inmenu", "C&ut"));
    actionCollection()->setDefaultShortcut(m_cutFilesAction, Qt::CTRL + Qt::Key_X);
    m_cutFilesAction->setToolTip(i18nc("@info:tooltip", "Click to cut the selected files"));
    connect(m_cutFilesAction, &QAction::triggered, this, &Part::slotCutFiles);

    m_copyFilesAction = actionCollection()->addAction(QStringLiteral("copy"));
    m_copyFilesAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    m_copyFilesAction->setText(i18nc("@action:inmenu", "C&opy"));
    actionCollection()->setDefaultShortcut(m_copyFilesAction, Qt::CTRL + Qt::Key_C);
    m_copyFilesAction->setToolTip(i18nc("@info:tooltip", "Click to copy the selected files"));
    connect(m_copyFilesAction, &QAction::triggered, this, &Part::slotCopyFiles);

    m_pasteFilesAction = actionCollection()->addAction(QStringLiteral("paste"));
    m_pasteFilesAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-paste")));
    m_pasteFilesAction->setText(i18nc("@action:inmenu", "Pa&ste"));
    actionCollection()->setDefaultShortcut(m_pasteFilesAction, Qt::CTRL + Qt::Key_V);
    m_pasteFilesAction->setToolTip(i18nc("@info:tooltip", "Click to paste the files here"));
    connect(m_pasteFilesAction, &QAction::triggered, this, QOverload<>::of(&Part::slotPasteFiles));

    m_propertiesAction = actionCollection()->addAction(QStringLiteral("properties"));
    m_propertiesAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    m_propertiesAction->setText(i18nc("@action:inmenu", "&Properties"));
    actionCollection()->setDefaultShortcut(m_propertiesAction, Qt::ALT + Qt::Key_Return);
    m_propertiesAction->setToolTip(i18nc("@info:tooltip", "Click to see properties for archive"));
    connect(m_propertiesAction, &QAction::triggered, this, &Part::slotShowProperties);

    m_editCommentAction = actionCollection()->addAction(QStringLiteral("edit_comment"));
    m_editCommentAction->setIcon(QIcon::fromTheme(QStringLiteral("document-edit")));
    actionCollection()->setDefaultShortcut(m_editCommentAction, Qt::ALT + Qt::Key_C);
    m_editCommentAction->setToolTip(i18nc("@info:tooltip", "Click to add or edit comment"));
    connect(m_editCommentAction, &QAction::triggered, this, &Part::slotShowComment);

    m_testArchiveAction = actionCollection()->addAction(QStringLiteral("test_archive"));
    m_testArchiveAction->setIcon(QIcon::fromTheme(QStringLiteral("checkmark")));
    m_testArchiveAction->setText(i18nc("@action:inmenu", "&Test Integrity"));
    actionCollection()->setDefaultShortcut(m_testArchiveAction, Qt::ALT + Qt::Key_T);
    m_testArchiveAction->setToolTip(i18nc("@info:tooltip", "Click to test the archive for integrity"));
    connect(m_testArchiveAction, &QAction::triggered, this, &Part::slotTestArchive);

    m_searchAction = actionCollection()->addAction(QStringLiteral("find_in_archive"));
    m_searchAction->setIcon(QIcon::fromTheme(QStringLiteral("search")));
    m_searchAction->setText(i18nc("@action:inmenu", "&Find Files"));
    actionCollection()->setDefaultShortcut(m_searchAction, Qt::CTRL + Qt::Key_F);
    m_searchAction->setToolTip(i18nc("@info:tooltip", "Click to search in archive"));
    connect(m_searchAction, &QAction::triggered, this, &Part::slotShowFind);

    connect(m_signalMapper, QOverload<int>::of(&QSignalMapper::mapped),
            this, &Part::slotOpenEntry);

    updateActions();
    updateQuickExtractMenu(m_extractArchiveAction);
    updateQuickExtractMenu(m_extractAction);
}

void Part::updateActions()
{
    const bool isWritable = isArchiveWritable();
    const Archive::Entry *entry = m_model->entryForIndex(m_filterModel->mapToSource(m_view->selectionModel()->currentIndex()));
    int selectedEntriesCount = m_view->selectionModel()->selectedRows().count();

    // We disable adding files if the archive is encrypted but the password is
    // unknown (this happens when opening existing non-he password-protected
    // archives). If we added files they would not get encrypted resulting in an
    // archive with a mixture of encrypted and unencrypted files.
    const bool isEncryptedButUnknownPassword = m_model->archive() &&
                                               m_model->archive()->encryptionType() != Archive::Unencrypted &&
                                               m_model->archive()->password().isEmpty();

    if (isEncryptedButUnknownPassword) {
        m_addFilesAction->setToolTip(xi18nc("@info:tooltip",
                                            "Adding files to existing password-protected archives with no header-encryption is currently not supported."
                                            "<nl/><nl/>Extract the files and create a new archive if you want to add files."));
        m_testArchiveAction->setToolTip(xi18nc("@info:tooltip",
                                               "Testing password-protected archives with no header-encryption is currently not supported."));
    } else {
        m_addFilesAction->setToolTip(i18nc("@info:tooltip", "Click to add files to the archive"));
        m_testArchiveAction->setToolTip(i18nc("@info:tooltip", "Click to test the archive for integrity"));
    }

    // Figure out if entry size is larger than preview size limit.
    const int maxPreviewSize = ArkSettings::previewFileSizeLimit() * 1024 * 1024;
    const bool limit = ArkSettings::limitPreviewFileSize();
    bool isPreviewable = (!limit || (limit && entry != nullptr && entry->property("size").toLongLong() < maxPreviewSize));

    const bool isDir = (entry == nullptr) ? false : entry->isDir();
    m_previewAction->setEnabled(!isBusy() &&
                                isPreviewable &&
                                !isDir &&
                                (selectedEntriesCount == 1));
    m_extractArchiveAction->setEnabled(!isBusy() &&
                                       (m_model->rowCount() > 0));
    m_extractAction->setEnabled(!isBusy() &&
                                (m_model->rowCount() > 0));
    m_saveAsAction->setEnabled(!isBusy() &&
                               m_model->rowCount() > 0);
    m_addFilesAction->setEnabled(!isBusy() &&
                                 isWritable &&
                                 !isEncryptedButUnknownPassword);
    m_deleteFilesAction->setEnabled(!isBusy() &&
                                    isWritable &&
                                    (selectedEntriesCount > 0));
    m_openFileAction->setEnabled(!isBusy() &&
                                 isPreviewable &&
                                 !isDir &&
                                 (selectedEntriesCount == 1));
    m_openFileWithAction->setEnabled(!isBusy() &&
                                     isPreviewable &&
                                     !isDir &&
                                     (selectedEntriesCount == 1));
    m_propertiesAction->setEnabled(!isBusy() &&
                                   m_model->archive());

    m_renameFileAction->setEnabled(!isBusy() &&
                                   isWritable &&
                                   (selectedEntriesCount == 1));
    m_cutFilesAction->setEnabled(!isBusy() &&
                                 isWritable &&
                                 (selectedEntriesCount > 0));
    m_copyFilesAction->setEnabled(!isBusy() &&
                                  isWritable &&
                                  (selectedEntriesCount > 0));
    m_pasteFilesAction->setEnabled(!isBusy() &&
                                   isWritable &&
                                   (selectedEntriesCount == 0 || (selectedEntriesCount == 1 && isDir)) &&
                                   (m_model->filesToMove.count() > 0 || m_model->filesToCopy.count() > 0));

    m_searchAction->setEnabled(!isBusy() &&
                               m_model->rowCount() > 0);

    m_commentView->setEnabled(!isBusy());
    m_commentMsgWidget->setEnabled(!isBusy());

    m_editCommentAction->setEnabled(false);
    m_testArchiveAction->setEnabled(false);

    if (m_model->archive()) {
        const KPluginMetaData metadata = PluginManager().preferredPluginFor(m_model->archive()->mimeType())->metaData();
        bool supportsWriteComment = ArchiveFormat::fromMetadata(m_model->archive()->mimeType(), metadata).supportsWriteComment();
        m_editCommentAction->setEnabled(!isBusy() &&
                                        supportsWriteComment);
        m_commentView->setReadOnly(!supportsWriteComment);
        m_editCommentAction->setText(m_model->archive()->hasComment() ? i18nc("@action:inmenu mutually exclusive with Add &Comment", "Edit &Comment") :
                                                                        i18nc("@action:inmenu mutually exclusive with Edit &Comment", "Add &Comment"));

        bool supportsTesting = ArchiveFormat::fromMetadata(m_model->archive()->mimeType(), metadata).supportsTesting();
        m_testArchiveAction->setEnabled(!isBusy() &&
                                        supportsTesting &&
                                        !isEncryptedButUnknownPassword);
    } else {
        m_commentView->setReadOnly(true);
        m_editCommentAction->setText(i18nc("@action:inmenu mutually exclusive with Edit &Comment", "Add &Comment"));
    }
}

void Part::slotShowComment()
{
    if (!m_commentBox->isVisible()) {
        m_commentBox->show();
        m_commentSplitter->setSizes(QList<int>() << static_cast<int>(m_view->height() * 0.6) << 1);
    }
    m_commentView->setFocus();
}

void Part::slotAddComment()
{
    CommentJob *job = m_model->archive()->addComment(m_commentView->toPlainText());
    if (!job) {
        return;
    }
    registerJob(job);
    job->start();
    m_commentMsgWidget->hide();
    if (m_commentView->toPlainText().isEmpty()) {
        m_commentBox->hide();
    }
}

void Part::slotTestArchive()
{
    TestJob *job = m_model->archive()->testArchive();
    if (!job) {
        return;
    }
    registerJob(job);
    connect(job, &KJob::result, this, &Part::slotTestingDone);
    job->start();
}

bool Part::isArchiveWritable() const
{
    return isReadWrite() && m_model->archive() && !m_model->archive()->isReadOnly();
}

bool Part::isCreatingNewArchive() const
{
    return arguments().metaData()[QStringLiteral("createNewArchive")] == QLatin1String("true");
}

void Part::createArchive()
{
    const QString fixedMimeType = arguments().metaData()[QStringLiteral("fixedMimeType")];
    m_model->createEmptyArchive(localFilePath(), fixedMimeType, m_model);

    if (arguments().metaData().contains(QStringLiteral("volumeSize"))) {
        m_model->archive()->setMultiVolume(true);
    }

    const QString password = arguments().metaData()[QStringLiteral("encryptionPassword")];
    if (!password.isEmpty()) {
        m_model->encryptArchive(password,
                                arguments().metaData()[QStringLiteral("encryptHeader")] == QLatin1String("true"));
    }
}

void Part::loadArchive()
{
    const QString fixedMimeType = arguments().metaData()[QStringLiteral("fixedMimeType")];
    auto job = m_model->loadArchive(localFilePath(), fixedMimeType, m_model);

    if (job) {
        registerJob(job);
        job->start();
    } else {
        updateActions();
    }
}

void Part::resetGui()
{
    m_messageWidget->hide();
    m_commentView->clear();
    m_commentBox->hide();
    m_infoPanel->setIndex(QModelIndex());
    // Also reset format-specific compression options.
    m_compressionOptions = CompressionOptions();
}

void Part::slotTestingDone(KJob* job)
{
    if (job->error() && job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    } else if (static_cast<TestJob*>(job)->testSucceeded()) {
        KMessageBox::information(widget(), i18n("The archive passed the integrity test."), i18n("Test Results"));
    } else {
        KMessageBox::error(widget(), i18n("The archive failed the integrity test."), i18n("Test Results"));
    }
}

void Part::updateQuickExtractMenu(QAction *extractAction)
{
    if (!extractAction) {
        return;
    }

    QMenu *menu = extractAction->menu();

    if (!menu) {
        menu = new QMenu();
        extractAction->setMenu(menu);
        connect(menu, &QMenu::triggered,
                this, &Part::slotQuickExtractFiles);

        // Remember to keep this action's properties as similar to
        // extractAction's as possible (except where it does not make
        // sense, such as the text or the shortcut).
        QAction *extractTo = menu->addAction(i18n("Extract To..."));
        extractTo->setIcon(extractAction->icon());
        extractTo->setToolTip(extractAction->toolTip());

        if (extractAction == m_extractArchiveAction) {
            connect(extractTo, &QAction::triggered,
                    this, &Part::slotExtractArchive);
        } else {
            connect(extractTo, &QAction::triggered,
                    this, &Part::slotShowExtractionDialog);
        }

        menu->addSeparator();

        QAction *header = menu->addAction(i18n("Quick Extract To..."));
        header->setEnabled(false);
        header->setIcon(QIcon::fromTheme(QStringLiteral("archive-extract")));
    }

    while (menu->actions().size() > 3) {
        menu->removeAction(menu->actions().constLast());
    }

    const KConfigGroup conf(KSharedConfig::openConfig(), "ExtractDialog");
    const QStringList dirHistory = conf.readPathEntry("DirHistory", QStringList());

    for (int i = 0; i < qMin(10, dirHistory.size()); ++i) {
        const QString dir = QUrl(dirHistory.value(i)).toString(QUrl::RemoveScheme | QUrl::NormalizePathSegments | QUrl::PreferLocalFile);
        if (QDir(dir).exists()) {
            QAction *newAction = menu->addAction(dir);
            newAction->setData(dir);
        }
    }
}

void Part::slotQuickExtractFiles(QAction *triggeredAction)
{
    // #190507: triggeredAction->data.isNull() means it's the "Extract to..."
    //          action, and we do not want it to run here
    if (!triggeredAction->data().isNull()) {
        QString userDestination = triggeredAction->data().toString();
        QString finalDestinationDirectory;
        const QString detectedSubfolder = detectSubfolder();
        qCDebug(ARK) << "Detected subfolder" << detectedSubfolder;

        if (m_model->archive()->hasMultipleTopLevelEntries()) {
            if (!userDestination.endsWith(QDir::separator())) {
                userDestination.append(QDir::separator());
            }
            finalDestinationDirectory = userDestination + detectedSubfolder;
            QDir(userDestination).mkdir(detectedSubfolder);
        } else {
            finalDestinationDirectory = userDestination;
        }

        qCDebug(ARK) << "Extracting to:" << finalDestinationDirectory;

        ExtractJob *job = m_model->extractFiles(filesAndRootNodesForIndexes(addChildren(getSelectedIndexes())), finalDestinationDirectory, ExtractionOptions());
        registerJob(job);

        connect(job, &KJob::result,
                this, &Part::slotExtractionDone);

        job->start();
    }
}

void Part::selectionChanged()
{
    m_infoPanel->setIndexes(getSelectedIndexes());
}

QModelIndexList Part::getSelectedIndexes()
{
    QModelIndexList list;
    foreach (const QModelIndex &i, m_view->selectionModel()->selectedRows()) {
        list.append(m_filterModel->mapToSource(i));
    }
    return list;
}

bool Part::openFile()
{
    qCDebug(ARK) << "Attempting to open archive" << localFilePath();

    resetGui();

    if (!isLocalFileValid()) {
        return false;
    }

    if (isCreatingNewArchive()) {
        createArchive();
        return true;
    }

    loadArchive();
    // Loading is async, we don't know yet whether we got a valid archive.
    return false;
}

bool Part::saveFile()
{
    return true;
}

bool Part::isBusy() const
{
    return m_busy;
}

KConfigSkeleton *Part::config() const
{
    return ArkSettings::self();
}

QList<Kerfuffle::SettingsPage*> Part::settingsPages(QWidget *parent) const
{
    QList<SettingsPage*> pages;
    pages.append(new GeneralSettingsPage(parent, i18nc("@title:tab", "General Settings"), QStringLiteral("go-home")));
    pages.append(new ExtractionSettingsPage(parent, i18nc("@title:tab", "Extraction Settings"), QStringLiteral("archive-extract")));
    pages.append(new PluginSettingsPage(parent, i18nc("@title:tab", "Plugin Settings"), QStringLiteral("plugins")));
    pages.append(new PreviewSettingsPage(parent, i18nc("@title:tab", "Preview Settings"), QStringLiteral("document-preview-archive")));

    return pages;
}

bool Part::isLocalFileValid()
{
    const QString localFile = localFilePath();
    const QFileInfo localFileInfo(localFile);

    if (localFileInfo.isDir()) {
        displayMsgWidget(KMessageWidget::Error, xi18nc("@info",
                                                       "<filename>%1</filename> is a directory.",
                                                       localFile));
        return false;
    }

    if (isCreatingNewArchive()) {
        if (localFileInfo.exists()) {
            if (!confirmAndDelete(localFile)) {
                displayMsgWidget(KMessageWidget::Error, xi18nc("@info",
                                                               "Could not overwrite <filename>%1</filename>. Check whether you have write permission.",
                                                               localFile));
                return false;
            }
        }

        displayMsgWidget(KMessageWidget::Information, xi18nc("@info", "The archive <filename>%1</filename> will be created as soon as you add a file.", localFile));
    } else {
        if (!localFileInfo.exists()) {
            displayMsgWidget(KMessageWidget::Error, xi18nc("@info", "The archive <filename>%1</filename> was not found.", localFile));
            return false;
        }

        if (!localFileInfo.isReadable()) {
            displayMsgWidget(KMessageWidget::Error, xi18nc("@info", "The archive <filename>%1</filename> could not be loaded, as it was not possible to read from it.", localFile));
            return false;
        }
    }

    return true;
}

bool Part::confirmAndDelete(const QString &targetFile)
{
    QFileInfo targetInfo(targetFile);
    const auto buttonCode = KMessageBox::warningYesNo(widget(),
                                                      xi18nc("@info",
                                                             "The archive <filename>%1</filename> already exists. Do you wish to overwrite it?",
                                                             targetInfo.fileName()),
                                                      i18nc("@title:window", "File Exists"),
                                                      KStandardGuiItem::overwrite(),
                                                      KStandardGuiItem::cancel());

    if (buttonCode != KMessageBox::Yes || !targetInfo.isWritable()) {
        return false;
    }

    qCDebug(ARK) << "Removing file" << targetFile;

    return QFile(targetFile).remove();
}

void Part::slotCompleted()
{
    if (isCreatingNewArchive()) {
        m_view->setDropsEnabled(true);
        updateActions();
        return;
    }

    // Existing archive, setup the view for it.
    m_view->sortByColumn(0, Qt::AscendingOrder);
    m_view->expandIfSingleFolder();
    m_view->header()->resizeSections(QHeaderView::ResizeToContents);
    m_view->setDropsEnabled(isArchiveWritable());

    if (!m_model->archive()->comment().isEmpty()) {
        m_commentView->setPlainText(m_model->archive()->comment());
        slotShowComment();
    } else {
        m_commentView->clear();
        m_commentBox->hide();
    }

    if (m_model->rowCount() == 0) {
        qCWarning(ARK) << "No entry listed by the plugin";
        displayMsgWidget(KMessageWidget::Warning, xi18nc("@info", "The archive is empty or Ark could not open its content."));
    } else if (m_model->rowCount() == 1) {
        if (m_model->archive()->mimeType().inherits(QStringLiteral("application/x-cd-image")) &&
            m_model->entryForIndex(m_model->index(0, 0))->fullPath() == QLatin1String("README.TXT")) {
            qCWarning(ARK) << "Detected ISO image with UDF filesystem";
            displayMsgWidget(KMessageWidget::Warning, xi18nc("@info", "Ark does not currently support ISO files with UDF filesystem."));
        }
    }

    if (arguments().metaData()[QStringLiteral("showExtractDialog")] == QLatin1String("true")) {
        QTimer::singleShot(0, this, &Part::slotShowExtractionDialog);
    }

    updateActions();
}

void Part::slotLoadingStarted()
{
    m_model->filesToMove.clear();
    m_model->filesToCopy.clear();
}

void Part::slotLoadingFinished(KJob *job)
{
    if (!job->error()) {
        emit completed();
        return;
    }

    // Loading failed or was canceled by the user (e.g. password dialog rejected).
    emit canceled(job->errorString());
    m_view->setDropsEnabled(false);
    m_model->reset();
    closeUrl();
    setFileNameFromArchive();
    updateActions();
    if (job->error() != KJob::KilledJobError) {
        displayMsgWidget(KMessageWidget::Error, xi18nc("@info", "Loading the archive <filename>%1</filename> failed with the following error:<nl/><message>%2</message>",
                                                       localFilePath(),
                                                       job->errorString()));
    }
}

void Part::setReadyGui()
{
    QApplication::restoreOverrideCursor();
    m_busy = false;

    if (m_statusBarExtension->statusBar()) {
        m_statusBarExtension->statusBar()->hide();
    }

    m_view->setEnabled(true);
    updateActions();
}

void Part::setBusyGui()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_busy = true;

    if (m_statusBarExtension->statusBar()) {
        m_statusBarExtension->statusBar()->show();
    }

    m_view->setEnabled(false);
    updateActions();
}

void Part::setFileNameFromArchive()
{
    const QString prettyName = url().fileName();

    m_infoPanel->setPrettyFileName(prettyName);
    m_infoPanel->updateWithDefaults();

    emit setWindowCaption(prettyName);
}

void Part::slotOpenEntry(int mode)
{
    QModelIndex index = m_filterModel->mapToSource(m_view->selectionModel()->currentIndex());
    Archive::Entry *entry = m_model->entryForIndex(index);

    // Don't open directories.
    if (entry->isDir()) {
        return;
    }

    // Don't open files bigger than the size limit.
    const int maxPreviewSize = ArkSettings::previewFileSizeLimit() * 1024 * 1024;
    if (ArkSettings::limitPreviewFileSize() && entry->property("size").toLongLong() >= maxPreviewSize) {
        return;
    }

    // We don't support opening symlinks.
    if (!entry->property("link").toString().isEmpty()) {
        displayMsgWidget(KMessageWidget::Information, i18n("Ark cannot open symlinks."));
        return;
    }

    // Extract the entry.
    if (!entry->fullPath().isEmpty()) {
        qCDebug(ARK) << "Opening with mode" << mode;
        m_openFileMode = static_cast<OpenFileMode>(mode);
        KJob *job = nullptr;

        if (m_openFileMode == Preview) {
            job = m_model->preview(entry);
            connect(job, &KJob::result, this, &Part::slotPreviewExtractedEntry);
        } else {
            job = (m_openFileMode == OpenFile) ? m_model->open(entry) : m_model->openWith(entry);
            connect(job, &KJob::result, this, &Part::slotOpenExtractedEntry);
        }

        registerJob(job);
        job->start();
    }
}

void Part::slotOpenExtractedEntry(KJob *job)
{
    if (!job->error()) {

        OpenJob *openJob = qobject_cast<OpenJob*>(job);
        Q_ASSERT(openJob);

        // Since the user could modify the file (unlike the Preview case),
        // we'll need to manually delete the temp dir in the Part destructor.
        m_tmpExtractDirList << openJob->tempDir();

        const QString fullName = openJob->validatedFilePath();
        if (isArchiveWritable()) {
            m_fileWatcher = new QFileSystemWatcher;
            connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &Part::slotWatchedFileModified);
            m_fileWatcher->addPath(fullName);
        } else {
            // If archive is readonly set temporarily extracted file to readonly as
            // well so user will be notified if trying to modify and save the file.
            QFile::setPermissions(fullName, QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
        }

        if (qobject_cast<OpenWithJob*>(job)) {
            const QList<QUrl> urls = {QUrl::fromUserInput(fullName, QString(), QUrl::AssumeLocalFile)};
            KRun::displayOpenWithDialog(urls, widget());
        } else {
            KRun::runUrl(QUrl::fromUserInput(fullName, QString(), QUrl::AssumeLocalFile),
                         QMimeDatabase().mimeTypeForFile(fullName).name(),
                         widget(), KRun::RunFlags());
        }
    } else if (job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    }
    setReadyGui();
}

void Part::slotPreviewExtractedEntry(KJob *job)
{
    if (!job->error()) {
        PreviewJob *previewJob = qobject_cast<PreviewJob*>(job);
        Q_ASSERT(previewJob);

        m_tmpExtractDirList << previewJob->tempDir();
        ArkViewer::view(previewJob->validatedFilePath());

    } else if (job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    }
    setReadyGui();
}

void Part::slotWatchedFileModified(const QString& file)
{
    qCDebug(ARK) << "Watched file modified:" << file;

    // Find the relative path of the file within the archive.
    QString relPath = file;
    foreach (QTemporaryDir *tmpDir, m_tmpExtractDirList) {
        relPath.remove(tmpDir->path()); //Remove tmpDir.
    }
    relPath = relPath.mid(1); //Remove leading slash.
    if (relPath.contains(QLatin1Char('/'))) {
        relPath = relPath.section(QLatin1Char('/'), 0, -2); //Remove filename.
    } else {
        // File is in the root of the archive, no path.
        relPath = QString();
    }

    // Set up a string for display in KMessageBox.
    QString prettyFilename;
    if (relPath.isEmpty()) {
        prettyFilename = file.section(QLatin1Char('/'), -1);
    } else {
        prettyFilename = relPath + QLatin1Char('/') + file.section(QLatin1Char('/'), -1);
    }

    if (KMessageBox::questionYesNo(widget(),
                               xi18n("The file <filename>%1</filename> was modified. Do you want to update the archive?",
                                     prettyFilename),
                               i18nc("@title:window", "File Modified")) == KMessageBox::Yes) {
        QStringList list = QStringList() << file;

        qCDebug(ARK) << "Updating file" << file << "with path" << relPath;
        slotAddFiles(list, nullptr, relPath);
    }
    // This is needed because some apps, such as Kate, delete and recreate
    // files when saving.
    m_fileWatcher->addPath(file);
}

void Part::slotError(const QString& errorMessage, const QString& details)
{
    if (details.isEmpty()) {
        KMessageBox::error(widget(), errorMessage);
    } else {
        KMessageBox::detailedError(widget(), errorMessage, details);
    }
}

QString Part::detectSubfolder() const
{
    if (!m_model) {
        return QString();
    }

    return m_model->archive()->subfolderName();
}

void Part::slotExtractArchive()
{
    if (m_view->selectionModel()->selectedRows().count() > 0) {
        m_view->selectionModel()->clear();
    }

    slotShowExtractionDialog();
}

void Part::slotShowExtractionDialog()
{
    if (!m_model) {
        return;
    }

    QPointer<Kerfuffle::ExtractionDialog> dialog(new Kerfuffle::ExtractionDialog);

    dialog.data()->setModal(true);

    if (m_view->selectionModel()->selectedRows().count() > 0) {
        dialog.data()->setShowSelectedFiles(true);
    }

    dialog.data()->setExtractToSubfolder(m_model->archive()->hasMultipleTopLevelEntries());
    dialog.data()->setSubfolder(detectSubfolder());

    dialog.data()->setCurrentUrl(QUrl::fromLocalFile(QFileInfo(m_model->archive()->fileName()).absolutePath()));

    dialog.data()->show();
    dialog.data()->restoreWindowSize();

    if (dialog.data()->exec()) {

        updateQuickExtractMenu(m_extractArchiveAction);
        updateQuickExtractMenu(m_extractAction);

        QVector<Archive::Entry*> files;

        // If the user has chosen to extract only selected entries, fetch these
        // from the QTreeView.
        if (!dialog.data()->extractAllFiles()) {
            files = filesAndRootNodesForIndexes(addChildren(getSelectedIndexes()));
        }

        qCDebug(ARK) << "Selected " << files;

        Kerfuffle::ExtractionOptions options;
        options.setPreservePaths(dialog->preservePaths());

        const QString destinationDirectory = dialog.data()->destinationDirectory().toLocalFile();
        ExtractJob *job = m_model->extractFiles(files, destinationDirectory, options);
        registerJob(job);

        connect(job, &KJob::result,
                this, &Part::slotExtractionDone);

        job->start();
    }

    delete dialog.data();
}

QModelIndexList Part::addChildren(const QModelIndexList &list) const
{
    Q_ASSERT(m_model);

    QModelIndexList ret = list;

    // Iterate over indexes in list and add all children.
    for (int i = 0; i < ret.size(); ++i) {
        QModelIndex index = ret.at(i);

        for (int j = 0; j < m_model->rowCount(index); ++j) {
            QModelIndex child = m_model->index(j, 0, index);
            if (!ret.contains(child)) {
                ret << child;
            }
        }
    }

    return ret;
}

QVector<Archive::Entry*> Part::filesForIndexes(const QModelIndexList& list) const
{
    QVector<Archive::Entry*> ret;

    foreach(const QModelIndex& index, list) {
        ret << m_model->entryForIndex(index);
    }

    return ret;
}

QVector<Kerfuffle::Archive::Entry*> Part::filesAndRootNodesForIndexes(const QModelIndexList& list) const
{
    QVector<Kerfuffle::Archive::Entry*> fileList;
    QStringList fullPathsList;

    foreach (const QModelIndex& index, list) {

        // Find the topmost unselected parent. This is done by iterating up
        // through the directory hierarchy and see if each parent is included
        // in the selection OR if the parent is already part of list.
        // The latter is needed for unselected folders which are subfolders of
        // a selected parent folder.
        QModelIndex selectionRoot = index.parent();
        while (m_view->selectionModel()->isSelected(selectionRoot) ||
               list.contains(selectionRoot)) {
            selectionRoot = selectionRoot.parent();
        }

        // Fetch the root node for the unselected parent.
        const QString rootFileName = selectionRoot.isValid()
            ? m_model->entryForIndex(selectionRoot)->fullPath()
            : QString();


        // Append index with root node to fileList.
        QModelIndexList alist = QModelIndexList() << index;
        foreach (Archive::Entry *entry, filesForIndexes(alist)) {
            const QString fullPath = entry->fullPath();
            if (!fullPathsList.contains(fullPath)) {
                entry->rootNode = rootFileName;
                fileList.append(entry);
                fullPathsList.append(fullPath);
            }
        }
    }
    return fileList;
}

void Part::slotExtractionDone(KJob* job)
{
    if (job->error() && job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    } else {
        ExtractJob *extractJob = qobject_cast<ExtractJob*>(job);
        Q_ASSERT(extractJob);

        if (ArkSettings::openDestinationFolderAfterExtraction()) {
            qCDebug(ARK) << "Shall open" << extractJob->destinationDirectory();
            QUrl destinationDirectory = QUrl::fromLocalFile(extractJob->destinationDirectory()).adjusted(QUrl::NormalizePathSegments);
            qCDebug(ARK) << "Shall open URL" << destinationDirectory;

            KRun::runUrl(destinationDirectory, QStringLiteral("inode/directory"), widget(), KRun::RunExecutables, QString(), QByteArray());
        }

        if (ArkSettings::closeAfterExtraction()) {
           emit quit();
        }
    }
}

void Part::slotAddFiles(const QStringList& filesToAdd, const Archive::Entry *destination, const QString &relPath)
{
    if (!m_model->archive() || filesToAdd.isEmpty()) {
        return;
    }

    QStringList withChildPaths;
    foreach (const QString& file, filesToAdd) {
        m_jobTempEntries.push_back(new Archive::Entry(nullptr, file));
        if (QFileInfo(file).isDir()) {
            withChildPaths << file + QLatin1Char('/');
            QDirIterator it(file, QDir::AllEntries | QDir::Readable | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString path = it.next();
                if (it.fileInfo().isDir()) {
                    path += QLatin1Char('/');
                }
                withChildPaths << path;
            }
        } else {
            withChildPaths << file;
        }
    }

    withChildPaths = ReadOnlyArchiveInterface::entryPathsFromDestination(withChildPaths, destination, 0);
    QList<const Archive::Entry*> conflictingEntries;
    bool error = m_model->conflictingEntries(conflictingEntries, withChildPaths, true);

    if (conflictingEntries.count() > 0) {
        QPointer<OverwriteDialog> overwriteDialog = new OverwriteDialog(widget(), conflictingEntries, m_model->entryIcons(), error);
        int ret = overwriteDialog->exec();
        delete overwriteDialog;
        if (ret == QDialog::Rejected) {
            qDeleteAll(m_jobTempEntries);
            m_jobTempEntries.clear();
            return;
        }
    }

    // GlobalWorkDir is used by AddJob and should contain the part of the
    // absolute path of files to be added that should NOT be included in the
    // directory structure within the archive.
    // Example: We add file "/home/user/somedir/somefile.txt" and want the file
    // to have the relative path within the archive "somedir/somefile.txt".
    // GlobalWorkDir is then: "/home/user"
    QString globalWorkDir = filesToAdd.first();

    // path represents the path of the file within the archive. This needs to
    // be removed from globalWorkDir, otherwise the files will be added to the
    // root of the archive. In the example above, path would be "somedir/".
    if (!relPath.isEmpty()) {
        globalWorkDir.remove(relPath);
        qCDebug(ARK) << "Adding" << filesToAdd << "to" << relPath;
    } else {
        qCDebug(ARK) << "Adding " << filesToAdd << ((destination == nullptr) ? QString() : QStringLiteral("to ") + destination->fullPath());
    }

    // Remove trailing slash (needed when adding dirs).
    if (globalWorkDir.right(1) == QLatin1String("/")) {
        globalWorkDir.chop(1);
    }

    // We need to override the global options with a working directory.
    CompressionOptions compOptions = m_compressionOptions;

    // Now take the absolute path of the parent directory.
    globalWorkDir = QFileInfo(globalWorkDir).dir().absolutePath();

    qCDebug(ARK) << "Detected GlobalWorkDir to be " << globalWorkDir;
    compOptions.setGlobalWorkDir(globalWorkDir);

    AddJob *job = m_model->addFiles(m_jobTempEntries, destination, compOptions);
    if (!job) {
        qDeleteAll(m_jobTempEntries);
        m_jobTempEntries.clear();
        return;
    }

    connect(job, &KJob::result,
            this, &Part::slotAddFilesDone);
    registerJob(job);
    job->start();
}

void Part::slotAddFiles()
{
    // Store options from CreateDialog if they are set.
    if (!m_compressionOptions.isCompressionLevelSet() && arguments().metaData().contains(QStringLiteral("compressionLevel"))) {
        m_compressionOptions.setCompressionLevel(arguments().metaData()[QStringLiteral("compressionLevel")].toInt());
    }
    if (m_compressionOptions.compressionMethod().isEmpty() && arguments().metaData().contains(QStringLiteral("compressionMethod"))) {
        m_compressionOptions.setCompressionMethod(arguments().metaData()[QStringLiteral("compressionMethod")]);
    }
    if (m_compressionOptions.encryptionMethod().isEmpty() && arguments().metaData().contains(QStringLiteral("encryptionMethod"))) {
        m_compressionOptions.setEncryptionMethod(arguments().metaData()[QStringLiteral("encryptionMethod")]);
    }
    if (!m_compressionOptions.isVolumeSizeSet() && arguments().metaData().contains(QStringLiteral("volumeSize"))) {
        m_compressionOptions.setVolumeSize(arguments().metaData()[QStringLiteral("volumeSize")].toULong());
    }

    const auto compressionMethods = m_model->archive()->property("compressionMethods").toStringList();
    qCDebug(ARK) << "compmethods:" << compressionMethods;
    if (compressionMethods.size() == 1) {
        m_compressionOptions.setCompressionMethod(compressionMethods.first());
    }

    QString dialogTitle = i18nc("@title:window", "Add Files");
    const Archive::Entry *destination = nullptr;
    if (m_view->selectionModel()->selectedRows().count() == 1) {
        destination = m_model->entryForIndex(m_filterModel->mapToSource(m_view->selectionModel()->currentIndex()));
        if (destination->isDir()) {
            dialogTitle = i18nc("@title:window", "Add Files to %1", destination->fullPath());;
        } else {
            destination = nullptr;
        }
    }

    qCDebug(ARK) << "Opening AddDialog with opts:" << m_compressionOptions;

    // #264819: passing widget() as the parent will not work as expected.
    //          KFileDialog will create a KFileWidget, which runs an internal
    //          event loop to stat the given directory. This, in turn, leads to
    //          events being delivered to widget(), which is a QSplitter, which
    //          in turn reimplements childEvent() and will end up calling
    //          QWidget::show() on the KFileDialog (thus showing it in a
    //          non-modal state).
    //          When KFileDialog::exec() is called, the widget is already shown
    //          and nothing happens.

    QPointer<AddDialog> dlg = new AddDialog(widget(),
                                            dialogTitle,
                                            m_lastUsedAddPath,
                                            m_model->archive()->mimeType(),
                                            m_compressionOptions);

    if (dlg->exec() == QDialog::Accepted) {
        qCDebug(ARK) << "Selected files:" << dlg->selectedFiles();
        qCDebug(ARK) << "Options:" << dlg->compressionOptions();
        m_compressionOptions = dlg->compressionOptions();
        slotAddFiles(dlg->selectedFiles(), destination, QString());
    }
    delete dlg;
}

void Part::slotCutFiles()
{
    QModelIndexList selectedRows = addChildren(getSelectedIndexes());
    m_model->filesToMove = ArchiveModel::entryMap(filesForIndexes(selectedRows));
    qCDebug(ARK) << "Entries marked to cut:" << m_model->filesToMove.values();
    m_model->filesToCopy.clear();
    foreach (const QModelIndex &row, m_cutIndexes) {
        m_view->dataChanged(row, row);
    }
    m_cutIndexes = selectedRows;
    foreach (const QModelIndex &row, m_cutIndexes) {
        m_view->dataChanged(row, row);
    }
    updateActions();
}

void Part::slotCopyFiles()
{
    m_model->filesToCopy = ArchiveModel::entryMap(filesForIndexes(addChildren(getSelectedIndexes())));
    qCDebug(ARK) << "Entries marked to copy:" << m_model->filesToCopy.values();
    foreach (const QModelIndex &row, m_cutIndexes) {
        m_view->dataChanged(row, row);
    }
    m_cutIndexes.clear();
    m_model->filesToMove.clear();
    updateActions();
}

void Part::slotRenameFile(const QString &name)
{
    if (name == QLatin1String(".") || name == QLatin1String("..") || name.contains(QLatin1Char('/'))) {
        displayMsgWidget(KMessageWidget::Error, i18n("Filename can't contain slashes and can't be equal to \".\" or \"..\""));
        return;
    }
    const Archive::Entry *entry = m_model->entryForIndex(m_filterModel->mapToSource(m_view->selectionModel()->currentIndex()));
    QVector<Archive::Entry*> entriesToMove = filesForIndexes(addChildren(getSelectedIndexes()));

    m_destination = new Archive::Entry();
    const QString &entryPath = entry->fullPath(NoTrailingSlash);
    const QString rootPath = entryPath.left(entryPath.count() - entry->name().count());
    auto path = rootPath + name;
    if (entry->isDir()) {
        path += QLatin1Char('/');
    }
    m_destination->setFullPath(path);

    slotPasteFiles(entriesToMove, m_destination, 1);
}

void Part::slotPasteFiles()
{
    m_destination = (m_view->selectionModel()->selectedRows().count() > 0)
                    ? m_model->entryForIndex(m_filterModel->mapToSource(m_view->selectionModel()->currentIndex()))
                    : nullptr;
    if (m_destination == nullptr) {
        m_destination = new Archive::Entry(nullptr, QString());
    } else {
        m_destination = new Archive::Entry(nullptr, m_destination->fullPath());
    }

    if (m_model->filesToMove.count() > 0) {
        // Changing destination to include new entry path if pasting only 1 entry.
        QVector<Archive::Entry*> entriesWithoutChildren = ReadOnlyArchiveInterface::entriesWithoutChildren(QVector<Archive::Entry*>::fromList(m_model->filesToMove.values()));
        if (entriesWithoutChildren.count() == 1) {
            const Archive::Entry *entry = entriesWithoutChildren.first();
            auto entryName = entry->name();
            if (entry->isDir()) {
                entryName += QLatin1Char('/');
            }
            m_destination->setFullPath(m_destination->fullPath() + entryName);
        }

        foreach (const Archive::Entry *entry, entriesWithoutChildren) {
            if (entry->isDir() && m_destination->fullPath().startsWith(entry->fullPath())) {
                KMessageBox::error(widget(),
                                   i18n("Folders can't be moved into themselves."),
                                   i18n("Moving a folder into itself"));
                delete m_destination;
                return;
            }
        }
        auto entryList = QVector<Archive::Entry*>::fromList(m_model->filesToMove.values());
        slotPasteFiles(entryList, m_destination, entriesWithoutChildren.count());
        m_model->filesToMove.clear();
    } else {
        auto entryList = QVector<Archive::Entry*>::fromList(m_model->filesToCopy.values());
        slotPasteFiles(entryList, m_destination, 0);
        m_model->filesToCopy.clear();
    }
    m_cutIndexes.clear();
    updateActions();
}

void Part::slotPasteFiles(QVector<Kerfuffle::Archive::Entry*> &files, Kerfuffle::Archive::Entry *destination, int entriesWithoutChildren)
{
    if (files.isEmpty()) {
        delete m_destination;
        return;
    }

    QStringList filesPaths = ReadOnlyArchiveInterface::entryFullPaths(files);
    QStringList newPaths = ReadOnlyArchiveInterface::entryPathsFromDestination(filesPaths, destination, entriesWithoutChildren);

    if (ArchiveModel::hasDuplicatedEntries(newPaths)) {
        displayMsgWidget(KMessageWidget::Error, i18n("Entries with the same names can't be pasted to the same destination."));
        delete m_destination;
        return;
    }

    QList<const Archive::Entry*> conflictingEntries;
    bool error = m_model->conflictingEntries(conflictingEntries, newPaths, false);

    if (conflictingEntries.count() != 0) {
        QPointer<OverwriteDialog> overwriteDialog = new OverwriteDialog(widget(), conflictingEntries, m_model->entryIcons(), error);
        int ret = overwriteDialog->exec();
        delete overwriteDialog;
        if (ret == QDialog::Rejected) {
            delete m_destination;
            return;
        }
    }

    if (entriesWithoutChildren > 0) {
        qCDebug(ARK) << "Moving" << files << "to" << destination;
    } else {
        qCDebug(ARK) << "Copying " << files << "to" << destination;
    }

    KJob *job;
    if (entriesWithoutChildren != 0) {
        job = m_model->moveFiles(files, destination, CompressionOptions());
    } else {
        job = m_model->copyFiles(files, destination, CompressionOptions());
    }

    if (job) {
        connect(job, &KJob::result,
                this, &Part::slotPasteFilesDone);
        registerJob(job);
        job->start();
    } else {
        delete m_destination;
    }
}

void Part::slotAddFilesDone(KJob* job)
{
    qDeleteAll(m_jobTempEntries);
    m_jobTempEntries.clear();
    if (job->error() && job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    } else {
        // Hide the "archive will be created as soon as you add a file" message.
        m_messageWidget->hide();

        // For multi-volume archive, we need to re-open the archive after adding files
        // because the name changes from e.g name.rar to name.part1.rar.
        if (m_model->archive()->isMultiVolume()) {
            qCDebug(ARK) << "Multi-volume archive detected, re-opening...";
            KParts::OpenUrlArguments args = arguments();
            args.metaData()[QStringLiteral("createNewArchive")] = QStringLiteral("false");
            setArguments(args);

            openUrl(QUrl::fromLocalFile(m_model->archive()->multiVolumeName()));
        }
    }
    m_cutIndexes.clear();
    m_model->filesToMove.clear();
    m_model->filesToCopy.clear();
}

void Part::slotPasteFilesDone(KJob *job)
{
    if (job->error() && job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    }
    m_cutIndexes.clear();
    m_model->filesToMove.clear();
    m_model->filesToCopy.clear();
}

void Part::slotDeleteFilesDone(KJob* job)
{
    if (job->error() && job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    }
    m_cutIndexes.clear();
    m_model->filesToMove.clear();
    m_model->filesToCopy.clear();
}

void Part::slotDeleteFiles()
{
    const int selectionsCount = m_view->selectionModel()->selectedRows().count();
    const auto reallyDelete =
        KMessageBox::questionYesNo(widget(),
                                   i18ncp("@info",
                                          "Deleting this file is not undoable. Are you sure you want to do this?",
                                          "Deleting these files is not undoable. Are you sure you want to do this?",
                                          selectionsCount),
                                   i18ncp("@title:window", "Delete File", "Delete Files", selectionsCount),
                                   KStandardGuiItem::del(),
                                   KStandardGuiItem::no(),
                                   QString(),
                                   KMessageBox::Dangerous | KMessageBox::Notify);

    if (reallyDelete == KMessageBox::No) {
        return;
    }

    DeleteJob *job = m_model->deleteFiles(filesForIndexes(addChildren(getSelectedIndexes())));
    connect(job, &KJob::result,
            this, &Part::slotDeleteFilesDone);
    registerJob(job);
    job->start();
}

void Part::slotShowProperties()
{
    m_model->countEntriesAndSize();
    QPointer<Kerfuffle::PropertiesDialog> dialog(new Kerfuffle::PropertiesDialog(0,
                                                                                 m_model->archive(),
                                                                                 m_model->numberOfFiles(),
                                                                                 m_model->numberOfFolders(),
                                                                                 m_model->uncompressedSize()));
    dialog.data()->show();
}

void Part::slotToggleInfoPanel(bool visible)
{
    if (visible) {
        m_splitter->setSizes(ArkSettings::splitterSizes());
        m_infoPanel->show();
    } else {
        // We need to save the splitterSizes before hiding, otherwise
        // Ark won't remember resizing done by the user.
        ArkSettings::setSplitterSizes(m_splitter->sizes());
        m_infoPanel->hide();
    }
}

void Part::slotSaveAs()
{
    QUrl saveUrl = QFileDialog::getSaveFileUrl(widget(), i18nc("@title:window", "Save Archive As"), url());

    if ((saveUrl.isValid()) && (!saveUrl.isEmpty())) {
        auto statJob = KIO::stat(saveUrl, KIO::StatJob::DestinationSide, 0);
        KJobWidgets::setWindow(statJob, widget());
        if (statJob->exec()) {
            int overwrite = KMessageBox::warningContinueCancel(widget(),
                                                               xi18nc("@info", "An archive named <filename>%1</filename> already exists. Are you sure you want to overwrite it?", saveUrl.fileName()),
                                                               QString(),
                                                               KStandardGuiItem::overwrite());

            if (overwrite != KMessageBox::Continue) {
                return;
            }
        }

        QUrl srcUrl = QUrl::fromLocalFile(localFilePath());

        if (!QFile::exists(localFilePath())) {
            if (url().isLocalFile()) {
                KMessageBox::error(widget(),
                                   xi18nc("@info", "The archive <filename>%1</filename> cannot be copied to the specified location. The archive does not exist anymore.", localFilePath()));

                return;
            } else {
                srcUrl = url();
            }
        }

        KIO::Job *copyJob = KIO::file_copy(srcUrl, saveUrl, -1, KIO::Overwrite);

        KJobWidgets::setWindow(copyJob, widget());
        copyJob->exec();
        if (copyJob->error()) {
            KMessageBox::error(widget(),
                               xi18nc("@info", "The archive could not be saved as <filename>%1</filename>. Try saving it to another location.", saveUrl.path()));
        }
    }
}

void Part::slotShowContextMenu()
{
    if (!factory()) {
        return;
    }

    QMenu *popup = static_cast<QMenu *>(factory()->container(QStringLiteral("context_menu"), this));
    popup->popup(QCursor::pos());
}

bool Part::eventFilter(QObject *target, QEvent *event)
{
    Q_UNUSED(target)

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *e = static_cast<QKeyEvent *>(event);
        if (e->key() == Qt::Key_Escape) {
            m_searchWidget->hide();
            m_searchLineEdit->clear();
            return true;
        }
    }
    return false;
}

void Part::slotShowFind()
{
    if (m_searchWidget->isVisible()) {
        m_searchLineEdit->selectAll();
    } else {
        m_searchWidget->show();
    }
    m_searchLineEdit->setFocus();
}

void Part::searchEdited(const QString &text)
{
    m_view->collapseAll();

    m_filterModel->setFilterFixedString(text);

    if(text.isEmpty()) {
        m_view->collapseAll();
        m_view->expandIfSingleFolder();
    } else {
        m_view->expandAll();
    }
}

void Part::displayMsgWidget(KMessageWidget::MessageType type, const QString& msg)
{
    // The widget could be already visible, so hide it.
    m_messageWidget->hide();
    m_messageWidget->setText(msg);
    m_messageWidget->setMessageType(type);
    m_messageWidget->animatedShow();
}

} // namespace Ark

#include "part.moc"
