/*
  -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*-

$Id$

ark: A program for modifying archives via a GUI.

Copyright (C)
2002: Helio Chissini de Castro <helio@conectiva.com.br>
2001: Roberto Selbach Teixeira <maragato@kde.org>
1999-2000: Corel Corporation (author: Emily Ezust  emilye@corel.com)
1999 Francois-Xavier Duranceau <duranceau@kde.org>
1997-1999 Robert Palmbos <palm9744@kettering.edu>

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

//
// Note: This is a KUniqueApplication.
// To debug add --nofork to the command line.
// Be aware that newInstance() will not be called in this case, but you
// can run ark from a console, and that will invoke it in the debugger.
//

// C includes
#include <stdlib.h>

// QT includes

// KDE includes
#include <klocale.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kio/job.h>
#include <kmainwindow.h>

// ark includes
#include "arkapp.h"
#include "arkwidget.h"

static KCmdLineOptions option[] =
{
    { "extract", I18N_NOOP( "Open extract dialog, quit when finished." ), 0},
    { "+[archive]", I18N_NOOP( "Open 'archive'" ), 0 },
    { 0, 0, 0 }
};

int main( int argc, char *argv[]  )
{
	KAboutData aboutData( "ark", I18N_NOOP( "Ark" ),
			"v2.1.9", I18N_NOOP("KDE Archiving tool"), 
			KAboutData::License_GPL,
			I18N_NOOP( "(c) 1997-2002, The Various Ark Developers" ) );
	
	aboutData.addAuthor( "Helio Chissini de Castro", I18N_NOOP( "Current maintainer" ),
			"helio@conectiva.com.br" );
	
	aboutData.addAuthor( "Roberto Teixeira", 0,
			"maragato@conectiva.com" );
	
	aboutData.addAuthor( "Francois-Xavier Duranceau", 0, "duranceau@kde.org" );
	
	aboutData.addAuthor( "Emily Ezust (Corel Corporation)", 0, "emilye@corel.com" );
	
	aboutData.addAuthor( "Michael Jarrett (Corel Corporation)", 0,
			"michaelj@corel.com" );
	
	aboutData.addAuthor( "Robert Palmbos", 0, "palm9744@kettering.edu" );
	
	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( option );
	
	kdDebug( 1601 ) << "Starting ark. argc=" << argc << "  First arg: " << (argc == 2 ? argv[1] : 0) << endl;
	
	if ( !ArkApplication::start() )
	{
		// Already running!
		kdDebug( 1601 ) << "Already running" << endl;
		exit( 0 );
	}
	
	if ( ArkApplication::getInstance()->isRestored() )
	{
		kdDebug( 1601 ) << "In main: Restore..." << endl;
		RESTORE( ArkWidget );
	}
	kdDebug( 1601 ) << "Starting ark..." << endl;
	
	return ArkApplication::getInstance()->exec();
}
