/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#include <stdlib.h>	// for getenv

// KDE includes
#include <kapp.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>

// ark includes
#include "arkdata.h"

// Key names in the arkrc config file
#define ARK_KEY "ark"
#define FAVORITE_KEY "ArchiveDirectory"
#define TAR_KEY "TarExe"
#define RECENT_KEY "Recent"

#define START_DIR_KEY "startDir"
#define OPEN_DIR_KEY "openDir"
#define EXTRACT_DIR_KEY "extractDir"
#define ADD_DIR_KEY "addDir"
#define LAST_OPEN_DIR_KEY "lastOpenDir"
#define LAST_EXTRACT_DIR_KEY "lastExtractDir"
#define LAST_ADD_DIR_KEY "lastAddDir"

#define START_MODE_KEY "startDirMode"
#define OPEN_MODE_KEY "openDirMode"
#define EXTRACT_MODE_KEY "extractDirMode"
#define ADD_MODE_KEY "addDirMode"

#define SAVE_ON_EXIT_KEY "saveOnExit"

/**
 * Constructs an ArkData object by reading the ark config file
 */
ArkData::ArkData()
{
	kc = KGlobal::config();
	readConfiguration();	
}

ArkData::~ArkData()
{

}

////////////////// READ CONFIGURATION ///////////////////////////////////

void ArkData::readConfiguration() {

        kc->setGroup( ARK_KEY );

	tar_exe = kc->readEntry( TAR_KEY, "tar");
	kdebug(0, 1601, "Tar command is %s", tar_exe.ascii());

	m_saveOnExit = kc->readBoolEntry( SAVE_ON_EXIT_KEY, true );
	kdebug(0, 1601, "SaveOnExit is %d", m_saveOnExit);

	readRecentFiles();
	readDirectories();
}



void ArkData::readRecentFiles()
{
	kdebug(0, 1601, "+readRecentFiles");
	
	QString s, name;
	kc->setGroup( ARK_KEY );
	for (int i=0; i < MAX_RECENT_FILES; i++)
	{
		name = QString("%1%2").arg(RECENT_KEY).arg(i);
		s = kc->readEntry(name);

		kdebug(0, 1601, "key %s is %s", name.ascii(), s.ascii());

		if (!s.isEmpty())
			recentFiles.append(s.ascii());
	}
	
	kdebug(0, 1601, "-readRecentFiles");

}

void ArkData::readDirectories()
{
	kdebug(0, 1601, "+readDirectories");
	
	kc->setGroup( ARK_KEY );

	favoriteDir = kc->readEntry( FAVORITE_KEY );

	if( favoriteDir.isEmpty() )
        	favoriteDir = getenv( "HOME" );
	
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
	
	kdebug(0 , 1601, "favorite dir is %s", favoriteDir.ascii() );
	kdebug(0 , 1601, "last open dir is %s", lastOpenDir.ascii() );
	kdebug(0 , 1601, "last xtr dir is %s", lastExtractDir.ascii() );
	kdebug(0 , 1601, "last add dir is %s", lastAddDir.ascii() );
	
	kdebug(0 , 1601, "start dir is %s", startDir.ascii() );
	kdebug(0 , 1601, "open dir is %s", openDir.ascii() );
	kdebug(0 , 1601, "xtr dir is %s", extractDir.ascii() );
	kdebug(0 , 1601, "add dir is %s", addDir.ascii() );

	kdebug(0 , 1601, "start mode is %d", startDirMode );		
	kdebug(0 , 1601, "open mode is %d", openDirMode );		
	kdebug(0 , 1601, "xtr mode is %d", extractDirMode );		
	kdebug(0 , 1601, "add mode is %d", addDirMode );		
	
	kdebug(0, 1601, "-readDirectories");
}


////////////////// WRITE CONFIGURATION ///////////////////////////////////

void ArkData::writeConfiguration()
{

	kdebug(0, 1601, "+writeConfiguration");

	if( !m_saveOnExit ){
		kdebug(0, 1601, "Don't save the config (exit)");

		writeRecentFiles();

		kc->setGroup( ARK_KEY );
		kc->writeEntry( SAVE_ON_EXIT_KEY, m_saveOnExit );
	}
	else{
		writeConfigurationNow();
	}
	kdebug(0, 1601, "-writeConfiguration");
}

void ArkData::writeConfigurationNow()
{
	kdebug(0, 1601, "+writeConfigurationNow");
	
	writeRecentFiles();
	writeDirectories();
	
	kc->setGroup( ARK_KEY );
	kc->writeEntry( TAR_KEY, tar_exe );
	kc->writeEntry( SAVE_ON_EXIT_KEY, m_saveOnExit );

	kdebug(0, 1601, "-writeConfigurationNow");
}

void ArkData::writeDirectories()
{
	kdebug(0 , 1601, "+writeDirectories");
	
	kc->setGroup( ARK_KEY );
	
	kc->writeEntry( FAVORITE_KEY, favoriteDir );

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

	kdebug(0 , 1601, "favorite dir is %s", favoriteDir.ascii() );

       	kdebug(0 , 1601, "last open dir is %s", lastOpenDir.ascii() );
	kdebug(0 , 1601, "last xtr dir is %s", lastExtractDir.ascii() );
	kdebug(0 , 1601, "last add dir is %s", lastAddDir.ascii() );
	
	kdebug(0 , 1601, "start dir is %s", startDir.ascii() );
	kdebug(0 , 1601, "open dir is %s", openDir.ascii() );
	kdebug(0 , 1601, "xtr dir is %s", extractDir.ascii() );
	kdebug(0 , 1601, "add dir is %s", addDir.ascii() );

	kdebug(0 , 1601, "start mode is %d", startDirMode );		
	kdebug(0 , 1601, "open mode is %d", openDirMode );		
	kdebug(0 , 1601, "xtr mode is %d", extractDirMode );		
	kdebug(0 , 1601, "add mode is %d", addDirMode );		

	kdebug(0 , 1601, "-writeDirectories");
}

void ArkData::writeRecentFiles()
{
	kdebug(0, 1601, "+writeRecentFiles");
	
	QString s, name;
	kc->setGroup( ARK_KEY );
	uint nb_recent = recentFiles.count();

	for (uint i=0; i < nb_recent; i++)
	{
		// name.sprintf("%s%d", RECENT_KEY, i);
		name = QString("%1%2").arg(RECENT_KEY).arg(i);
		kc->writeEntry(name, recentFiles.at(i));

		kdebug(0, 1601, "key %s is %s", name.ascii(), recentFiles.at(i));
	}
	
	kdebug(0, 1601, "-writeRecentFiles");
}


//////////////////////////////////////////////////////////////////////////

QString ArkData::getTarCommand() const
{
	return QString(tar_exe);
}

void ArkData::setTarCommand(const QString& _cmd)
{
	tar_exe = _cmd;
}


QString ArkData::getFavoriteDir() const
{
    return QString(favoriteDir);
}

void ArkData::setFavoriteDir(const QString& _dir)
{
        favoriteDir = _dir;
}


QStrList * ArkData::getRecentFiles()
{
	return &recentFiles;
}

void ArkData::addRecentFile(const QString& _filename)
{
	uint nb = recentFiles.count();
	uint i=0;

	while (i<nb)
	{
		if( recentFiles.at(i) == _filename ){
			recentFiles.remove(i);			
			kdebug(0 , 1601, "found and removed");
		}
        	i++;
	}	
	recentFiles.insert(0, _filename.ascii());
	if (recentFiles.count() > MAX_RECENT_FILES)
		recentFiles.removeLast();
	
	kdebug(0 , 1601, "-addRecentFile");
}

QString ArkData::getStartDir() const
{
    switch(startDirMode){
    	case LAST_OPEN_DIR : return QString(lastOpenDir);
    	case FIXED_START_DIR : return QString(startDir);
    	case FAVORITE_DIR : return QString(favoriteDir);
    	default : kdebug(0 , 1601, "Error in switch !"); return QString::null;
    }
}

QString ArkData::getFixedStartDir() const
{
	return QString( startDir );
}

int ArkData::getStartDirMode() const
{
	return startDirMode;
}

void ArkData::setStartDirCfg(const QString& dir, int mode)
{
    startDir = dir;
    startDirMode = mode;
}

QString ArkData::getOpenDir() const
{
    switch(openDirMode){
    	case LAST_OPEN_DIR : return QString(lastOpenDir);
    	case FIXED_OPEN_DIR : return QString(openDir);
    	case FAVORITE_DIR : return QString(favoriteDir);
    	default : kdebug(0, 1601, "Error in switch !"); return QString::null;
    }
}

QString ArkData::getFixedOpenDir() const
{
	return QString( openDir );
}

int ArkData::getOpenDirMode() const
{
	return openDirMode;
}

void ArkData::setLastOpenDir(const QString& dir)
{
	lastOpenDir = dir;
	kdebug(0, 1601, "last open dir is %s", dir.ascii());
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
    	default : kdebug(0, 1601, "Error in switch !"); return QString::null;
    }
}

QString ArkData::getFixedExtractDir() const
{
	return QString( extractDir );
}

int ArkData::getExtractDirMode() const
{
	return extractDirMode;
}

void ArkData::setLastExtractDir(const QString& dir)
{
	lastExtractDir = dir;
}

void ArkData::setExtractDirCfg(const QString& dir, int mode)
{
	extractDir = dir;
	extractDirMode = mode;
}

QString ArkData::getAddDir()
{
	switch(addDirMode){
		case LAST_ADD_DIR : return QString(lastAddDir);
		case FIXED_ADD_DIR : return QString(addDir);
		case FAVORITE_DIR : return QString(favoriteDir);
		default : kdebug(0 , 1601, "Error in switch !"); return QString::null;
	}
}

QString ArkData::getFixedAddDir() const
{
	return QString( addDir );
}

int ArkData::getAddDirMode() const
{
	return addDirMode;
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

QString ArkData::getSelectRegExp() const
{
	return m_regExp;
}

void ArkData::setSelectRegExp(const QString& _exp)
{	
	m_regExp = _exp;
}

const QString ArkData::getFilter()
{
	return i18n(
		"*.zip *.tar.gz *.tar.bz2|All valid archives\n"
		"*.zip|Zip archive (*.zip)\n"
		"*.tar.gz *.tgz |Tar compressed with gzip (*.tar.gz *.tgz)\n"
		"*.tbz2 *.tar.bz2|Tar compressed with bzip2 (*.tar.bz2 *.tbz2)\n" 
		); 
}


void ArkData::setaddPath( bool b)
{
	addPath = b;
}


void ArkData::setSaveOnExitChecked( bool _b )
{
	m_saveOnExit = _b;
}

bool ArkData::isSaveOnExitChecked()
{
	return m_saveOnExit;
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
