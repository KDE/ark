//  -*-C++-*-           emacs magic for .h files
/*
    ark: A program for modifying archives via a GUI.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Module added 1999

Modifications to this program were made by Corel Corporation, November, 1999.
All such modifications are copyright (C) 1999 Corel Corporation and are 
licensed under the terms of the GNU General Public License

*/

#ifndef __CSETTINGS__
#define __CSETTINGS__

#include <qstring.h>

class KConfig;

// config labels
#define ARCH_DIR QString("ArchiveDirectory")
#define TAR_EXE QString("TarExe")
#define ADD_ONLY_NEW QString("AddOnlyNew")
#define STORE_FULL_PATH QString("StoreFullPath")
#define OVERWRITE_FILES QString("OverwriteFiles")
#define FILES_TO_LOWER QString("FilesToLower")
#define PRESERVE_PERMS QString("PreservePermissions")
#define STR_TRUE QString("true")
#define STR_FALSE QString("false")

// The settings class contains the user-defined settings.
// It is instantiated as a global object.

class CSettings
{
public:
  CSettings() : m_bOverwrite(false), m_bToLower(false),
    m_bPreservePermissions(true) {}

  void readConfig(const KConfig *pConfig);
  void writeConfig(KConfig *pConfig);

  // inspector functions
  QString getFavDir() const { return m_strFavDir; }
  QString getTarExe() const { return m_strTarExe; }
  bool addingOnlyNew() const { return m_bAddOnlyNew; }
  bool storingFullPaths() const { return m_bStoreFullPath; }
  bool overwriteFiles() const { return m_bOverwrite; }
  bool filesToLower() const { return m_bToLower; }
  bool preservingPerms() const { return m_bPreservePermissions; }

  // set functions
  void setFavDir(const QString & strFavDir) { m_strFavDir = strFavDir; }
  void setTarExe(const QString & strTarExe) { m_strTarExe = strTarExe; }
  void setAddOnlyNew(bool bVal) { m_bAddOnlyNew = bVal; }
  void setStoreFullPath(bool bVal) { m_bStoreFullPath = bVal; }
  void setOverwrite(bool bVal) { m_bOverwrite = bVal; }
  void setFilesToLower(bool bVal) { m_bToLower = bVal; }
  void setPreservePerms(bool bVal) { m_bPreservePermissions = bVal; }  
  
private:  // data
  QString m_strFavDir;    // favorite directory
  QString m_strTarExe;    // which tar (apparently people sometimes have two!)
  bool m_bAddOnlyNew;
  bool m_bStoreFullPath;
  bool m_bOverwrite;
  bool m_bToLower;
  bool m_bPreservePermissions;

private: // methods
  CSettings(const CSettings &s) {}   // forbidden copy constructor

};

#endif //  __CSETTINGS__
