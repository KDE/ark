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

#ifndef BKADD_H
#define BKADD_H
int add(VolInfo* volInfo, const char* srcPathAndName, BkDir* destDir, 
        const char* nameToUse);
int addDirContents(VolInfo* volInfo, const char* srcPath, BkDir* destDir);
#endif
