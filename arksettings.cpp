/*

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


// Qt includes
#include <qdir.h>

// KDE includes
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>

// ark includes
#include "arksettings.h"

// Key names in the arkrc config file
#define GENERIC_GROUP "generic"
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
#define ADD_REPLACEONLYWITHNEWER "replaceOnlyWithNewer"
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

#define FULLPATHS "fullPaths"
#define RAR_UPPER "rarToUpper"
#define RAR_LOWER "rarToLower"
#define LHA_GENERIC "lhaGeneric"
#define TAR_USE_ABS_PATHNAMES "tarUseAbsPathnames"


// #define ARK_SETTINGS_DEBUG

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

  tar_exe = kc->readPathEntry( TAR_KEY, "tar");
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "Tar command is " << tar_exe << endl;
#endif

  m_saveOnExit = kc->readBoolEntry( SAVE_ON_EXIT_KEY, true );
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "SaveOnExit is " << m_saveOnExit << endl;
#endif

  fullPath = kc->readBoolEntry(FULLPATHS, false);

  readDirectories();
  readGenericProperties();
  readZipProperties();
  readZooProperties();
  readLhaProperties();
  readRarProperties();
  readTarProperties();
}

void ArkSettings::readDirectories()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+readDirectories" << endl;
#endif

  kc->setGroup( ARK_GROUP );

  favoriteDir = kc->readPathEntry( FAVORITE_KEY, QDir::homeDirPath() );

  startDir = kc->readPathEntry( START_DIR_KEY );
  openDir = kc->readPathEntry( OPEN_DIR_KEY );
  extractDir = kc->readPathEntry( EXTRACT_DIR_KEY );
  addDir = kc->readPathEntry( ADD_DIR_KEY );

  lastOpenDir = kc->readPathEntry( LAST_OPEN_DIR_KEY );
  lastExtractDir = kc->readPathEntry( LAST_EXTRACT_DIR_KEY );
  lastAddDir = kc->readPathEntry( LAST_ADD_DIR_KEY );

  startDirMode = kc->readNumEntry( START_MODE_KEY, LAST_OPEN_DIR);
  openDirMode = kc->readNumEntry( OPEN_MODE_KEY, LAST_OPEN_DIR);
  extractDirMode = kc->readNumEntry( EXTRACT_MODE_KEY, LAST_EXTRACT_DIR);
  addDirMode = kc->readNumEntry( ADD_MODE_KEY, LAST_ADD_DIR);

#ifdef ARK_SETTINGS_DEBUG
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
#endif
}

void ArkSettings::readGenericProperties()
{
	kc->setGroup(GENERIC_GROUP);

	m_bExtractOverwrite = kc->readBoolEntry(EXTRACT_OVERWRITE, false);
	m_bAddReplaceOnlyWithNewer = kc->readBoolEntry(ADD_REPLACEONLYWITHNEWER,
						    false);
}

void ArkSettings::readTarProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+readTarProperties" << endl;
#endif

  kc->setGroup( TAR_GROUP );
  m_tarPreservePerms = kc->readBoolEntry(PRESERVE_PERMS, false);
  m_tarUseAbsPathnames = kc->readBoolEntry(TAR_USE_ABS_PATHNAMES, false);

#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-readTarProperties" << endl;
#endif
}

void ArkSettings::readLhaProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+readLhaProperties" << endl;
#endif
  kc->setGroup(LHA_GROUP);
  m_lhaAddGeneric = kc->readBoolEntry(LHA_GENERIC, false);
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-readLhaProperties" << endl;
#endif
}

void ArkSettings::readArProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+ArkSettings::readArProperties" << endl;
#endif
  kc->setGroup(AR_GROUP);
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-ArkSettings::readArProperties" << endl;
#endif
}


void ArkSettings::readZooProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+readZooProperties" << endl;
#endif
  kc->setGroup(ZOO_GROUP);

#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-readZooProperties" << endl;
#endif
}

void ArkSettings::readRarProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+readRarProperties" << endl;
#endif
  kc->setGroup(RAR_GROUP);
  m_rarToLower = kc->readBoolEntry(RAR_LOWER, false);
  m_rarToUpper = kc->readBoolEntry(RAR_UPPER, false);
  m_rarStoreSymlinks = kc->readBoolEntry(RAR_STORE_SYM_LINKS, true);
  m_rarRecurseSubdirs = kc->readBoolEntry(RAR_RECURSE_SUBDIRS, true);

#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-readRarProperties" << endl;
#endif
}

void ArkSettings::readZipProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+readZipProperties" << endl;
#endif

  kc->setGroup( ZIP_GROUP );

  m_zipExtractJunkPaths = kc->readBoolEntry( EXTRACT_JUNKPATHS, false );
  m_zipExtractLowerCase = kc->readBoolEntry( EXTRACT_LOWERCASE, false );

  m_zipAddRecurseDirs = kc->readBoolEntry( ADD_RECURSEDIRS, true );
  m_zipAddJunkDirs = kc->readBoolEntry( ADD_JUNKDIRS, true );
  m_zipAddMSDOS = kc->readBoolEntry( ADD_MSDOS, false );
  m_zipAddConvertLF = kc->readBoolEntry( ADD_CONVERTLF, false );
  m_zipStoreSymlinks = kc->readBoolEntry( ZIP_STORE_SYM_LINKS, true );
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-readZipProperties" << endl;
#endif
}



////////////////// WRITE CONFIGURATION ///////////////////////////////////

void ArkSettings::writeConfiguration()
{
#ifdef ARK_SETTINGS_DEBUG

  kdDebug(1601) << "+writeConfiguration" << endl;
#endif

  if( !m_saveOnExit ){
#ifdef ARK_SETTINGS_DEBUG
    kdDebug(1601) << "Don't save the config (exit)" << endl;
#endif

    kc->setGroup( ARK_GROUP );
    kc->writeEntry( SAVE_ON_EXIT_KEY, m_saveOnExit );
  }
  else
    {
      writeConfigurationNow();
    }
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-writeConfiguration" << endl;
#endif
}

void ArkSettings::writeConfigurationNow()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+writeConfigurationNow" << endl;
#endif

  writeDirectories();
  writeGenericProperties();
  writeZipProperties();
  writeTarProperties();
  writeZooProperties();
  writeRarProperties();
  writeLhaProperties();

  kc->setGroup( ARK_GROUP );
  kc->writePathEntry( TAR_KEY, tar_exe );
  kc->writeEntry( SAVE_ON_EXIT_KEY, m_saveOnExit );
  kc->writeEntry(FULLPATHS, fullPath);

  kc->sync();

#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-writeConfigurationNow" << endl;
#endif
}

void ArkSettings::writeDirectories()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+writeDirectories" << endl;
#endif

  kc->setGroup( ARK_GROUP );

  kc->writePathEntry( FAVORITE_KEY, favoriteDir );

  kc->writePathEntry(START_DIR_KEY, startDir);
  kc->writePathEntry(OPEN_DIR_KEY, openDir);
  kc->writePathEntry(EXTRACT_DIR_KEY, extractDir);
  kc->writePathEntry(ADD_DIR_KEY, addDir);
  kc->writePathEntry(LAST_OPEN_DIR_KEY, lastOpenDir);
  kc->writePathEntry(LAST_EXTRACT_DIR_KEY, lastExtractDir);
  kc->writePathEntry(LAST_ADD_DIR_KEY, lastAddDir);

  kc->writeEntry(START_MODE_KEY, startDirMode);
  kc->writeEntry(OPEN_MODE_KEY, openDirMode);
  kc->writeEntry(EXTRACT_MODE_KEY, extractDirMode);
  kc->writeEntry(ADD_MODE_KEY, addDirMode);

#ifdef ARK_SETTINGS_DEBUG
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
#endif
}

void ArkSettings::writeGenericProperties()
{
	kc->setGroup(GENERIC_GROUP);

	kc->writeEntry(EXTRACT_OVERWRITE, m_bExtractOverwrite);
	kc->writeEntry(ADD_REPLACEONLYWITHNEWER, m_bAddReplaceOnlyWithNewer);
}

void ArkSettings::writeTarProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+ArkSettings::writeTarProperties" << endl;
#endif

  kc->setGroup( TAR_GROUP );

  kc->writeEntry(PRESERVE_PERMS, m_tarPreservePerms);
  kc->writeEntry(TAR_USE_ABS_PATHNAMES, m_tarUseAbsPathnames);

#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-ArkSettings::writeTarProperties" << endl;
#endif
}

void ArkSettings::writeArProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+ArkSettings::writeArProperties" << endl;
#endif
  kc->setGroup(AR_GROUP);
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-ArkSettings::writeArProperties" << endl;
#endif
}

void ArkSettings::writeZooProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+ArkSettings::writeZooProperties" << endl;
#endif
  kc->setGroup(ZOO_GROUP);
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-ArkSettings::writeZooProperties" << endl;
#endif
}

void ArkSettings::writeLhaProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+ArkSettings::writeLhaProperties" << endl;
#endif
  kc->setGroup(LHA_GROUP);
  kc->writeEntry(LHA_GENERIC, m_lhaAddGeneric);
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-ArkSettings::writeLhaProperties" << endl;
#endif
}

void ArkSettings::writeRarProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+ArkSettings::writeRarProperties" << endl;
#endif
  kc->setGroup(RAR_GROUP);
  kc->writeEntry(RAR_LOWER, m_rarToLower);
  kc->writeEntry(RAR_UPPER, m_rarToUpper);
  kc->writeEntry(RAR_STORE_SYM_LINKS, m_rarStoreSymlinks);
  kc->writeEntry(RAR_RECURSE_SUBDIRS, m_rarRecurseSubdirs);
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-ArkSettings::writeRarProperties" << endl;
#endif
}

void ArkSettings::writeZipProperties()
{
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "+writeZipProperties" << endl;
#endif

  kc->setGroup( ZIP_GROUP );
  kc->writeEntry( EXTRACT_JUNKPATHS, m_zipExtractJunkPaths );
  kc->writeEntry( EXTRACT_LOWERCASE, m_zipExtractLowerCase );
  kc->writeEntry( ADD_RECURSEDIRS, m_zipAddRecurseDirs );
  kc->writeEntry( ADD_JUNKDIRS, m_zipAddJunkDirs );
  kc->writeEntry( ADD_MSDOS, m_zipAddMSDOS );
  kc->writeEntry( ADD_CONVERTLF, m_zipAddConvertLF );
  kc->writeEntry( ZIP_STORE_SYM_LINKS, m_zipStoreSymlinks );
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "-writeZipProperties" << endl;
#endif
}

//////////////////////////////////////////////////////////////////////////

QString ArkSettings::getStartDir() const
{
  switch(startDirMode)
    {
  case LAST_OPEN_DIR:
    return lastOpenDir;
  case FIXED_START_DIR:
    return startDir;
  case FAVORITE_DIR:
    return favoriteDir;
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
      return lastOpenDir;
    case FIXED_OPEN_DIR:
      return openDir;
    case FAVORITE_DIR:
      return favoriteDir;
    default:
      kdDebug(1601) << "Error in switch !" << endl;
      return QString("");
    }
}

void ArkSettings::setLastOpenDir(const QString& dir)
{
  lastOpenDir = dir;
#ifdef ARK_SETTINGS_DEBUG
  kdDebug(1601) << "last open dir is " << dir << endl;
#endif
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
    return lastExtractDir;
  case FIXED_EXTRACT_DIR:
    return extractDir;
  case FAVORITE_DIR:
    return favoriteDir;
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
    return lastAddDir;
  case FIXED_ADD_DIR:
    return addDir;
  case FAVORITE_DIR:
    return favoriteDir;
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

void ArkSettings::clearShellOutput()
{
  delete m_lastShellOutput;
  m_lastShellOutput = new QString;
}



