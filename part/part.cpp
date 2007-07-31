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
#include <KActionCollection>
#include <KIcon>
#include <KTempDir>
#include <KDebug>
#include <KMessageBox>
#include <KVBox>
#include <KRun>
#include <KFileDialog>

#include <QTreeView>
#include <QCursor>
#include <QAction>
#include <QSplitter>
#include <QVBoxLayout>

typedef KParts::GenericFactory<Part> Factory;
K_EXPORT_COMPONENT_FACTORY( libarkpart, Factory );

Part::Part( QWidget *parentWidget, QObject *parent, const QStringList& args )
	: KParts::ReadWritePart( parent ), m_model( new ArchiveModel( this ) ), m_previewDir( 0 )
{
	Q_UNUSED( args );
	setComponentData( Factory::componentData() );
	setXMLFile( "ark_part.rc" );



	KVBox *mainWidget = new KVBox( parentWidget );
	setWidget( mainWidget );

	m_jobTracker = new JobTracker( mainWidget );

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

	m_model->setJobTracker( m_jobTracker );
}

Part::~Part()
{
}

void Part::setupView()
{
	m_view->setSelectionMode( QAbstractItemView::ExtendedSelection );
	m_view->setSelectionBehavior( QAbstractItemView::SelectRows );
	m_view->setModel( m_model );
	m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	m_view->setAlternatingRowColors( true );
	m_view->setAnimated( true );

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
	m_previewAction->setIcon( KIcon( "ark-view" ) );
	m_previewAction->setStatusTip( i18n( "Click to preview the selected file" ) );
	connect( m_previewAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotPreview() ) );

	m_extractFilesAction = actionCollection()->addAction( "extract" );
	m_extractFilesAction->setText( i18n( "E&xtract..." ) );
	m_extractFilesAction->setIcon( KIcon( "ark-extract" ) );
	m_extractFilesAction->setStatusTip( i18n( "Click to open an extraction dialog, where you can choose to extract either all files or just the selected ones" ) );
	connect( m_extractFilesAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotExtractFiles() ) );

	m_addFilesAction = actionCollection()->addAction( "add" );
	m_addFilesAction->setIcon( KIcon( "ark-addfile" ) );
	m_addFilesAction->setText( i18n( "Add &File..." ) );
	m_addFilesAction->setStatusTip( i18n( "Click to add files to the archive" ) );
	connect( m_addFilesAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotAddFiles() ) );

	m_addDirAction = actionCollection()->addAction( "add-dir" );
	m_addDirAction->setIcon( KIcon( "ark-adddir" ) );
	m_addDirAction->setText( i18n( "Add Fo&lder..." ) );
	m_addDirAction->setStatusTip( i18n( "Click to add a folder to the archive" ) );
	connect( m_addDirAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotAddDir() ) );

	m_deleteFilesAction = actionCollection()->addAction( "delete" );
	m_deleteFilesAction->setIcon( KIcon( "ark-delete" ) );
	m_deleteFilesAction->setText( i18n( "De&lete" ) );
	m_deleteFilesAction->setStatusTip( i18n( "Click to delete the selected files" ) );
	connect( m_deleteFilesAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotDeleteFiles() ) );

	updateActions();
}

void Part::updateActions()
{
	bool isWritable = m_model->archive() && ( !m_model->archive()->isReadOnly() );

	m_previewAction->setEnabled( ( m_view->selectionModel()->selectedRows().count() == 1 ) &&isPreviewable( m_view->selectionModel()->currentIndex() ) );
	m_extractFilesAction->setEnabled( m_model->archive() );
	m_addFilesAction->setEnabled( isWritable );
	m_addDirAction->setEnabled( isWritable );
	m_deleteFilesAction->setEnabled( ( m_view->selectionModel()->selectedRows().count() > 0 )
	                                 && isWritable );
}

bool Part::isPreviewable( const QModelIndex & index )
{
	return index.isValid() && ( !m_model->entryForIndex( index )[ IsDirectory ].toBool() );
}

void Part::selectionChanged()
{
	m_infoPanel->setIndex( m_view->selectionModel()->currentIndex() );
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
	updateActions();

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

void Part::slotLoadingStarted()
{
	QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
}

void Part::slotLoadingFinished()
{
	QApplication::restoreOverrideCursor();
}

void Part::slotPreview()
{
	slotPreview( m_view->selectionModel()->currentIndex() );
}

void Part::slotPreview( const QModelIndex & index )
{
	Q_ASSERT( m_previewDir == 0 );
	const ArchiveEntry& entry =  m_model->entryForIndex( index );
	if ( !entry.isEmpty() )
	{
		m_previewDir = new KTempDir();
		ExtractJob *job = m_model->extractFile( entry[ OriginalFileName ], m_previewDir->name(), false );
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
	delete job;
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

void Part::slotExtractFiles()
{
	kDebug( 1601 ) << k_funcinfo << endl;

	ExtractionDialog dialog;
	if ( m_view->selectionModel()->selectedRows().count() > 0 )
	{
		dialog.showSelectedFilesOption();
	}

	if ( dialog.exec() )
	{
		ArkSettings::setOpenDestinationFolderAfterExtraction( dialog.openDestinationAfterExtraction() );
		ArkSettings::setLastExtractionFolder( dialog.destinationDirectory().path() );

		QList<QVariant> files = selectedFiles();
		ExtractJob *job = m_model->extractFiles( files, dialog.destinationDirectory().path(), false );
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
		files << entry[ OriginalFileName ];
	}

	return files;
}

void Part::slotExtractionDone( KJob* job )
{
	kDebug( 1601 ) << k_funcinfo << endl;
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
	kDebug( 1601 ) << k_funcinfo << endl;
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
	kDebug( 1601 ) << k_funcinfo << endl;
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
	kDebug( 1601 ) << k_funcinfo << endl;
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
	kDebug( 1601 ) << k_funcinfo << endl;
	if ( job->error() )
	{
		KMessageBox::error( widget(), job->errorString() );
	}
}

void Part::slotDeleteFiles()
{
	kDebug( 1601 ) << k_funcinfo << endl;
	DeleteJob *job = m_model->deleteFiles( selectedFiles() );
	connect( job, SIGNAL( result( KJob* ) ),
	         this, SLOT( slotDeleteFilesDone( KJob* ) ) );
	job->start();
}
