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

#ifndef BKGET_H
#define BKGET_H
off_t estimateIsoSize(const BkDir* tree, int filenameTypes);
int getDirFromString(const BkDir* tree, const char* pathStr, 
                     BkDir** dirFoundPtr);
#endif
