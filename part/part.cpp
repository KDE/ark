#include "part.h"
#include "archivemodel.h"

#include <KParts/GenericFactory>
#include <KAboutData>
#include <QTreeView>

typedef KParts::GenericFactory<Part> Factory;
K_EXPORT_COMPONENT_FACTORY( libarkpartnew, Factory );

Part::Part( QWidget *parentWidget, QObject *parent, const QStringList& args )
	: KParts::ReadWritePart( parent ), m_model( 0 ), m_view( new QTreeView( parentWidget ) )
{
	Q_UNUSED( args );
	setComponentData( Factory::componentData() );
	setXMLFile( "ark_part_new.rc" );
	setWidget( m_view );
}

Part::~Part()
{
}

KAboutData* Part::createAboutData()
{
	return new KAboutData( "ark", 0, ki18n( "ArkPart" ), "3.0" );
}

bool Part::openFile()
{
	delete m_model;
	m_model = new ArchiveModel( Arch::factory( localFilePath() ), this );
	m_view->setModel( m_model );

	return true;
}

bool Part::saveFile()
{
	return true;
}
