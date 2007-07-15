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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bk.h"
#include "bkInternal.h"
#include "bkError.h"
#include "bkPath.h"
#include "bkMangle.h"

bool findDirByNewPath(const NewPath* path, BkDir* tree, BkDir** dir)
{
    bool dirFound;
    int count;
    BkFileBase* child;
    
    *dir = tree;
    for(count = 0; count < path->numChildren; count++)
    /* each directory in the path */
    {
        child = (*dir)->children;
        dirFound = false;
        while(child != NULL && !dirFound)
        /* find the directory */
        {
            if(strcmp(child->name, path->children[count]) == 0)
            {
                if( !IS_DIR(child->posixFileMode) )
                    return false;
                
                dirFound = true;
                *dir = BK_DIR_PTR(child);
            }
            else
                child = child->next;
        }
        if(!dirFound)
            return false;
    }
    /* END FIND dir to add to */
    
    return true;
}

/******************************************************************************
* freeDirToWriteContents()
* Recursively deletes all the dynamically allocated contents of dir.
* */
void freeDirToWriteContents(DirToWrite* dir)
{
    BaseToWrite* currentChild;
    BaseToWrite* nextChild;
    
    currentChild = dir->children;
    while(currentChild != NULL)
    {
        nextChild = currentChild->next;
        
        if( IS_DIR(currentChild->posixFileMode) )
        {
            freeDirToWriteContents(DIRTW_PTR(currentChild));
        }
        else if( IS_REG_FILE(currentChild->posixFileMode) )
        {
            if(!FILETW_PTR(currentChild)->onImage)
                free(FILETW_PTR(currentChild)->pathAndName);
        }
        
        free(currentChild);
        
        currentChild = nextChild;
    }
}

void freePathContents(NewPath* path)
{
    int count;
    
    for(count = 0; count < path->numChildren; count++)
    {
        /* if the path was not allocated properly (maybe ran out of memory)
        * the first unallocated item is null */
        if(path->children[count] == NULL)
            break;
        
        free(path->children[count]);
    }
    
    if(path->children != NULL)
        free(path->children);
}

int getLastNameFromPath(const char* srcPathAndName, char* lastName)
{
    int count;
    int srcLen;
    int lastCharIndex;
    int firstCharIndex;
    bool lastCharFound;
    int count2;
    
    srcLen = strlen(srcPathAndName);
    
    /* FIND the last name */
    lastCharIndex = srcLen;
    lastCharFound = false;
    for(count = srcLen; count >= 0; count--)
    {
        if(srcPathAndName[count] != '/')
        {
            if(!lastCharFound)
            {
                lastCharIndex = count;
                lastCharFound = true;

                firstCharIndex = lastCharIndex;
            }
            else
            {
                firstCharIndex = count;
            }
        }
        else
        {
            if(lastCharFound)
                break;
        }
    }
    if(!lastCharFound)
        return BKERROR_MISFORMED_PATH;
    /* END FIND the last name */
    
    if(lastCharIndex - firstCharIndex > NCHARS_FILE_ID_MAX_STORE - 1)
        return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
    
    /* copy the name */
    for(count = firstCharIndex, count2 = 0; count <= lastCharIndex; count++, count2++)
    {
        lastName[count2] = srcPathAndName[count];
    }
    lastName[count2] = '\0';
    
    return 1;
}

int makeNewPathFromString(const char* strPath, NewPath* pathPath)
{
    int count;
    int pathStrLen;
    
    pathStrLen = strlen(strPath);
    
    if(strPath[0] != '/')
        return BKERROR_MISFORMED_PATH;
    
    /* count number of children */
    pathPath->numChildren = 0;
    for(count = 1; count < pathStrLen; count++)
    {
        if(strPath[count] != '/' && strPath[count - 1] == '/')
            pathPath->numChildren++;
    }
    
    if(pathPath->numChildren == 0)
    {
        pathPath->children = NULL;
        return 1;
    }
    
    pathPath->children = (char**)malloc(sizeof(char*) * pathPath->numChildren);
    if(pathPath->children == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    int numChildrenDone = 0;
    int nextChildLen = 0;
    const char* nextChild = &(strPath[1]);
    for(count = 1; count <= pathStrLen; count++)
    {
        if(strPath[count] == '/' || (strPath[count] == '\0' && strPath[count - 1] != '/'))
        {
            if(strPath[count] == '/' && strPath[count - 1] == '/')
            /* double slash */
            {
                nextChild = &(strPath[count + 1]);
                continue;
            }
            else
            /* this is the end of the string or the slash following a dir name  */
            {
                pathPath->children[numChildrenDone] = (char*)malloc(nextChildLen + 1);
                if(pathPath->children[numChildrenDone] == NULL)
                    return BKERROR_OUT_OF_MEMORY;
                
                strncpy(pathPath->children[numChildrenDone], nextChild, nextChildLen);
                pathPath->children[numChildrenDone][nextChildLen] = '\0';
                
                numChildrenDone++;
                nextChildLen = 0;
                
                nextChild = &(strPath[count + 1]);
            }
        }
        else
        {
            nextChildLen++;
        }
    }
    
    if(numChildrenDone != pathPath->numChildren)
        return BKERROR_SANITY;
    
    return 1;
}

/******************************************************************************
* nameIsValid()
* Checks each character in name to see whether it's allowed in an identifier
* */
bool nameIsValid(const char* name)
{
    int count;
    int nameLen;
    
    nameLen = strlen(name);
    
    for(count = 0; count < nameLen; count++)
    {
        /* can be any ascii char between decimal 32 and 126 except '/' (47) */
        if(name[count] < 32 || name[count] > 126 || name[count] == 47)
            return false;
    }
    
    return true;
}

#ifdef DEBUG
void printDirToWrite(DirToWrite* dir, int level, int filenameTypes)
{
    BaseToWrite* child;
    int count;
    
    child = dir->children;
    while(child != NULL)
    {
        if(IS_DIR(child->posixFileMode))
        {
            if(filenameTypes & FNTYPE_9660)
            {
                for(count = 0; count < level; count ++)
                    printf("  ");
                printf("dir9 '%s'\n", child->name9660);fflush(NULL);
            }
            
            if(filenameTypes & FNTYPE_JOLIET)
            {
                for(count = 0; count < level; count ++)
                    printf("  ");
                printf("dirJ '%s'\n", child->nameJoliet);fflush(NULL);
            }
            
            if(filenameTypes & FNTYPE_ROCKRIDGE)
            {
                for(count = 0; count < level; count ++)
                    printf("  ");
                printf("dirR '%s'\n", child->nameRock);fflush(NULL);
            }
            
            printDirToWrite(DIRTW_PTR(child), level + 1, filenameTypes);
        }
        
        child = child->next;
    }
    
    child = dir->children;
    while(child != NULL)
    {
        if(!IS_DIR(child->posixFileMode))
        {
            if(filenameTypes & FNTYPE_9660)
            {
                for(count = 0; count < level; count ++)
                    printf("  ");
                printf("file9 '%s'\n", child->name9660);fflush(NULL);
            }
            
            if(filenameTypes & FNTYPE_JOLIET)
            {
                for(count = 0; count < level; count ++)
                    printf("  ");
                printf("fileJ '%s'\n", child->nameJoliet);fflush(NULL);
            }
            
            if(filenameTypes & FNTYPE_ROCKRIDGE)
            {
                for(count = 0; count < level; count ++)
                    printf("  ");
                printf("fileR '%s'\n", child->nameRock);fflush(NULL);
            }
        }
        
        child = child->next;
    }
}
#endif /* DEBUG */
