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

#include <kapp.h>
#include <klocale.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <qmessagebox.h>
#include "arkwidget.h"
#include "arkapp.h"

int main( int argc, char *argv[]  )
{
    ArkApplication ark( argc, argv, "ark" );
    ArkWidget *arkWin = 0;
    QValueList<QCString> params;

     if (ark.isRestored())
     {
       kdebug(0, 1601, "In main: Restore...");
       RESTORE(ArkWidget);
     }
     else 
     {
       kdebug(0, 1601, "In main: New ArkWidget...");
	  arkWin = new ArkWidget();
     }
     
     ark.setMainWidget(arkWin);
     
     for (int i=0; i < argc; ++i)
     {
	  params.append(argv[i]);
     }
   
     ark.newInstance(params);

     DCOPClient *client = ark.dcopClient();

     kdebug(0, 1601, "Got client...");

     if (!client->attach()) 
       {
	 QMessageBox::warning(arkWin, i18n("Error connecting to DCOP server"),
	    i18n("There was an error connecting to the Desktop\n"
		 "communications server.  Please make sure that\n"
                 "the 'dcopserver' process has been started, and\n"
		 "then try again.\n"));
	 exit(1);
       }

     kdebug(0, 1601, "Registering...");

     client->registerAs("ark");

     kdebug(0, 1601, "Showing...");

     arkWin->show();

     kdebug(0, 1601, "Resizing...");

     arkWin->resize(640, 300);
     return ark.exec();
}
