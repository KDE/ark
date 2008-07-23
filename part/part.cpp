/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
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
#include "infopanel.h"
#include "arkviewer.h"
#include "extractiondialog.h"
#include "kerfuffle/jobs.h"
#include "settings.h"
#include "jobtracker.h"

#include <KParts/GenericFactory>
#include <KApplication>
#include <KAboutData>
#include <KDebug>
#include <KAction>
#include <KSelectAction>
#include <KActionCollection>
#include <KIcon>
#include <KTempDir>
#include <KMessageBox>
#include <KVBox>
#include <KRun>
#include <KFileDialog>
#include <KConfigGroup>


#include <QTreeView>
#include <QCursor>
#include <QAction>
#include <QSplitter>
#include <QVBoxLayout>
#include <QTimer>
#include <QMenu>

typedef KParts::GenericFactory<Part> Factory;
K_EXPORT_COMPONENT_FACTORY( libarkpart, Factory )

Part::Part( QWidget *parentWidget, QObject *parent, const QStringList& args )
	: KParts::ReadWritePart( parent ), m_model( new ArchiveModel( this ) ), m_previewDir( 0 ), m_busy( false )
{
	Q_UNUSED( args );
	setComponentData( Factory::componentData() );
	setXMLFile( "ark_part.rc" );

	KVBox *mainWidget = new KVBox( parentWidget );
	setWidget( mainWidget );

	QSplitter *splitter = new QSplitter( Qt::Horizontal, mainWidget );
	m_view = new QTreeView( mainWidget );
	m_infoPanel = new InfoPanel( m_model, mainWidget );
	splitter->addWidget( m_view );
	splitter->addWidget( m_infoPanel );

	setupView();
	setupActions();

	connect( m_model, SIGNAL( loadingStarted() ),
	         this, SLOT( slotLoadingStarted() ) );
	connect( m_model, SIGNAL( loadingFinished() ),
	         this, SLOT( slotLoadingFinished() ) );
	connect( m_model, SIGNAL( error( const QString&, const QString& ) ),
	         this, SLOT( slotError( const QString&, const QString& ) ) );

	m_statusBarExtension = new KParts::StatusBarExtension( this );
	QTimer::singleShot( 0, this, SLOT( createJobTracker() ) );
}

Part::~Part()
{
}

void Part::createJobTracker()
{
	m_jobTracker = new JobTracker;
	m_model->setJobTracker( m_jobTracker );
	m_statusBarExtension->addStatusBarItem( m_jobTracker->widget(0), 0, true );
	m_jobTracker->widget(0)->hide();
}

void Part::setupView()
{
	m_view->setSelectionMode( QAbstractItemView::ExtendedSelection );
	m_view->setModel( m_model );
	m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	m_view->setAlternatingRowColors( true );
	m_view->setAnimated( true );
	m_view->setColumnWidth( 0, 150 );
	m_view->setAllColumnsShowFocus( true );

	connect( m_view->selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
	         this, SLOT( updateActions() ) );
	connect( m_view->selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
	         this, SLOT( selectionChanged() ) );
	connect( m_view, SIGNAL( activated( const QModelIndex & ) ),
	         this, SLOT( slotPreview( const QModelIndex & ) ) );
	connect( m_model, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex& ) ),
	         this, SLOT( adjustColumns( const QModelIndex &, const QModelIndex& ) ) );
}

void Part::setupActions()
{
	m_previewAction = actionCollection()->addAction( "preview" );
	m_previewAction->setText( i18nc( "to preview a file inside an archive", "Pre&view" ) );
	m_previewAction->setIcon( KIcon( "document-preview-archive" ) );
	m_previewAction->setStatusTip( i18n( "Click to preview the selected file" ) );
	connect( m_previewAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotPreview() ) );

	m_extractFilesAction = actionCollection()->addAction( "extract" );
	m_extractFilesAction->setText( i18n( "E&xtract..." ) );
	m_extractFilesAction->setIcon( KIcon( "archive-extract" ) );
	m_extractFilesAction->setStatusTip( i18n( "Click to open an extraction dialog, where you can choose to extract either all files or just the selected ones" ) );
	connect( m_extractFilesAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotExtractFiles() ) );

	m_addFilesAction = actionCollection()->addAction( "add" );
	m_addFilesAction->setIcon( KIcon( "archive-insert" ) );
	m_addFilesAction->setText( i18n( "Add &File..." ) );
	m_addFilesAction->setStatusTip( i18n( "Click to add files to the archive" ) );
	connect( m_addFilesAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotAddFiles() ) );

	m_addDirAction = actionCollection()->addAction( "add-dir" );
	m_addDirAction->setIcon( KIcon( "archive-insert-directory" ) );
	m_addDirAction->setText( i18n( "Add Fo&lder..." ) );
	m_addDirAction->setStatusTip( i18n( "Click to add a folder to the archive" ) );
	connect( m_addDirAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotAddDir() ) );

	m_deleteFilesAction = actionCollection()->addAction( "delete" );
	m_deleteFilesAction->setIcon( KIcon( "archive-remove" ) );
	m_deleteFilesAction->setText( i18n( "De&lete" ) );
	m_deleteFilesAction->setStatusTip( i18n( "Click to delete the selected files" ) );
	connect( m_deleteFilesAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotDeleteFiles() ) );

	updateActions();
}

void Part::updateActions()
{
	bool isWritable = m_model->archive() && ( !m_model->archive()->isReadOnly() );

	m_previewAction->setEnabled( !isBusy() && ( m_view->selectionModel()->selectedRows().count() == 1 )
	                             && isPreviewable( m_view->selectionModel()->currentIndex() ) );
	m_extractFilesAction->setEnabled( !isBusy() && ( m_model->rowCount() > 0 ) );
	m_addFilesAction->setEnabled( !isBusy() && isWritable );
	m_addDirAction->setEnabled( !isBusy() && isWritable );
	m_deleteFilesAction->setEnabled( !isBusy() && ( m_view->selectionModel()->selectedRows().count() > 0 )
	                                 && isWritable );

	QMenu *m = m_extractFilesAction->menu();
	if (!m)
	{
		m = new QMenu("Recent folders");
		m_extractFilesAction->setMenu(m);
		connect(m, SIGNAL(triggered(QAction*)),
				this, SLOT(slotQuickExtractFiles(QAction*)));
		QAction *header = m->addAction(i18n("Quick extract to..."));
		header->setEnabled(false);
		header->setIcon(KIcon( "archive-extract" ) );
	}

	while (m->actions().size() > 1)
	{
		m->removeAction(m->actions().last());
	}

	KConfigGroup conf( KGlobal::config(), "DirSelect Dialog" );
	QStringList dirHistory = conf.readPathEntry( "History Items", QStringList() );

	for(int i = 0; i < dirHistory.size(); ++i)
	{
		QAction *newAction = m->addAction(KUrl(dirHistory.at(i)).path());
		newAction->setData(KUrl(dirHistory.at(i)).path());
	}


}

void Part::slotQuickExtractFiles(QAction *triggeredAction)
{
	kDebug() << "Extract to " << triggeredAction->data().toString();

	QString userDestination = triggeredAction->data().toString();
	QString finalDestinationDirectory;
	QString detectedSubfolder = detectSubfolder();

	if (!isSingleFolderArchive())
	{
		finalDestinationDirectory = userDestination + 
			QDir::separator() + detectedSubfolder;
		QDir( userDestination ).mkdir(detectedSubfolder);
	}
	else finalDestinationDirectory = userDestination;

	QList<QVariant> files = selectedFiles();
	ExtractJob *job = m_model->extractFiles( files, finalDestinationDirectory, true );
	m_jobTracker->registerJob( job );

	connect( job, SIGNAL( result( KJob* ) ),
			this, SLOT( slotExtractionDone( KJob * ) ) );

	job->start();

}

bool Part::isPreviewable( const QModelIndex & index )
{
	return index.isValid() && ( !m_model->entryForIndex( index )[ IsDirectory ].toBool() );
}

void Part::selectionChanged()
{
	m_infoPanel->setIndexes( m_view->selectionModel()->selectedRows() );
}

KAboutData* Part::createAboutData()
{
	return new KAboutData( "ark", 0, ki18n( "ArkPart" ), "3.0" );
}

bool Part::openFile()
{
	Kerfuffle::Archive *archive = Kerfuffle::factory( localFilePath() );
	m_model->setArchive( archive );
	m_infoPanel->setIndex( QModelIndex() );

	if (archive != 0 && arguments().metaData()["showExtractDialog"] == "true")
	{
		QTimer::singleShot( 0, this, SLOT( slotExtractFiles() ) );
	}

	return ( archive != 0 );
}

bool Part::saveFile()
{
	return true;
}

QStringList Part::supportedMimeTypes() const
{
	return Kerfuffle::supportedMimeTypes();
}

QStringList Part::supportedWriteMimeTypes() const
{
	return Kerfuffle::supportedWriteMimeTypes();
}

void Part::slotLoadingStarted()
{
	QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
	m_busy = true;
	updateActions();
	emit busy();
}

void Part::slotLoadingFinished()
{
	QApplication::restoreOverrideCursor();
	m_busy = false;
	m_view->resizeColumnToContents( 0 );
	updateActions();
	emit ready();
}

void Part::slotPreview()
{
	slotPreview( m_view->selectionModel()->currentIndex() );
}

void Part::slotPreview( const QModelIndex & index )
{
	Q_ASSERT( m_previewDir == 0 );
	if ( !isPreviewable( index ) ) return;
	const ArchiveEntry& entry =  m_model->entryForIndex( index );
	if ( !entry.isEmpty() )
	{
		m_previewDir = new KTempDir();
		ExtractJob *job = m_model->extractFile( entry[ InternalID ], m_previewDir->name(), false );
		m_jobTracker->registerJob( job );
		connect( job, SIGNAL( result( KJob* ) ),
		         this, SLOT( slotPreviewExtracted( KJob* ) ) );
		job->start();
	}
}

void Part::slotPreviewExtracted( KJob *job )
{
	if ( !job->error() )
	{
		ArkViewer viewer( widget() );
		const ArchiveEntry& entry =  m_model->entryForIndex( m_view->selectionModel()->currentIndex() );
		QString name = entry[ FileName ].toString().split( '/', QString::SkipEmptyParts ).last();
		if ( !viewer.view( m_previewDir->name() + '/' + name ) )
		{
			KMessageBox::sorry( widget(), i18n( "The internal viewer cannot preview this file." ) );
		}
	}
	else
	{
		KMessageBox::error( widget(), job->errorString() );
	}
	delete m_previewDir;
	m_previewDir = 0;
}

void Part::slotError( const QString& errorMessage, const QString& details )
{
	if ( details.isEmpty() )
	{
		KMessageBox::error( widget(), errorMessage );
	}
	else
	{
		KMessageBox::detailedError( widget(), errorMessage, details );
	}
}

bool Part::isSingleFolderArchive()
{
	if (m_model->rowCount() == 1)
	{
		QModelIndex i = m_model->index(0,0);
		ArchiveEntry root = m_model->entryForIndex(i);

		if (root[IsDirectory].toBool())
		{
			return true;
		}
		
	}
	return false;
}

QString Part::detectSubfolder()
{
	if (!m_model) return QString();

	if (isSingleFolderArchive())
	{
		QModelIndex i = m_model->index(0,0);
		ArchiveEntry root = m_model->entryForIndex(i);

		return root[FileName].toString();
		
	}
	else
	{
		if (!m_model->archive()) return QString();
		QFileInfo f(m_model->archive()->fileName());
		return f.baseName();
	}
}

void Part::slotExtractFiles()
{
	kDebug( 1601 ) ;
	//TODO: return if there are no model currently set

	ExtractionDialog dialog;
	if ( m_view->selectionModel()->selectedRows().count() > 0 )
	{
		dialog.showSelectedFilesOption();
	}

	QString detectedSubfolder = detectSubfolder();
	dialog.setSubfolder(detectedSubfolder);

	if ( dialog.exec() )
	{
		ArkSettings::setOpenDestinationFolderAfterExtraction( dialog.openDestinationAfterExtraction() );
		ArkSettings::setLastExtractionFolder( dialog.destinationDirectory().path() );

		ArkSettings::self()->writeConfig();

		//this is done to update the quick extract menu
		updateActions();

		QString destinationDirectory;
		if (!isSingleFolderArchive())
		{
			destinationDirectory =  dialog.destinationDirectory().path() + 
				QDir::separator() + dialog.subfolder();
			QDir(dialog.destinationDirectory().path()).mkdir(dialog.subfolder());
		}
		else destinationDirectory = dialog.destinationDirectory().path();

		QList<QVariant> files = selectedFiles();
		ExtractJob *job = m_model->extractFiles( files, destinationDirectory, true );
		m_jobTracker->registerJob( job );

		connect( job, SIGNAL( result( KJob* ) ),
		         this, SLOT( slotExtractionDone( KJob * ) ) );

		job->start();
	}
}

QList<QVariant> Part::selectedFiles()
{
	QList<QVariant> files;

	foreach( const QModelIndex & index, m_view->selectionModel()->selectedRows() )
	{
		const ArchiveEntry& entry = m_model->entryForIndex( index );
		files << entry[ InternalID ];
	}

	return files;
}

void Part::slotExtractionDone( KJob* job )
{
	kDebug( 1601 ) ;
	if ( job->error() )
	{
		KMessageBox::error( widget(), job->errorString() );
	}
	else
	{
		if ( ArkSettings::openDestinationFolderAfterExtraction() )
		{
			KRun::runUrl( KUrl( ArkSettings::lastExtractionFolder() ), "inode/directory", widget() );
		}
	}
}

void Part::adjustColumns( const QModelIndex & topleft, const QModelIndex& bottomRight )
{
	kDebug( 1601 ) ;
	int firstColumn= topleft.column();
	int lastColumn = bottomRight.column();
	do
	{
		m_view->resizeColumnToContents(firstColumn);
		firstColumn++;
	} while (firstColumn < lastColumn);
}

void Part::slotAddFiles()
{
	kDebug( 1601 ) ;
	QStringList filesToAdd = KFileDialog::getOpenFileNames( KUrl( "kfiledialog:///ArkAddFiles" ), QString(), widget(), i18n( "Add Files" ) );

	if ( !filesToAdd.isEmpty() )
	{
		AddJob *job = m_model->addFiles( filesToAdd );
		connect( job, SIGNAL( result( KJob* ) ),
		         this, SLOT( slotAddFilesDone( KJob* ) ) );
		job->start();
	}
}

void Part::slotAddDir()
{
	kDebug( 1601 ) ;
	QString dirToAdd = KFileDialog::getExistingDirectory( KUrl( "kfiledialog:///ArkAddFiles" ), widget(), i18n( "Add Folder" ) );

	if ( !dirToAdd.isEmpty() )
	{
		QStringList list;
		list << dirToAdd;

		AddJob *job = m_model->addFiles( list );
		connect( job, SIGNAL( result( KJob* ) ),
		         this, SLOT( slotAddFilesDone( KJob* ) ) );
		job->start();
	}
}

void Part::slotAddFilesDone( KJob* job )
{
	kDebug( 1601 ) ;
	if ( job->error() )
	{
		KMessageBox::error( widget(), job->errorString() );
	}
}

void Part::slotDeleteFiles()
{
	kDebug( 1601 ) ;
	DeleteJob *job = m_model->deleteFiles( selectedFiles() );
	connect( job, SIGNAL( result( KJob* ) ),
	         this, SLOT( slotDeleteFilesDone( KJob* ) ) );
	job->start();
}
