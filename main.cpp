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
#include <KDebug>
#include <KCmdLineArgs>
#include <KAboutData>
#include <KLocale>

// ark includes
#include "arkapp.h"

extern "C" KDE_EXPORT int kdemain( int argc, char *argv[]  )
{
	KAboutData aboutData( "ark", 0, ki18n( "Ark" ),
	                      "2.9.9001", ki18n( "KDE Archiving tool" ),
	                      KAboutData::License_GPL,
	                      ki18n( "(c) 1997-2005, The Various Ark Developers" )
	                    );

	aboutData.addAuthor( ki18n("Henrique Pinto"),
	                     ki18n( "Maintainer" ),
	                     "henrique.pinto@kdemail.net" );
	aboutData.addAuthor( ki18n("Helio Chissini de Castro"),
	                     ki18n( "Former maintainer" ),
	                     "helio@kde.org" );
	aboutData.addAuthor( ki18n("Georg Robbers"),
	                     KLocalizedString(),
	                     "Georg.Robbers@urz.uni-hd.de" );
	aboutData.addAuthor( ki18n("Roberto Selbach Teixeira"),
	                     KLocalizedString(),
	                     "maragato@kde.org" );
	aboutData.addAuthor( ki18n("Francois-Xavier Duranceau"),
	                     KLocalizedString(),
	                     "duranceau@kde.org" );
	aboutData.addAuthor( ki18n("Emily Ezust (Corel Corporation)"),
	                     KLocalizedString(),
	                     "emilye@corel.com" );
	aboutData.addAuthor( ki18n("Michael Jarrett (Corel Corporation)"),
	                     KLocalizedString(),
	                     "michaelj@corel.com" );
	aboutData.addAuthor( ki18n("Robert Palmbos"),
	                     KLocalizedString(),
	                     "palm9744@kettering.edu" );

	aboutData.addCredit( ki18n("Bryce Corkins"),
	                     ki18n( "Icons" ),
	                     "dbryce@attglobal.net" );
	aboutData.addCredit( ki18n("Liam Smit"),
	                     ki18n( "Ideas, help with the icons" ),
	                     "smitty@absamail.co.za" );

	KCmdLineArgs::init( argc, argv, &aboutData );

	KCmdLineOptions option;
	option.add("extract", ki18n( "Open extract dialog, quit when finished" ));
	option.add("extract-to", ki18n( "Extract 'archive' to 'folder'. Quit when finished.\n"
                               "'folder' will be created if it does not exist."));
	option.add("add", ki18n( "Ask for the name of the archive to add 'files' to. Quit when finished." ));
	option.add("add-to", ki18n( "Add 'files' to 'archive'. Quit when finished.\n'archive' "
                           "will be created if it does not exist." ));
	option.add("guess-name", ki18n( "Used with '--extract-to'. When specified, 'archive'\n"
                               "will be extracted to a subfolder of 'folder'\n"
                               "whose name will be the name of 'archive' without the filename extension."));
	option.add("+[folder]", ki18n( "Folder to extract to" ));
	option.add("+[files]", ki18n( "Files to be added" ));
	option.add("+[archive]", ki18n( "Open 'archive'" ));
	KCmdLineArgs::addCmdLineOptions( option );
	KCmdLineArgs::addTempFileOption();

	if ( !ArkApplication::start() )
	{
		// Already running!
		kDebug( 1601 ) << "Already running" << endl;
		exit( 0 );
	}

	if ( ArkApplication::getInstance()->isSessionRestored() )
	{
		kDebug( 1601 ) << "In main: Restore..." << endl;
		RESTORE( MainWindow );
	}

	return ArkApplication::getInstance()->exec();
}

// kate: space-indent off; tab-width 4;
