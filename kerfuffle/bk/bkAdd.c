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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bk.h"
#include "bkPath.h"
#include "bkAdd.h"
#include "bkError.h"
#include "bkGet.h"
#include "bkMangle.h"
#include "bkLink.h"
#include "bkMisc.h"

int add(VolInfo* volInfo, const char* srcPathAndName, BkDir* destDir)
{
    int rc;
    char lastName[NCHARS_FILE_ID_MAX_STORE];
    BkFileBase* oldHead; /* of the children list */
    struct stat statStruct;
    
    if(volInfo->stopOperation)
        return BKERROR_OPER_CANCELED_BY_USER;
    
    maybeUpdateProgress(volInfo);
    
    rc = getLastNameFromPath(srcPathAndName, lastName);
    if(rc <= 0)
        return rc;
    
    if(strcmp(lastName, ".") == 0 || strcmp(lastName, "..") == 0)
        return BKERROR_NAME_INVALID;
    
    if( !nameIsValid(lastName) )
        return BKERROR_NAME_INVALID_CHAR;
    
    oldHead = destDir->children;
    
    if(volInfo->followSymLinks)
        rc = stat(srcPathAndName, &statStruct);
    else
        rc = lstat(srcPathAndName, &statStruct);
    if(rc == -1)
        return BKERROR_STAT_FAILED;
    
    if( IS_DIR(statStruct.st_mode) )
    {
        BkDir* newDir;
        
        newDir = malloc(sizeof(BkDir));
        if(newDir == NULL)
            return BKERROR_OUT_OF_MEMORY;
        
        bzero(newDir, sizeof(BkDir));
        
        strcpy(BK_BASE_PTR(newDir)->name, lastName);
        
        BK_BASE_PTR(newDir)->posixFileMode = statStruct.st_mode;
        
        BK_BASE_PTR(newDir)->next = oldHead;
        
        newDir->children = NULL;
        
        /* ADD dir contents */
        rc = addDirContents(volInfo, srcPathAndName, newDir);
        if(rc < 0)
        {
            free(newDir);
            return rc;
        }
        /* END ADD dir contents */
        
        destDir->children = BK_BASE_PTR(newDir);
    }
    else if( IS_REG_FILE(statStruct.st_mode) )
    {
        BkFile* newFile;
        
        if(statStruct.st_size > 0xFFFFFFFF)
        /* size won't fit in a 32bit variable on the iso */
            return BKERROR_ADD_FILE_TOO_BIG;
        
        newFile = malloc(sizeof(BkFile));
        if(newFile == NULL)
            return BKERROR_OUT_OF_MEMORY;
        
        bzero(newFile, sizeof(BkFile));
        
        strcpy(BK_BASE_PTR(newFile)->name, lastName);
        
        BK_BASE_PTR(newFile)->posixFileMode = statStruct.st_mode;
        
        BK_BASE_PTR(newFile)->next = oldHead;
        
        newFile->size = statStruct.st_size;
        
        newFile->onImage = false;
        
        newFile->position = 0;
        
        newFile->pathAndName = malloc(strlen(srcPathAndName) + 1);
        strcpy(newFile->pathAndName, srcPathAndName);
        
        if( volInfo->scanForDuplicateFiles)
        {
            BkHardLink* newLink;
            
            rc = findInHardLinkTable(volInfo, 0, newFile->pathAndName, 
                                     statStruct.st_size, false, &newLink);
            if(rc < 0)
            {
                free(newFile);
                return rc;
            }
            
            if(newLink == NULL)
            /* not found */
            {
                rc = addToHardLinkTable(volInfo, 0, newFile->pathAndName, 
                                        statStruct.st_size, false, &newLink);
                if(rc < 0)
                {
                    free(newFile);
                    return rc;
                }
            }
            
            newFile->location = newLink;
        }
        
        destDir->children = BK_BASE_PTR(newFile);
    }
    else if( IS_SYMLINK(statStruct.st_mode) )
    {
        BkSymLink* newSymLink;
        ssize_t numChars;
        
        newSymLink = malloc(sizeof(BkSymLink));
        if(newSymLink == NULL)
            return BKERROR_OUT_OF_MEMORY;
        
        bzero(newSymLink, sizeof(BkSymLink));
        
        strcpy(BK_BASE_PTR(newSymLink)->name, lastName);
        
        BK_BASE_PTR(newSymLink)->posixFileMode = statStruct.st_mode;
        
        BK_BASE_PTR(newSymLink)->next = oldHead;
        
        numChars = readlink(srcPathAndName, newSymLink->target, 
                            NCHARS_SYMLINK_TARGET_MAX - 1);
        if(numChars == -1)
        {
            free(newSymLink);
            return BKERROR_OPEN_READ_FAILED;
        }
        newSymLink->target[numChars] = '\0';
        
        destDir->children = BK_BASE_PTR(newSymLink);
    }
    else
        return BKERROR_NO_SPECIAL_FILES;
    
    return 1;
}

int addDirContents(VolInfo* volInfo, const char* srcPath, BkDir* destDir)
{
    int rc;
    int srcPathLen;
    char* newSrcPathAndName;
    
    /* vars to read contents of a dir on fs */
    DIR* srcDir;
    struct dirent* dirEnt;
    
    srcPathLen = strlen(srcPath);
    
    /* including the new name and the possibly needed trailing '/' */
    newSrcPathAndName = malloc(srcPathLen + NCHARS_FILE_ID_MAX_STORE + 1);
    if(newSrcPathAndName == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    strcpy(newSrcPathAndName, srcPath);
    if(srcPath[srcPathLen - 1] != '/')
    {
        strcat(newSrcPathAndName, "/");
        srcPathLen++;
    }
    
    srcDir = opendir(srcPath);
    if(srcDir == NULL)
    {
        free(newSrcPathAndName);
        return BKERROR_OPENDIR_FAILED;
    }
    
    /* it may be possible but in any case very unlikely that readdir() will fail
    * if it does, it returns NULL (same as end of dir) */
    while( (dirEnt = readdir(srcDir)) != NULL )
    {
        if( strcmp(dirEnt->d_name, ".") == 0 || strcmp(dirEnt->d_name, "..") == 0 )
        /* ignore "." and ".." */
            continue;
        
        if(strlen(dirEnt->d_name) > NCHARS_FILE_ID_MAX_STORE - 1)
        {
            closedir(srcDir);
            free(newSrcPathAndName);
            
            return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
        }
        
        /* append file/dir name */
        strcpy(newSrcPathAndName + srcPathLen, dirEnt->d_name);
        
        rc = add(volInfo, newSrcPathAndName, destDir);
        if(rc <= 0 && rc != BKWARNING_OPER_PARTLY_FAILED)
        {
            bool goOn;
            
            if(volInfo->warningCbk != NULL && !volInfo->stopOperation)
            /* perhaps the user wants to ignore this failure */
            {
                snprintf(volInfo->warningMessage, BK_WARNING_MAX_LEN, 
                         "Failed to add item '%s': '%s'",
                         dirEnt->d_name, 
                         bk_get_error_string(rc));
                goOn = volInfo->warningCbk(volInfo->warningMessage);
                rc = BKWARNING_OPER_PARTLY_FAILED;
            }
            else
                goOn = false;
            
            if(goOn)
                continue;
            else
            {
                volInfo->stopOperation = true;
                closedir(srcDir);
                free(newSrcPathAndName);
                return rc;
            }
        }
    }
    
    free(newSrcPathAndName);
    
    rc = closedir(srcDir);
    if(rc != 0)
    /* exotic error */
        return BKERROR_EXOTIC;
    
    return 1;
}

int bk_add(VolInfo* volInfo, const char* srcPathAndName, 
           const char* destPathStr, void(*progressFunction)(VolInfo*))
{
    int rc;
    NewPath destPath;
    char lastName[NCHARS_FILE_ID_MAX_STORE];
    
    /* vars to find the dir in the tree */
    BkDir* destDirInTree;
    bool dirFound;
    
    volInfo->progressFunction = progressFunction;
    
    rc = makeNewPathFromString(destPathStr, &destPath);
    if(rc <= 0)
    {
        freePathContents(&destPath);
        return rc;
    }
    
    rc = getLastNameFromPath(srcPathAndName, lastName);
    if(rc <= 0)
    {
        freePathContents(&destPath);
        return rc;
    }
    
    dirFound = findDirByNewPath(&destPath, &(volInfo->dirTree), &destDirInTree);
    if(!dirFound)
    {
        freePathContents(&destPath);
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    }
    
    freePathContents(&destPath);
    
    if(itemIsInDir(lastName, destDirInTree))
        return BKERROR_DUPLICATE_ADD;
    
    volInfo->stopOperation = false;
    
    rc = add(volInfo, srcPathAndName, destDirInTree);
    if(rc <= 0)
        return rc;
    
    return 1;
}

/*******************************************************************************
* bk_add_boot_record()
* Source boot file must be exactly the right size if floppy emulation requested.
* */
int bk_add_boot_record(VolInfo* volInfo, const char* srcPathAndName, 
                       int bootMediaType)
{
    struct stat statStruct;
    int rc;
    
    if(bootMediaType != BOOT_MEDIA_NO_EMULATION &&
       bootMediaType != BOOT_MEDIA_1_2_FLOPPY &&
       bootMediaType != BOOT_MEDIA_1_44_FLOPPY &&
       bootMediaType != BOOT_MEDIA_2_88_FLOPPY)
    {
        return BKERROR_ADD_UNKNOWN_BOOT_MEDIA;
    }
    
    rc = stat(srcPathAndName, &statStruct);
    if(rc == -1)
        return BKERROR_STAT_FAILED;
    
    if( (bootMediaType == BOOT_MEDIA_1_2_FLOPPY &&
         statStruct.st_size != 1228800) ||
        (bootMediaType == BOOT_MEDIA_1_44_FLOPPY &&
         statStruct.st_size != 1474560) ||
        (bootMediaType == BOOT_MEDIA_2_88_FLOPPY &&
         statStruct.st_size != 2949120) )
    {
        return BKERROR_ADD_BOOT_RECORD_WRONG_SIZE;
    }
    
    volInfo->bootMediaType = bootMediaType;
    
    volInfo->bootRecordSize = statStruct.st_size;
    
    volInfo->bootRecordIsOnImage = false;
    
    /* make copy of the source path and name */
    if(volInfo->bootRecordPathAndName != NULL)
        free(volInfo->bootRecordPathAndName);
    volInfo->bootRecordPathAndName = malloc(strlen(srcPathAndName) + 1);
    if(volInfo->bootRecordPathAndName == NULL)
    {
        volInfo->bootMediaType = BOOT_MEDIA_NONE;
        return BKERROR_OUT_OF_MEMORY;
    }
    strcpy(volInfo->bootRecordPathAndName, srcPathAndName);
    
    /* this is the wrong function to use if you want a visible one */
    volInfo->bootRecordIsVisible = false;
    
    return 1;
}

/*******************************************************************************
* bk_create_dir()
* 
* */
int bk_create_dir(VolInfo* volInfo, const char* destPathStr, 
                  const char* newDirName)
{
    int nameLen;
    BkDir* destDir;
    int rc;
    BkFileBase* oldHead;
    BkDir* newDir;
    
    nameLen = strlen(newDirName);
    if(nameLen > NCHARS_FILE_ID_MAX_STORE - 1)
        return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
    if(nameLen == 0)
        return BKERROR_BLANK_NAME;
    
    if(strcmp(newDirName, ".") == 0 || strcmp(newDirName, "..") == 0)
        return BKERROR_NAME_INVALID;
    
    if( !nameIsValid(newDirName) )
        return BKERROR_NAME_INVALID_CHAR;
    
    rc = getDirFromString(&(volInfo->dirTree), destPathStr, &destDir);
    if(rc <= 0)
        return rc;
    
    if(itemIsInDir(newDirName, destDir))
        return BKERROR_DUPLICATE_CREATE_DIR;
    
    oldHead = destDir->children;
    
    newDir = malloc(sizeof(BkDir));
    if(newDir == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    strcpy(BK_BASE_PTR(newDir)->name, newDirName);
    
    BK_BASE_PTR(newDir)->posixFileMode = volInfo->posixDirDefaults;
    
    BK_BASE_PTR(newDir)->next = oldHead;
    
    newDir->children = NULL;
    
    destDir->children = BK_BASE_PTR(newDir);
    
    return 1;
}

/*******************************************************************************
* itemIsInDir()
* checks the contents of a directory (files and dirs) to see whether it
* has an item named 
* */
bool itemIsInDir(const char* name, const BkDir* dir)
{
    BkFileBase* child;
    
    child = dir->children;
    while(child != NULL)
    {
        if(strcmp(child->name, name) == 0)
            return true;
        child = child->next;
    }
    
    return false;
}
