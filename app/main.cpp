/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include <KApplication>
#include <KCmdLineArgs>
#include <KDebug>
#include <KLocale>
#include <KAboutData>
#include <QByteArray>

#include "mainwindow.h"
#include "batchextract.h"

int main( int argc, char **argv )
{
	KAboutData aboutData( "ark", 0, ki18n( "Ark" ),
	                      "2.11.999", ki18n( "KDE Archiving tool" ),
	                      KAboutData::License_GPL,
	                      ki18n( "(c) 1997-2008, The Various Ark Developers" ),
	                      KLocalizedString(),
	                      "http://utils.kde.org/projects/ark"
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
	aboutData.addCredit( ki18n( "Andrew Smith" ),
	                     ki18n( "bkisofs code" ),
	                     QByteArray(),
	                     "http://littlesvr.ca/misc/contactandrew.php" );
	aboutData.setProgramIconName( "utilities-file-archiver" );

	KCmdLineArgs::init( argc, argv, &aboutData );

	KCmdLineOptions option;
	option.add("+[url]", ki18n( "URL of an archive to be opened" ));
	option.add("dialog", ki18n("Show the extract dialog after opening archive"));
	option.add("batch", ki18n("Use the batch interface instead of the usual dialog"));
	option.add("destination <file>", ki18n("Destination folder to extract to."));
	KCmdLineArgs::addCmdLineOptions( option );
	KCmdLineArgs::addTempFileOption();

	KApplication application;

	if ( application.isSessionRestored() ) {
		RESTORE( MainWindow );
	} else {
		// open any given URLs
		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

		if (args->isSet("batch")) {
			BatchExtract batchExtract;

			for (int i = 0; i < args->count(); ++i) {
				batchExtract.addInput(args->url(i));
			}

			if (args->isSet("dialog")) {
				if (!batchExtract.showExtractDialog()) {
					return 0;
				}
			}

			batchExtract.setDestinationFolder(args->getOption("destination"));

			batchExtract.startExtraction();
		} else {

			MainWindow *window = new MainWindow;
			if(!window->loadPart()) { // if loading the part fails 
				return -1;
			}

			if (args->count()) {
				kDebug() << "trying to open" << args->url(0);

				if (args->isSet("extract")) {
					window->setShowExtractDialog(true);
				}
				window->openUrl(args->url(0));
			}
			window->show();
		}
	}

	return application.exec();


}
