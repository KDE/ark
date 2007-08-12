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

#ifndef BKEXTRACT_H
#define BKEXTRACT_H
int copyByteBlock(VolInfo* volInfo, int src, int dest, unsigned numBytes);
int extract(VolInfo* volInfo, BkDir* parentDir, char* nameToExtract, 
            const char* destDir, const char* nameToUse, bool keepPermissions);
int extractDir(VolInfo* volInfo, BkDir* srcDir, const char* destDir, 
               const char* nameToUse, bool keepPermissions);
int extractFile(VolInfo* volInfo, BkFile* srcFileInTree, const char* destDir, 
                const char* nameToUse, bool keepPermissions);
int extractSymlink(BkSymLink* srcLink, const char* destDir, 
                   const char* nameToUse);
#endif
