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

#include <string.h>
#include <stdio.h>
#include <time.h>

/* no header for this when compiled with -std=c99 */
struct tm *localtime_r(const time_t *timep, struct tm *result);

/* epoch time -> 8.4.26.1 */
void epochToLongString(time_t epoch, char* longString)
{
    struct tm timeStruct;
    
    localtime_r(&epoch, &timeStruct);
    
    sprintf(longString, "%4d%02d%02d%02d%02d%02d%02d",
                        timeStruct.tm_year + 1900,
                        timeStruct.tm_mon + 1,
                        timeStruct.tm_mday,
                        timeStruct.tm_hour,
                        timeStruct.tm_min,
                        timeStruct.tm_sec,
                        0);
    
    /* this may not be 7.1.1 but since it's 0 who cares */
    longString[16] = 0;
}

/* epoch time -> 9.1.5 */
void epochToShortString(time_t epoch, char* shortString)
{
    struct tm timeStruct;
    
    localtime_r(&epoch, &timeStruct);
    
    shortString[0] = timeStruct.tm_year;
    shortString[1] = timeStruct.tm_mon + 1;
    shortString[2] = timeStruct.tm_mday;
    shortString[3] = timeStruct.tm_hour;
    shortString[4] = timeStruct.tm_min;
    shortString[5] = timeStruct.tm_sec;
    
    /* gmt offset */
    shortString[6] = 0;
}

/* 8.4.26.1 -> epoch time */
void longStringToEpoch(const char* longString, time_t* epoch)
{
    char str[5];
    int number;
    struct tm timeStruct;
    
    /* no daylight savings setting available */
    timeStruct.tm_isdst = -1;
    
    /* YEAR */
    strncpy(str, longString, 4);
    str[4] = '\0';
    
    sscanf(str, "%d", &number);
    
    timeStruct.tm_year = number - 1900;
    /* END YEAR */
    
    /* MONTH */
    strncpy(str, longString + 4, 2);
    str[2] = '\0';
    
    sscanf(str, "%d", &number);
    
    timeStruct.tm_mon = number - 1;
    /* END MONTH */
    
    /* DAY */
    strncpy(str, longString + 6, 2);
    str[2] = '\0';
    
    sscanf(str, "%d", &number);
    
    timeStruct.tm_mday = number;
    /* END DAY */
    
    /* HOUR */
    strncpy(str, longString + 8, 2);
    str[2] = '\0';
    
    sscanf(str, "%d", &number);
    
    timeStruct.tm_hour = number;
    /* END HOUR */
    
    /* MINUTE */
    strncpy(str, longString + 10, 2);
    str[2] = '\0';
    
    sscanf(str, "%d", &number);
    
    timeStruct.tm_min = number;
    /* END MINUTE */
    
    /* SECOND */
    strncpy(str, longString + 12, 2);
    str[2] = '\0';
    
    sscanf(str, "%d", &number);
    
    timeStruct.tm_sec = number;
    /* END SECOND */
    
    *epoch = mktime(&timeStruct);
}
