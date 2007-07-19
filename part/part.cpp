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

#include <KParts/GenericFactory>
#include <KApplication>
#include <KAboutData>
#include <KDebug>
#include <KActionCollection>
#include <KIcon>
#include <KTempDir>
#include <KDebug>
#include <KMessageBox>

#include <QTreeView>
#include <QCursor>
#include <QAction>
#include <QSplitter>

typedef KParts::GenericFactory<Part> Factory;
K_EXPORT_COMPONENT_FACTORY( libarkpart, Factory );

Part::Part( QWidget *parentWidget, QObject *parent, const QStringList& args )
	: KParts::ReadWritePart( parent ), m_model( new ArchiveModel( this ) ), m_previewDir( 0 )
{
	Q_UNUSED( args );
	setComponentData( Factory::componentData() );
	setXMLFile( "ark_part.rc" );

	QSplitter *mainWidget = new QSplitter( Qt::Horizontal, parentWidget );
	setWidget( mainWidget );
	m_view = new QTreeView( parentWidget );
	m_infoPanel = new InfoPanel( parentWidget );
	mainWidget->addWidget( m_view );
	mainWidget->addWidget( m_infoPanel );

	setupView();
	setupActions();

	connect( m_model, SIGNAL( loadingStarted() ),
	         this, SLOT( slotLoadingStarted() ) );
	connect( m_model, SIGNAL( loadingFinished() ),
	         this, SLOT( slotLoadingFinished() ) );
	connect( m_model, SIGNAL( error( const QString&, const QString& ) ),
	         this, SLOT( slotError( const QString&, const QString& ) ) );
}

Part::~Part()
{
}

void Part::setupView()
{
	m_view->setSelectionMode( QAbstractItemView::ExtendedSelection );
	m_view->setModel( m_model );
	m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	connect( m_view->selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
	         this, SLOT( updateActions() ) );
	connect( m_view->selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
	         this, SLOT( selectionChanged() ) );
	connect( m_view, SIGNAL( activated( const QModelIndex & ) ),
	         this, SLOT( slotPreview( const QModelIndex & ) ) );
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

	updateActions();
}

void Part::updateActions()
{
	m_previewAction->setEnabled( m_view->selectionModel()->currentIndex().isValid() );
}

void Part::selectionChanged()
{
	m_infoPanel->setEntry( m_model->entryForIndex( m_view->selectionModel()->currentIndex() ) );
}

KAboutData* Part::createAboutData()
{
	return new KAboutData( "ark", 0, ki18n( "ArkPart" ), "3.0" );
}

bool Part::openFile()
{
	Arch *archive = Arch::factory( localFilePath() );
	m_model->setArchive( archive );
	m_infoPanel->setEntry( ArchiveEntry() );

	return ( archive != 0 );
}

bool Part::saveFile()
{
	return true;
}

QStringList Part::supportedMimeTypes() const
{
	return Arch::supportedMimeTypes();
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
		connect( m_model, SIGNAL( extractionFinished( bool ) ),
		         this, SLOT( slotPreviewExtracted( bool ) ) );
		m_model->extractFile( entry[ FileName ], m_previewDir->name() );
	}
}

void Part::slotPreviewExtracted( bool success )
{
	disconnect( m_model, SIGNAL( extractionFinished( bool ) ),
	            this, SLOT( slotPreviewExtracted( bool ) ) );
	if ( success )
	{
		ArkViewer viewer( widget() );
		const ArchiveEntry& entry =  m_model->entryForIndex( m_view->selectionModel()->currentIndex() );
		if ( !viewer.view( m_previewDir->name() + '/' + entry[ FileName ].toString() ) )
		{
			KMessageBox::sorry( widget(), i18n( "The internal viewer cannot preview this file." ) );
		}
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

void Part::slotExtractFiles()
{
	kDebug( 1601 ) << k_funcinfo << endl;
	foreach( const QModelIndex &index, m_view->selectionModel()->selectedRows() )
	{
		const ArchiveEntry & entry = m_model->entryForIndex( index );
		kDebug( 1601 ) << k_funcinfo << "Entry: " << entry[ FileName ].toString() << endl;
	}
	ExtractionDialog dialog;
	if ( dialog.exec() )
	{
		kDebug( 1601 ) << k_funcinfo << "implement me!" << endl;
	}
}
