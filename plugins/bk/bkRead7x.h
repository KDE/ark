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

#ifndef BKREAD7X_H
#define BKREAD7X_H
/*******************************************************************************
* bkRead7x
* functions to read simple variables as described in sections 7.x of iso9660
* not including filenames (7.4, 7.5, 7.6)
* 
* if they are stored in both byte orders, the appropriate one is read into
* the parameter but the return is 2x the size of that variable
*
* */

int read711(int image, unsigned char* value);
int read721(int image, unsigned short* value);
int read731(int image, unsigned* value);
int read733(int image, unsigned* value);
void read733FromCharArray(unsigned char* array, unsigned* value);
#endif
