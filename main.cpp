/*

ark: A program for modifying archives via a GUI.

Copyright (C)
2002-2003: Helio Chissini de Castro <helio@conectiva.com.br>
2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

//
// Note: This is a KUniqueApplication.
// To debug add --nofork to the command line.
// Be aware that newInstance() will not be called in this case, but you
// can run ark from a console, and that will invoke it in the debugger.
//

// C includes
#include <stdlib.h>

// KDE includes
#include <kdebug.h>
#include <dcopclient.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kdelibs_export.h>
// ark includes
#include "arkapp.h"

static KCmdLineOptions option[] =
{
    { "extract", I18N_NOOP( "Open extract dialog, quit when finished" ), 0 },
    { "extract-to", I18N_NOOP( "Extract 'archive' to 'folder'. Quit when finished.\n"
                               "'folder' will be created if it does not exist."), 0 },
    { "add", I18N_NOOP( "Ask for the name of the archive to add 'files' to. Quit when finished." ), 0 },
    { "add-to", I18N_NOOP( "Add 'files' to 'archive'. Quit when finished.\n'archive' "
                           "will be created if it does not exist." ), 0 },
    { "guess-name", I18N_NOOP( "Used with '--extract-to'. When specified, 'archive'\n"
                               "will be extracted to a subfolder of 'folder'\n"
                               "whose name will be the name of 'archive' without the filename extension."), 0 },
    { "+[folder]", I18N_NOOP( "Folder to extract to" ), 0 },
    { "+[files]", I18N_NOOP( "Files to be added" ), 0 },
    { "+[archive]", I18N_NOOP( "Open 'archive'" ), 0 },
    KCmdLineLastOption
};

extern "C" KDE_EXPORT int kdemain( int argc, char *argv[]  )
{
	KAboutData aboutData( "ark", I18N_NOOP( "Ark" ),
	                      "2.6.4", I18N_NOOP( "KDE Archiving tool" ),
	                      KAboutData::License_GPL,
	                      I18N_NOOP( "(c) 1997-2006, The Various Ark Developers" )
	                    );

	aboutData.addAuthor( "Henrique Pinto",
	                     I18N_NOOP( "Maintainer" ),
	                     "henrique.pinto@kdemail.net" );
	aboutData.addAuthor( "Charis Kouzinopoulos",
	                     0,
	                     "kouzinopoulos@gmail.com" );
	aboutData.addAuthor( "Helio Chissini de Castro",
	                     I18N_NOOP( "Former maintainer" ),
	                     "helio@kde.org" );
	aboutData.addAuthor( "Georg Robbers",
	                     0,
	                     "Georg.Robbers@urz.uni-hd.de" );
	aboutData.addAuthor( "Roberto Selbach Teixeira",
	                     0,
	                     "maragato@kde.org" );
	aboutData.addAuthor( "Francois-Xavier Duranceau",
	                     0,
	                     "duranceau@kde.org" );
	aboutData.addAuthor( "Emily Ezust (Corel Corporation)",
	                     0,
	                     "emilye@corel.com" );
	aboutData.addAuthor( "Michael Jarrett (Corel Corporation)",
	                     0,
	                     "michaelj@corel.com" );
	aboutData.addAuthor( "Robert Palmbos",
	                     0,
	                     "palm9744@kettering.edu" );

	aboutData.addCredit( "Bryce Corkins",
	                     I18N_NOOP( "Icons" ),
	                     "dbryce@attglobal.net" );
	aboutData.addCredit( "Liam Smit",
	                     I18N_NOOP( "Ideas, help with the icons" ),
	                     "smitty@absamail.co.za" );

	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( option );
	KCmdLineArgs::addTempFileOption();

	if ( !ArkApplication::start() )
	{
		// Already running!
		kdDebug( 1601 ) << "Already running" << endl;
		exit( 0 );
	}

	if ( ArkApplication::getInstance()->isRestored() )
	{
		kdDebug( 1601 ) << "In main: Restore..." << endl;
		RESTORE( MainWindow );
	}

	return ArkApplication::getInstance()->exec();
}

// kate: space-indent off; tab-width 4;
