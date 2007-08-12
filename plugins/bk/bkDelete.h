/******************************* LICENSE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public License as published by the Free Software 
* Foundation; version 2 of the license.
****************************** END LICENSE ***********************************/

/******************************************************************************
* Author:
* Andrew Smith, http://littlesvr.ca/misc/contactandrew.php
*
* Copyright 2005-2007 Andrew Smith <andrew-smith@mail.ru>
*
* Contributors:
* 
******************************************************************************/

#ifndef bkDelete_h
#define bkDelete_h

#include "bkInternal.h"

void deleteDirContents(VolInfo* volInfo, BkDir* dir);
void deleteNode(VolInfo* volInfo, BkDir* parentDir, char* nodeToDeleteName);
void deleteRegFileContents(VolInfo* volInfo, BkFile* file);

#endif
