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

/******************************************************************************
* Functions in this file write to volInfo.imageForWriting and are probably
* unsutable for anything else, except for the ones that write to arrays.
******************************************************************************/

#include <unistd.h>

#include "bkInternal.h"
#include "bkWrite7x.h"
#include "bkCache.h"
#include "bkError.h"

int write711(VolInfo* volInfo, unsigned char value)
{
    return wcWrite(volInfo, (char*)&value, 1);
}

int write721(VolInfo* volInfo, unsigned short value)
{
    unsigned char preparedValue[2];
    
    write721ToByteArray(preparedValue, value);
    
    return wcWrite(volInfo, (char*)preparedValue, 2);
}

void write721ToByteArray(unsigned char* dest, unsigned short value)
{
    dest[0] = value & 0xFF;
    dest[1] = (value >> 8) & 0xFF;
}

int write722(VolInfo* volInfo, unsigned short value)
{
    char preparedValue[2];
    
    preparedValue[0] = (value >> 8) & 0xFF;
    preparedValue[1] = value & 0xFF;
    
    return wcWrite(volInfo, preparedValue, 2);
}

int write723(VolInfo* volInfo, unsigned short value)
{
    char preparedValue[4];
    
    preparedValue[0] = value & 0xFF;
    preparedValue[1] = (value >> 8) & 0xFF;
    preparedValue[2] = preparedValue[1];
    preparedValue[3] = preparedValue[0];
    
    return wcWrite(volInfo, preparedValue, 4);
}

int write731(VolInfo* volInfo, unsigned value)
{
    unsigned char preparedValue[4];
    
    write731ToByteArray(preparedValue, value);
    
    return wcWrite(volInfo, (char*)preparedValue, 4);
}

void write731ToByteArray(unsigned char* dest, unsigned value)
{
    dest[0] = value & 0xFF;
    dest[1] = (value >> 8) & 0xFF;
    dest[2] = (value >> 16) & 0xFF;
    dest[3] = value >> 24;
}

int write732(VolInfo* volInfo, unsigned value)
{
    char preparedValue[4];
    
    preparedValue[0] = (value >> 24);
    preparedValue[1] = (value >> 16) & 0xFF;
    preparedValue[2] = (value >> 8) & 0xFF;
    preparedValue[3] = value & 0xFF;
    
    return wcWrite(volInfo, preparedValue, 4);
}

int write733(VolInfo* volInfo, unsigned value)
{
    char preparedValue[8];
    
    write733ToByteArray((unsigned char*)preparedValue, value);
    
    return wcWrite(volInfo, preparedValue, 8);
}

void write733ToByteArray(unsigned char* dest, unsigned value)
{
    dest[0] = value & 0xFF;
    dest[1] = (value >> 8) & 0xFF;
    dest[2] = (value >> 16) & 0xFF;
    dest[3] = value >> 24;

    dest[4] = dest[3];
    dest[5] = dest[2];
    dest[6] = dest[1];
    dest[7] = dest[0];
}
