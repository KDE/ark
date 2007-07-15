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
* 31 dec 2006: these functions turned out to be so heavy on cpu usage that 
* performance went down significantly for writing to anything, including 
* the local filesystem. So now they don't do anything.
* 07 Jan 2007: deleted the said caching functions
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bkInternal.h"
#include "bkCache.h"

int wcSeekForward(VolInfo* volInfo, off_t numBytes)
{
    lseek(volInfo->imageForWriting, numBytes, SEEK_CUR);
    
    return 1;
}

int wcSeekSet(VolInfo* volInfo, off_t position)
{
    lseek(volInfo->imageForWriting, position, SEEK_SET);
    
    return 1;
}

off_t wcSeekTell(VolInfo* volInfo)
{
    return lseek(volInfo->imageForWriting, 0, SEEK_CUR);
}

int wcWrite(VolInfo* volInfo, const char* block, off_t numBytes)
{
    int rc;
    rc = write(volInfo->imageForWriting, block, numBytes);
    if(rc != numBytes)
        return BKERROR_WRITE_GENERIC;
    
    if(volInfo->writeProgressFunction != NULL)
    {
        time_t timeNow;
        time(&timeNow);
        
        if(timeNow - volInfo->lastTimeCalledProgress >= 1)
        {
            struct stat statStruct;
            double percentComplete;
            fstat(volInfo->imageForWriting, &statStruct);
            percentComplete = (double)statStruct.st_size * 100 / 
                              volInfo->estimatedIsoSize + 1;
            
            /* estimate isn't perfect */
            if(percentComplete > 100)
                percentComplete = 100;
            else if (percentComplete < 0)
                percentComplete = 0;
            
            volInfo->writeProgressFunction(volInfo, percentComplete);
            volInfo->lastTimeCalledProgress = timeNow;
        }
    }
    
    return 1;
}
