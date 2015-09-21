/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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
#include "app/logging.h"
#include "archivemodel.h"
#include "archiveview.h"
#include "arkviewer.h"
#include "dnddbusinterfaceadaptor.h"
#include "infopanel.h"
#include "jobtracker.h"
#include "kerfuffle/archive_kerfuffle.h"
#include "kerfuffle/extractiondialog.h"
#include "kerfuffle/extractionsettingspage.h"
#include "kerfuffle/jobs.h"
#include "kerfuffle/settings.h"
#include "kerfuffle/previewsettingspage.h"

#include <KAboutData>
#include <KActionCollection>
#include <KConfigGroup>
#include <KGuiItem>
#include <KIO/Job>
#include <KJobWidgets>
#include <KIO/StatJob>
#include <KMessageBox>
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
#include <QPointer>
#include <QSplitter>
#include <QTimer>
#include <QtDBus/QtDBus>
#include <QFileDialog>
#include <QIcon>
#include <QInputDialog>

Q_LOGGING_CATEGORY(PART, "ark.part", QtWarningMsg)

using namespace Kerfuffle;

K_PLUGIN_FACTORY(Factory, registerPlugin<Ark::Part>();)

namespace Ark
{

static quint32 s_instanceCounter = 1;

Part::Part(QWidget *parentWidget, QObject *parent, const QVariantList& args)
        : KParts::ReadWritePart(parent),
          m_splitter(Q_NULLPTR),
          m_busy(false),
          m_jobTracker(Q_NULLPTR)
{
    Q_UNUSED(args)
    setComponentData(*createAboutData(), false);

    new DndExtractAdaptor(this);

    const QString pathName = QStringLiteral("/DndExtract/%1").arg(s_instanceCounter++);
    if (!QDBusConnection::sessionBus().registerObject(pathName, this)) {
        qCCritical(PART) << "Could not register a D-Bus object for drag'n'drop";
    }

    // m_vlayout is needed for later insertion of QMessageWidget
    QWidget *mainWidget = new QWidget;
    m_vlayout = new QVBoxLayout;
    m_model = new ArchiveModel(pathName, this);
    m_splitter = new QSplitter(Qt::Horizontal, parentWidget);
    m_view = new ArchiveView;
    m_infoPanel = new InfoPanel(m_model);

    setWidget(mainWidget);
    mainWidget->setLayout(m_vlayout);

    // Configure the QVBoxLayout and add widgets
    m_vlayout->setContentsMargins(0,0,0,0);
    m_vlayout->addWidget(m_splitter);

    // Add widgets to the QSplitter.
    m_splitter->addWidget(m_view);
    m_splitter->addWidget(m_infoPanel);

    // Read settings from config file and show/hide infoPanel.
    if (!ArkSettings::showInfoPanel()) {
        m_infoPanel->hide();
    } else {
        m_splitter->setSizes(ArkSettings::splitterSizes());
    }

    setupView();
    setupActions();

    connect(m_model, SIGNAL(loadingStarted()),
            this, SLOT(slotLoadingStarted()));
    connect(m_model, SIGNAL(loadingFinished(KJob*)),
            this, SLOT(slotLoadingFinished(KJob*)));
    connect(m_model, SIGNAL(droppedFiles(QStringList,QString)),
            this, SLOT(slotAddFiles(QStringList,QString)));
    connect(m_model, SIGNAL(error(QString,QString)),
            this, SLOT(slotError(QString,QString)));

    connect(this, SIGNAL(busy()),
            this, SLOT(setBusyGui()));
    connect(this, SIGNAL(ready()),
            this, SLOT(setReadyGui()));
    connect(this, SIGNAL(completed()),
            this, SLOT(setFileNameFromArchive()));

    m_statusBarExtension = new KParts::StatusBarExtension(this);

    setXMLFile(QStringLiteral("ark_part.rc"));
}

Part::~Part()
{
    qDeleteAll(m_previewDirList);

    // Only save splitterSizes if infopanel is visible,
    // because we don't want to store zero size for infopanel.
    if (m_showInfoPanelAction->isChecked()) {
        ArkSettings::setSplitterSizes(m_splitter->sizes());
    }
    ArkSettings::setShowInfoPanel(m_showInfoPanelAction->isChecked());
    ArkSettings::self()->save();

    m_extractFilesAction->menu()->deleteLater();
}

KAboutData *Part::createAboutData()
{
    return new KAboutData(QStringLiteral("ark"),
                          i18n("ArkPart"),
                          QStringLiteral("3.0"));
}

void Part::registerJob(KJob* job)
{
    if (!m_jobTracker) {
        m_jobTracker = new JobTracker(widget());
        m_statusBarExtension->addStatusBarItem(m_jobTracker->widget(0), 0, true);
        m_jobTracker->widget(job)->show();
    }
    m_jobTracker->registerJob(job);

    emit busy();
    connect(job, SIGNAL(result(KJob*)), this, SIGNAL(ready()));
}

// TODO: One should construct a KUrl out of localPath in order to be able to handle
//       non-local destinations (ie. trash:/ or a remote location)
//       See bugs #189322 and #204323.
void Part::extractSelectedFilesTo(const QString& localPath)
{
    qCDebug(PART) << "Extract to" << localPath;
    if (!m_model) {
        return;
    }

    Kerfuffle::ExtractionOptions options;
    options[QStringLiteral("PreservePaths")] = true;
    options[QStringLiteral("RemoveRootNode")] = true;

    // Create and start the ExtractJob.
    ExtractJob *job = m_model->extractFiles(filesAndRootNodesForIndexes(addChildren(m_view->selectionModel()->selectedRows())), localPath, options);
    registerJob(job);
    job->start();
}

void Part::setupView()
{
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);

    m_view->setModel(m_model);

    m_view->setSortingEnabled(true);

    connect(m_view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateActions()));
    connect(m_view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    if (QApplication::style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, 0, m_view)) {
        connect(m_view, SIGNAL(clicked(QModelIndex)),
                this, SLOT(slotClicked(QModelIndex)));
    } else {
        connect(m_view, SIGNAL(doubleClicked(QModelIndex)),
                this, SLOT(slotClicked(QModelIndex)));
    }

    connect(m_view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotShowContextMenu()));

    connect(m_model, SIGNAL(columnsInserted(QModelIndex,int,int)),
            this, SLOT(adjustColumns()));
}

void Part::slotClicked(QModelIndex)
{
    // Only open the viewer if a CTRL or SHIFT key is not pressed
    if (QGuiApplication::keyboardModifiers() != Qt::ShiftModifier &&
        QGuiApplication::keyboardModifiers() != Qt::ControlModifier) {
        slotPreviewWithInternalViewer();
    }
}

void Part::setupActions()
{
    m_showInfoPanelAction = new KToggleAction(i18nc("@action:inmenu", "Show information panel"), this);
    actionCollection()->addAction(QStringLiteral( "show-infopanel" ), m_showInfoPanelAction);
    m_showInfoPanelAction->setChecked(ArkSettings::showInfoPanel());
    connect(m_showInfoPanelAction, SIGNAL(triggered(bool)),
            this, SLOT(slotToggleInfoPanel(bool)));

    m_saveAsAction = KStandardAction::saveAs(this, SLOT(slotSaveAs()), actionCollection());

    m_previewChooseAppAction = actionCollection()->addAction(QStringLiteral("openwith"));
    m_previewChooseAppAction->setText(i18nc("open a file with external program", "Open &With..."));
    m_previewChooseAppAction->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    m_previewChooseAppAction->setStatusTip(i18n("Click to open the selected file with an external program"));
    connect(m_previewChooseAppAction, SIGNAL(triggered(bool)), this, SLOT(slotPreviewWithExternalProgram()));

    m_previewAction = actionCollection()->addAction(QStringLiteral("preview"));
    m_previewAction->setText(i18nc("to preview a file inside an archive", "Pre&view"));
    m_previewAction->setIcon(QIcon::fromTheme(QStringLiteral("document-preview-archive")));
    m_previewAction->setStatusTip(i18n("Click to preview the selected file"));
    actionCollection()->setDefaultShortcuts(m_previewAction, QList<QKeySequence>() << Qt::Key_Return << Qt::Key_Space);
    connect(m_previewAction, SIGNAL(triggered(bool)),
            this, SLOT(slotPreviewWithInternalViewer()));

    m_extractFilesAction = actionCollection()->addAction(QStringLiteral("extract"));
    m_extractFilesAction->setText(i18n("E&xtract"));
    m_extractFilesAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-extract")));
    m_extractFilesAction->setStatusTip(i18n("Click to open an extraction dialog, where you can choose to extract either all files or just the selected ones"));
    actionCollection()->setDefaultShortcut(m_extractFilesAction, Qt::CTRL + Qt::Key_E);
    connect(m_extractFilesAction, SIGNAL(triggered(bool)),
            this, SLOT(slotExtractFiles()));

    m_addFilesAction = actionCollection()->addAction(QStringLiteral("add"));
    m_addFilesAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-insert")));
    m_addFilesAction->setText(i18n("Add &File..."));
    m_addFilesAction->setStatusTip(i18n("Click to add files to the archive"));
    connect(m_addFilesAction, SIGNAL(triggered(bool)),
            this, SLOT(slotAddFiles()));

    m_addDirAction = actionCollection()->addAction(QStringLiteral("add-dir"));
    m_addDirAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-insert-directory")));
    m_addDirAction->setText(i18n("Add Fo&lder..."));
    m_addDirAction->setStatusTip(i18n("Click to add a folder to the archive"));
    connect(m_addDirAction, SIGNAL(triggered(bool)),
            this, SLOT(slotAddDir()));

    m_deleteFilesAction = actionCollection()->addAction(QStringLiteral("delete"));
    m_deleteFilesAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-remove")));
    m_deleteFilesAction->setText(i18n("De&lete"));
    actionCollection()->setDefaultShortcut(m_deleteFilesAction, Qt::Key_Delete);
    m_deleteFilesAction->setStatusTip(i18n("Click to delete the selected files"));
    connect(m_deleteFilesAction, SIGNAL(triggered(bool)),
            this, SLOT(slotDeleteFiles()));

    updateActions();
}

void Part::updateActions()
{
    bool isWritable = m_model->archive() && (!m_model->archive()->isReadOnly());

    m_previewAction->setEnabled(!isBusy() && (m_view->selectionModel()->selectedRows().count() == 1)
                                && isPreviewable(m_view->selectionModel()->currentIndex()));
    m_extractFilesAction->setEnabled(!isBusy() && (m_model->rowCount() > 0));
    m_addFilesAction->setEnabled(!isBusy() && isWritable);
    m_addDirAction->setEnabled(!isBusy() && isWritable);
    m_deleteFilesAction->setEnabled(!isBusy() && (m_view->selectionModel()->selectedRows().count() > 0)
                                    && isWritable);
    m_previewChooseAppAction->setEnabled(!isBusy() && (m_view->selectionModel()->selectedRows().count() > 0)
                                    && isWritable);

    QMenu *menu = m_extractFilesAction->menu();
    if (!menu) {
        menu = new QMenu;
        m_extractFilesAction->setMenu(menu);
        connect(menu, SIGNAL(triggered(QAction*)),
                this, SLOT(slotQuickExtractFiles(QAction*)));

        // Remember to keep this action's properties as similar to
        // m_extractFilesAction's as possible (except where it does not make
        // sense, such as the text or the shortcut).
        QAction *extractTo = menu->addAction(i18n("Extract To..."));
        extractTo->setIcon(m_extractFilesAction->icon());
        extractTo->setStatusTip(m_extractFilesAction->statusTip());
        connect(extractTo, SIGNAL(triggered(bool)), SLOT(slotExtractFiles()));

        menu->addSeparator();

        QAction *header = menu->addAction(i18n("Quick Extract To..."));
        header->setEnabled(false);
        header->setIcon(QIcon::fromTheme(QStringLiteral("archive-extract")));
    }

    while (menu->actions().size() > 3) {
        menu->removeAction(menu->actions().last());
    }

    const KConfigGroup conf(KSharedConfig::openConfig(), "DirSelect Dialog");
    const QStringList dirHistory = conf.readPathEntry("History Items", QStringList());

    for (int i = 0; i < qMin(10, dirHistory.size()); ++i) {
        const QString dir = QUrl(dirHistory.value(i)).toString(QUrl::RemoveScheme | QUrl::NormalizePathSegments | QUrl::PreferLocalFile);
        QAction *newAction = menu->addAction(dir);
        newAction->setData(dir);
    }
}

void Part::slotQuickExtractFiles(QAction *triggeredAction)
{
    // #190507: triggeredAction->data.isNull() means it's the "Extract to..."
    //          action, and we do not want it to run here
    if (!triggeredAction->data().isNull()) {
        const QString userDestination = triggeredAction->data().toString();
        qCDebug(PART) << "Extract to user dest" << userDestination;
        QString finalDestinationDirectory;
        const QString detectedSubfolder = detectSubfolder();
        qCDebug(PART) << "Detected subfolder" << detectedSubfolder;

        if (!isSingleFolderArchive()) {
            finalDestinationDirectory = userDestination +
                                        QDir::separator() + detectedSubfolder;
            QDir(userDestination).mkdir(detectedSubfolder);
        } else {
            finalDestinationDirectory = userDestination;
        }

        qCDebug(PART) << "Extract to final dest" << finalDestinationDirectory;

        Kerfuffle::ExtractionOptions options;
        options[QStringLiteral("PreservePaths")] = true;
        QList<QVariant> files = filesAndRootNodesForIndexes(m_view->selectionModel()->selectedRows());
        ExtractJob *job = m_model->extractFiles(files, finalDestinationDirectory, options);
        registerJob(job);

        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotExtractionDone(KJob*)));

        job->start();
    }
}

bool Part::isPreviewable(const QModelIndex& index) const
{
    const int maxPreviewSize = ArkSettings::previewFileSizeLimit() * 1024 * 1024;
    const bool limit = ArkSettings::limitPreviewFileSize();
    const qlonglong size = m_model->entryForIndex(index) [ Size ].toLongLong();

    return index.isValid()
           && (!m_model->entryForIndex(index)[ IsDirectory ].toBool())
           && (!limit || (limit && size < maxPreviewSize));
}

void Part::selectionChanged()
{
    m_infoPanel->setIndexes(m_view->selectionModel()->selectedRows());
}

bool Part::openFile()
{
    qCDebug(PART) << "Attempting to open archive" << localFilePath();

    const QString localFile(localFilePath());
    const QFileInfo localFileInfo(localFile);
    const bool creatingNewArchive =
        arguments().metaData()[QStringLiteral("createNewArchive")] == QLatin1String("true");

    if (localFileInfo.isDir()) {
        KMessageBox::error(widget(), xi18nc("@info",
                                            "<filename>%1</filename> is a directory.",
                                            localFile));
        return false;
    }

    if (creatingNewArchive) {
        if (localFileInfo.exists()) {
            int overwrite =  KMessageBox::questionYesNo(widget(), xi18nc("@info", "The archive <filename>%1</filename> already exists. Would you like to open it instead?", localFile), i18nc("@title:window", "File Exists"), KGuiItem(i18n("Open File")), KStandardGuiItem::cancel());

            if (overwrite == KMessageBox::No) {
                return false;
            }
        }
    } else {
        if (!localFileInfo.exists()) {
            KMessageBox::sorry(widget(), xi18nc("@info", "The archive <filename>%1</filename> was not found.", localFile), i18nc("@title:window", "Error Opening Archive"));
            return false;
        }

        if (!localFileInfo.isReadable()) {
            displayMsgWidget(KMessageWidget::Warning, xi18nc("@info", "The archive <filename>%1</filename> could not be loaded, as it was not possible to read from it.", localFile));
            return false;
        }
    }

    QScopedPointer<Kerfuffle::Archive> archive(Kerfuffle::Archive::create(localFile, m_model));

    if ((!archive) || ((creatingNewArchive) && (archive->isReadOnly()))) {
        QStringList mimeTypeList;
        QHash<QString, QString> mimeTypes;

        if (creatingNewArchive) {
            mimeTypeList = Kerfuffle::supportedWriteMimeTypes();
        } else {
            mimeTypeList = Kerfuffle::supportedMimeTypes();
        }

        QMimeDatabase db;
        foreach(const QString& mime, mimeTypeList) {
            const QMimeType mimeType = db.mimeTypeForName(mime);
            if (mimeType.isValid()) {
                // Key = "application/zip", Value = "Zip Archive"
                mimeTypes[mime] = mimeType.comment();
            }
        }

        QStringList mimeComments(mimeTypes.values());
        mimeComments.sort();

        bool ok;
        QString item;

        if (creatingNewArchive) {
            item = QInputDialog::getItem(widget(), i18nc("@title:window", "Invalid Archive Type"),
                                         i18nc("@info", "Ark cannot create archives of the type you have chosen.<br />Please choose another archive type below."),
                                         mimeComments, 0, false, &ok);
        } else {
            item = QInputDialog::getItem(widget(), i18nc("@title:window", "Unable to Determine Archive Type"),
                                         i18nc("@info", "Ark was unable to determine the archive type of the filename.<br />Please choose the correct archive type below."),
                                         mimeComments, 0, false, &ok);
        }

        if ((!ok) || (item.isEmpty())) {
            return false;
        }

        // Delete archive and point to a new one
        archive.reset(Kerfuffle::Archive::create(localFile, mimeTypes.key(item), m_model));
    }

    if (!archive) {
        KMessageBox::sorry(widget(), xi18nc("@info", "Ark was not able to open the archive <filename>%1</filename>. No plugin capable of handling the file was found.", localFile), i18nc("@title:window", "Error Opening Archive"));
        return false;
    }

    KJob *job = m_model->setArchive(archive.take());
    registerJob(job);
    job->start();
    m_infoPanel->setIndex(QModelIndex());

    if (arguments().metaData()[QStringLiteral("showExtractDialog")] == QLatin1String("true")) {
        QTimer::singleShot(0, this, SLOT(slotExtractFiles()));
    }

    const QString password = arguments().metaData()[QStringLiteral("encryptionPassword")];
    if (!password.isEmpty()) {
        m_model->setPassword(password);

        if (arguments().metaData()[QStringLiteral("encryptHeader")] == QLatin1String("true")) {
            m_model->enableHeaderEncryption();
        }
    }

    return true;
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
    pages.append(new ExtractionSettingsPage(parent, i18n("Extraction settings"), QStringLiteral("archive-extract")));
    pages.append(new PreviewSettingsPage(parent, i18n("Preview settings"), QStringLiteral("document-preview-archive")));

    return pages;
}

void Part::slotLoadingStarted()
{
}

void Part::slotLoadingFinished(KJob *job)
{
    if (job->error()) {
        if (arguments().metaData()[QStringLiteral("createNewArchive")] != QLatin1String("true")) {
            if (job->error() != KJob::KilledJobError) {
                KMessageBox::error(widget(),
                                   xi18nc("@info", "Loading the archive <filename>%1</filename> failed with the following error:<nl/><nl/><message>%2</message>",
                                          localFilePath(),
                                          job->errorText()),
                                   i18nc("@title:window", "Error Opening Archive"));
            }

            // The file failed to open, so reset the open archive, info panel and caption.
            m_model->setArchive(Q_NULLPTR);

            m_infoPanel->setPrettyFileName(QString());
            m_infoPanel->updateWithDefaults();

            emit setWindowCaption(QString());
        }
    }

    m_view->sortByColumn(0, Qt::AscendingOrder);
    m_view->expandToDepth(0);


    // After loading all files, resize the columns to fit all fields
    m_view->header()->resizeSections(QHeaderView::ResizeToContents);

    updateActions();
}

void Part::setReadyGui()
{
    QApplication::restoreOverrideCursor();
    m_busy = false;
    m_view->setEnabled(true);
    updateActions();
}

void Part::setBusyGui()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_busy = true;
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

void Part::slotPreviewWithInternalViewer()
{
    preview(m_view->selectionModel()->currentIndex(), InternalViewer);
}

void Part::slotPreviewWithExternalProgram()
{
    preview(m_view->selectionModel()->currentIndex(), ExternalProgram);
}

void Part::preview(const QModelIndex &index, PreviewMode mode)
{
    if (!isPreviewable(index)) {
        return;
    }

    const ArchiveEntry& entry =  m_model->entryForIndex(index);

    if (entry[Link].toBool()) {
        displayMsgWidget(KMessageWidget::Information, i18n("Preview is not possible for symlinks."));
        return;
    }

    if (!entry.isEmpty()) {
        Kerfuffle::ExtractionOptions options;
        options[QStringLiteral("PreservePaths")] = true;

        m_previewDirList.append(new QTemporaryDir);
        m_previewMode = mode;
        ExtractJob *job = m_model->extractFile(entry[InternalID], m_previewDirList.last()->path(), options);

        registerJob(job);
        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotPreviewExtracted(KJob*)));
        job->start();
    }
}

void Part::slotPreviewExtracted(KJob *job)
{
    // FIXME: the error checking here isn't really working
    //        if there's an error or an overwrite dialog,
    //        the preview dialog will be launched anyway
    if (!job->error()) {
        const ArchiveEntry& entry =
            m_model->entryForIndex(m_view->selectionModel()->currentIndex());

        ExtractJob *extractJob = qobject_cast<ExtractJob*>(job);
        Q_ASSERT(extractJob);
        QString fullName = extractJob->destinationDirectory() + QLatin1Char('/') + entry[FileName].toString();

        // Make sure a maliciously crafted archive with parent folders named ".." do
        // not cause the previewed file path to be located outside the temporary
        // directory, resulting in a directory traversal issue.
        fullName.remove(QStringLiteral("../"));

        // TODO: get rid of m_previewMode by extending ExtractJob with a PreviewJob.
        // This would prevent race conditions if we ever stop disabling
        // the whole UI while extracting a file to preview it.
        switch (m_previewMode) {
        case InternalViewer:
            ArkViewer::view(fullName, widget());
            break;
        case ExternalProgram:
            QList<QUrl> list;
            list.append(QUrl::fromUserInput(fullName, QString(), QUrl::AssumeLocalFile));
            KRun::displayOpenWithDialog(list, widget(), true);
            break;
        }
    } else if (job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    }
    setReadyGui();
}

void Part::slotError(const QString& errorMessage, const QString& details)
{
    if (details.isEmpty()) {
        KMessageBox::error(widget(), errorMessage);
    } else {
        KMessageBox::detailedError(widget(), errorMessage, details);
    }
}

bool Part::isSingleFolderArchive() const
{
    return m_model->archive()->isSingleFolderArchive();
}

QString Part::detectSubfolder() const
{
    if (!m_model) {
        return QString();
    }

    return m_model->archive()->subfolderName();
}

void Part::slotExtractFiles()
{
    if (!m_model) {
        return;
    }

    QPointer<Kerfuffle::ExtractionDialog> dialog(new Kerfuffle::ExtractionDialog);

    dialog.data()->setModal(true);

    if (m_view->selectionModel()->selectedRows().count() > 0) {
        dialog.data()->setShowSelectedFiles(true);
    }

    dialog.data()->setSingleFolderArchive(isSingleFolderArchive());
    dialog.data()->setSubfolder(detectSubfolder());

    dialog.data()->setCurrentUrl(QUrl::fromLocalFile(QFileInfo(m_model->archive()->fileName()).absolutePath()));

    dialog.data()->show();
    dialog.data()->restoreWindowSize();

    if (dialog.data()->exec()) {
        //this is done to update the quick extract menu
        updateActions();

        QVariantList files;

        // If the user has chosen to extract only selected entries, fetch these
        // from the QTreeView.
        if (!dialog.data()->extractAllFiles()) {
            files = filesAndRootNodesForIndexes(addChildren(m_view->selectionModel()->selectedRows()));
        }

        qCDebug(PART) << "Selected " << files;

        Kerfuffle::ExtractionOptions options;

        if (dialog.data()->preservePaths()) {
            options[QStringLiteral("PreservePaths")] = true;
        }
        options[QStringLiteral("FollowExtractionDialogSettings")] = true;

        const QString destinationDirectory = dialog.data()->destinationDirectory().toDisplayString(QUrl::PreferLocalFile);
        ExtractJob *job = m_model->extractFiles(files, destinationDirectory, options);
        registerJob(job);

        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotExtractionDone(KJob*)));

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

QList<QVariant> Part::filesForIndexes(const QModelIndexList& list) const
{
    QVariantList ret;

    foreach(const QModelIndex& index, list) {
        const ArchiveEntry& entry = m_model->entryForIndex(index);
        ret << entry[InternalID].toString();
    }

    return ret;
}

QList<QVariant> Part::filesAndRootNodesForIndexes(const QModelIndexList& list) const
{
    QVariantList fileList;

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
        const QString rootInternalID =
            m_model->entryForIndex(selectionRoot).value(InternalID).toString();


        // Append index with root node to fileList.
        QModelIndexList alist = QModelIndexList() << index;
        foreach (const QVariant &file, filesForIndexes(alist)) {
            QVariant v = QVariant::fromValue(fileRootNodePair(file.toString(), rootInternalID));
            if (!fileList.contains(v)) {
                fileList.append(v);
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

        const bool followExtractionDialogSettings =
            extractJob->extractionOptions().value(QStringLiteral("FollowExtractionDialogSettings"), false).toBool();
        if (!followExtractionDialogSettings) {
            return;
        }

        if (ArkSettings::openDestinationFolderAfterExtraction()) {
            qCDebug(PART) << "Shall open" << extractJob->destinationDirectory();
            QUrl destinationDirectory = QUrl::fromLocalFile(extractJob->destinationDirectory()).adjusted(QUrl::NormalizePathSegments);
            qCDebug(PART) << "Shall open URL" << destinationDirectory;

            KRun::runUrl(destinationDirectory, QStringLiteral("inode/directory"), widget());
        }

        if (ArkSettings::closeAfterExtraction()) {
           emit quit();
        }
    }
}

void Part::adjustColumns()
{
    m_view->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

void Part::slotAddFiles(const QStringList& filesToAdd, const QString& path)
{
    if (filesToAdd.isEmpty()) {
        return;
    }

    qCDebug(PART) << "Adding " << filesToAdd << " to " << path;
    qCDebug(PART) << "Warning, for now the path argument is not implemented";

    QStringList cleanFilesToAdd(filesToAdd);
    for (int i = 0; i < cleanFilesToAdd.size(); ++i) {
        QString& file = cleanFilesToAdd[i];
        if (QFileInfo(file).isDir()) {
            if (!file.endsWith(QLatin1Char( '/' ))) {
                file += QLatin1Char( '/' );
            }
        }
    }

    CompressionOptions options;

    QString firstPath = cleanFilesToAdd.first();
    if (firstPath.right(1) == QLatin1String("/")) {
        firstPath.chop(1);
    }
    firstPath = QFileInfo(firstPath).dir().absolutePath();

    qCDebug(PART) << "Detected relative path to be " << firstPath;
    options[QStringLiteral("GlobalWorkDir")] = firstPath;

    AddJob *job = m_model->addFiles(cleanFilesToAdd, options);
    if (!job) {
        return;
    }

    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(slotAddFilesDone(KJob*)));
    registerJob(job);
    job->start();
}

void Part::slotAddFiles()
{
    // #264819: passing widget() as the parent will not work as expected.
    //          KFileDialog will create a KFileWidget, which runs an internal
    //          event loop to stat the given directory. This, in turn, leads to
    //          events being delivered to widget(), which is a QSplitter, which
    //          in turn reimplements childEvent() and will end up calling
    //          QWidget::show() on the KFileDialog (thus showing it in a
    //          non-modal state).
    //          When KFileDialog::exec() is called, the widget is already shown
    //          and nothing happens.

    const QStringList filesToAdd = QFileDialog::getOpenFileNames(widget(), i18nc("@title:window", "Add Files"));

    slotAddFiles(filesToAdd);
}

void Part::slotAddDir()
{
    const QString dirToAdd = QFileDialog::getExistingDirectory(widget(), i18nc("@title:window", "Add Folder"));

    if (!dirToAdd.isEmpty()) {
        slotAddFiles(QStringList() << dirToAdd);
    }
}

void Part::slotAddFilesDone(KJob* job)
{
    if (job->error() && job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    }
}

void Part::slotDeleteFilesDone(KJob* job)
{
    if (job->error() && job->error() != KJob::KilledJobError) {
        KMessageBox::error(widget(), job->errorString());
    }
}

void Part::slotDeleteFiles()
{
    const int reallyDelete =
        KMessageBox::questionYesNo(widget(),
                                   i18n("Deleting these files is not undoable. Are you sure you want to do this?"),
                                   i18nc("@title:window", "Delete files"),
                                   KStandardGuiItem::del(),
                                   KStandardGuiItem::cancel(),
                                   QString(),
                                   KMessageBox::Dangerous | KMessageBox::Notify);

    if (reallyDelete == KMessageBox::No) {
        return;
    }

    DeleteJob *job = m_model->deleteFiles(filesForIndexes(addChildren(m_view->selectionModel()->selectedRows())));
    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(slotDeleteFilesDone(KJob*)));
    registerJob(job);
    job->start();
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

void Part::displayMsgWidget(KMessageWidget::MessageType type, QString msg)
{
  KMessageWidget *msgWidget = new KMessageWidget;
  msgWidget->setText(msg);
  msgWidget->setMessageType(type);
  m_vlayout->insertWidget(0, msgWidget);
  msgWidget->animatedShow();
}

} // namespace Ark

#include "part.moc"
