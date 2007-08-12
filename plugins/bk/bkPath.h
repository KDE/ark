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

#ifndef bkPath_h
#define bkPath_h

#include "bkInternal.h"

bool findDirByNewPath(const NewPath* path, BkDir* tree, BkDir** dir);
bool findBaseByNewPath(NewPath* path, BkDir* tree, BkFileBase** base);
void freeDirToWriteContents(DirToWrite* dir);
void freePathContents(NewPath* path);
int getLastNameFromPath(const char* srcPathAndName, char* lastName);
int makeNewPathFromString(const char* strPath, NewPath* pathPath);
bool nameIsValid(const char* name);
void printDirToWrite(DirToWrite* dir, int level, int filenameTypes);

#endif
