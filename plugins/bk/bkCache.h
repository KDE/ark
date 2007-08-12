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

#ifndef BKCACHE_H
#define BKCACHE_H
int wcSeekForward(VolInfo* volInfo, off_t numBytes);
int wcSeekSet(VolInfo* volInfo, off_t position);
off_t wcSeekTell(VolInfo* volInfo);
int wcWrite(VolInfo* volInfo, const char* block, off_t numBytes);
#endif
