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
#include "archivemodel.h"
#include "archiveview.h"
#include "arkviewer.h"
#include "dnddbusinterfaceadaptor.h"
#include "infopanel.h"
#include "jobtracker.h"
#include "kerfuffle/archive.h"
#include "kerfuffle/extractiondialog.h"
#include "kerfuffle/jobs.h"
#include "kerfuffle/settings.h"

#include <KAboutData>
#include <KAction>
#include <KActionCollection>
#include <KApplication>
#include <KConfigGroup>
#include <KDebug>
#include <KFileDialog>
#include <KGuiItem>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <KIcon>
#include <KInputDialog>
#include <KMessageBox>
#include <KPluginFactory>
#include <KRun>
#include <KSelectAction>
#include <KStandardDirs>
#include <KStandardGuiItem>
#include <KTempDir>
#include <KToggleAction>
#include <KXMLGUIFactory>

#include <QAction>
#include <QCursor>
#include <QHeaderView>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QScopedPointer>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <QWeakPointer>
#include <QtDBus/QtDBus>

using namespace Kerfuffle;

K_PLUGIN_FACTORY(Factory, registerPlugin<Ark::Part>();)
K_EXPORT_PLUGIN(Factory("ark"))

namespace Ark
{

static quint32 s_instanceCounter = 1;

Part::Part(QWidget *parentWidget, QObject *parent, const QVariantList& args)
        : KParts::ReadWritePart(parent),
          m_splitter(0),
          m_busy(false),
          m_jobTracker(0)
{
    Q_UNUSED(args)
    setComponentData(Factory::componentData(), false);

    new DndExtractAdaptor(this);

    const QString pathName = QString(QLatin1String("/DndExtract/%1")).arg(s_instanceCounter++);
    if (!QDBusConnection::sessionBus().registerObject(pathName, this)) {
        kFatal() << "Could not register a D-Bus object for drag'n'drop";
    }

    m_model = new ArchiveModel(pathName, this);

    m_splitter = new QSplitter(Qt::Horizontal, parentWidget);
    setWidget(m_splitter);

    m_view = new ArchiveView;
    m_infoPanel = new InfoPanel(m_model);

    m_splitter->addWidget(m_view);
    m_splitter->addWidget(m_infoPanel);

    QList<int> splitterSizes = ArkSettings::splitterSizes();
    if (splitterSizes.isEmpty()) {
        splitterSizes.append(200);
        splitterSizes.append(100);
    }
    m_splitter->setSizes(splitterSizes);

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

    setXMLFile(QLatin1String( "ark_part.rc" ));
}

Part::~Part()
{
    factory()->removeClient(this);

    saveSplitterSizes();

    m_extractFilesAction->menu()->deleteLater();
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
    kDebug() << "Extract to " << localPath;
    if (!m_model) {
        return;
    }

    if (m_view->selectionModel()->selectedRows().count() != 1) {
        m_view->selectionModel()->setCurrentIndex(m_view->currentIndex(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    if (m_view->selectionModel()->selectedRows().count() != 1) {
        return;
    }

    QVariant internalRoot;
    kDebug() << "valid " << m_view->currentIndex().parent().isValid();
    if (m_view->currentIndex().parent().isValid()) {
        internalRoot = m_model->entryForIndex(m_view->currentIndex().parent()).value(InternalID);
    }

    if (internalRoot.isNull()) {
        //we have the special case valid parent, but the parent does not
        //actually correspond to an item in the archive, but an automatically
        //created folder. for now, we will just use the filename of the node
        //instead, but for plugins that rely on a non-filename value as the
        //InternalId, this WILL break things. TODO find a solution
        internalRoot = m_model->entryForIndex(m_view->currentIndex().parent()).value(FileName);
    }

    QList<QVariant> files = selectedFilesWithChildren();
    if (files.isEmpty()) {
        return;
    }

    kDebug() << "selected files are " << files;
    Kerfuffle::ExtractionOptions options;
    options[QLatin1String( "PreservePaths" )] = true;
    if (!internalRoot.isNull()) {
        options[QLatin1String("RootNode")] = internalRoot;
    }

    ExtractJob *job = m_model->extractFiles(files, localPath, options);
    registerJob(job);

    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(slotExtractionDone(KJob*)));

    job->start();
}

void Part::setupView()
{
    m_view->setModel(m_model);

    m_view->setSortingEnabled(true);

    connect(m_view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateActions()));
    connect(m_view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    //TODO: fix an actual eventhandler
    connect(m_view, SIGNAL(itemTriggered(QModelIndex)),
            this, SLOT(slotPreview(QModelIndex)));

    connect(m_model, SIGNAL(columnsInserted(QModelIndex,int,int)),
            this, SLOT(adjustColumns()));
}

void Part::setupActions()
{
    KToggleAction *showInfoPanelAction = new KToggleAction(i18nc("@action:inmenu", "Show information panel"), this);
    actionCollection()->addAction(QLatin1String( "show-infopanel" ), showInfoPanelAction);
    showInfoPanelAction->setChecked(m_splitter->sizes().at(1) > 0);
    connect(showInfoPanelAction, SIGNAL(triggered(bool)),
            this, SLOT(slotToggleInfoPanel(bool)));

    m_saveAsAction = KStandardAction::saveAs(this, SLOT(slotSaveAs()), actionCollection());

    m_previewAction = actionCollection()->addAction(QLatin1String( "preview" ));
    m_previewAction->setText(i18nc("to preview a file inside an archive", "Pre&view"));
    m_previewAction->setIcon(KIcon( QLatin1String( "document-preview-archive" )));
    m_previewAction->setStatusTip(i18n("Click to preview the selected file"));
    m_previewAction->setShortcuts(QList<QKeySequence>() << Qt::Key_Return << Qt::Key_Space);
    connect(m_previewAction, SIGNAL(triggered(bool)),
            this, SLOT(slotPreview()));

    m_extractFilesAction = actionCollection()->addAction(QLatin1String( "extract" ));
    m_extractFilesAction->setText(i18n("E&xtract"));
    m_extractFilesAction->setIcon(KIcon( QLatin1String( "archive-extract" )));
    m_extractFilesAction->setStatusTip(i18n("Click to open an extraction dialog, where you can choose to extract either all files or just the selected ones"));
    m_extractFilesAction->setShortcut(QKeySequence( QLatin1String( "Ctrl+E" ) ));
    connect(m_extractFilesAction, SIGNAL(triggered(bool)),
            this, SLOT(slotExtractFiles()));

    m_addFilesAction = actionCollection()->addAction(QLatin1String( "add" ));
    m_addFilesAction->setIcon(KIcon( QLatin1String( "archive-insert" )));
    m_addFilesAction->setText(i18n("Add &File..."));
    m_addFilesAction->setStatusTip(i18n("Click to add files to the archive"));
    connect(m_addFilesAction, SIGNAL(triggered(bool)),
            this, SLOT(slotAddFiles()));

    m_addDirAction = actionCollection()->addAction(QLatin1String( "add-dir" ));
    m_addDirAction->setIcon(KIcon( QLatin1String( "archive-insert-directory" )));
    m_addDirAction->setText(i18n("Add Fo&lder..."));
    m_addDirAction->setStatusTip(i18n("Click to add a folder to the archive"));
    connect(m_addDirAction, SIGNAL(triggered(bool)),
            this, SLOT(slotAddDir()));

    m_deleteFilesAction = actionCollection()->addAction(QLatin1String( "delete" ));
    m_deleteFilesAction->setIcon(KIcon( QLatin1String( "archive-remove" )));
    m_deleteFilesAction->setText(i18n("De&lete"));
    m_deleteFilesAction->setShortcut(Qt::Key_Delete);
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
        header->setIcon(KIcon( QLatin1String( "archive-extract" )));
    }

    while (menu->actions().size() > 3) {
        menu->removeAction(menu->actions().last());
    }

    const KConfigGroup conf(KGlobal::config(), "DirSelect Dialog");
    const QStringList dirHistory = conf.readPathEntry("History Items", QStringList());

    for (int i = 0; i < qMin(10, dirHistory.size()); ++i) {
        const KUrl dirUrl(dirHistory.at(i));
        QAction *newAction = menu->addAction(dirUrl.pathOrUrl());
        newAction->setData(dirUrl.pathOrUrl());
    }
}

void Part::slotQuickExtractFiles(QAction *triggeredAction)
{
    // #190507: triggeredAction->data.isNull() means it's the "Extract to..."
    //          action, and we do not want it to run here
    if (!triggeredAction->data().isNull()) {
        kDebug() << "Extract to " << triggeredAction->data().toString();

        const QString userDestination = triggeredAction->data().toString();
        QString finalDestinationDirectory;
        const QString detectedSubfolder = detectSubfolder();

        if (!isSingleFolderArchive()) {
            finalDestinationDirectory = userDestination +
                                        QDir::separator() + detectedSubfolder;
            QDir(userDestination).mkdir(detectedSubfolder);
        } else {
            finalDestinationDirectory = userDestination;
        }

        Kerfuffle::ExtractionOptions options;
        options[QLatin1String( "PreservePaths" )] = true;
        QList<QVariant> files = selectedFiles();
        ExtractJob *job = m_model->extractFiles(files, finalDestinationDirectory, options);
        registerJob(job);

        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotExtractionDone(KJob*)));

        job->start();
    }
}

bool Part::isPreviewable(const QModelIndex& index) const
{
    return index.isValid() && (!m_model->entryForIndex(index)[ IsDirectory ].toBool());
}

void Part::selectionChanged()
{
    m_infoPanel->setIndexes(m_view->selectionModel()->selectedRows());
}

KAboutData* Part::createAboutData()
{
    return new KAboutData("ark", 0, ki18n("ArkPart"), "3.0");
}

bool Part::openFile()
{
    const QString localFile(localFilePath());
    const QFileInfo localFileInfo(localFile);
    const bool creatingNewArchive =
        arguments().metaData()[QLatin1String("createNewArchive")] == QLatin1String("true");

    if (localFileInfo.isDir()) {
        KMessageBox::error(NULL, i18nc("@info",
                                       "<filename>%1</filename> is a directory.",
                                       localFile));
        return false;
    }

    if (creatingNewArchive) {
        if (localFileInfo.exists()) {
            int overwrite =  KMessageBox::questionYesNo(NULL, i18nc("@info", "The archive <filename>%1</filename> already exists. Would you like to open it instead?", localFile), i18nc("@title:window", "File Exists"), KGuiItem(i18n("Open File")), KStandardGuiItem::cancel());

            if (overwrite == KMessageBox::No) {
                return false;
            }
        }
    } else {
        if (!localFileInfo.exists()) {
            KMessageBox::sorry(NULL, i18nc("@info", "The archive <filename>%1</filename> was not found.", localFile), i18nc("@title:window", "Error Opening Archive"));
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

        foreach(const QString& mime, mimeTypeList) {
            KMimeType::Ptr mimePtr(KMimeType::mimeType(mime));
            if (mimePtr) {
                // Key = "application/zip", Value = "Zip Archive"
                mimeTypes[mime] = mimePtr->comment();
            }
        }

        QStringList mimeComments(mimeTypes.values());
        mimeComments.sort();

        bool ok;
        QString item;

        if (creatingNewArchive) {
            item = KInputDialog::getItem(i18nc("@title:window", "Invalid Archive Type"),
                                         i18nc("@info", "Ark cannot create archives of the type you have chosen.<nl/><nl/>Please choose another archive type below."),
                                         mimeComments, 0, false, &ok);
        } else {
            item = KInputDialog::getItem(i18nc("@title:window", "Unable to Determine Archive Type"),
                                         i18nc("@info", "Ark was unable to determine the archive type of the filename.<nl/><nl/>Please choose the correct archive type below."),
                                         mimeComments,
                                         0,
                                         false,
                                         &ok);
        }

        if ((!ok) || (item.isEmpty())) {
            return false;
        }

        archive.reset(Kerfuffle::Archive::create(localFile, mimeTypes.key(item), m_model));
    }

    if (!archive) {
        KMessageBox::sorry(NULL, i18nc("@info", "Ark was not able to open the archive <filename>%1</filename>. No plugin capable of handling the file was found.", localFile), i18nc("@title:window", "Error Opening Archive"));
        return false;
    }

    KJob *job = m_model->setArchive(archive.take());
    registerJob(job);
    job->start();
    m_infoPanel->setIndex(QModelIndex());

    if (arguments().metaData()[QLatin1String( "showExtractDialog" )] == QLatin1String( "true" )) {
        QTimer::singleShot(0, this, SLOT(slotExtractFiles()));
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

void Part::slotLoadingStarted()
{
}

void Part::slotLoadingFinished(KJob *job)
{
    kDebug();

    if (job->error()) {
        if (arguments().metaData()[QLatin1String( "createNewArchive" )] != QLatin1String( "true" )) {
            KMessageBox::sorry(NULL, i18nc("@info", "Loading the archive <filename>%1</filename> failed with the following error: <message>%2</message>", localFilePath(), job->errorText()), i18nc("@title:window", "Error Opening Archive"));

            // The file failed to open, so reset the open archive, info panel and caption.
            m_model->setArchive(NULL);

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
    kDebug();
    QApplication::restoreOverrideCursor();
    m_busy = false;
    m_view->setEnabled(true);
    updateActions();
}

void Part::setBusyGui()
{
    kDebug();
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

void Part::slotPreview()
{
    slotPreview(m_view->selectionModel()->currentIndex());
}

void Part::slotPreview(const QModelIndex & index)
{
    if (!isPreviewable(index)) {
        return;
    }

    const ArchiveEntry& entry =  m_model->entryForIndex(index);

    if (!entry.isEmpty()) {
        Kerfuffle::ExtractionOptions options;
        options[QLatin1String( "PreservePaths" )] = true;

        ExtractJob *job = m_model->extractFile(entry[ InternalID ], m_previewDir.name(), options);
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

        QString fullName =
            m_previewDir.name() + QLatin1Char('/') + entry[FileName].toString();

        // Make sure a maliciously crafted archive with parent folders named ".." do
        // not cause the previewed file path to be located outside the temporary
        // directory, resulting in a directory traversal issue.
        fullName.remove(QLatin1String("../"));

        ArkViewer::view(fullName, widget());
    } else {
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

    QWeakPointer<Kerfuffle::ExtractionDialog> dialog = new Kerfuffle::ExtractionDialog;

    if (m_view->selectionModel()->selectedRows().count() > 0) {
        dialog.data()->setShowSelectedFiles(true);
    }

    dialog.data()->setSingleFolderArchive(isSingleFolderArchive());
    dialog.data()->setSubfolder(detectSubfolder());

    dialog.data()->setCurrentUrl(QFileInfo(m_model->archive()->fileName()).path());

    if (dialog.data()->exec()) {
        //this is done to update the quick extract menu
        updateActions();

        QVariantList files;

        //if the user has chosen to extract only selected entries, fetch these
        //from the listview
        if (!dialog.data()->extractAllFiles()) {
            files = selectedFilesWithChildren();
        }

        kDebug() << "Selected " << files;

        Kerfuffle::ExtractionOptions options;

        if (dialog.data()->preservePaths()) {
            options[QLatin1String("PreservePaths")] = true;
        }

        options[QLatin1String("FollowExtractionDialogSettings")] = true;

        const QString destinationDirectory = dialog.data()->destinationDirectory().pathOrUrl();
        ExtractJob *job = m_model->extractFiles(files, destinationDirectory, options);
        registerJob(job);

        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(slotExtractionDone(KJob*)));

        job->start();
    }

    delete dialog.data();
}

QList<QVariant> Part::selectedFilesWithChildren()
{
    Q_ASSERT(m_model);

    QModelIndexList toIterate = m_view->selectionModel()->selectedRows();

    for (int i = 0; i < toIterate.size(); ++i) {
        QModelIndex index = toIterate.at(i);

        for (int j = 0; j < m_model->rowCount(index); ++j) {
            QModelIndex child = m_model->index(j, 0, index);
            if (!toIterate.contains(child)) {
                toIterate << child;
            }
        }
    }

    QVariantList ret;
    foreach(const QModelIndex & index, toIterate) {
        const ArchiveEntry& entry = m_model->entryForIndex(index);
        if (entry.contains(InternalID)) {
            ret << entry[ InternalID ];
        }
    }
    return ret;
}

QList<QVariant> Part::selectedFiles()
{
    QStringList toSort;

    foreach(const QModelIndex & index, m_view->selectionModel()->selectedRows()) {
        const ArchiveEntry& entry = m_model->entryForIndex(index);
        toSort << entry[ InternalID ].toString();
    }

    toSort.sort();
    QVariantList ret;
    foreach(const QString &i, toSort) {
        ret << i;
    }
    return ret;
}

void Part::slotExtractionDone(KJob* job)
{
    kDebug();
    if (job->error()) {
        KMessageBox::error(widget(), job->errorString());
    } else {
        ExtractJob *extractJob = qobject_cast<ExtractJob*>(job);
        Q_ASSERT(extractJob);

        const bool followExtractionDialogSettings =
            extractJob->extractionOptions().value(QLatin1String("FollowExtractionDialogSettings"), false).toBool();
        if (!followExtractionDialogSettings) {
            return;
        }

        if (ArkSettings::openDestinationFolderAfterExtraction()) {

            KUrl destinationDirectory(extractJob->destinationDirectory());
            destinationDirectory.cleanPath();

            KRun::runUrl(destinationDirectory, QLatin1String("inode/directory"), widget());
        }

        if (ArkSettings::closeAfterExtraction()) {
           emit quit();
        }
    }
}

void Part::adjustColumns()
{
    kDebug();

    m_view->header()->setResizeMode(0, QHeaderView::ResizeToContents);
}

void Part::slotAddFiles(const QStringList& filesToAdd, const QString& path)
{
    if (filesToAdd.isEmpty()) {
        return;
    }

    kDebug() << "Adding " << filesToAdd << " to " << path;
    kDebug() << "Warning, for now the path argument is not implemented";

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
    if (firstPath.right(1) == QLatin1String( "/" )) {
        firstPath.chop(1);
    }
    firstPath = QFileInfo(firstPath).dir().absolutePath();

    kDebug() << "Detected relative path to be " << firstPath;
    options[QLatin1String( "GlobalWorkDir" )] = firstPath;

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
    kDebug();

    // #264819: passing widget() as the parent will not work as expected.
    //          KFileDialog will create a KFileWidget, which runs an internal
    //          event loop to stat the given directory. This, in turn, leads to
    //          events being delivered to widget(), which is a QSplitter, which
    //          in turn reimplements childEvent() and will end up calling
    //          QWidget::show() on the KFileDialog (thus showing it in a
    //          non-modal state).
    //          When KFileDialog::exec() is called, the widget is already shown
    //          and nothing happens.
    const QStringList filesToAdd =
        KFileDialog::getOpenFileNames(KUrl("kfiledialog:///ArkAddFiles"),
                                      QString(), widget()->parentWidget(),
                                      i18nc("@title:window", "Add Files"));

    slotAddFiles(filesToAdd);
}

void Part::slotAddDir()
{
    kDebug();
    const QString dirToAdd = KFileDialog::getExistingDirectory(KUrl("kfiledialog:///ArkAddFiles"), widget(), i18nc("@title:window", "Add Folder"));

    if (!dirToAdd.isEmpty()) {
        slotAddFiles(QStringList() << dirToAdd);
    }
}

void Part::slotAddFilesDone(KJob* job)
{
    kDebug();
    if (job->error()) {
        KMessageBox::error(widget(), job->errorString());
    }
}

void Part::slotDeleteFilesDone(KJob* job)
{
    kDebug();
    if (job->error()) {
        KMessageBox::error(widget(), job->errorString());
    }
}

void Part::slotDeleteFiles()
{
    kDebug();

    const int reallyDelete =
        KMessageBox::questionYesNo(NULL,
                                   i18n("Deleting these files is not undoable. Are you sure you want to do this?"),
                                   i18nc("@title:window", "Delete files"),
                                   KStandardGuiItem::del(),
                                   KStandardGuiItem::cancel(),
                                   QString(),
                                   KMessageBox::Dangerous | KMessageBox::Notify);

    if (reallyDelete == KMessageBox::No) {
        return;
    }

    DeleteJob *job = m_model->deleteFiles(selectedFilesWithChildren());
    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(slotDeleteFilesDone(KJob*)));
    registerJob(job);
    job->start();
}

void Part::slotToggleInfoPanel(bool visible)
{
    QList<int> splitterSizes;

    if (visible) {
        splitterSizes = ArkSettings::splitterSizesWithBothWidgets();
    } else {
        splitterSizes = m_splitter->sizes();
        ArkSettings::setSplitterSizesWithBothWidgets(splitterSizes);
        splitterSizes[1] = 0;
    }

    m_splitter->setSizes(splitterSizes);
    saveSplitterSizes();
}

void Part::saveSplitterSizes()
{
    ArkSettings::setSplitterSizes(m_splitter->sizes());
    ArkSettings::self()->writeConfig();
}

void Part::slotSaveAs()
{
    KUrl saveUrl = KFileDialog::getSaveUrl(KUrl(QLatin1String( "kfiledialog:///ArkSaveAs/" ) + url().fileName()), QString(), widget());

    if ((saveUrl.isValid()) && (!saveUrl.isEmpty())) {
        if (KIO::NetAccess::exists(saveUrl, KIO::NetAccess::DestinationSide, widget())) {
            int overwrite = KMessageBox::warningContinueCancel(widget(),
                                                               i18nc("@info", "An archive named <filename>%1</filename> already exists. Are you sure you want to overwrite it?", saveUrl.fileName()),
                                                               QString(),
                                                               KStandardGuiItem::overwrite());

            if (overwrite != KMessageBox::Continue) {
                return;
            }
        }

        KUrl srcUrl = KUrl::fromPath(localFilePath());

        if (!QFile::exists(localFilePath())) {
            if (url().isLocalFile()) {
                KMessageBox::error(widget(),
                                   i18nc("@info", "The archive <filename>%1</filename> cannot be copied to the specified location. The archive does not exist anymore.", localFilePath()));

                return;
            } else {
                srcUrl = url();
            }
        }

        KIO::Job *copyJob = KIO::file_copy(srcUrl, saveUrl, -1, KIO::Overwrite);

        if (!KIO::NetAccess::synchronousRun(copyJob, widget())) {
            KMessageBox::error(widget(),
                               i18nc("@info", "The archive could not be saved as <filename>%1</filename>. Try saving it to another location.", saveUrl.pathOrUrl()));
        }
    }
}

} // namespace Ark
