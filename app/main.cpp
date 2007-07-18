#include <KApplication>
#include <KCmdLineArgs>
#include <KLocale>
#include <KAboutData>
#include <QByteArray>

#include "mainwindow.h"

int main( int argc, char **argv )
{
	KAboutData aboutData( "ark", 0, ki18n( "Ark" ),
	                      "2.9.999", ki18n( "KDE Archiving tool" ),
	                      KAboutData::License_GPL,
	                      ki18n( "(c) 1997-2007, The Various Ark Developers" )
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

	KCmdLineArgs::init( argc, argv, &aboutData );

	KCmdLineOptions option;
	option.add("+[url]", ki18n( "URL of an archive to be opened" ));
	KCmdLineArgs::addCmdLineOptions( option );
	KCmdLineArgs::addTempFileOption();

	KApplication application;

	if ( application.isSessionRestored() )
	{
		RESTORE( MainWindow );
	}
	else
	{
		// TODO open the url if the user supplied one
		MainWindow *window = new MainWindow;
		window->show();
	}

	return application.exec();


}
