#include <stdlib.h>
#include "iostream.h"

// Qt includes
#include <qstring.h>

// KDE includes
#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>

#include "arkdata.h"


/**
 * Constructs an ArkData object by reading the ark config file
 */
ArkData::ArkData()
{
	kc = kapp->getConfig();
	readConfigFile();	
}

void ArkData::readConfigFile() {

        kc->setGroup( ARK_KEY );

    favoriteDir = kc->readEntry( FAVORITE_KEY );

    if( favoriteDir.isEmpty() )
        favoriteDir = getenv( "HOME" );
    cerr << "Favorite dir is " << favoriteDir.ascii() << "\n";

	tar_exe = kc->readEntry( TAR_KEY, "tar");
	cerr << "Tar command is " << tar_exe.ascii() << "\n";

    startDir = kc->readEntry( START_DIR_KEY );
    openDir = kc->readEntry( OPEN_DIR_KEY );
    extractDir = kc->readEntry( EXTRACT_DIR_KEY );
    addDir = kc->readEntry( ADD_DIR_KEY );

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
    return QString(favoriteDir);
}

void ArkData::setFavoriteDir(const QString fd)
{
        kc->setGroup( ARK_KEY );
        favoriteDir = fd;
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

const QString ArkData::getAddDir()
{
    return QString(addDir);
}

void ArkData::setAddDir(const QString dir)
{
    addDir = dir;
}

const QString ArkData::getExtractDir()
{
    return QString(extractDir);
}

void ArkData::setExtractDir(const QString dir)
{
    extractDir = dir;
}

const QString ArkData::getOpenDir()
{
    return QString(openDir);
}

void ArkData::setOpenDir(const QString dir)
{
    openDir = dir;
}

const QString ArkData::getStartDir()
{
    return QString(startDir);
}

void ArkData::setStartDir(const QString dir)
{
    startDir = dir;
}


const QString ArkData::getFilter()
{
	return QString(i18n("*.zip *.tar.gz *.tar.bz2 *.ar *.lha|All valid archives\n"
			"*.zip|Zip archive (*.zip)\n"
            "*.tar.gz *.tgz |Tar compressed with gzip (*.tar.gz *.tgz)\n"
            "*.tbz2 *.tar.bz2|Tar compressed with bzip2 (*.tar.bz2 *.tbz2)\n"
			"*.ar|Ar archive (*.ar)\n"
			"*.lha|Lha archive (*.lha)"));
}

void ArkData::writeConfiguration(){
	
	writeRecentFiles();
    kc->setGroup( ARK_KEY );
    kc->writeEntry( TAR_KEY, tar_exe );
    kc->writeEntry( FAVORITE_KEY, favoriteDir );

    kc->writeEntry( START_DIR_KEY, startDir);
    kc->writeEntry( OPEN_DIR_KEY, openDir);
    kc->writeEntry( EXTRACT_DIR_KEY, extractDir);
    kc->writeEntry( ADD_DIR_KEY, addDir);
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

void ArkData::setaddPath( bool& b)
{
	addPath = b;
}

void ArkData::setonlyUpdate( bool& b)
{
	onlyUpdate = b;
}

bool ArkData::getonlyUpdate()
{
	return onlyUpdate;
}

bool ArkData::getaddPath()
{
	return addPath;
}
