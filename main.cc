/*  KZip (c) 1997 Robert Palmbos  */
/*  All code is covered by the GNU Public License */

#include "kzip.h"
#include <kapp.h>

int main( int argc, char *argv[]  )
{
	QString Zip( "" );
	for( int i=1; i<argc; i++ )
	{
		if( strstr( "-caption", argv[i] ) )
			i++;
		else{		
			Zip = argv[i];
			break;
		}
	}
	KApplication kzip( argc, argv );
	KZipWidget *kzipwin = new KZipWidget;
	kzip.setMainWidget( kzipwin );
	//kzip.restoreTopLevelGeometry();
	kzipwin->show();
	//kzip.restoreTopLevelGeometry(); // This is the only way I could everything
	                                // to show up correctly

	if( !Zip.isEmpty() )
		kzipwin->showZip( Zip );
	kzip.exec();
}
