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
#include <stdlib.h>
#include <stdio.h>
#include <kconfig.h>
#include "settings.h"
//#define DEBUG1

/******************************************************************
 *              implementation of CSettings                       *
 ******************************************************************/


//////////////////////////////////////////////////////////////////////
/////////////////////////// readConfig ///////////////////////////////
//////////////////////////////////////////////////////////////////////

void CSettings::readConfig(const KConfig *pConfig)
{
  m_strFavDir = pConfig->readEntry(ARCH_DIR);
  if (m_strFavDir.isEmpty() )
  {
    m_strFavDir = getenv("HOME");
  }

  m_strTarExe = pConfig->readEntry(TAR_EXE);
  if (m_strTarExe.isEmpty() )
  {
    FILE *pOutput = popen("which tar", "r");
    char strPath[BUFSIZ];
    fscanf(pOutput, "%s", strPath);
    m_strTarExe = strPath;
    pclose(pOutput);
  }

  QString strAddOnlyNew = pConfig->readEntry(ADD_ONLY_NEW);
  bool bVal = false;
  if (strAddOnlyNew == STR_TRUE)
  {
    bVal = true;
  }
  // otherwise, it's empty or false, so we keep the value false.
  m_bAddOnlyNew = bVal;

  QString strStoreFullPath = pConfig->readEntry(STORE_FULL_PATH);
  bVal = false;
  if (strStoreFullPath == STR_TRUE)
  {
    bVal = true;
  }
  // otherwise, it's empty or false, so we keep the value false.
  m_bStoreFullPath = bVal;

#if 0
// Currently, we don't save these. The user decides at each extract operation.

  QString strOverwrite =  pConfig->readEntry(OVERWRITE_FILES);
  bVal = false;
   if (strOverwrite == STR_TRUE)
  {
    bVal = true;
  }
  // otherwise, it's empty or false, so we keep the value false.
  m_bOverwrite = bVal;

  QString strToLower =  pConfig->readEntry(FILES_TO_LOWER);
  bVal = false;
  if (strToLower == STR_TRUE)
  {
    bVal = true;
  }
  // otherwise, it's empty or false, so we keep the value false.
  m_bToLower = bVal;

  QString strPreservePerms =  pConfig->readEntry(PRESERVE_PERMS);
  bVal = true;
  if (strPreservePerms == STR_FALSE)
  {
    bVal = false;
  }
  // otherwise, it's empty or true, so we keep the value true.
  m_bPreservePermissions = bVal;
#endif
}

//////////////////////////////////////////////////////////////////////
////////////////////////// writeConfig ///////////////////////////////
//////////////////////////////////////////////////////////////////////

void CSettings::writeConfig(KConfig *pConfig)
{
#ifdef DEBUG1
  fprintf(stderr, "Entered writeConfig()\n");
#endif

#ifdef DEBUG1
  fprintf(stderr, "Favorite directory is %s\n", (const char *)m_strFavDir);
#endif


  pConfig->writeEntry(ARCH_DIR.copy(), m_strFavDir);
  pConfig->writeEntry(TAR_EXE, m_strTarExe);
  pConfig->writeEntry(ADD_ONLY_NEW,
		      m_bAddOnlyNew? STR_TRUE : STR_FALSE);
  pConfig->writeEntry(STORE_FULL_PATH,
		      m_bStoreFullPath? STR_TRUE : STR_FALSE);

  pConfig->writeEntry(OVERWRITE_FILES,
		      m_bOverwrite? STR_TRUE : STR_FALSE);
  pConfig->writeEntry(FILES_TO_LOWER,
		      m_bToLower? STR_TRUE : STR_FALSE);
  pConfig->writeEntry(PRESERVE_PERMS,
		      m_bPreservePermissions? STR_TRUE : STR_FALSE);

  
  pConfig->sync();   // make sure it's really saved

#ifdef DEBUG1
  fprintf(stderr, "Left writeConfig()\n");
#endif
}
