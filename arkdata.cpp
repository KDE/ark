#include <stdlib.h>
#include "iostream.h"

// Qt includes
#include <qstring.h>

// KDE includes
#include <klocale.h>
#include <kconfig.h>
#include <kapp.h>

#include "arkdata.h"


ArkData::ArkData()
{
	kc = kapp->getConfig();
	readConfigFile();	
}

void ArkData::readConfigFile() {

        kc->setGroup( ARK_KEY );

	fav_dir = kc->readEntry( FAVORITE_KEY );

	if( fav_dir.isEmpty() )
		fav_dir = getenv( "HOME" );
	cerr << "Favorite dir is " << fav_dir.ascii() << "\n";

	tar_exe = kc->readEntry( TAR_KEY, "tar");
	cerr << "Tar command is " << tar_exe.ascii() << "\n";

	readRecentFiles();
//	QString startpoint;
//	startpoint = kc->readEntry( "CurrentLocation" );
//	
//	if( startpoint == "Favorites" )
//		showFavorite();
//	else
//		if( startpoint != "None" )
//			showZip( startpoint );
}

ArkData::~ArkData()
{

}


void ArkData::readRecentFiles()
{
	QString s, name;
	kc->setGroup( ARK_KEY );
	for (int i=0; i < MAX_RECENT_FILES; i++)
	{
		name.sprintf("%s%d", RECENT_KEY, i);
		s = kc->readEntry(name);
		cerr << "key " << name << " is " << s << "\n";
		if (!s.isEmpty())
			recentFiles.append(s);
	}

}

void ArkData::writeRecentFiles()
{
	QString s, name;
	kc->setGroup( ARK_KEY );
	uint nb_recent = recentFiles.count();

	for (uint i=0; i < nb_recent; i++)
	{
		name.sprintf("%s%d", RECENT_KEY, i);
		kc->writeEntry(name, recentFiles.at(i));
		cerr << "key " << name << " is " << recentFiles.at(i) << "\n";
	}

}

const QString ArkData::getTarCommand()
{
	return QString(tar_exe);
}

void ArkData::setTarCommand(const QString cmd)
{
        kc->setGroup( ARK_KEY );
	tar_exe = cmd;
        kc->writeEntry( TAR_KEY, cmd );
}


const QString ArkData::getFavoriteDir()
{
	return QString(fav_dir);
}

void ArkData::setFavoriteDir(const QString fd)
{
        kc->setGroup( ARK_KEY );
	fav_dir = fd;
        kc->writeEntry( FAVORITE_KEY, fd );
}


QStrList * ArkData::getRecentFiles()
{
	return &recentFiles;
}

void ArkData::addRecentFile(const QString& filename)
{
	uint nb = recentFiles.count();
	uint i=0;

	while (i<nb)
	{
		if( recentFiles.at(i) == filename ){
			recentFiles.remove(i);			
			cerr << "found and removed\n";
		}
        	i++;
	}	
	recentFiles.insert(0, filename);
	if (recentFiles.count() > MAX_RECENT_FILES)
		recentFiles.removeLast();
	
	cerr << "left addRecentFile\n";
}

const QString ArkData::getFilter()
{
	return QString(i18n("*.zip *.tar.gz *.tar.bz2 *.ar *.lha|All valid archives\n"
			"*.zip|Zip archive (*.zip)\n"
			"*.tar.gz|Tar compressed archive (*.tar.gz *.tar.bz2)\n"
			"*.ar|Ar archive (*.ar)\n"
			"*.lha|Lha archive (*.lha)"));
}

void ArkData::writeConfiguration(){
	
	writeRecentFiles();
}

/*
void ArkWidget::saveProperties( KConfig *kc ) {
	QString loc_key( "CurrentLocation" );
	
	if( arch != 0 )
		kc->writeEntry( loc_key, arch->getName() );
	else
		if( listing != 0 )
			kc->writeEntry( loc_key, "Favorites" );
		else
			kc->writeEntry( loc_key, "None" );
	
	// I would prefer to just delete all the widgets, but kwm gets confused
	// if ark quits in the middle of session management
	QString ex( "rm -rf "+tmpdir );
	system( ex );
}

void ArkWidget::readProperties( KConfig *kc ) {
	QString startpoint;
	startpoint = kc->readEntry( "CurrentLocation" );
	
	if( startpoint == "Favorites" )
		showFavorite();
	else
		if( startpoint != "None" )
			showZip( startpoint );
}

*/

