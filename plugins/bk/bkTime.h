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

#ifndef BKTIME_H
#define BKTIME_H
void epochToLongString(time_t epoch, char* longString);
void epochToShortString(time_t epoch, char* shortString);
void longStringToEpoch(const char* longString, time_t* epoch);
#endif
