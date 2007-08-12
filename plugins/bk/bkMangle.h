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

#ifndef BKMANGLE_H
#define BKMANGLE_H
bool charIsValid9660(char theChar);
bool charIsValidJoliet(char theChar);
unsigned hashString(const char *str, unsigned int length);
int mangleDir(const BkDir* origDir, DirToWrite* newDir, int filenameTypes);
void mangleNameFor9660(const char* origName, char* newName, bool isADir);
void mangleNameForJoliet(const char* origName, char* newName, bool appendHash);
void shortenNameFor9660(const char* origName, char* newName);
#endif
