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
#include "mainwindow.h"
#include "part/interface.h"

#include <KParts/ComponentFactory>
#include <KMessageBox>
#include <KApplication>
#include <KLocale>
#include <KActionCollection>
#include <KStandardAction>
#include <KFileDialog>
#include <KRecentFilesAction>
#include <KGlobal>

MainWindow::MainWindow( QWidget * )
	: KParts::MainWindow( )
{
	setXMLFile( "arkui.rc" );
	m_part = KParts::ComponentFactory::createPartInstanceFromLibrary<KParts::ReadWritePart>( "libarkpart", this, this );
	if ( !m_part )
	{
		KMessageBox::error( this, i18n( "Unable to find Ark's KPart component, please check your installation." ) );
		return;
	}

	setupActions();
	statusBar();

	m_part->setObjectName( "ArkPart" );
	setCentralWidget( m_part->widget() );
	createGUI( m_part );

	if ( !initialGeometrySet() )
	{
		resize( 640, 480 );
	}
	setAutoSaveSettings( "MainWindow" );
	
}

MainWindow::~MainWindow()
{
	if ( m_recentFilesAction )
	{
		m_recentFilesAction->saveEntries( KGlobal::config()->group( "Recent Files" ) );
	}
}

void MainWindow::setupActions()
{
	KStandardAction::open( this, SLOT( openArchive() ), actionCollection() );
	KStandardAction::quit( this, SLOT( quit() ), actionCollection() );

	m_recentFilesAction = KStandardAction::openRecent( this, SLOT( openUrl( const KUrl& ) ), actionCollection() );
	m_recentFilesAction->setToolBarMode( KRecentFilesAction::MenuMode );
	m_recentFilesAction->setToolButtonPopupMode( QToolButton::DelayedPopup );
	m_recentFilesAction->setIconText( i18n( "Open" ) );
	m_recentFilesAction->setStatusTip( i18n( "Click to open an archive, click and hold to open a recently-opened archive" ) );
	m_recentFilesAction->setToolTip( i18n( "Open an archive" ) );
	m_recentFilesAction->loadEntries( KGlobal::config()->group( "Recent Files" ) );
	connect( m_recentFilesAction, SIGNAL( triggered() ),
	         this, SLOT( openArchive() ) );
}

void MainWindow::openArchive()
{
	Interface *iface = qobject_cast<Interface*>( m_part );
	Q_ASSERT( iface );
	KUrl url = KFileDialog::getOpenUrl( KUrl( "kfiledialog:///ArkOpenDir"),
	                                    iface->supportedMimeTypes().join( " " ),
	                                    this );
	openUrl( url );
}

void MainWindow::openUrl( const KUrl& url )
{
	if ( !url.isEmpty() )
	{
		if ( m_part->openUrl( url ) )
		{
			m_recentFilesAction->addUrl( url );
		}
		else
		{
			m_recentFilesAction->removeUrl( url );
		}
	}
}

void MainWindow::quit()
{
	kapp->closeAllWindows();
}
