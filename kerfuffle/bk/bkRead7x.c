/******************************* LICENCE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public Licence as published by the Free Software 
* Foundation; version 2 of the licence.
****************************** END LICENCE ***********************************/

/******************************************************************************
* Author:
* Andrew Smith, http://littlesvr.ca/misc/contactandrew.php
*
* Contributors:
* 
******************************************************************************/

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#include "bkRead7x.h"

int read711(int image, unsigned char* value)
{
    return read(image, value, 1);
}

int read721(int image, unsigned short* value)
{
    int rc;
    unsigned char array[2];
    
    rc = read(image, array, 2);
    if(rc != 2)
        return rc;
    
    *value = array[1];
    *value <<= 8;
    *value |= array[0];
    
    return rc;
}

int read731(int image, unsigned* value)
{
    int rc;
    unsigned char array[4];
    
    rc = read(image, array, 4);
    if(rc != 4)
        return rc;
    
    *value = array[3];
    *value <<= 8;
    *value |= array[2];
    *value <<= 8;
    *value |= array[1];
    *value <<= 8;
    *value |= array[0];
    
    return rc;
}

int read733(int image, unsigned* value)
{
    int rc;
    unsigned char both[8];
    
    rc = read(image, &both, 8);
    if(rc != 8)
        return rc;
    
    read733FromCharArray(both, value);
    
    return rc;
}

void read733FromCharArray(unsigned char* array, unsigned* value)
{
    *value = array[3];
    *value <<= 8;
    *value |= array[2];
    *value <<= 8;
    *value |= array[1];
    *value <<= 8;
    *value |= array[0];
}
