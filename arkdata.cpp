#include <stdlib.h>
#include "iostream.h"

// KDE includes
#include <kapp.h>
#include <klocale.h>

// ark includes
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
	readDirectories();
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
	cerr << "Entered writeRecentFiles\n";
	
	QString s, name;
	kc->setGroup( ARK_KEY );
	uint nb_recent = recentFiles.count();

	for (uint i=0; i < nb_recent; i++)
	{
		name.sprintf("%s%d", RECENT_KEY, i);
		kc->writeEntry(name, recentFiles.at(i));
		cerr << "key " << name << " is " << recentFiles.at(i) << "\n";
	}
	
	cerr << "Exited writeRecentFiles\n";
}


void ArkData::readDirectories()
{
	kc->setGroup( ARK_KEY );
	
	startDir = kc->readEntry( START_DIR_KEY );
	openDir = kc->readEntry( OPEN_DIR_KEY );
	extractDir = kc->readEntry( EXTRACT_DIR_KEY );
	addDir = kc->readEntry( ADD_DIR_KEY );
	
	lastOpenDir = kc->readEntry( LAST_OPEN_DIR_KEY );
	lastExtractDir = kc->readEntry( LAST_EXTRACT_DIR_KEY );
	lastAddDir = kc->readEntry( LAST_ADD_DIR_KEY );
	
	startDirMode = kc->readNumEntry( START_MODE_KEY, LAST_OPEN_DIR);
	openDirMode = kc->readNumEntry( OPEN_MODE_KEY, LAST_OPEN_DIR);
	extractDirMode = kc->readNumEntry( EXTRACT_MODE_KEY, LAST_EXTRACT_DIR);
	addDirMode = kc->readNumEntry( ADD_MODE_KEY, LAST_ADD_DIR);
	
	cerr << "last open dir is " << lastOpenDir << "\n";
	cerr << "last xtr dir is " << lastExtractDir << "\n";
	cerr << "last add dir is " << lastAddDir << "\n";
	
	cerr << "start dir is " << startDir << "\n";
	cerr << "open dir is " << openDir << "\n";
	cerr << "xtr dir is " << extractDir << "\n";
	cerr << "add dir is " << addDir << "\n";

	cerr << "start mode is " << startDirMode << "\n";		
	cerr << "open mode is " << openDirMode << "\n";		
	cerr << "xtr mode is " << extractDirMode << "\n";		
	cerr << "add mode is " << addDirMode << "\n";		
}


void ArkData::writeDirectories()
{
	cerr << "Entered writeDirectories\n";
	
	kc->setGroup( ARK_KEY );
	
        kc->writeEntry(START_DIR_KEY, startDir);
        kc->writeEntry(OPEN_DIR_KEY, openDir);
        kc->writeEntry(EXTRACT_DIR_KEY, extractDir);
        kc->writeEntry(ADD_DIR_KEY, addDir);
        kc->writeEntry(LAST_OPEN_DIR_KEY, lastOpenDir);
        kc->writeEntry(LAST_EXTRACT_DIR_KEY, lastExtractDir);
        kc->writeEntry(LAST_ADD_DIR_KEY, lastAddDir);

        kc->writeEntry(START_MODE_KEY, startDirMode);
        kc->writeEntry(OPEN_MODE_KEY, openDirMode);
        kc->writeEntry(EXTRACT_MODE_KEY, extractDirMode);
        kc->writeEntry(ADD_MODE_KEY, addDirMode);

	cerr << "Exited writeDirectories\n";
}


QString ArkData::getTarCommand() const
{
	return QString(tar_exe);
}

void ArkData::setTarCommand(const QString& cmd)
{
        kc->setGroup( ARK_KEY );
	tar_exe = cmd;
        kc->writeEntry( TAR_KEY, cmd );
}


QString ArkData::getFavoriteDir() const
{
    return QString(favoriteDir);
}

void ArkData::setFavoriteDir(const QString& fd)
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

QString ArkData::getStartDir()
{
    switch(startDirMode){
    	case LAST_OPEN_DIR : return QString(lastOpenDir);
    	case FIXED_START_DIR : return QString(startDir);
    	case FAVORITE_DIR : return QString(favoriteDir);
    	default : cerr << "Error in switch !\n"; return QString::null;
    }
}

void ArkData::setStartDirCfg(const QString& dir, int mode)
{
    startDir = dir;
    startDirMode = mode;
}

QString ArkData::getOpenDir()
{
    switch(openDirMode){
    	case LAST_OPEN_DIR : return QString(lastOpenDir);
    	case FIXED_OPEN_DIR : return QString(openDir);
    	case FAVORITE_DIR : return QString(favoriteDir);
    	default : cerr << "Error in switch !\n"; return QString::null;
    }
}

void ArkData::setLastOpenDir(const QString& dir)
{
    lastOpenDir = dir;
    cerr << "last open dir is " << dir << "\n";
}

void ArkData::setOpenDirCfg(const QString& dir, int mode)
{
    openDir = dir;
    openDirMode = mode;
}

QString ArkData::getExtractDir()
{
    switch(extractDirMode){
    	case LAST_EXTRACT_DIR : return QString(lastExtractDir);
    	case FIXED_EXTRACT_DIR : return QString(extractDir);
    	case FAVORITE_DIR : return QString(favoriteDir);
    	default : cerr << "Error in switch !\n"; return QString::null;
    }
}

void ArkData::setLastExtractDir(const QString& dir)
{
    lastExtractDir = dir;
}

void ArkData::setExtractDirCfg(const QString& dir, int mode)
{
    lastExtractDir = dir;
    extractDirMode = mode;
}

QString ArkData::getAddDir()
{
    switch(addDirMode){
    	case LAST_ADD_DIR : return QString(lastAddDir);
    	case FIXED_ADD_DIR : return QString(addDir);
    	case FAVORITE_DIR : return QString(favoriteDir);
    	default : cerr << "Error in switch !\n"; return QString::null;
    }
}

void ArkData::setLastAddDir(const QString& dir)
{
    lastAddDir = dir;
}

void ArkData::setAddDirCfg(const QString& dir, int mode)
{
    addDir = dir;
    addDirMode = mode;
}


const QString ArkData::getFilter()
{
	return QString(i18n(
			"*.zip *.tar.gz *.tar.bz2|All valid archives\n"
			"*.zip|Zip archive (*.zip)\n"
	            "*.tar.gz *.tgz |Tar compressed with gzip (*.tar.gz *.tgz)\n"
        	    "*.tbz2 *.tar.bz2|Tar compressed with bzip2 (*.tar.bz2 *.tbz2)\n"
			));
}

void ArkData::writeConfiguration(){
	
	writeRecentFiles();
	writeDirectories();
	
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

void ArkData::setaddPath( bool b)
{
	addPath = b;
}

void ArkData::setonlyUpdate( bool b)
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
