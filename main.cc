/*
    ark: A program for modifying archives via a GUI.
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
#include "arkwidget.h"
#include <config.h>
#include <kapp.h>
#include <unistd.h>
#include <sys/param.h> 

int main( int argc, char *argv[]  )
{
	QString Zip( "" );

	KApplication ark( argc, argv );

	if( ark.isRestored() )
	{
		int n=1;
		while( KTopLevelWidget::canBeRestored(n)){
			ArkWidget *arkWin = new ArkWidget();
			arkWin->restore(n);
			arkWin->show();
			n++;
		}
	} else {
		for( int i=1; i<argc; i++ )
		{
			if( argv[i][0] == '/' )
				Zip = argv[i];
			else{
				char currentWD[KDEMAXPATHLEN];
				getcwd(currentWD, KDEMAXPATHLEN);
				(Zip = currentWD).append("/").append(argv[i]);
			}
			break;
		}
		ArkWidget *arkWin = new ArkWidget();
		arkWin->show();
		if( !Zip.isEmpty() )
			arkWin->showZip( Zip );
	}

	ark.exec();
}
