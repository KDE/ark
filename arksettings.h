/*

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

#ifndef ARKSETTINGS_H
#define ARKSETTINGS_H

// Qt includes
#include <qstrlist.h>
#include <qstring.h>

// KDE includes
#include <kconfig.h>


#define MAX_RECENT_FILES 5


class ArkSettings
{
public:
  ArkSettings();
  ~ArkSettings();

  void readZipProperties();
  void writeZipProperties();

  void readTarProperties();
  void writeTarProperties();
	
  enum DirPolicy {
    FAVORITE_DIR=1, FIXED_START_DIR,
    LAST_OPEN_DIR, FIXED_OPEN_DIR,
    LAST_EXTRACT_DIR, FIXED_EXTRACT_DIR,
    LAST_ADD_DIR, FIXED_ADD_DIR
  };

  const QString getFilter();
	
  QString getTarCommand() const { return tar_exe; }
  void setTarCommand(const QString& _cmd) { tar_exe = _cmd; }
	
  QString getFavoriteDir() const { return QString(favoriteDir); }
  void setFavoriteDir(const QString& _dir) { favoriteDir = _dir; }

  QString getStartDir() const;
  QString getFixedStartDir() const { return QString( startDir ); }
  int getStartDirMode() const { return startDirMode; }
  void setStartDirCfg(const QString& dir, int mode);

  QString getOpenDir() const;
  QString getFixedOpenDir() const { return QString( openDir ); }
  int getOpenDirMode() const { return openDirMode; }
  void setLastOpenDir(const QString& dir);
  void setOpenDirCfg(const QString& dir, int mode);

  QString getExtractDir();
  QString getFixedExtractDir() const { return QString( extractDir ); }
  int getExtractDirMode() const { return extractDirMode; }
  void setLastExtractDir(const QString& dir) { lastExtractDir = dir; }
  void setExtractDirCfg(const QString& dir, int mode);

  QString getAddDir();
  QString getFixedAddDir() const { return QString( addDir ); }
  int getAddDirMode() const { return addDirMode; }
  void setLastAddDir(const QString& dir) { lastAddDir = dir; }
  void setAddDirCfg(const QString& dir, int mode);

  QStringList * getRecentFiles() { return &recentFiles;}
  void addRecentFile(const QString& filename);

  void setSaveOnExitChecked( bool _b) { m_saveOnExit = _b; }
  bool isSaveOnExitChecked() { return m_saveOnExit; }
	
  void setaddPath( bool b) { fullPath = b; }
  bool getaddPath() { return fullPath; }

  void setReplaceOnlyNew(bool _b) { replaceOnlyNewerFiles = _b; }
  bool getReplaceOnlyNew() { return replaceOnlyNewerFiles; }

  void setSelectRegExp(const QString& _exp) { m_regExp = _exp; }

  QString getSelectRegExp() const { return m_regExp; }

  void appendShellOutputData( const char * _data ) {
    m_lastShellOutput->append( _data ); }

  void clearShellOutput();
  QString * getLastShellOutput() const { return m_lastShellOutput; }

  void setZipExtractOverwrite(bool _b) { m_zipExtractOverwrite = _b; }
  bool getZipExtractOverwrite() { return m_zipExtractOverwrite; }
	
  void setZipExtractJunkPaths( bool _b ) { m_zipExtractJunkPaths = _b; }
  bool getZipExtractJunkPaths() { return m_zipExtractJunkPaths; }
	
  void setZipExtractLowerCase( bool _b ) {  m_zipExtractLowerCase = _b; }
  bool getZipExtractLowerCase() {   return m_zipExtractLowerCase; }

  void setZipAddRecurseDirs( bool _b ) { m_zipAddRecurseDirs = _b; }
  bool getZipAddRecurseDirs() { return m_zipAddRecurseDirs; }

  void setZipAddJunkDirs( bool _b ) { m_zipAddJunkDirs = _b; }
  bool getZipAddJunkDirs() { return m_zipAddJunkDirs; }

  void setZipAddMSDOS( bool _b ) { m_zipAddMSDOS = _b; }
  bool getZipAddMSDOS() { return m_zipAddMSDOS; }

  void setZipAddConvertLF( bool _b ) { m_zipAddConvertLF = _b; }
  bool getZipAddConvertLF() { return m_zipAddConvertLF; }

  void setTarPreservePerms(bool _b) { m_tarPreservePerms = _b; }
  bool getTarPreservePerms() { return m_tarPreservePerms; } 

  void setTarOverwriteFiles(bool _b) { m_tarOverwrite = _b; }
  bool getTarOverwriteFiles() { return m_tarOverwrite; }

  void setTarToLower(bool _b) { m_tarToLower = _b; }
  bool getTarToLower() { return m_tarToLower; }

  

  void setTmpDir( QString _dir ) { m_tmpDir = _dir; }
  QString getTmpDir() const { return m_tmpDir; }	
  void writeConfiguration();
  void writeConfigurationNow();
  void readConfiguration();
	
  KConfig * getKConfig() { return kc; };
	
 private:
  KConfig *kc;

  QString favoriteDir;
  QString tar_exe;

	// Directories options
  QString tmpdir;
  QString startDir;
  int startDirMode;
	
  QString openDir;
  QString lastOpenDir;
  int openDirMode;
	
  QString extractDir;
  QString lastExtractDir;
  int extractDirMode;
	
  QString addDir;
  QString lastAddDir;
  int addDirMode;
	
  QString * m_lastShellOutput;
	
  bool m_saveOnExit;

  bool contextRow;
  QStringList recentFiles;
	
  bool m_zipExtractOverwrite;
  bool m_zipExtractJunkPaths;
  bool m_zipExtractLowerCase;
	
  bool m_zipAddRecurseDirs;
  bool m_zipAddJunkDirs;
  bool m_zipAddMSDOS;
  bool m_zipAddConvertLF;

  bool m_tarPreservePerms;
  bool m_tarToLower;
  bool m_tarOverwrite;

  bool fullPath, replaceOnlyNewerFiles;

  QString m_regExp;

  QString m_tmpDir;
	
  void readRecentFiles();
  void writeRecentFiles();
	
  void readDirectories();
  void writeDirectories();
	
};

#endif /* ARKSETTINGS_H */
