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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#include "bkInternal.h"
#include "bkLink.h"

int addToHardLinkTable(VolInfo* volInfo, off_t position, char* pathAndName, 
                       unsigned size, bool onImage, BkHardLink** newLink)
{
    int rc;
    
    *newLink = malloc(sizeof(BkHardLink));
    if(*newLink == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    bzero(*newLink, sizeof(BkHardLink));
    
    (*newLink)->onImage = onImage;
    (*newLink)->position = position;
    if(pathAndName != NULL)
    {
        (*newLink)->pathAndName = malloc(strlen(pathAndName) + 1);
        if((*newLink)->pathAndName == NULL)
            return BKERROR_OUT_OF_MEMORY;
        strcpy((*newLink)->pathAndName, pathAndName);
    }
    (*newLink)->size = size;
    (*newLink)->next = volInfo->fileLocations;
    
    if(size < MAX_NBYTES_HARDLINK_HEAD)
        (*newLink)->headSize = size;
    else
        (*newLink)->headSize = MAX_NBYTES_HARDLINK_HEAD;
    
    rc = readFileHead(volInfo, position, pathAndName, (*newLink)->onImage, 
                      (*newLink)->head, (*newLink)->headSize);
    if(rc <= 0)
        return rc;
    
    volInfo->fileLocations = *newLink;
    
    return 1;
}

/* returns 2 if yes 1 if not
* works even if file1 == file2 */
int filesAreSame(VolInfo* volInfo, int file1, off_t posFile1, 
                 int file2, off_t posFile2, unsigned size)
{
    off_t origPosFile1;
    off_t origPosFile2;
    int numBlocks;
    int sizeLastBlock;
    int count;
    int rc;
    bool sameSoFar;
    
    if(size == 0)
        return 2;
    
    origPosFile1 = lseek(file1, 0, SEEK_CUR);
    origPosFile2 = lseek(file2, 0, SEEK_CUR);
    
    numBlocks = size / READ_WRITE_BUFFER_SIZE;
    sizeLastBlock = size % READ_WRITE_BUFFER_SIZE;
    
    sameSoFar = true;
    for(count = 0; count < numBlocks; count++)
    {
        lseek(file1, posFile1, SEEK_SET);
        rc = read(file1, volInfo->readWriteBuffer, READ_WRITE_BUFFER_SIZE);
        if(rc != READ_WRITE_BUFFER_SIZE)
            return BKERROR_READ_GENERIC;
        posFile1 = lseek(file1, 0, SEEK_CUR);
        
        lseek(file2, posFile2, SEEK_SET);
        rc = read(file2, volInfo->readWriteBuffer2, READ_WRITE_BUFFER_SIZE);
        if(rc != READ_WRITE_BUFFER_SIZE)
            return BKERROR_READ_GENERIC;
        posFile2 = lseek(file2, 0, SEEK_CUR);
        
        if( memcmp(volInfo->readWriteBuffer, volInfo->readWriteBuffer2, 
            READ_WRITE_BUFFER_SIZE) != 0 )
        {
            sameSoFar = false;
            break;
        }
    }
    
    if(sameSoFar && sizeLastBlock > 0)
    {
        lseek(file1, posFile1, SEEK_SET);
        rc = read(file1, volInfo->readWriteBuffer, sizeLastBlock);
        if(rc != sizeLastBlock)
            return BKERROR_READ_GENERIC;
        
        lseek(file2, posFile2, SEEK_SET);
        rc = read(file2, volInfo->readWriteBuffer2, sizeLastBlock);
        if(rc != sizeLastBlock)
            return BKERROR_READ_GENERIC;
        
        if(memcmp(volInfo->readWriteBuffer, volInfo->readWriteBuffer2, sizeLastBlock) != 0)
            sameSoFar = false;
    }
    
    lseek(file1, origPosFile1, SEEK_SET);
    lseek(file2, origPosFile2, SEEK_SET);
    
    if(sameSoFar)
        return 2;
    else
        return 1;
}

/* returns 2 if found 1 if not found */
int findInHardLinkTable(VolInfo* volInfo, off_t position, 
                        char* pathAndName, unsigned size,
                        bool onImage, BkHardLink** foundLink)
{
    BkHardLink* currentLink;
    unsigned char head[MAX_NBYTES_HARDLINK_HEAD];
    int headSize;
    int rc;
    
    *foundLink = NULL;
    
    if(size < MAX_NBYTES_HARDLINK_HEAD)
        headSize = size;
    else
        headSize = MAX_NBYTES_HARDLINK_HEAD;
    
    rc = readFileHead(volInfo, position, pathAndName, onImage, head, headSize);
    if(rc <= 0)
        return rc;
    
    currentLink = volInfo->fileLocations;
    while(currentLink != NULL)
    {
        if(size == currentLink->size)
        {
            if( memcmp(head, currentLink->head, headSize) == 0 )
            {
                int origFile;
                int origFileWasOpened;
                off_t origFileOffset;
                int newFile;
                bool newFileWasOpened;
                off_t newFileOffset;
                
                /* set up for reading original file */
                if(currentLink->onImage)
                {
                    origFile = volInfo->imageForReading;
                    origFileWasOpened = false;
                    origFileOffset = currentLink->position;
                }
                else
                {
                    origFile = open(pathAndName, O_RDONLY);
                    if(origFile == -1)
                        return BKERROR_OPEN_READ_FAILED;
                    origFileWasOpened = true;
                    origFileOffset = 0;
                }
                
                /* set up for reading new file */
                if(onImage)
                {
                    newFile = volInfo->imageForReading;
                    newFileWasOpened = false;
                    newFileOffset = position;
                }
                else
                {
                    newFile = open(pathAndName, O_RDONLY);
                    if(newFile == -1)
                    {
                        if(origFileWasOpened)
                            close(origFile);
                        return BKERROR_OPEN_READ_FAILED;
                    }
                    newFileWasOpened = true;
                    newFileOffset = 0;
                }
                
                rc = filesAreSame(volInfo, origFile, origFileOffset, 
                                  newFile, newFileOffset, size);
                
                if(origFileWasOpened)
                    close(origFile);
                if(newFileWasOpened)
                    close(newFile);
                
                if(rc < 0)
                    return rc;
                
                if(rc == 2)
                {
                    *foundLink = currentLink;
                    return 2;
                }
            }
        }
        
        currentLink = currentLink->next;
    }
    
    return 1;
}

int readFileHead(VolInfo* volInfo, off_t position, char* pathAndName, 
                 bool onImage, unsigned char* dest, unsigned numBytes)
{
    int srcFile;
    bool srcFileWasOpened;
    off_t origPos;
    int rc;
    
    if(onImage)
    {
        srcFile = volInfo->imageForReading;
        origPos = lseek(volInfo->imageForReading, 0, SEEK_CUR);
        lseek(volInfo->imageForReading, position, SEEK_SET);
        srcFileWasOpened = false;
    }
    else
    {
        srcFile = open(pathAndName, O_RDONLY);
        if(srcFile == -1)
            return BKERROR_OPEN_READ_FAILED;
        srcFileWasOpened = true;
    }
    
    rc = read(srcFile, dest, numBytes);
    
    if(!srcFileWasOpened)
        lseek(volInfo->imageForReading, origPos, SEEK_SET);
    else
        close(srcFile);
    
    if(rc != numBytes)
        return BKERROR_READ_GENERIC;
    
    return 1;
}

void resetWriteStatus(BkHardLink* fileLocations)
{
    BkHardLink* currentNode;
    
    currentNode = fileLocations;
    while(currentNode != NULL)
    {
        currentNode->extentNumberWrittenTo = 0;
        
        currentNode = currentNode->next;
    }
}
