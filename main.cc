/*
    ark: A program for modifying archives via a GUI.

    Copyright (C)
    1997-1999 Robert Palmbos <palm9744@kettering.edu>
    1999 Francois-Xavier Duranceau <duranceau@kde.org>

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


*/

// ark includes
#include "arkwidget.h"

#include <iostream.h>
#include <unistd.h>
#include <sys/param.h> 

// KDE includes
#include <kapp.h>

int main( int argc, char *argv[]  )
{
    QString Zip( "" );
    KApplication ark( argc, argv, "ark" );

    if( ark.isRestored() )
    {
	RESTORE(ArkWidget);
    }
    else
    {
	for (int i = 1; i < argc; ++i)
	{
	    if (argv[i][0] == '/')
	    {
		Zip = argv[i];
		break;
	    }
	    else if (argv[i][0] == '-') // not that we have any options yet!
	    {
	    }
	    else
	    {
		char currentWD[MAXPATHLEN];
		getcwd(currentWD, MAXPATHLEN);
		(Zip = currentWD).append("/").append(argv[i]);
		break;
	    }
	}
	
	ArkWidget *arkWin = new ArkWidget;

	QObject::connect(qApp, SIGNAL(lastWindowClosed()), arkWin, SLOT(quit()));
 
	arkWin->show();
	if (!Zip.isEmpty())
	    arkWin->file_open(Zip);
	return ark.exec();
    }
}
