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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "bk.h"
#include "bkInternal.h"
#include "bkError.h"
#include "bkGet.h"
#include "bkPath.h"

/*******************************************************************************
* bk_estimate_iso_size()
* Public function
* Estimates the size of the directory trees + file contents on the iso
* */
off_t bk_estimate_iso_size(const VolInfo* volInfo, int filenameTypes)
{
    /* reset alreadyCounted flags */
    BkHardLink* currentLink;
    currentLink = volInfo->fileLocations;
    while(currentLink != NULL)
    {
        currentLink->alreadyCounted = false;
        
        currentLink = currentLink->next;
    }
    
    return estimateIsoSize(&(volInfo->dirTree), filenameTypes);
}

/*******************************************************************************
* bk_get_creation_time()
* Public function
* */
time_t bk_get_creation_time(const VolInfo* volInfo)
{
    return volInfo->creationTime;
}

/*******************************************************************************
* bk_get_dir_from_string()
* public function
* gets a pointer to a Dir in tree described by the string pathStr
* */
int bk_get_dir_from_string(const VolInfo* volInfo, const char* pathStr, 
                           BkDir** dirFoundPtr)
{
    return getDirFromString(&(volInfo->dirTree), pathStr, dirFoundPtr);
}

/*******************************************************************************
* bk_get_permissions()
* public function
* gets the permissions (not all of the posix info) for an item (file, dir, etc.)
* */
int bk_get_permissions(VolInfo* volInfo, const char* pathAndName, 
                       mode_t* permissions)
{
    int rc;
    NewPath srcPath;
    BkFileBase* base;
    bool itemFound;
    
    if(permissions == NULL)
        return BKERROR_GET_PERM_BAD_PARAM;
    
    rc = makeNewPathFromString(pathAndName, &srcPath);
    if(rc <= 0)
    {
        freePathContents(&srcPath);
        return rc;
    }
    
    itemFound = findBaseByNewPath(&srcPath, &(volInfo->dirTree), &base);
    
    freePathContents(&srcPath);
    
    if(!itemFound)
        return BKERROR_ITEM_NOT_FOUND_ON_IMAGE;
    
    *permissions = base->posixFileMode & 0777;
    
    return 1;
}

/*******************************************************************************
* bk_get_publisher()
* Public function
* Returns a pointer to the string in volInfo that holds the volume name.
* */
const char* bk_get_publisher(const VolInfo* volInfo)
{
    return volInfo->publisher;
}

/*******************************************************************************
* bk_get_volume_name()
* Public function
* Returns a pointer to the string in volInfo that holds the volume name.
* */
const char* bk_get_volume_name(const VolInfo* volInfo)
{
    return volInfo->volId;
}

/*******************************************************************************
* estimateIsoSize()
* Recursive
* Estimate the size of the directory trees + file contents on the iso
* */
off_t estimateIsoSize(const BkDir* tree, int filenameTypes)
{
    off_t estimateDrSize;
    off_t thisDirSize;
    int numItems; /* files and directories */
    BkFileBase* child;
    
    thisDirSize = 0;
    numItems = 0;
    
    child = tree->children;
    while(child != NULL)
    {
        if(IS_DIR(child->posixFileMode))
        {
            thisDirSize += estimateIsoSize(BK_DIR_PTR(child), filenameTypes);
        }
        else if(IS_REG_FILE(child->posixFileMode))
        {
            if(BK_FILE_PTR(child)->location == NULL ||
               !BK_FILE_PTR(child)->location->alreadyCounted)
            {
                thisDirSize += BK_FILE_PTR(child)->size;
                thisDirSize += BK_FILE_PTR(child)->size % NBYTES_LOGICAL_BLOCK;
            }
            if(BK_FILE_PTR(child)->location != NULL)
                BK_FILE_PTR(child)->location->alreadyCounted = true;
        }
        
        numItems++;
        
        child = child->next;
    }
    
    estimateDrSize = 70;
    if(filenameTypes & FNTYPE_JOLIET)
        estimateDrSize += 70;
    if(filenameTypes & FNTYPE_ROCKRIDGE)
        estimateDrSize += 70;
    
    thisDirSize += 68 + (estimateDrSize * numItems);
    thisDirSize += NBYTES_LOGICAL_BLOCK - (68 + (estimateDrSize * numItems)) % NBYTES_LOGICAL_BLOCK;
    
    return thisDirSize;
}

/*******************************************************************************
* getDirFromString()
* recursive
* gets a pointer to a Dir in tree described by the string pathStr
* */
int getDirFromString(const BkDir* tree, const char* pathStr, BkDir** dirFoundPtr)
{
    int count;
    int pathStrLen;
    bool stopLooking;
    /* name of the directory in the path this instance of the function works on */
    char* currentDirName;
    BkFileBase* child;
    int rc;
    
    pathStrLen = strlen(pathStr);
    
    if(pathStrLen == 1 && pathStr[0] == '/')
    /* root, special case */
    {
        /* cast to prevent compiler const warning */
        *dirFoundPtr = (BkDir*)tree;
        return 1;
    }
    
    if(pathStrLen < 3 || pathStr[0] != '/' || pathStr[1] == '/' || 
       pathStr[pathStrLen - 1] != '/')
        return BKERROR_MISFORMED_PATH;
    
    stopLooking = false;
    for(count = 2; count < pathStrLen && !stopLooking; count++)
    /* find the first directory in the path */
    {
        if(pathStr[count] == '/')
        /* found it */
        {
            /* make a copy of the string to use with strcmp */
            currentDirName = (char*)malloc(count);
            if(currentDirName == NULL)
                return BKERROR_OUT_OF_MEMORY;
            
            strncpy(currentDirName, &(pathStr[1]), count - 1);
            currentDirName[count - 1] = '\0';
            
            child = tree->children;
            while(child != NULL && !stopLooking)
            /* each child directory in tree */
            {
                if( strcmp(child->name, currentDirName) == 0 &&
                    IS_DIR(child->posixFileMode) )
                /* found the right child directory */
                {
                    if(pathStr[count + 1] == '\0')
                    /* this is the directory i'm looking for */
                    {
                        *dirFoundPtr = BK_DIR_PTR(child);
                        stopLooking = true;
                        rc = 1;
                    }
                    else
                    /* intermediate directory, go further down the tree */
                    {
                        rc = getDirFromString(BK_DIR_PTR(child), 
                                              &(pathStr[count]), dirFoundPtr);
                        if(rc <= 0)
                        {
                            free(currentDirName);
                            return rc;
                        }
                        stopLooking = true;
                    }
                        
                }
                
                child = child->next;
            }
            
            free(currentDirName);
            
            if(!stopLooking)
                return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
        } /* if(found it) */
    } /* for(find the first directory in the path) */
    
    /* can't see how i could get here but to keep the compiler happy */
    return 1;
}
