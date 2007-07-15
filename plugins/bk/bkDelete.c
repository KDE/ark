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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <strings.h>
#include <stdio.h>

#include "bk.h"
#include "bkInternal.h"
#include "bkPath.h"
#include "bkError.h"
#include "bkDelete.h"

/*******************************************************************************
* bk_delete_boot_record()
* deletes whatever reference to a boot record volinfo has
* */
void bk_delete_boot_record(VolInfo* volInfo)
{
    volInfo->bootMediaType = BOOT_MEDIA_NONE;
    
    if(volInfo->bootRecordPathAndName != NULL)
    {
        free(volInfo->bootRecordPathAndName);
        volInfo->bootRecordPathAndName = NULL;
    }
}

int bk_delete(VolInfo* volInfo, const char* pathAndName)
{
    int rc;
    NewPath path;
    bool dirFound;
    BkDir* parentDir;
    
    if(path.numChildren == 0)
    {
        freePathContents(&path);
        return BKERROR_DELETE_ROOT;
    }
    
    rc = makeNewPathFromString(pathAndName, &path);
    if(rc <= 0)
    {
        freePathContents(&path);
        return rc;
    }
    
    /* i want the parent directory */
    path.numChildren--;
    dirFound = findDirByNewPath(&path, &(volInfo->dirTree), &parentDir);
    path.numChildren++;
    if(!dirFound)
    {
        freePathContents(&path);
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    }
    
    deleteNode(volInfo, parentDir, path.children[path.numChildren - 1]);
    
    freePathContents(&path);
    
    return 1;
}

void deleteNode(VolInfo* volInfo, BkDir* parentDir, char* nodeToDeleteName)
{
    BkFileBase** childPtr;
    BkFileBase* nodeToFree;
    
    childPtr = &(parentDir->children);
    while(*childPtr != NULL)
    {
        if( strcmp((*childPtr)->name, nodeToDeleteName) == 0 )
        {
            nodeToFree = *childPtr;
            
            *childPtr = (*childPtr)->next;
            
            if( IS_DIR(nodeToFree->posixFileMode) )
            {
                deleteDirContents(volInfo, BK_DIR_PTR(nodeToFree));
            }
            else if ( IS_REG_FILE(nodeToFree->posixFileMode) )
            {
                deleteRegFileContents(volInfo, BK_FILE_PTR(nodeToFree));
            }
            /* else the free below will be enough */
            
            free(nodeToFree);
            
            break;
        }
        
        childPtr = &((*childPtr)->next);
    }
}

/*******************************************************************************
* deleteDirContents()
* deletes all the contents of a directory
* recursive
* */
void deleteDirContents(VolInfo* volInfo, BkDir* dir)
{
    BkFileBase* child;
    BkFileBase* nextChild;
    
    child = dir->children;
    while(child != NULL)
    {
        nextChild = child->next;
        
        deleteNode(volInfo, dir, child->name);
        
        child = nextChild;
    }
}

/* delete the contents of the BkFile structure, not the actual file contents */
void deleteRegFileContents(VolInfo* volInfo, BkFile* file)
{
    if( file->onImage )
        free( file->pathAndName );
    
    /* check whether file is being used as a boot record */
    if(volInfo->bootMediaType != BOOT_MEDIA_NONE &&
       volInfo->bootMediaType == BOOT_MEDIA_NO_EMULATION)
    {
        if(volInfo->bootRecordIsVisible && 
           volInfo->bootRecordOnImage == file)
        {
            /* and stop using it. perhaps insert a hook here one day to
            * let the user know the boot record has been/will be deleted */
            bk_delete_boot_record(volInfo);
        }
    }
}
