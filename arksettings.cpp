/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (Emily Ezust, emilye@corel.com)

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
#include "arksettings.h"

// Key names in the arkrc config file
#define ARK_GROUP "ark"
#define TAR_GROUP "Tar"
#define ZIP_GROUP "Zip"

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

#define EXTRACT_OVERWRITE "extractOverwrite"
#define EXTRACT_JUNKPATHS "extractJunkPaths"
#define EXTRACT_LOWERCASE "extractLowerCase"

#define ADD_RECURSEDIRS "recurseDirs"
#define ADD_JUNKDIRS "junkDirs"
#define ADD_ABSPATHS "absPaths"
#define ADD_RELPATHS "relPaths"
#define ADD_MSDOS "forceMSDOS"
#define ADD_CONVERTLF "convertLF2CRLF"

#define SAVE_ON_EXIT_KEY "saveOnExit"

#define PRESERVE_PERMS "preservePerms"
#define TOLOWER "toLower"
#define REPLACEONLYNEWER "replaceOnlyNewer"
#define FULLPATHS "fullPaths"
#define TAR_OVERWRITE "tarOverwrite"

/**
 * Constructs an ArkSettings object by reading the ark config file
 */
ArkSettings::ArkSettings()
{
  m_lastShellOutput = new QString;
	
  kc = KGlobal::config();
  readConfiguration();	
}

ArkSettings::~ArkSettings()
{
  delete m_lastShellOutput;
  m_lastShellOutput = 0;
}

////////////////// READ CONFIGURATION ///////////////////////////////////

void ArkSettings::readConfiguration() 
{
  kDebugInfo( 1601, "+ArkSettings::readConfiguration()");

  kc->setGroup( ARK_GROUP );

  tar_exe = kc->readEntry( TAR_KEY, "tar");
  kDebugInfo( 1601, "Tar command is %s", tar_exe.ascii());


  m_saveOnExit = kc->readBoolEntry( SAVE_ON_EXIT_KEY, true );
  kDebugInfo( 1601, "SaveOnExit is %d", m_saveOnExit);

  fullPath = kc->readBoolEntry(FULLPATHS, false);
  replaceOnlyNewerFiles = kc->readBoolEntry(REPLACEONLYNEWER, false);

  readRecentFiles();
  readDirectories();

  kDebugInfo( 1601, "-ArkSettings::readConfiguration()");
}



void ArkSettings::readRecentFiles()
{
  kDebugInfo( 1601, "+readRecentFiles");
	
  QString s, name;
  kc->setGroup( ARK_GROUP );
  for (int i=0; i < MAX_RECENT_FILES; i++)
    {
      name = QString("%1%2").arg(RECENT_KEY).arg(i);
      s = kc->readEntry(name);

      kDebugInfo( 1601, "key %s is %s", name.ascii(), s.ascii());

      if (!s.isEmpty())
	recentFiles.append(s.local8Bit());
    }
	
  kDebugInfo( 1601, "-readRecentFiles");

}

void ArkSettings::readDirectories()
{
  kDebugInfo( 1601, "+readDirectories");
	
  kc->setGroup( ARK_GROUP );

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
	
  kDebugInfo( 1601, "favorite dir is %s", favoriteDir.ascii() );
  kDebugInfo( 1601, "last open dir is %s", lastOpenDir.ascii() );
  kDebugInfo( 1601, "last xtr dir is %s", lastExtractDir.ascii() );
  kDebugInfo( 1601, "last add dir is %s", lastAddDir.ascii() );
	
  kDebugInfo( 1601, "start dir is %s", startDir.ascii() );
  kDebugInfo( 1601, "open dir is %s", openDir.ascii() );
  kDebugInfo( 1601, "xtr dir is %s", extractDir.ascii() );
  kDebugInfo( 1601, "add dir is %s", addDir.ascii() );

  kDebugInfo( 1601, "start mode is %d", startDirMode );		
  kDebugInfo( 1601, "open mode is %d", openDirMode );		
  kDebugInfo( 1601, "xtr mode is %d", extractDirMode );		
  kDebugInfo( 1601, "add mode is %d", addDirMode );		
	
  kDebugInfo( 1601, "-readDirectories");
}

void ArkSettings::readTarProperties()
{
  kDebugInfo( 1601, "+readTarProperties");	
	
  kc->setGroup( TAR_GROUP );
  m_tarPreservePerms = kc->readBoolEntry(PRESERVE_PERMS, false);
  m_tarToLower = kc->readBoolEntry(TOLOWER, false);
  m_tarOverwrite = kc->readBoolEntry(TAR_OVERWRITE, false);

  kDebugInfo( 1601, "-readTarProperties");	
}

void ArkSettings::readZipProperties()
{
  kDebugInfo( 1601, "+readZipProperties");	
	
  kc->setGroup( ZIP_GROUP );

  m_zipExtractOverwrite = kc->readBoolEntry( EXTRACT_OVERWRITE, true );
  m_zipExtractJunkPaths = kc->readBoolEntry( EXTRACT_JUNKPATHS, false );
  m_zipExtractOverwrite = kc->readBoolEntry( EXTRACT_LOWERCASE, false );

  m_zipAddRecurseDirs = kc->readBoolEntry( ADD_RECURSEDIRS, false );
  m_zipAddJunkDirs = kc->readBoolEntry( ADD_JUNKDIRS, false );
  m_zipAddMSDOS = kc->readBoolEntry( ADD_MSDOS, false );
  m_zipAddConvertLF = kc->readBoolEntry( ADD_CONVERTLF, false );
	
  kDebugInfo( 1601, "-readZipProperties");	
}



////////////////// WRITE CONFIGURATION ///////////////////////////////////

void ArkSettings::writeConfiguration()
{

  kDebugInfo( 1601, "+writeConfiguration");

  if( !m_saveOnExit ){
    kDebugInfo( 1601, "Don't save the config (exit)");

    writeRecentFiles();

    kc->setGroup( ARK_GROUP );
    kc->writeEntry( SAVE_ON_EXIT_KEY, m_saveOnExit );
  }
  else
    {
      writeConfigurationNow();
    }
  kDebugInfo( 1601, "-writeConfiguration");
}

void ArkSettings::writeConfigurationNow()
{
  kDebugInfo( 1601, "+writeConfigurationNow");
	
  writeRecentFiles();
  writeDirectories();
  writeZipProperties();
  writeTarProperties();

  kc->setGroup( ARK_GROUP );
  kc->writeEntry( TAR_KEY, tar_exe );
  kc->writeEntry( SAVE_ON_EXIT_KEY, m_saveOnExit );
  kc->writeEntry(FULLPATHS, fullPath);
  kc->writeEntry(REPLACEONLYNEWER, replaceOnlyNewerFiles);

  kDebugInfo( 1601, "-writeConfigurationNow");
}

void ArkSettings::writeDirectories()
{
  kDebugInfo( 1601, "+writeDirectories");
	
  kc->setGroup( ARK_GROUP );
	
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

  kDebugInfo( 1601, "favorite dir is %s", favoriteDir.ascii() );

  kDebugInfo( 1601, "last open dir is %s", lastOpenDir.ascii() );
  kDebugInfo( 1601, "last xtr dir is %s", lastExtractDir.ascii() );
  kDebugInfo( 1601, "last add dir is %s", lastAddDir.ascii() );
	
  kDebugInfo( 1601, "start dir is %s", startDir.ascii() );
  kDebugInfo( 1601, "open dir is %s", openDir.ascii() );
  kDebugInfo( 1601, "xtr dir is %s", extractDir.ascii() );
  kDebugInfo( 1601, "add dir is %s", addDir.ascii() );

  kDebugInfo( 1601, "start mode is %d", startDirMode );		
  kDebugInfo( 1601, "open mode is %d", openDirMode );		
  kDebugInfo( 1601, "xtr mode is %d", extractDirMode );		
  kDebugInfo( 1601, "add mode is %d", addDirMode );		

  kDebugInfo( 1601, "-writeDirectories");
}

void ArkSettings::writeRecentFiles()
{
  kDebugInfo( 1601, "+writeRecentFiles");
	
  QString s, name;
  kc->setGroup( ARK_GROUP );
  uint nb_recent = recentFiles.count();

  for (uint i=0; i < nb_recent; i++)
    {
      // name.sprintf("%s%d", RECENT_KEY, i);
      name = QString("%1%2").arg(RECENT_KEY).arg(i);
      kc->writeEntry(name, recentFiles[i]);

      kDebugInfo( 1601, "key %s is %s", name.ascii(), recentFiles[i].ascii());
    }
	
  kDebugInfo( 1601, "-writeRecentFiles");
}

void ArkSettings::writeTarProperties()
{
  kDebugInfo(1601, "+ArkSettings::writeTarProperties");

  kc->setGroup( TAR_GROUP );

  kc->writeEntry(PRESERVE_PERMS, m_tarPreservePerms);
  kc->writeEntry(TOLOWER, m_tarToLower);
  kc->writeEntry(TAR_OVERWRITE, m_tarOverwrite);

  kDebugInfo(1601, "-ArkSettings::writeTarProperties");
}

void ArkSettings::writeZipProperties()
{
  kDebugInfo( 1601, "+writeZipProperties");
	
  kc->setGroup( ZIP_GROUP );

  kDebugInfo( 1601, "m_zipExtract	Overwrite = %d", m_zipExtractOverwrite);
  kc->writeEntry( EXTRACT_OVERWRITE, m_zipExtractOverwrite );
  kc->writeEntry( EXTRACT_JUNKPATHS, m_zipExtractJunkPaths );
  kc->writeEntry( EXTRACT_LOWERCASE, m_zipExtractLowerCase );

  kc->writeEntry( ADD_RECURSEDIRS, m_zipAddRecurseDirs );
  kc->writeEntry( ADD_JUNKDIRS, m_zipAddJunkDirs );
  kc->writeEntry( ADD_MSDOS, m_zipAddMSDOS );
  kc->writeEntry( ADD_CONVERTLF, m_zipAddConvertLF );
	
  kDebugInfo( 1601, "-writeZipProperties");
}

//////////////////////////////////////////////////////////////////////////


void ArkSettings::addRecentFile(const QString& _filename)
{
  uint nb = recentFiles.count();
  uint i=0;

  while (i<nb)
    {
      if( recentFiles[i] == _filename ){
	recentFiles.remove(recentFiles.at(i));			
	kDebugInfo( 1601, "found and removed");
      }
      i++;
    }	
  recentFiles.prepend(_filename.local8Bit());
  if (recentFiles.count() > MAX_RECENT_FILES)
    recentFiles.remove(recentFiles.last());
	
  kDebugInfo( 1601, "-addRecentFile");
}

QString ArkSettings::getStartDir() const
{
  switch(startDirMode){
  case LAST_OPEN_DIR : return QString(lastOpenDir);
  case FIXED_START_DIR : return QString(startDir);
  case FAVORITE_DIR : return QString(favoriteDir);
  default : kDebugInfo( 1601, "Error in switch !"); return QString::null;
  }
}

void ArkSettings::setStartDirCfg(const QString& dir, int mode)
{
  startDir = dir;
  startDirMode = mode;
}

QString ArkSettings::getOpenDir() const
{
  switch(openDirMode){
  case LAST_OPEN_DIR : return QString(lastOpenDir);
  case FIXED_OPEN_DIR : return QString(openDir);
  case FAVORITE_DIR : return QString(favoriteDir);
  default : kDebugInfo( 1601, "Error in switch !"); return QString::null;
  }
}

void ArkSettings::setLastOpenDir(const QString& dir)
{
  lastOpenDir = dir;
  kDebugInfo( 1601, "last open dir is %s", dir.ascii());
}

void ArkSettings::setOpenDirCfg(const QString& dir, int mode)
{
  openDir = dir;
  openDirMode = mode;
}

QString ArkSettings::getExtractDir()
{
  switch(extractDirMode){
  case LAST_EXTRACT_DIR : return QString(lastExtractDir);
  case FIXED_EXTRACT_DIR : return QString(extractDir);
  case FAVORITE_DIR : return QString(favoriteDir);
  default : kDebugInfo( 1601, "Error in switch !"); return QString::null;
  }
}

void ArkSettings::setExtractDirCfg(const QString& dir, int mode)
{
  extractDir = dir;
  extractDirMode = mode;
}

QString ArkSettings::getAddDir()
{
  switch(addDirMode){
  case LAST_ADD_DIR : return QString(lastAddDir);
  case FIXED_ADD_DIR : return QString(addDir);
  case FAVORITE_DIR : return QString(favoriteDir);
  default : kDebugInfo( 1601, "Error in switch !"); return QString::null;
  }
}

void ArkSettings::setAddDirCfg(const QString& dir, int mode)
{
  addDir = dir;
  addDirMode = mode;
}

const QString ArkSettings::getFilter()
{
  return i18n(
	      "*.zip *.tar.gz *.tar.Z *.tgz *.taz *.tzo *.tar.bz2 *.tbz2 *.tar.bz *.tar *.lzh |All valid archives\n"
	      "*.zip|Zip archive (*.zip)\n"
	      "*.tar.gz *.tgz |Tar compressed with gzip (*.tar.gz *.tgz)\n"
	      "*.tbz2 *.tar.bz2|Tar compressed with bzip2 (*.tar.bz2 *.tbz2)\n"
	      "*.lzh|Lha archive (*.lzh)\n"
	      ); 
}

void ArkSettings::clearShellOutput()
{
  delete m_lastShellOutput;
  m_lastShellOutput = new QString;
}

