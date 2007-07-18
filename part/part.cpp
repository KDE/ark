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

#include <KParts/GenericFactory>
#include <KApplication>
#include <KAboutData>
#include <KDebug>
#include <KActionCollection>
#include <KIcon>
#include <KHBox>
#include <KDebug>

#include <QTreeView>
#include <QCursor>
#include <QAction>
#include <QSplitter>

typedef KParts::GenericFactory<Part> Factory;
K_EXPORT_COMPONENT_FACTORY( libarkpart, Factory );

Part::Part( QWidget *parentWidget, QObject *parent, const QStringList& args )
	: KParts::ReadWritePart( parent ), m_model( new ArchiveModel( this ) )
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
}

Part::~Part()
{
}

void Part::setupView()
{
	m_view->setSelectionMode( QAbstractItemView::ExtendedSelection );
	m_view->setModel( m_model );
	connect( m_view->selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
	         this, SLOT( updateActions() ) );
	connect( m_view->selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
	         this, SLOT( selectionChanged() ) );
}

void Part::setupActions()
{
	m_previewAction = actionCollection()->addAction( "preview" );
	m_previewAction->setText( i18nc( "to preview a file inside an archive", "Pre&view" ) );
	m_previewAction->setIcon( KIcon( "ark-view" ) );
	m_previewAction->setStatusTip( i18n( "Click to preview the selected file" ) );
	connect( m_previewAction, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotPreview() ) );

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
}
