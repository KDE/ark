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

#ifndef BKLINK_H
#define BKLINK_H
int addToHardLinkTable(VolInfo* volInfo, off_t position, char* pathAndName, 
                       unsigned size, bool onImage, BkHardLink** newLink);
int filesAreSame(VolInfo* volInfo, int file1, off_t posFile1, 
                 int file2, off_t posFile2, unsigned size);
int findInHardLinkTable(VolInfo* volInfo, off_t position, 
                        char* pathAndName, unsigned size,
                        bool onImage, BkHardLink** foundLink);
int readFileHead(VolInfo* volInfo, off_t position, char* pathAndName, 
                 bool onImage, unsigned char* dest, int numBytes);
void resetWriteStatus(BkHardLink* fileLocations);
#endif
