#include "mainwindow.h"

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
	m_part = KParts::ComponentFactory::createPartInstanceFromLibrary<KParts::ReadWritePart>( "libarkpartnew", this, this );
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
	KUrl url = KFileDialog::getOpenUrl();
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
