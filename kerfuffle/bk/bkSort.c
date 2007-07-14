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
#include <sys/types.h>

#include "bk.h"
#include "bkInternal.h"
#include "bkSort.h"

/* strings cannot be equal */
bool rightIsBigger(char* leftStr, char* rightStr)
{
    int leftLen;
    int rightLen;
    int count;
    bool resultFound;
    bool rc;
    
    leftLen = strlen(leftStr);
    rightLen = strlen(rightStr);
    
    resultFound = false;
    for(count = 0; count < leftLen && count < rightLen && !resultFound; count++)
    {
        if(rightStr[count] > leftStr[count])
        {
            resultFound = true;
            rc = true;
        }
        else if(rightStr[count] < leftStr[count])
        {
            resultFound = true;
            rc = false;
        }
    }
    
    if(!resultFound)
    /* strings are the same up to the length of the shorter one */
    {
        if(rightLen > leftLen)
            rc = true;
        else
            rc = false;
    }
    
    return rc;
}

void sortDir(DirToWrite* dir, int filenameType)
{
    BaseToWrite* child;
    
    child = dir->children;
    while(child != NULL)
    {
        if(IS_DIR(child->posixFileMode))
            sortDir(DIRTW_PTR(child), filenameType);
        
        child = child->next;
    }
    
    BaseToWrite** outerPtr;
    BaseToWrite** innerPtr;
    
    outerPtr = &(dir->children);
    while(*outerPtr != NULL)
    {
        innerPtr = &((*outerPtr)->next);
        while(*innerPtr != NULL)
        {
            if( (filenameType & FNTYPE_JOLIET &&
                 rightIsBigger((*innerPtr)->nameJoliet, (*outerPtr)->nameJoliet)) || 
                (filenameType & FNTYPE_9660 &&
                 rightIsBigger((*innerPtr)->name9660, (*outerPtr)->name9660)) )
            {
                BaseToWrite* outer = *outerPtr;
                BaseToWrite* inner = *innerPtr;
                
                if( (*outerPtr)->next != *innerPtr )
                {
                    *outerPtr = inner;
                    *innerPtr = outer;
                    
                    BaseToWrite* oldInnerNext = inner->next;
                    inner->next = outer->next;
                    outer->next = oldInnerNext;
                }
                else
                {
                    *outerPtr = inner;
                    innerPtr = &(inner->next);
                    
                    BaseToWrite* oldInnerNext = inner->next;
                    inner->next = outer;
                    outer->next = oldInnerNext;
                }
            }
            
            innerPtr = &((*innerPtr)->next);
        }
        
        outerPtr = &((*outerPtr)->next);
    }
}
