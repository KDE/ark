/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

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
#define ZOO_GROUP "Zoo"
#define RAR_GROUP "Rar"
#define LHA_GROUP "Lha"
#define AR_GROUP "Ar"

#define FAVORITE_KEY "ArchiveDirectory"
#define TAR_KEY "TarExe"

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

#define ZIP_STORE_SYM_LINKS "zipStoreSymlinks"
#define RAR_STORE_SYM_LINKS "rarStoreSymlinks"
#define RAR_RECURSE_SUBDIRS "rarRecurseSubdirs"

#define SAVE_ON_EXIT_KEY "saveOnExit"

#define PRESERVE_PERMS "preservePerms"
#define TAR_REPLACE_ONLY_WITH_NEWER "tarReplaceOnlyWithNewer"
#define ZIP_REPLACE_ONLY_WITH_NEWER "zipReplaceOnlyWithNewer"
#define LHA_REPLACE_ONLY_WITH_NEWER "lhaReplaceOnlyWithNewer"
#define AR_REPLACE_ONLY_WITH_NEWER "arReplaceOnlyWithNewer"
#define ZOO_REPLACE_ONLY_WITH_NEWER "zooReplaceOnlyWithNewer"
#define RAR_REPLACE_ONLY_WITH_NEWER "rarReplaceOnlyWithNewer"

#define FULLPATHS "fullPaths"
#define TAR_OVERWRITE "tarOverwrite"
#define ZOO_OVERWRITE "zooOverwrite"
#define RAR_OVERWRITE "rarOverwrite"
#define RAR_UPPER "rarToUpper"
#define RAR_LOWER "rarToLower"
#define LHA_GENERIC "lhaGeneric"
#define TAR_USE_ABS_PATHNAMES "tarUseAbsPathnames"

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
  kdDebug(1601) << "+ArkSettings::readConfiguration()" << endl;

  kc->setGroup( ARK_GROUP );

  tar_exe = kc->readEntry( TAR_KEY, "tar");
  kdDebug(1601) << "Tar command is " << tar_exe << endl;


  m_saveOnExit = kc->readBoolEntry( SAVE_ON_EXIT_KEY, true );
  kdDebug(1601) << "SaveOnExit is " << m_saveOnExit << endl;

  fullPath = kc->readBoolEntry(FULLPATHS, false);

  readDirectories();
  readZipProperties();
  readZooProperties();
  readLhaProperties();
  readRarProperties();
  readTarProperties();

  kdDebug(1601) << "-ArkSettings::readConfiguration()" << endl;
}

void ArkSettings::readDirectories()
{
  kdDebug(1601) << "+readDirectories" << endl;
	
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
	
  kdDebug(1601) << "favorite dir is " << favoriteDir << endl;
  kdDebug(1601) << "last open dir is " << lastOpenDir << endl;
  kdDebug(1601) << "last xtr dir is " << lastExtractDir << endl;
  kdDebug(1601) << "last add dir is " << lastAddDir << endl;
	
  kdDebug(1601) << "start dir is " << startDir << endl;
  kdDebug(1601) << "open dir is " << openDir << endl;
  kdDebug(1601) << "xtr dir is " << extractDir << endl;
  kdDebug(1601) << "add dir is " << addDir << endl;

  kdDebug(1601) << "start mode is " << startDirMode << endl;
  kdDebug(1601) << "open mode is " << openDirMode << endl;
  kdDebug(1601) << "xtr mode is " << extractDirMode << endl;
  kdDebug(1601) << "add mode is " << addDirMode << endl;
	
  kdDebug(1601) << "-readDirectories" << endl;
}

void ArkSettings::readTarProperties()
{
  kdDebug(1601) << "+readTarProperties" << endl;
	
  kc->setGroup( TAR_GROUP );
  m_tarPreservePerms = kc->readBoolEntry(PRESERVE_PERMS, false);
  m_tarOverwrite = kc->readBoolEntry(TAR_OVERWRITE, false);
  m_tarUseAbsPathnames = kc->readBoolEntry(TAR_USE_ABS_PATHNAMES, false);
  m_tarReplaceOnlyWithNewer = kc->readBoolEntry(TAR_REPLACE_ONLY_WITH_NEWER,
						false);

  kdDebug(1601) << "-readTarProperties" << endl;
}

void ArkSettings::readLhaProperties()
{
  kdDebug(1601) << "+readLhaProperties" << endl;
  kc->setGroup(LHA_GROUP);
  m_lhaAddGeneric = kc->readBoolEntry(LHA_GENERIC, false);
  m_lhaReplaceOnlyWithNewer = kc->readBoolEntry(LHA_REPLACE_ONLY_WITH_NEWER,
						false);
  kdDebug(1601) << "-readLhaProperties" << endl;
}

void ArkSettings::readArProperties()
{
  kdDebug(1601) << "+ArkSettings::readArProperties" << endl;
  kc->setGroup(AR_GROUP);
  m_arReplaceOnlyWithNewer = kc->readBoolEntry(AR_REPLACE_ONLY_WITH_NEWER,
					       false);
  kdDebug(1601) << "-ArkSettings::readArProperties" << endl;
}


void ArkSettings::readZooProperties()
{
  kdDebug(1601) << "+readZooProperties" << endl;
  kc->setGroup(ZOO_GROUP);
  m_zooOverwrite = kc->readBoolEntry(ZOO_OVERWRITE, false);
  m_zooReplaceOnlyWithNewer = kc->readBoolEntry(ZOO_REPLACE_ONLY_WITH_NEWER,
						false);

  kdDebug(1601) << "-readZooProperties" << endl;
}

void ArkSettings::readRarProperties()
{
  kdDebug(1601) << "+readRarProperties" << endl;
  kc->setGroup(RAR_GROUP);
  m_rarOverwrite = kc->readBoolEntry(RAR_OVERWRITE, false);
  m_rarToLower = kc->readBoolEntry(RAR_LOWER, false);
  m_rarToUpper = kc->readBoolEntry(RAR_UPPER, false);
  m_rarStoreSymlinks = kc->readBoolEntry(RAR_STORE_SYM_LINKS, true);
  m_rarReplaceOnlyWithNewer = kc->readBoolEntry(RAR_REPLACE_ONLY_WITH_NEWER,
						false);
  m_rarRecurseSubdirs = kc->readBoolEntry(RAR_RECURSE_SUBDIRS, true);

  kdDebug(1601) << "-readRarProperties" << endl;
}

void ArkSettings::readZipProperties()
{
  kdDebug(1601) << "+readZipProperties" << endl;
	
  kc->setGroup( ZIP_GROUP );

  m_zipExtractOverwrite = kc->readBoolEntry( EXTRACT_OVERWRITE, true );
  m_zipExtractJunkPaths = kc->readBoolEntry( EXTRACT_JUNKPATHS, false );
  m_zipExtractOverwrite = kc->readBoolEntry( EXTRACT_LOWERCASE, false );

  m_zipAddRecurseDirs = kc->readBoolEntry( ADD_RECURSEDIRS, false );
  m_zipAddJunkDirs = kc->readBoolEntry( ADD_JUNKDIRS, false );
  m_zipAddMSDOS = kc->readBoolEntry( ADD_MSDOS, false );
  m_zipAddConvertLF = kc->readBoolEntry( ADD_CONVERTLF, false );
  m_zipStoreSymlinks = kc->readBoolEntry( ZIP_STORE_SYM_LINKS, true );
  m_zipReplaceOnlyWithNewer = kc->readBoolEntry(ZIP_REPLACE_ONLY_WITH_NEWER,
						false);
  kdDebug(1601) << "-readZipProperties" << endl;
}



////////////////// WRITE CONFIGURATION ///////////////////////////////////

void ArkSettings::writeConfiguration()
{

  kdDebug(1601) << "+writeConfiguration" << endl;

  if( !m_saveOnExit ){
    kdDebug(1601) << "Don't save the config (exit)" << endl;

    kc->setGroup( ARK_GROUP );
    kc->writeEntry( SAVE_ON_EXIT_KEY, m_saveOnExit );
  }
  else
    {
      writeConfigurationNow();
    }
  kdDebug(1601) << "-writeConfiguration" << endl;
}

void ArkSettings::writeConfigurationNow()
{
  kdDebug(1601) << "+writeConfigurationNow" << endl;

  writeDirectories();
  writeZipProperties();
  writeTarProperties();
  writeZooProperties();
  writeRarProperties();
  writeLhaProperties();

  kc->setGroup( ARK_GROUP );
  kc->writeEntry( TAR_KEY, tar_exe );
  kc->writeEntry( SAVE_ON_EXIT_KEY, m_saveOnExit );
  kc->writeEntry(FULLPATHS, fullPath);

  kc->sync();

  kdDebug(1601) << "-writeConfigurationNow" << endl;
}

void ArkSettings::writeDirectories()
{
  kdDebug(1601) << "+writeDirectories" << endl;
	
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

  kdDebug(1601) << "favorite dir is " << favoriteDir << endl;

  kdDebug(1601) << "last open dir is " << lastOpenDir << endl;
  kdDebug(1601) << "last xtr dir is " << lastExtractDir << endl;
  kdDebug(1601) << "last add dir is " << lastAddDir << endl;
	
  kdDebug(1601) << "start dir is " << startDir << endl;
  kdDebug(1601) << "open dir is " << openDir << endl;
  kdDebug(1601) << "xtr dir is " << extractDir << endl;
  kdDebug(1601) << "add dir is " << addDir << endl;

  kdDebug(1601) << "start mode is " << startDirMode << endl;
  kdDebug(1601) << "open mode is " << openDirMode << endl;
  kdDebug(1601) << "xtr mode is " << extractDirMode << endl;
  kdDebug(1601) << "add mode is " << addDirMode << endl;

  kdDebug(1601) << "-writeDirectories" << endl;
}

void ArkSettings::writeTarProperties()
{
  kdDebug(1601) << "+ArkSettings::writeTarProperties" << endl;

  kc->setGroup( TAR_GROUP );

  kc->writeEntry(PRESERVE_PERMS, m_tarPreservePerms);
  kc->writeEntry(TAR_OVERWRITE, m_tarOverwrite);
  kc->writeEntry(TAR_USE_ABS_PATHNAMES, m_tarUseAbsPathnames);
  kc->writeEntry(TAR_REPLACE_ONLY_WITH_NEWER, m_tarReplaceOnlyWithNewer);

  kdDebug(1601) << "-ArkSettings::writeTarProperties" << endl;
}

void ArkSettings::writeArProperties()
{
  kdDebug(1601) << "+ArkSettings::writeArProperties" << endl;
  kc->setGroup(AR_GROUP);
  kc->writeEntry(AR_REPLACE_ONLY_WITH_NEWER, m_arReplaceOnlyWithNewer);
  kdDebug(1601) << "-ArkSettings::writeArProperties" << endl;
}

void ArkSettings::writeZooProperties()
{
  kdDebug(1601) << "+ArkSettings::writeZooProperties" << endl;
  kc->setGroup(ZOO_GROUP);
  kc->writeEntry(ZOO_OVERWRITE, m_zooOverwrite);
  kc->writeEntry(ZOO_REPLACE_ONLY_WITH_NEWER, m_zooReplaceOnlyWithNewer);

  kdDebug(1601) << "-ArkSettings::writeZooProperties" << endl;
}

void ArkSettings::writeLhaProperties()
{
  kdDebug(1601) << "+ArkSettings::writeLhaProperties" << endl;
  kc->setGroup(LHA_GROUP);
  kc->writeEntry(LHA_GENERIC, m_lhaAddGeneric);
  kc->writeEntry(LHA_REPLACE_ONLY_WITH_NEWER, m_lhaReplaceOnlyWithNewer);
  kdDebug(1601) << "-ArkSettings::writeLhaProperties" << endl;
}

void ArkSettings::writeRarProperties()
{
  kdDebug(1601) << "+ArkSettings::writeRarProperties" << endl;
  kc->setGroup(RAR_GROUP);
  kc->writeEntry(RAR_OVERWRITE, m_rarOverwrite);
  kc->writeEntry(RAR_LOWER, m_rarToLower);
  kc->writeEntry(RAR_UPPER, m_rarToUpper);
  kc->writeEntry(RAR_STORE_SYM_LINKS, m_rarStoreSymlinks);
  kc->writeEntry(RAR_REPLACE_ONLY_WITH_NEWER, m_rarReplaceOnlyWithNewer);
  kc->writeEntry(RAR_RECURSE_SUBDIRS, m_rarRecurseSubdirs);

  kdDebug(1601) << "-ArkSettings::writeRarProperties" << endl;
}

void ArkSettings::writeZipProperties()
{
  kdDebug(1601) << "+writeZipProperties" << endl;
	
  kc->setGroup( ZIP_GROUP );

  kdDebug(1601) << "m_zipExtract	Overwrite = " << m_zipExtractOverwrite << endl;
  kc->writeEntry( EXTRACT_OVERWRITE, m_zipExtractOverwrite );
  kc->writeEntry( EXTRACT_JUNKPATHS, m_zipExtractJunkPaths );
  kc->writeEntry( EXTRACT_LOWERCASE, m_zipExtractLowerCase );

  kc->writeEntry( ADD_RECURSEDIRS, m_zipAddRecurseDirs );
  kc->writeEntry( ADD_JUNKDIRS, m_zipAddJunkDirs );
  kc->writeEntry( ADD_MSDOS, m_zipAddMSDOS );
  kc->writeEntry( ADD_CONVERTLF, m_zipAddConvertLF );
  kc->writeEntry( ZIP_STORE_SYM_LINKS, m_zipStoreSymlinks );
  kc->writeEntry( ZIP_REPLACE_ONLY_WITH_NEWER, m_zipReplaceOnlyWithNewer);

  kdDebug(1601) << "-writeZipProperties" << endl;
}

//////////////////////////////////////////////////////////////////////////

QString ArkSettings::getStartDir() const
{
  switch(startDirMode)
    {
  case LAST_OPEN_DIR:
    return QString(lastOpenDir);
  case FIXED_START_DIR:
    return QString(startDir);
  case FAVORITE_DIR:
    return QString(favoriteDir);
  default:
    kdDebug(1601) << "Error in switch !" << endl;
    return QString("");
  }
}

void ArkSettings::setStartDirCfg(const QString& dir, int mode)
{
  startDir = dir;
  startDirMode = mode;
}

QString ArkSettings::getOpenDir() const
{
  switch(openDirMode)
    {
    case LAST_OPEN_DIR:
      return QString(lastOpenDir);
    case FIXED_OPEN_DIR:
      return QString(openDir);
    case FAVORITE_DIR:
      return QString(favoriteDir);
    default:
      kdDebug(1601) << "Error in switch !" << endl;
      return QString("");
    }
}

void ArkSettings::setLastOpenDir(const QString& dir)
{
  lastOpenDir = dir;
  kdDebug(1601) << "last open dir is " << dir << endl;
}

void ArkSettings::setOpenDirCfg(const QString& dir, int mode)
{
  openDir = dir;
  openDirMode = mode;
}

QString ArkSettings::getExtractDir()
{
  switch(extractDirMode)
    {
  case LAST_EXTRACT_DIR:
    return QString(lastExtractDir);
  case FIXED_EXTRACT_DIR:
    return QString(extractDir);
  case FAVORITE_DIR:
    return QString(favoriteDir);
  default:
    kdDebug(1601) << "Error in switch !" << endl;
    return QString("");
    }
}

void ArkSettings::setExtractDirCfg(const QString& dir, int mode)
{
  extractDir = dir;
  extractDirMode = mode;
}

QString ArkSettings::getAddDir()
{
  switch(addDirMode)
    {
  case LAST_ADD_DIR:
    return QString(lastAddDir);
  case FIXED_ADD_DIR:
    return QString(addDir);
  case FAVORITE_DIR:
    return QString(favoriteDir);
  default:
    kdDebug(1601) << "Error in switch !" << endl;
    return QString("");
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
	      "*.zip *.tar.gz *.tar.Z *.tar.lzo *.tgz *.taz *.tzo *.tar.bz2 *.tar.bz *.tar *.lzh *.gz *.lzo *.Z *.bz *.bz2 *.zoo *.rar *.a|All valid archives\n"
	      " *.tar.gz *.tar.Z *.tgz *.taz *.tzo *.tar.bz2 *.tar.bz *.tar.lzo *.tar |Compressed or Plain Tar archives\n"
	      "*.gz *.bz *.bz2 *.lzo *.Z|Compressed Files\n"
	      "*.zip|Zip archives (*.zip)\n"
	      "*.lzh|Lha archives (*.lzh)\n"
	      "*.zoo|Zoo archives (*.zoo)\n"
	      "*.rar|Rar archives (*.rar)\n"
	      "*.a|Ar archives (*.a)\n"
	      ); 
}

void ArkSettings::clearShellOutput()
{
  delete m_lastShellOutput;
  m_lastShellOutput = new QString;
}

