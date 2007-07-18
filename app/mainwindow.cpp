#include "mainwindow.h"

#include <KParts/ComponentFactory>
#include <KMessageBox>
#include <KApplication>
#include <KLocale>
#include <KActionCollection>
#include <KStandardAction>
#include <KFileDialog>

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
}

void MainWindow::setupActions()
{
	KStandardAction::open( this, SLOT( openArchive() ), actionCollection() );
	KStandardAction::quit( this, SLOT( quit() ), actionCollection() );
}

void MainWindow::openArchive()
{
	KUrl url = KFileDialog::getOpenUrl();

	if ( !url.isEmpty() )
	{
		m_part->openUrl( url );
	}
}

void MainWindow::quit()
{
	kapp->closeAllWindows();
}
