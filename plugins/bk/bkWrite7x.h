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

#ifndef BKWRITE7X_H
#define BKWRITE7X_H
/*******************************************************************************
* bkWrite7x
* functions to write simple variables as described in sections 7.x of iso9660
* not including filenames (7.4, 7.5, 7.6)
* 
* */

int write711(VolInfo* volInfo, unsigned char value);
int write721(VolInfo* volInfo, unsigned short value);
void write721ToByteArray(unsigned char* dest, unsigned short value);
int write722(VolInfo* volInfo, unsigned short value);
int write723(VolInfo* volInfo, unsigned short value);
int write731(VolInfo* volInfo, unsigned value);
void write731ToByteArray(unsigned char* dest, unsigned value);
int write732(VolInfo* volInfo, unsigned value);
int write733(VolInfo* volInfo, unsigned value);
void write733ToByteArray(unsigned char* dest, unsigned value);
#endif
