#include "kerfuffle/arch.h"
#include "archivemodel.h"
#include <QtGui>
#include <KApplication>
#include <kcmdlineargs.h>
#include <klocale.h>

int main( int argc, char **argv )
{
	KCmdLineArgs::init( argc,argv, "archivemodeltest", 0, ki18n( "archivemodeltest" ), 0 );
	KApplication app;

	qRegisterMetaType<ArchiveEntry>( "ArchiveEntry" );

	ArchiveModel model( Arch::archFactory( UNKNOWN_FORMAT, "/home/kde-devel/ark_test.tar.gz", "wololo" ) );
	QTreeView view;
	view.setModel( &model );
	view.show();

	return app.exec();
}
