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

#ifndef ARKSETTINGS_H
#define ARKSETTINGS_H

class QString;
class KConfig;

class ArkSettings
{
private:
  ArkSettings();

public:
  static ArkSettings * self();

  ~ArkSettings();

  void readGenericProperties();
  void writeGenericProperties();

  void readZipProperties();
  void writeZipProperties();

  void readTarProperties();
  void writeTarProperties();
	
  void readZooProperties();
  void writeZooProperties();

  void readRarProperties();
  void writeRarProperties();

  void readLhaProperties();
  void writeLhaProperties();

  void readArProperties();
  void writeArProperties();

  enum DirPolicy {
    FAVORITE_DIR=1, FIXED_START_DIR,
    LAST_OPEN_DIR, FIXED_OPEN_DIR,
    LAST_EXTRACT_DIR, FIXED_EXTRACT_DIR,
    LAST_ADD_DIR, FIXED_ADD_DIR
  };

  QString getTarCommand() const { return tar_exe; }
  void setTarCommand(const QString& _cmd) { tar_exe = _cmd; }
	
  QString getFavoriteDir() const { return favoriteDir; }
  void setFavoriteDir(const QString& _dir) { favoriteDir = _dir; }

  QString getStartDir() const;
  QString getFixedStartDir() const { return startDir; }
  int getStartDirMode() const { return startDirMode; }
  void setStartDirCfg(const QString& dir, int mode);

  QString getOpenDir() const;
  QString getFixedOpenDir() const { return openDir; }
  int getOpenDirMode() const { return openDirMode; }
  void setLastOpenDir(const QString& dir);
  void setOpenDirCfg(const QString& dir, int mode);

  QString getExtractDir();
  QString getFixedExtractDir() const { return extractDir; }
  int getExtractDirMode() const { return extractDirMode; }
  void setLastExtractDir(const QString& dir) { lastExtractDir = dir; }
  void setExtractDirCfg(const QString& dir, int mode);

  QString getAddDir();
  QString getFixedAddDir() const { return addDir; }
  int getAddDirMode() const { return addDirMode; }
  void setLastAddDir(const QString& dir) { lastAddDir = dir; }
  void setAddDirCfg(const QString& dir, int mode);

  void setaddPath( bool b) { fullPath = b; }
  bool getaddPath() { return fullPath; }

  void setSelectRegExp(const QString& _exp) { m_regExp = _exp; }

  QString getSelectRegExp() const { return m_regExp; }

  void appendShellOutputData( const char * _data ) {
    m_lastShellOutput->append( _data ); }

  void clearShellOutput();
  QString * getLastShellOutput() const { return m_lastShellOutput; }

  /* Generic config functions: Ideally, ALL config options move here, but
	for now we must consider options not globally supported */
  bool getExtractOverwrite() const { return m_bExtractOverwrite; }
  void setExtractOverwrite(bool _b) { m_bExtractOverwrite = _b; }

  bool getAddReplaceOnlyWithNewer() const { return m_bAddReplaceOnlyWithNewer; }
  void setAddReplaceOnlyWithNewer(bool _b) { m_bAddReplaceOnlyWithNewer = _b; }
  /***/

  void setLhaGeneric(bool _b) { m_lhaAddGeneric = _b; }
  bool getLhaGeneric() { return m_lhaAddGeneric; }

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

  void setZipStoreSymlinks( bool _b ) { m_zipStoreSymlinks = _b; }
  bool getZipStoreSymlinks() { return m_zipStoreSymlinks; }

  void setTarPreservePerms(bool _b) { m_tarPreservePerms = _b; }
  bool getTarPreservePerms() { return m_tarPreservePerms; } 

  void setTarUseAbsPathnames(bool _b) { m_tarUseAbsPathnames = _b; }
  bool getTarUseAbsPathnames() { return m_tarUseAbsPathnames; }

  bool getRarExtractLowerCase() { return m_rarToLower;}
  void setRarExtractLowerCase(bool _b) { m_rarToLower = _b; }

  bool getRarExtractUpperCase() { return m_rarToUpper;}
  void setRarExtractUpperCase(bool _b) { m_rarToUpper = _b; }

  bool getRarStoreSymlinks() { return m_rarStoreSymlinks;}
  void setRarStoreSymlinks(bool _b) { m_rarStoreSymlinks = _b; }

  bool getRarRecurseSubdirs() { return  m_rarRecurseSubdirs;}
  void setRarRecurseSubdirs(bool _b) { m_rarRecurseSubdirs = _b; }

  void writeConfiguration();
  void writeConfigurationNow();
  void readConfiguration();
	
  KConfig * getKConfig() { return kc; };
	
 private:
  static ArkSettings * m_pSelf;

  KConfig *kc;

  QString favoriteDir;
  QString tar_exe;

	// Directories options
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

  bool contextRow;

  // Generics
  bool m_bExtractOverwrite;
  bool m_bAddReplaceOnlyWithNewer;

	
  bool m_lhaAddGeneric;
	
  bool m_zipExtractJunkPaths;
  bool m_zipExtractLowerCase;

  bool m_zipAddRecurseDirs;
  bool m_zipAddJunkDirs;
  bool m_zipAddMSDOS;
  bool m_zipAddConvertLF;
  bool m_zipStoreSymlinks;

  bool m_tarPreservePerms;
  bool m_tarUseAbsPathnames;

  bool m_rarToLower;
  bool m_rarToUpper;
  bool m_rarStoreSymlinks;
  bool m_rarRecurseSubdirs;

  bool fullPath, replaceOnlyNewerFiles;
  QString m_regExp;
  QString m_tmpDir;	
  void readDirectories();
  void writeDirectories();
};

#endif /* ARKSETTINGS_H */
