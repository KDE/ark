/*
    KZip: A program for modifying archives via a GUI.
    Copyright (C) 1997 Robert Palmbos

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    I can be reached at: 
    
    robertp@macatawa.org

*/
#include "kzip.h"
#include <kapp.h>
#include <unistd.h>
#include <sys/param.h> 

int main( int argc, char *argv[]  )
{
	QString Zip( "" );
	for( int i=1; i<argc; i++ )
	{
		if( strstr( "-caption", argv[i] ) )
			i++;
		else{
			if( argv[i][0] == '/' )
				Zip = argv[i];
			else{
				char currentWD[MAXPATHLEN];
				getcwd(currentWD, MAXPATHLEN);
				(Zip = currentWD).append("/").append(argv[i]);
			}
			break;
		}
	}
	KApplication kzip( argc, argv );
	KZipWidget *kzipwin = new KZipWidget;
	//kzip.setMainWidget( kzipwin );
	kzipwin->show();

	if( !Zip.isEmpty() )
		kzipwin->showZip( Zip );
	kzip.exec();
}
