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
* unsutable for anything else.
******************************************************************************/

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <strings.h>

#include "bk.h"
#include "bkInternal.h"
#include "bkWrite7x.h"
#include "bkTime.h"
#include "bkWrite.h"
#include "bkMangle.h"
#include "bkError.h"
#include "bkSort.h"
#include "bkPath.h"
#include "bkCache.h"
#include "bkRead7x.h"
#include "bkLink.h"

/******************************************************************************
* bk_write_image()
* Writes everything from first to last byte of the iso.
* Public function.
* */
int bk_write_image(const char* newImagePathAndName, VolInfo* volInfo, 
                   time_t creationTime, int filenameTypes, 
                   void(*progressFunction)(VolInfo*, double))
{
    int rc;
    struct stat statStruct;
    DirToWrite newTree;
    off_t svdOffset;
    off_t pRealRootDrOffset;
    int pRootDirSize;
    off_t sRealRootDrOffset;
    int sRootDirSize;
    off_t lPathTable9660Loc;
    off_t mPathTable9660Loc;
    int pathTable9660Size;
    off_t lPathTableJolietLoc;
    off_t mPathTableJolietLoc;
    int pathTableJolietSize;
    off_t bootCatalogSectorNumberOffset;
    off_t currPos;
    
    volInfo->writeProgressFunction = progressFunction;
    volInfo->stopOperation = false;
    
    volInfo->estimatedIsoSize = bk_estimate_iso_size(volInfo, filenameTypes);
    progressFunction(volInfo, 0);
    
    rc = stat(newImagePathAndName, &statStruct);
    if(rc == 0 && statStruct.st_ino == volInfo->imageForReadingInode)
        return BKERROR_SAVE_OVERWRITE;
    
    /* because mangleDir works on dir's children i need to 
    * copy the root manually */
    bzero(&newTree, sizeof(DirToWrite));
    newTree.base.posixFileMode = volInfo->dirTree.base.posixFileMode;
    
    printf("mangling\n");fflush(NULL);
    /* create tree to write */
    rc = mangleDir(&(volInfo->dirTree), &newTree, filenameTypes);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        return rc;
    }
    
    printf("opening '%s' for writing\n", newImagePathAndName);fflush(NULL);
    volInfo->imageForWriting = open(newImagePathAndName, 
                                    O_WRONLY | O_CREAT | O_TRUNC, 
                                    S_IRUSR | S_IWUSR);
    if(volInfo->imageForWriting == -1)
    {
        freeDirToWriteContents(&newTree);
        return BKERROR_OPEN_WRITE_FAILED;
    }
    
    printf("writing blank at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
    /* system area, always zeroes */
    rc = writeByteBlock(volInfo, 0, NBYTES_LOGICAL_BLOCK * NLS_SYSTEM_AREA);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        close(volInfo->imageForWriting);
        unlink(newImagePathAndName);
        return rc;
    }
    
    /* skip pvd (1 block), write it after files */
    wcSeekForward(volInfo, NBYTES_LOGICAL_BLOCK);
    
    if(volInfo->bootMediaType != BOOT_MEDIA_NONE)
    {
        /* el torito volume descriptor */
        rc = writeElToritoVd(volInfo, &bootCatalogSectorNumberOffset);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
    }
    
    if(filenameTypes & FNTYPE_JOLIET)
    /* skip svd (1 block), write it after pvd */
    {
        svdOffset = wcSeekTell(volInfo);
        wcSeekForward(volInfo, NBYTES_LOGICAL_BLOCK);
    }
    
    printf("writing terminator at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
    /* volume descriptor set terminator */
    rc = writeVdsetTerminator(volInfo);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        close(volInfo->imageForWriting);
        unlink(newImagePathAndName);
        return rc;
    }
    
    if(volInfo->bootMediaType != BOOT_MEDIA_NONE)
    {
        /* write boot catalog sector number */
        currPos = wcSeekTell(volInfo);
        wcSeekSet(volInfo, bootCatalogSectorNumberOffset);
        rc = write731(volInfo, currPos / NBYTES_LOGICAL_BLOCK);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
        wcSeekSet(volInfo, currPos);
        
        /* write el torito booting catalog */
        rc = writeElToritoBootCatalog(volInfo, &(volInfo->bootRecordSectorNumberOffset));
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
    }
    
    /* MAYBE write boot record file now */
    if(volInfo->bootMediaType != BOOT_MEDIA_NONE && 
       !volInfo->bootRecordIsVisible)
    {
        int blankSize;
        int srcFile; /* either the old image or the boot record file on 
                     * the regular filesystem */
        bool srcFileOpened;
        
        /* set up source file pointer */
        if(volInfo->bootRecordIsOnImage)
        {
            srcFile = volInfo->imageForReading;
            lseek(volInfo->imageForReading, volInfo->bootRecordOffset, SEEK_SET);
            srcFileOpened = false;
        }
        else
        {
            srcFile = open(volInfo->bootRecordPathAndName, O_RDONLY);
            if(srcFile == -1)
            {
                freeDirToWriteContents(&newTree);
                close(volInfo->imageForWriting);
                unlink(newImagePathAndName);
                return BKERROR_OPEN_READ_FAILED;
            }
            srcFileOpened = true;
        }
        
        /* write boot record sector number */
        currPos = wcSeekTell(volInfo);
        wcSeekSet(volInfo, volInfo->bootRecordSectorNumberOffset);
        
        rc = write731(volInfo, currPos / NBYTES_LOGICAL_BLOCK);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            if(srcFileOpened)
                close(srcFile);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
        wcSeekSet(volInfo, currPos);
        
        /* file contents */
        rc = writeByteBlockFromFile(srcFile, volInfo, volInfo->bootRecordSize);
        if(rc < 0)
        {
            freeDirToWriteContents(&newTree);
            if(srcFileOpened)
                close(srcFile);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
        
        blankSize = NBYTES_LOGICAL_BLOCK - 
                    volInfo->bootRecordSize % NBYTES_LOGICAL_BLOCK;
        
        /* fill the last sector with 0s */
        rc = writeByteBlock(volInfo, 0x00, blankSize);
        if(rc < 0)
        {
            freeDirToWriteContents(&newTree);
            if(srcFileOpened)
                close(srcFile);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
        
        if(srcFileOpened)
            close(srcFile);
    }
    /* END MAYBE write boot record file now */
    
    printf("sorting 9660\n");
    sortDir(&newTree, FNTYPE_9660);
    
    pRealRootDrOffset = wcSeekTell(volInfo);
    
    printf("writing primary directory tree at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
    /* 9660 and maybe rockridge dir tree */
    rc = writeDir(volInfo, &newTree, 0, 0, 0, creationTime, 
                  filenameTypes & (FNTYPE_9660 | FNTYPE_ROCKRIDGE), true);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        close(volInfo->imageForWriting);
        unlink(newImagePathAndName);
        return rc;
    }
    
    pRootDirSize = rc;
    
    /* joliet dir tree */
    if(filenameTypes & FNTYPE_JOLIET)
    {
        printf("sorting joliet\n");
        sortDir(&newTree, FNTYPE_JOLIET);
        
        printf("writing supplementary directory tree at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
        sRealRootDrOffset = wcSeekTell(volInfo);
        
        rc = writeDir(volInfo, &newTree, 0, 0, 0, creationTime, 
                      FNTYPE_JOLIET, true);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
        
        sRootDirSize = rc;
    }
    
    printf("writing 9660 path tables at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
    
    lPathTable9660Loc = wcSeekTell(volInfo);
    rc = writePathTable(volInfo, &newTree, true, FNTYPE_9660);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        close(volInfo->imageForWriting);
        unlink(newImagePathAndName);
        return rc;
    }
    pathTable9660Size = rc;
    
    mPathTable9660Loc = wcSeekTell(volInfo);
    rc = writePathTable(volInfo, &newTree, false, FNTYPE_9660);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        close(volInfo->imageForWriting);
        unlink(newImagePathAndName);
        return rc;
    }
    
    if(filenameTypes & FNTYPE_JOLIET)
    {
        printf("writing joliet path tables at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
        lPathTableJolietLoc = wcSeekTell(volInfo);
        rc = writePathTable(volInfo, &newTree, true, FNTYPE_JOLIET);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
        pathTableJolietSize = rc;
        
        mPathTableJolietLoc = wcSeekTell(volInfo);
        rc = writePathTable(volInfo, &newTree, false, FNTYPE_JOLIET);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
    }
    
    printf("writing files at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
    resetWriteStatus(volInfo->fileLocations);
    /* all files and offsets/sizes */
    rc = writeFileContents(volInfo, &newTree, filenameTypes);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        close(volInfo->imageForWriting);
        unlink(newImagePathAndName);
        return rc;
    }
    
    if(filenameTypes & FNTYPE_ROCKRIDGE)
    {
        printf("writing long NMs at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
        rc = writeLongNMsInDir(volInfo, &newTree);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
    }
    
    wcSeekSet(volInfo, NBYTES_LOGICAL_BLOCK * NLS_SYSTEM_AREA);
    
    printf("writing pvd at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
    rc = writeVolDescriptor(volInfo, pRealRootDrOffset, 
                            pRootDirSize, lPathTable9660Loc, mPathTable9660Loc, 
                            pathTable9660Size, creationTime, true);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        close(volInfo->imageForWriting);
        unlink(newImagePathAndName);
        return rc;
    }
    
    if(filenameTypes & FNTYPE_JOLIET)
    {
        wcSeekSet(volInfo, svdOffset);
        
        printf("writing svd at %X\n", (int)wcSeekTell(volInfo));fflush(NULL);
        rc = writeVolDescriptor(volInfo, sRealRootDrOffset, 
                                sRootDirSize, lPathTableJolietLoc, mPathTableJolietLoc, 
                                pathTableJolietSize, creationTime, false);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            close(volInfo->imageForWriting);
            unlink(newImagePathAndName);
            return rc;
        }
    }
    
    printf("freeing memory\n");fflush(NULL);
    freeDirToWriteContents(&newTree);
    close(volInfo->imageForWriting);
    
    return 1;
}

/******************************************************************************
* bootInfoTableChecksum()
* Calculate the checksum to be written into the boot info table.
* */
int bootInfoTableChecksum(int oldImage, FileToWrite* file, unsigned* checksum)
{
    ssize_t rc;
    int rc2;
    int srcFile;
    unsigned char* contents;
    unsigned count;
    
    if(file->size % 4 != 0)
        return BKERROR_WRITE_BOOT_FILE_4;
    
    contents = malloc(file->size);
    if(contents == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    if(file->onImage)
    /* read file from original image */
    {
        lseek(oldImage, file->offset, SEEK_SET);
        
        rc = read(oldImage, contents, file->size);
        if(rc == -1 || rc != (int)(file->size))
        {
            free(contents);
            return BKERROR_READ_GENERIC;
        }
    }
    else
    /* read file from fs */
    {
        srcFile = open(file->pathAndName, O_RDONLY);
        if(srcFile == -1)
        {
            free(contents);
            return BKERROR_OPEN_READ_FAILED;
        }
        
        rc = read(srcFile, contents, file->size);
        if(rc == -1 || rc != (int)(file->size))
        {
            close(srcFile);
            free(contents);
            return BKERROR_READ_GENERIC;
        }
        
        rc2 = close(srcFile);
        if(rc2 < 0)
        {
            free(contents);
            return BKERROR_EXOTIC;
        }
    }
    
    *checksum = 0;
    /* do 32 bit checksum starting from byte 64
    * because i check above that the file is divisible by 4 i will not be 
    * reading wrong memory */
    for(count = 64; count < file->size; count += 4)
    {
        unsigned toAdd;
        
        toAdd = *(contents + count) | (*(contents + count + 1) << 8) | 
                (*(contents + count + 2) << 16) | (*(contents + count + 3) << 24);
        
        *checksum += toAdd;
    }
    
    free(contents);
    
    return 1;
}

/******************************************************************************
* countDirsOnLevel()
* a 'level' is described in ecma119 6.8.2
* it's needed for path tables, don't remember exactly what for
* */
int countDirsOnLevel(const DirToWrite* dir, int targetLevel, int thisLevel)
{
    BaseToWrite* child;
    int sum;
    
    if(targetLevel == thisLevel)
    {
        return 1;
    }
    else
    {
        sum = 0;
        
        child = dir->children;
        while(child != NULL)
        {
            if( IS_DIR(child->posixFileMode) )
                sum += countDirsOnLevel(DIRTW_PTR(child), targetLevel, thisLevel + 1);
            
            child = child->next;
        }
        
        return sum;
    }
}

/******************************************************************************
* countTreeHeight()
* caller should set heightSoFar to 1
* */
int countTreeHeight(const DirToWrite* dir, int heightSoFar)
{
    BaseToWrite* child;
    int maxHeight;
    int thisHeight;
    
    maxHeight = heightSoFar;
    child = dir->children;
    while(child != NULL)
    {
        if( IS_DIR(child->posixFileMode) )
        {
            thisHeight = countTreeHeight(DIRTW_PTR(child), heightSoFar + 1);
            
            if(thisHeight > maxHeight)
                maxHeight = thisHeight;
        }
        
        child = child->next;
    }
    
    return maxHeight;
}

/******************************************************************************
* elToritoChecksum()
* Algorithm: the sum of all words, including the checksum must trunkate to 
* a 16-bit 0x0000
* */
unsigned short elToritoChecksum(const unsigned char* record)
{
    short sum;
    int i;
    
    sum = 0;
    for(i = 0; i < 32; i += 2)
    {
        sum += *(record + i) | (*(record + i + 1) << 8);
    }
    
    return 0xFFFF - sum + 1;
}

/******************************************************************************
* writeByteBlock()
* Fills numBytes with byteToWrite.

* */
int writeByteBlock(VolInfo* volInfo, unsigned char byteToWrite, int numBytes)
{
    int rc;
    int count;
    int numBlocks;
    int sizeLastBlock;
    
    memset(volInfo->readWriteBuffer, byteToWrite, READ_WRITE_BUFFER_SIZE);
    
    numBlocks = numBytes / READ_WRITE_BUFFER_SIZE;
    sizeLastBlock = numBytes % READ_WRITE_BUFFER_SIZE;
    
    for(count = 0; count < numBlocks; count++)
    {
        rc = wcWrite(volInfo, volInfo->readWriteBuffer, READ_WRITE_BUFFER_SIZE);
        if(rc <= 0)
            return rc;
    }
    
    if(sizeLastBlock > 0)
    {
        rc = wcWrite(volInfo, volInfo->readWriteBuffer, sizeLastBlock);
        if(rc <= 0)
            return rc;
    }
    
    return 1;
}

/******************************************************************************
* writeByteBlockFromFile()
* copies numBytes from src into the image to write in blocks of 10K
* */
int writeByteBlockFromFile(int src, VolInfo* volInfo, unsigned numBytes)
{
    int rc;
    int count;
    int numBlocks;
    int sizeLastBlock;
    
    numBlocks = numBytes / READ_WRITE_BUFFER_SIZE;
    sizeLastBlock = numBytes % READ_WRITE_BUFFER_SIZE;
    
    for(count = 0; count < numBlocks; count++)
    {
        if(volInfo->stopOperation)
            return BKERROR_OPER_CANCELED_BY_USER;

        rc = read(src, volInfo->readWriteBuffer, READ_WRITE_BUFFER_SIZE);
        if(rc != READ_WRITE_BUFFER_SIZE)
            return BKERROR_READ_GENERIC;
        rc = wcWrite(volInfo, volInfo->readWriteBuffer, READ_WRITE_BUFFER_SIZE);
        if(rc <= 0)
            return rc;
    }
    
    if(sizeLastBlock > 0)
    {
        rc = read(src, volInfo->readWriteBuffer, sizeLastBlock);
        if(rc != sizeLastBlock)
                return BKERROR_READ_GENERIC;
        rc = wcWrite(volInfo, volInfo->readWriteBuffer, sizeLastBlock);
        if(rc <= 0)
                return rc;
    }
    
    return 1;
}

/******************************************************************************
* writeDir()
* Writes the contents of a directory. Also writes locations and sizes of
* directory records for directories but not for files.
* Returns data length of the dir written.
* */
int writeDir(VolInfo* volInfo, DirToWrite* dir, int parentLbNum, 
             int parentNumBytes, int parentPosix, time_t recordingTime, 
             int filenameTypes, bool isRoot)
{
    int rc;
    
    off_t startPos;
    int numUnusedBytes;
    off_t endPos;
    
    DirToWrite selfDir; /* will have a different filename */
    DirToWrite parentDir;
    
    BaseToWrite* child;
    
    if(wcSeekTell(volInfo) % NBYTES_LOGICAL_BLOCK != 0)
        return BKERROR_SANITY;
    
    /* names other then 9660 are not used for self and parent */
    selfDir.base.name9660[0] = 0x00;
    selfDir.base.posixFileMode = dir->base.posixFileMode;
    
    parentDir.base.name9660[0] = 0x01;
    parentDir.base.name9660[1] = '\0';
    if(isRoot)
        parentDir.base.posixFileMode = selfDir.base.posixFileMode;
    else
        parentDir.base.posixFileMode = parentPosix;
    
    startPos = wcSeekTell(volInfo);
    
    if( startPos % NBYTES_LOGICAL_BLOCK != 0 )
    /* this should never happen */
        return BKERROR_SANITY;
    
    if(filenameTypes & FNTYPE_JOLIET)
        dir->extentNumber2 = startPos / NBYTES_LOGICAL_BLOCK;
    else
        dir->base.extentNumber = startPos / NBYTES_LOGICAL_BLOCK;
    
    /* write self */
    if(isRoot)
    {
        rc = writeDr(volInfo, BASETW_PTR(&selfDir), recordingTime, true, true, true, filenameTypes);
        if(rc < 0)
            return rc;
        
        if(filenameTypes & FNTYPE_JOLIET)
            dir->base.extentLocationOffset2 = selfDir.base.extentLocationOffset2;
        else
            dir->base.extentLocationOffset = selfDir.base.extentLocationOffset;
    }
    else
    {
        rc = writeDr(volInfo, BASETW_PTR(&selfDir), recordingTime, true, true, false, filenameTypes);
        if(rc < 0)
            return rc;
    }
    if(rc < 0)
        return rc;
    
    /* write parent */
    rc = writeDr(volInfo, BASETW_PTR(&parentDir), recordingTime, true, true, false, filenameTypes);
    if(rc < 0)
        return rc;
    
    child = dir->children;
    
    /* WRITE children drs */
    while(child != NULL)
    {
        if(IS_DIR(child->posixFileMode))
        {
            rc = writeDr(volInfo, child, recordingTime, 
                         true,  false, false, filenameTypes);
        }
        else
        {
            rc = writeDr(volInfo, child, recordingTime, 
                         false,  false, false, filenameTypes);
        }
        if(rc < 0)
            return rc;
        
        child = child->next;
    }
    /* END WRITE children drs */
    
    /* write blank to conclude extent */
    numUnusedBytes = NBYTES_LOGICAL_BLOCK - 
                     wcSeekTell(volInfo) % NBYTES_LOGICAL_BLOCK;
    rc = writeByteBlock(volInfo, 0x00, numUnusedBytes);
    if(rc < 0)
        return rc;
    
    if(filenameTypes & FNTYPE_JOLIET)
        dir->dataLength2 = wcSeekTell(volInfo) - startPos;
    else
        dir->dataLength = wcSeekTell(volInfo) - startPos;
    
    /* write subdirectories */
    child = dir->children;
    while(child != NULL)
    {
        if(IS_DIR(child->posixFileMode))
        {
            if(filenameTypes & FNTYPE_JOLIET)
            {
                rc = writeDir(volInfo, DIRTW_PTR(child), dir->extentNumber2, 
                              dir->dataLength2, BASETW_PTR(dir)->posixFileMode, recordingTime,
                              filenameTypes, false);
            }
            else
            {
                rc = writeDir(volInfo, DIRTW_PTR(child), BASETW_PTR(dir)->extentNumber, 
                              dir->dataLength, BASETW_PTR(dir)->posixFileMode, recordingTime,
                              filenameTypes, false);
            }
            if(rc < 0)
                return rc;
        }
        
        child = child->next;
    }
    
    endPos = wcSeekTell(volInfo);
    
    /* SELF extent location and size */
    if(filenameTypes & FNTYPE_JOLIET)
        wcSeekSet(volInfo, selfDir.base.extentLocationOffset2);
    else
        wcSeekSet(volInfo, selfDir.base.extentLocationOffset);
    
    if(filenameTypes & FNTYPE_JOLIET)
    {
        rc = write733(volInfo, dir->extentNumber2);
        if(rc <= 0)
            return rc;
        
        rc = write733(volInfo, dir->dataLength2);
        if(rc <= 0)
            return rc;
    }
    else
    {
        rc = write733(volInfo, BASETW_PTR(dir)->extentNumber);
        if(rc <= 0)
            return rc;
        
        rc = write733(volInfo, dir->dataLength);
        if(rc <= 0)
            return rc;
    }
    /* END SELF extent location and size */
    
    /* PARENT extent location and size */
    if(filenameTypes & FNTYPE_JOLIET)
        wcSeekSet(volInfo, parentDir.base.extentLocationOffset2);
    else
        wcSeekSet(volInfo, parentDir.base.extentLocationOffset);
    
    if(parentLbNum == 0)
    /* root, parent is same as self */
    {
        if(filenameTypes & FNTYPE_JOLIET)
        {
            rc = write733(volInfo, dir->extentNumber2);
            if(rc <= 0)
                return rc;
            
            rc = write733(volInfo, dir->dataLength2);
            if(rc <= 0)
                return rc;
        }
        else
        {
            rc = write733(volInfo, BASETW_PTR(dir)->extentNumber);
            if(rc <= 0)
                return rc;
            
            rc = write733(volInfo, dir->dataLength);
            if(rc <= 0)
                return rc;
        }
    }
    else
    /* normal parent */
    {
        rc = write733(volInfo, parentLbNum);
        if(rc <= 0)
            return rc;
        
        rc = write733(volInfo, parentNumBytes);
        if(rc <= 0)
            return rc;
    }
    /* END PARENT extent location and size */
    
    /* ALL subdir extent locations and sizes */
    child = dir->children;
    while(child != NULL)
    {
        if(IS_DIR(child->posixFileMode))
        {
            if(filenameTypes & FNTYPE_JOLIET)
            {
                wcSeekSet(volInfo, child->extentLocationOffset2);
                
                rc = write733(volInfo, DIRTW_PTR(child)->extentNumber2);
                if(rc <= 0)
                    return rc;
                
                rc = write733(volInfo, DIRTW_PTR(child)->dataLength2);
                if(rc <= 0)
                    return rc;
            }
            else
            {
                wcSeekSet(volInfo, child->extentLocationOffset);
                
                rc = write733(volInfo, child->extentNumber);
                if(rc <= 0)
                    return rc;
                
                rc = write733(volInfo, DIRTW_PTR(child)->dataLength);
                if(rc <= 0)
                    return rc;
            }
        }
        
        child = child->next;
    }
    /* END ALL subdir extent locations and sizes */
    
    wcSeekSet(volInfo, endPos);
    
    if(filenameTypes & FNTYPE_JOLIET)
        return dir->dataLength2;
    else
        return dir->dataLength;
}

/******************************************************************************
* writeDr()
* Writes a directory record.
* Note that it uses only the members of DirToWrite and FileToWrite that are
* the same.
* */
int writeDr(VolInfo* volInfo, BaseToWrite* node, time_t recordingTime, bool isADir, 
            bool isSelfOrParent, bool isFirstRecord, int filenameTypes)
{
    int rc;
    unsigned char byte;
    char aString[256];
    unsigned short aShort;
    off_t startPos;
    off_t endPos;
    unsigned char lenFileId;
    unsigned char recordLen;
    
    /* look at the end of the function for an explanation */
    writeDrStartLabel:
    
    startPos = wcSeekTell(volInfo);
    
    /* record length is recorded in the end */
    wcSeekForward(volInfo, 1);
    
    /* extended attribute record length */
    byte = 0;
    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    
    if(filenameTypes & FNTYPE_JOLIET)
        node->extentLocationOffset2 = wcSeekTell(volInfo);
    else
        node->extentLocationOffset = wcSeekTell(volInfo);
    
    /* location of extent not recorded in this function */
    wcSeekForward(volInfo, 8);
    
    /* data length not recorded in this function */
    wcSeekForward(volInfo, 8);
    
    /* RECORDING time and date */
    epochToShortString(recordingTime, aString);
    
    rc = write711(volInfo, aString[0]);
    if(rc <= 0)
        return rc;
    rc = write711(volInfo, aString[1]);
    if(rc <= 0)
        return rc;
    rc = write711(volInfo, aString[2]);
    if(rc <= 0)
        return rc;
    rc = write711(volInfo, aString[3]);
    if(rc <= 0)
        return rc;
    rc = write711(volInfo, aString[4]);
    if(rc <= 0)
        return rc;
    rc = write711(volInfo, aString[5]);
    if(rc <= 0)
        return rc;
    rc = write711(volInfo, aString[6]);
    if(rc <= 0)
        return rc;
    /* END RECORDING time and date */
    
    /* FILE flags  */
    if(isADir)
    /* (only directory bit on) */
        byte = 0x02;
    else
    /* nothing on */
        byte = 0x00;
    
    rc = wcWrite(volInfo, (char*)&byte, 1);
    if(rc <= 0)
        return rc;
    /* END FILE flags  */
    
    /* file unit size (always 0, non-interleaved mode) */
    byte = 0;
    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    
    /* interleave gap size (also always 0, non-interleaved mode) */
    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    
    /* volume sequence number (always 1) */
    aShort = 1;
    rc = write723(volInfo, aShort);
    if(rc <= 0)
        return rc;
    
    /* LENGTH of file identifier */
    if(isSelfOrParent)
        lenFileId = 1;
    else
    {
        if(filenameTypes & FNTYPE_JOLIET)
            lenFileId = 2 * strlen(node->nameJoliet);
        else
            /*if(isADir) see microsoft comment below */
                lenFileId = strlen(node->name9660);
            /*else
                lenFileId = strlen(node->name9660) + 2; */
    }
    
    rc = write711(volInfo, lenFileId);
    if(rc <= 0)
        return rc;
    /* END LENGTH of file identifier */
    
    /* FILE identifier */
    if(isSelfOrParent)
    {
        /* that byte has 0x00 or 0x01 */
        rc = write711(volInfo, node->name9660[0]);
        if(rc <= 0)
            return rc;
    }
    else
    {
        if(filenameTypes & FNTYPE_JOLIET)
        {
            rc = writeJolietStringField(volInfo, node->nameJoliet, 
                                        2 * strlen(node->nameJoliet));
            if(rc < 0)
                return rc;
        }
        else
        {
            /* ISO9660 requires ";1" after the filename (not directory name) 
            * but the windows NT/2K boot loaders cannot find NTLDR inside
            * the I386 directory because they are looking for "NTLDR" not 
            * "NTLDR;1". i guess if microsoft can do it, i can do it. filenames
            * on images written by me do not end with ";1"
            if(isADir)
            {*/
                /* the name */
                rc = wcWrite(volInfo, node->name9660, lenFileId);
                if(rc <= 0)
                    return rc;
            /*}
            else
            {
                rc = writeWrapper(image, node->name9660, lenFileId - 2);
                if(rc <= 0)
                    return rc;
                
                rc = writeWrapper(image, ";1", 2);
                if(rc <= 0)
                    return rc;
            }*/
        }
    }
    /* END FILE identifier */
    
    /* padding field */
    if(lenFileId % 2 == 0)
    {
        byte = 0;
        rc = write711(volInfo, byte);
        if(rc <= 0)
            return rc;
    }
    
    if(filenameTypes & FNTYPE_ROCKRIDGE)
    {
        if(isFirstRecord)
        {
            rc = writeRockSP(volInfo);
            if(rc < 0)
                return rc;
            
            rc = writeRockER(volInfo);
            if(rc < 0)
                return rc;
        }
        
        rc = writeRockPX(volInfo, node->posixFileMode, isADir);
        if(rc < 0)
            return rc;
        
        if(!isSelfOrParent)
        {
            if(wcSeekTell(volInfo) - startPos < strlen(node->nameRock) + 5)
            /* have no room for the NM entry in this directory record */
            {
                node->offsetForCE = wcSeekTell(volInfo);
                /* leave room for CE entry */
                wcSeekForward(volInfo, 28);
            }
            else
            {
                rc = writeRockNM(volInfo, node->nameRock, strlen(node->nameRock), false);
                if(rc < 0)
                    return rc;
            }
            
            if(IS_SYMLINK(node->posixFileMode))
            {
                rc = writeRockSL(volInfo, SYMLINKTW_PTR(node), true);
                if(rc < 0)
                    return rc;
            }
        }
    }
    
    /* RECORD length */
    endPos = wcSeekTell(volInfo);
    
    wcSeekSet(volInfo, startPos);
    
    recordLen = endPos - startPos;
    rc = write711(volInfo, recordLen);
    if(rc <= 0)
        return rc;
    
    wcSeekSet(volInfo, endPos);
    /* END RECORD length */
    
    /* the goto is good! really!
    * if, after writing the record we see that the record is in two logical
    * sectors (that's not allowed by iso9660) we erase the record just
    * written, write zeroes to the end of the first logical sector
    * (as required by iso9660) and restart the function, which will write
    * the same record again but at the beginning of the next logical sector
    * yeah, so don't complain :) */
    
    if(endPos / NBYTES_LOGICAL_BLOCK > startPos / NBYTES_LOGICAL_BLOCK)
    /* crossed a logical sector boundary while writing the record */
    {
        wcSeekSet(volInfo, startPos);
        
        /* overwrite a piece of the record written in this function
        * (the piece that's in the first sector) with zeroes */
        rc = writeByteBlock(volInfo, 0x00, recordLen - endPos % NBYTES_LOGICAL_BLOCK);
        if(rc < 0)
            return rc;
        
        goto writeDrStartLabel;
    }
    
    return 1;
}

/******************************************************************************
* writeElToritoBootCatalog()
* Write the el torito boot catalog (validation entry and inital/default entry).
* Returns the offset where the boot record sector number should
* be written (7.3.1).
* */
int writeElToritoBootCatalog(VolInfo* volInfo, 
                             off_t* bootRecordSectorNumberOffset)
{
    unsigned char buffer[NBYTES_LOGICAL_BLOCK];
    int rc;
    
    bzero(buffer, NBYTES_LOGICAL_BLOCK);
    
    if(wcSeekTell(volInfo) % NBYTES_LOGICAL_BLOCK != 0)
    /* file pointer not at sector boundary */
        return BKERROR_SANITY;
    
    /* SETUP VALIDATION entry (first 20 bytes of boot catalog) */
    /* header, must be 1 */
    buffer[0] = 1;
    /* platform id, 0 for x86 (bzero at start took care of this) */
    /* 2 bytes reserved, must be 0 (bzero at start took care of this) */
    /* 24 bytes id string for manufacturer/developer of cdrom */
    strncpy((char*)&(buffer[4]), "Edited with ISO Master", 22);
    /* key byte 0x55 */
    buffer[30] = 0x55;
    /* key byte 0xAA */
    buffer[31] = 0xAA;
    
    /* checksum */
    write721ToByteArray(&(buffer[28]), elToritoChecksum(buffer));
    /* END SETUP VALIDATION validation entry (first 20 bytes of boot catalog) */
    
    /* SETUP INITIAL entry (next 20 bytes of boot catalog) */
    /* boot indicator. 0x88 = bootable */
    buffer[32] = 0x88;
    /* boot media type */
    if(volInfo->bootMediaType == BOOT_MEDIA_NO_EMULATION)
        buffer[33] = 0;
    else if(volInfo->bootMediaType == BOOT_MEDIA_1_2_FLOPPY)
        buffer[33] = 1;
    else if(volInfo->bootMediaType == BOOT_MEDIA_1_44_FLOPPY)
        buffer[33] = 2;
    else if(volInfo->bootMediaType == BOOT_MEDIA_2_88_FLOPPY)
        buffer[33] = 3;
    else if(volInfo->bootMediaType == BOOT_MEDIA_HARD_DISK)
        buffer[33] = 4;
    /* load segment leave it at 0 */
    /* system type, leave it at 0 */
    /* 1 byte unused, leave it at 0 */
    /* sector count. i have yet to see a boot record with a sector count 
    * that's not 4 */
    write721ToByteArray(&(buffer[38]), 4);
    /* logical block number of boot record file. this is not known until 
    * after that file is written */
    *bootRecordSectorNumberOffset = wcSeekTell(volInfo) + 40;
    /* the rest is unused, leave it at 0 */
    /* END SETUP INITIAL entry (next 20 bytes of boot catalog) */
    
    rc = wcWrite(volInfo, (char*)buffer, NBYTES_LOGICAL_BLOCK);
    if(rc <= 0)
        return rc;
    
    return 1;
}

/******************************************************************************
* writeElToritoVd()
* Write the el torito volume descriptor.
* Returns the offset where the boot catalog sector number should 
* be written (7.3.1).
* */
int writeElToritoVd(VolInfo* volInfo, off_t* bootCatalogSectorNumberOffset)
{
    char buffer[NBYTES_LOGICAL_BLOCK];
    int rc;
    
    bzero(buffer, NBYTES_LOGICAL_BLOCK);
    
    if(wcSeekTell(volInfo) % NBYTES_LOGICAL_BLOCK != 0)
    /* file pointer not at sector boundary */
        return BKERROR_SANITY;
    
    /* SETUP BOOT record volume descriptor sector */
    /* boot record indicator, must be 0 (bzero at start took care of this) */
    /* iso9660 identifier, must be "CD001" */
    strncpy((char*)buffer + 1, "CD001", 5);
    /* version, must be 1 */
    buffer[6] = 1;
    /* boot system identifier, must be 32 bytes "EL TORITO SPECIFICATION" 
    * padded with 0x00 (bzero at start took care of this) */
    strncpy(&(buffer[7]), "EL TORITO SPECIFICATION", 23);
    /* unused 32 bytes, must be 0 (bzero at start took care of this) */
    /* boot catalog location, 4 byte intel format. written later. */
    *bootCatalogSectorNumberOffset = wcSeekTell(volInfo) + 71;
    /*write731ToByteArray(&(buffer[71]), bootCatalogSectorNumber);*/
    /* the rest of this sector is unused, must be set to 0 */
    /* END SETUP BOOT record volume descriptor sector */
    
    rc = wcWrite(volInfo, buffer, NBYTES_LOGICAL_BLOCK);
    if(rc <= 0)
        return rc;
    
    return 1;
}

/******************************************************************************
* writeFileContents()
* Write file contents into an extent and also write the file's location and 
* size into the directory records back in the tree.
* Also write location and size for symbolic links.
* */
int writeFileContents(VolInfo* volInfo, DirToWrite* dir, int filenameTypes)
{
    int rc;
    
    BaseToWrite* child;
    int numUnusedBytes;
    int srcFile;
    off_t endPos;
    
    child = dir->children;
    while(child != NULL)
    /* each file in current directory */
    {
        if(volInfo->stopOperation)
            return BKERROR_OPER_CANCELED_BY_USER;
        
        if(wcSeekTell(volInfo) % NBYTES_LOGICAL_BLOCK != 0)
            return BKERROR_SANITY;
        
        if( IS_REG_FILE(child->posixFileMode) )
        {
            bool needToCopy = true;
            
            child->extentNumber = wcSeekTell(volInfo) / NBYTES_LOGICAL_BLOCK;
            if(volInfo->scanForDuplicateFiles)
            {
                if(FILETW_PTR(child)->location->extentNumberWrittenTo == 0)
                /* file not yet written */
                {
                    FILETW_PTR(child)->location->extentNumberWrittenTo = child->extentNumber;
                }
                else
                {
                    child->extentNumber = FILETW_PTR(child)->location->extentNumberWrittenTo;
                    needToCopy = false;
                }
            }
            
            if(volInfo->bootMediaType != BOOT_MEDIA_NONE && 
               volInfo->bootRecordIsVisible &&
               FILETW_PTR(child)->origFile == volInfo->bootRecordOnImage)
            /* this file is the boot record. write its sector number in 
            * the boot catalog */
            {
                off_t currPos;
                
                currPos = wcSeekTell(volInfo);
                
                wcSeekSet(volInfo, volInfo->bootRecordSectorNumberOffset);
                rc = write731(volInfo, child->extentNumber);
                if(rc <= 0)
                    return rc;
                
                wcSeekSet(volInfo, currPos);
            }
            
            if(needToCopy)
            {
                if(FILETW_PTR(child)->onImage)
                /* copy file from original image to new one */
                {
                    lseek(volInfo->imageForReading, FILETW_PTR(child)->offset, 
                          SEEK_SET);
                    
                    rc = writeByteBlockFromFile(volInfo->imageForReading, 
                                                volInfo, FILETW_PTR(child)->size);
                    if(rc < 0)
                        return rc;
                }
                else
                /* copy file from fs to new image */
                {
                    /* UPDATE the file's size, in case it's changed since we added it */
                    struct stat statStruct;
                    
                    rc = stat(FILETW_PTR(child)->pathAndName, &statStruct);
                    if(rc != 0)
                        return BKERROR_STAT_FAILED;
                    
                    FILETW_PTR(child)->size = statStruct.st_size;
                    /* UPDATE the file's size, in case it's changed since we added it */
                    
                    srcFile = open(FILETW_PTR(child)->pathAndName, O_RDONLY);
                    if(srcFile == -1)
                        return BKERROR_OPEN_READ_FAILED;
                    
                    rc = writeByteBlockFromFile(srcFile, 
                                                volInfo, FILETW_PTR(child)->size);
                    if(rc < 0)
                    {
                        close(srcFile);
                        return rc;
                    }
                    
                    rc = close(srcFile);
                    if(rc < 0)
                        return BKERROR_EXOTIC;
                }
                
                /* fill extent with zeroes */
                numUnusedBytes = NBYTES_LOGICAL_BLOCK - 
                                 wcSeekTell(volInfo) % NBYTES_LOGICAL_BLOCK;
                rc = writeByteBlock(volInfo, 0x00, numUnusedBytes);
                if(rc < 0)
                    return rc;
            }
            
            endPos = wcSeekTell(volInfo);
            
            if(volInfo->bootMediaType != BOOT_MEDIA_NONE && 
               volInfo->bootRecordIsVisible &&
               FILETW_PTR(child)->origFile == volInfo->bootRecordOnImage)
            /* this file is the boot record. assume it's isolinux and write the 
            * boot info table */
            {
                unsigned char bootInfoTable[56];
                unsigned checksum;
                
                bzero(bootInfoTable, 56);
                
                /* go to the offset in the file where the boot info table is */
                wcSeekSet(volInfo, child->extentNumber * 
                          NBYTES_LOGICAL_BLOCK + 8);
                
                /* sector number of pvd */
                write731ToByteArray(bootInfoTable, 16);
                /* sector number of boot file (this one) */
                write731ToByteArray(bootInfoTable + 4, child->extentNumber);
                /* boot file length in bytes */
                write731ToByteArray(bootInfoTable + 8, FILETW_PTR(child)->size);
                /* 32 bit checksum (the sum of all the 32-bit words in the boot
                * file starting at byte offset 64 */
                rc = bootInfoTableChecksum(volInfo->imageForReading, FILETW_PTR(child), &checksum);
                if(rc <= 0)
                    return rc;
                write731ToByteArray(bootInfoTable + 12, checksum);
                /* the rest is reserved, leave at zero */
                
                rc = wcWrite(volInfo, (char*)bootInfoTable, 56);
                if(rc <= 0)
                    return rc;
            }
            
            /* WRITE file location and size */
            wcSeekSet(volInfo, child->extentLocationOffset);
            
            rc = write733(volInfo, child->extentNumber);
            if(rc <= 0)
                return rc;
            
            rc = write733(volInfo, FILETW_PTR(child)->size);
            if(rc <= 0)
                return rc;
            
            if(filenameTypes & FNTYPE_JOLIET)
            /* also update location and size on joliet tree */
            {
                wcSeekSet(volInfo, child->extentLocationOffset2);
                
                rc = write733(volInfo, child->extentNumber);
                if(rc <= 0)
                    return rc;
                
                rc = write733(volInfo, FILETW_PTR(child)->size);
                if(rc <= 0)
                    return rc;
            }
            
            wcSeekSet(volInfo, endPos);
            /* END WRITE file location and size */
        }
        else if( IS_DIR(child->posixFileMode) )
        {
            rc = writeFileContents(volInfo, DIRTW_PTR(child), filenameTypes);
            if(rc < 0)
                return rc;
        }
        else if( IS_SYMLINK(child->posixFileMode) )
        {
            /* WRITE symlink location and size (0) */
            endPos = wcSeekTell(volInfo);
            
            wcSeekSet(volInfo, child->extentLocationOffset);
            
            rc = write733(volInfo, 0);
            if(rc <= 0)
                return rc;
            
            rc = write733(volInfo, 0);
            if(rc <= 0)
                return rc;
            
            if(filenameTypes & FNTYPE_JOLIET)
            /* also update location and size on joliet tree */
            {
                wcSeekSet(volInfo, child->extentLocationOffset2);
                
                rc = write733(volInfo, 0);
                if(rc <= 0)
                    return rc;
                
                rc = write733(volInfo, 0);
                if(rc <= 0)
                    return rc;
            }
            
            wcSeekSet(volInfo, endPos);
            /* END WRITE symlink location and size (0)  */
        }
        
        child = child->next;
        
    } /* while(nextFile != NULL) */
    
    return 1;
}

/* field size must be even. !!check all calls to make sure */
int writeJolietStringField(VolInfo* volInfo, const char* name, int fieldSize)
{
    char jolietName[512]; /* don't see why would ever want 
                          * to write a longer one */
    int srcCount;
    int destCount;
    int rc;
    
    srcCount = 0;
    destCount = 0;
    while(name[srcCount] != '\0' && destCount < fieldSize)
    {
        /* first byte zero */
        jolietName[destCount] = 0x00;
        /* second byte character */
        jolietName[destCount + 1] = name[srcCount];
        
        srcCount += 1;
        destCount += 2;
    }
    
    while(destCount < fieldSize)
    /* pad with ucs2 spaces */
    {
        jolietName[destCount] = 0x00;
        jolietName[destCount + 1] = ' ';
        
        destCount += 2;
    }
    
    rc = wcWrite(volInfo, jolietName, destCount);
    if(rc <= 0)
        return rc;
    
    return 1;
}

/* write NM that won't fit in a directory record */
int writeLongNM(VolInfo* volInfo, BaseToWrite* node)
{
    off_t startPos;
    int fullNameLen;
    unsigned char CErecord[28];
    bool fitsInOneNM;
    int firstNMlen;
    off_t endPos;
    int rc;
    int lenOfCE;
    
    startPos = wcSeekTell(volInfo);
    
    fullNameLen = strlen(node->nameRock);
    
    /* should have checked for this before getting into this function */
    if(fullNameLen > 255)
        return BKERROR_SANITY;
    
    if(fullNameLen > 250)
    {
        fitsInOneNM = false;
        firstNMlen = 250;
    }
    else
    {
        fitsInOneNM = true;
        firstNMlen = fullNameLen;
    }
    
    /* NM record(s) */
    if(fitsInOneNM)
    {
        rc = writeRockNM(volInfo, node->nameRock, firstNMlen, false);
        if(rc <= 0)
            return rc;
    }
    else
    {
        rc = writeRockNM(volInfo, node->nameRock, firstNMlen, true);
        if(rc <= 0)
            return rc;
        rc = writeRockNM(volInfo, node->nameRock + firstNMlen, fullNameLen - firstNMlen, false);
        if(rc <= 0)
            return rc;
    }
    
    lenOfCE = wcSeekTell(volInfo) - startPos;
    
    /* write blank to conclude extent */
    rc = writeByteBlock(volInfo, 0x00, NBYTES_LOGICAL_BLOCK - 
                        wcSeekTell(volInfo) % NBYTES_LOGICAL_BLOCK);
    if(rc < 0)
        return rc;
    
    endPos = wcSeekTell(volInfo);
    
    /* CE record back in the directory record */
    wcSeekSet(volInfo, node->offsetForCE);
    
    CErecord[0] = 'C';
    CErecord[1] = 'E';
    CErecord[2] = 28; /* length */
    CErecord[3] = 1; /* version */
    write733ToByteArray(CErecord + 4, startPos / NBYTES_LOGICAL_BLOCK); /* block location */
    /* i'm always using 1 logical block per name */
    write733ToByteArray(CErecord + 12, 0); /* offset to start */
    write733ToByteArray(CErecord + 20, lenOfCE); /* length */
    
    rc = wcWrite(volInfo, (char*)CErecord, CErecord[2]);
    if(rc <= 0)
        return rc;
    /* END CE record back in the directory record */
    
    wcSeekSet(volInfo, endPos);
    
    return 1;
}

/* write all NMs in the tree that won't fit in directory records */
int writeLongNMsInDir(VolInfo* volInfo, DirToWrite* dir)
{
    BaseToWrite* child;
    int rc;
    
    child = dir->children;
    while(child != NULL)
    {
        if(child->offsetForCE != 0)
        {
            rc = writeLongNM(volInfo, child);
            if(rc <= 0)
                return rc;
        }
        
        if( IS_DIR(child->posixFileMode) )
        {
            rc = writeLongNMsInDir(volInfo, DIRTW_PTR(child));
            if(rc <= 0)
                return rc;
        }
        
        child = child->next;
    }
    
    return 1;
}

/* returns path table size (number of bytes not counting the blank) */
int writePathTable(VolInfo* volInfo, const DirToWrite* tree, bool isTypeL, 
                   int filenameType)
{
    int treeHeight;
    int count;
    int level;
    int* dirsPerLevel; /* a dynamic array of the number of dirs per level */
    int numDirsSoFar;
    off_t origPos;
    int numBytesWritten;
    int rc;
    
    origPos = wcSeekTell(volInfo);
    
    if(origPos % NBYTES_LOGICAL_BLOCK != 0)
        return BKERROR_SANITY;
    
    treeHeight = countTreeHeight(tree, 1);
    
    dirsPerLevel = malloc(sizeof(int) * treeHeight);
    if(dirsPerLevel == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    for(count = 0; count < treeHeight; count++)
    {
        dirsPerLevel[count] = countDirsOnLevel(tree, count + 1, 1);
    }
    
    for(level = 1; level <= treeHeight; level++)
    {
        if(level == 1)
        /* numDirsSoFar = parent dir num */
            numDirsSoFar = 1;
        else if(level == 2)
            numDirsSoFar = 1;
        else
        {
            /* ex. when i am on level 4 i want number of dirs on levels 1 + 2 */
            numDirsSoFar = 0;
            for(count = 0; count < level - 2; count++)
            {
                numDirsSoFar += dirsPerLevel[count];
            }
        }
        
        rc = writePathTableRecordsOnLevel(volInfo, tree, isTypeL, filenameType, 
                                          level, 1, &numDirsSoFar);
        if(rc < 0)
        {
            free(dirsPerLevel);
            return rc;
        }
    }
    
    numBytesWritten = wcSeekTell(volInfo) - origPos;
    
    /* blank to conclude extent */
    rc = writeByteBlock(volInfo, 0x00, NBYTES_LOGICAL_BLOCK - 
                        numBytesWritten % NBYTES_LOGICAL_BLOCK);
    if(rc < 0)
    {
        free(dirsPerLevel);
        return rc;
    }
    
    free(dirsPerLevel);
    
    return numBytesWritten;
}

int writePathTableRecordsOnLevel(VolInfo* volInfo, const DirToWrite* dir, 
                                 bool isTypeL, int filenameType, 
                                 int targetLevel, int thisLevel,
                                 int* parentDirNum)
{
    int rc;
    BaseToWrite* child;
    
    unsigned char fileIdLen;
    unsigned char byte;
    unsigned exentLocation;
    unsigned short parentDirId; /* copy of *parentDirNum */
    static const char rootId = 0x00;
    
    if(thisLevel == targetLevel)
    /* write path table record */
    {
        /* LENGTH  of directory identifier */
        if(targetLevel == 1)
        /* root */
            fileIdLen = 1;
        else
        {
            if(filenameType & FNTYPE_JOLIET)
            {
                fileIdLen = 2 * strlen(BASETW_PTR(dir)->nameJoliet);
            }
            else
            {
                fileIdLen = strlen(BASETW_PTR(dir)->name9660);
            }
        }
        
        rc = write711(volInfo, fileIdLen);
        if(rc <= 0)
            return rc;
        /* END LENGTH  of directory identifier */
        
        /* extended attribute record length */
        byte = 0;
        rc = write711(volInfo, byte);
        if(rc <= 0)
            return rc;
        
        /* LOCATION of extent */
        if(filenameType & FNTYPE_JOLIET)
            exentLocation = dir->extentNumber2;
        else
            exentLocation = BASETW_PTR(dir)->extentNumber;
        
        if(isTypeL)
            rc = write731(volInfo, exentLocation);
        else
            rc = write732(volInfo, exentLocation);
        if(rc <= 0)
            return rc;
        /* END LOCATION of extent */
        
        /* PARENT directory number */
        parentDirId = *parentDirNum;
        
        if(isTypeL)
            rc = write721(volInfo, parentDirId);
        else
            rc = write722(volInfo, parentDirId);
        
        if(rc <= 0)
            return rc;
        /* END PARENT directory number */
        
        /* DIRECTORY identifier */
        if(targetLevel == 1)
        /* root */
        {
            rc = wcWrite(volInfo, &rootId, 1);
            if(rc <= 0)
                return rc;
        }
        else
        {
            if(filenameType & FNTYPE_JOLIET)
            {
                rc = writeJolietStringField(volInfo, BASETW_PTR(dir)->nameJoliet, fileIdLen);
                if(rc < 0)
                    return rc;
            }
            else
            {
                rc = wcWrite(volInfo, BASETW_PTR(dir)->name9660, fileIdLen);
                if(rc <= 0)
                    return rc;
            }
        }
        /* END DIRECTORY identifier */
        
        /* padding field */
        if(fileIdLen % 2 != 0)
        {
            byte = 0;
            rc = write711(volInfo, byte);
            if(rc <= 0)
                return rc;
        }
        
    }
    else /* if(thisLevel < targetLevel) */
    {
        child = dir->children;
        while(child != NULL)
        {
            if( IS_DIR(child->posixFileMode) )
            {
                if(thisLevel == targetLevel - 2)
                /* am now going throught the list of dirs where the parent is */
                {
                    if(targetLevel != 2)
                    /* first and second level have the same parent: 1 */
                    {
                        (*parentDirNum)++;
                    }
                }
                
                rc = writePathTableRecordsOnLevel(volInfo, DIRTW_PTR(child), isTypeL,
                                                  filenameType, targetLevel, 
                                                  thisLevel + 1, parentDirNum);
                if(rc < 0)
                    return rc;
            }
            
            child = child->next;
        }
    }
    
    return 1;
}

/* This doesn't need support for CE because it's only written in one place,
* the root 'self' directory record. */
int writeRockER(VolInfo* volInfo)
{
    int rc;
    char record[46];
    
    /* identification */
    record[0] = 'E';
    record[1] = 'R';
    
    /* record length */
    record[2] = 46;
    
    /* entry version */
    record[3] = 1;
    
    /* extension identifier length */
    record[4] = 10;
    
    /* extension descriptor length */
    record[5] = 10;
    
    /* extension source length */
    record[6] = 18;
    
    /* extension version */
    record[7] = 1;
    
    /* extension identifier */
    strncpy(&(record[8]), "IEEE_P1282", 10);
    
    /* extension descriptor */
    strncpy(&(record[18]), "DRAFT_1_12", 10);
    
    /* extension source */
    strncpy(&(record[28]), "ADOPTED_1994_07_08", 18);
    
    rc = wcWrite(volInfo, record, 46);
    if(rc <= 0)
        return rc;
    
    return 1;
}

int writeRockNM(VolInfo* volInfo, char* name, int nameLen, bool doesContinue)
{
    int rc;
    char recordStart[5];
    
    /* identification */
    recordStart[0] = 'N';
    recordStart[1] = 'M';
    
    /* record length */
    recordStart[2] = 5 + nameLen;
    
    /* entry version */
    recordStart[3] = 1;
    
    /* flags */
    if(doesContinue)
        recordStart[4] = 0x01;
    else
        recordStart[4] = 0;
    
    rc = wcWrite(volInfo, recordStart, 5);
    if(rc <= 0)
        return rc;
    
    rc = wcWrite(volInfo, name, nameLen);
    if(rc <= 0)
        return rc;
    
    return 1;
}

/* the slackware cd has 36 byte PX entries, missing the file serial number
* so i will do the same */
int writeRockPX(VolInfo* volInfo, unsigned posixFileMode, bool isADir)
{
    int rc;
    unsigned char record[36];
    unsigned posixFileLinks;
    
    /* identification */
    record[0] = 'P';
    record[1] = 'X';
    
    /* record length */
    record[2] = 36;
    
    /* entry version */
    record[3] = 1;
    
    /* posix file mode */
    write733ToByteArray(&(record[4]), posixFileMode);
    
    /* POSIX file links */
    /*
    * this i think is number of subdirectories + 2 (self and parent)
    * and 1 for a file
    * it's probably not used on read-only filesystems
    * to add it, i would need to pass the number of links in a parent dir
    * recursively in writeDir(). brrrrr.
    */
    if(isADir)
        posixFileLinks = 2;
    else
        posixFileLinks = 1;
    
    write733ToByteArray(&(record[12]), posixFileLinks);
    /* END POSIX file links */
    
    /* posix file user id, posix file group id */
    bzero(&(record[20]), 16);
    
    rc = wcWrite(volInfo, (char*)record, 36);
    if(rc <= 0)
        return rc;
    
    return 1;
}

int writeRockSL(VolInfo* volInfo, SymLinkToWrite* symlink, bool doWrite)
{
    int stringCount;
    int targetLen;
    int numBytesNeeded;
    int numBytesToSkip;
    unsigned char* record;
    int recordCount;
    int rc;
    
    targetLen = strlen(symlink->target);
    
    /* figure out how much room i need */
    numBytesNeeded = 0;
    numBytesToSkip = 0;
    stringCount = 0;
    while(stringCount < targetLen)
    {
        int numBytesToSkip;
        char* nextSlash;
        
        if(symlink->target[stringCount] == '/')
        /* root (/) */
        {
            numBytesNeeded += 2;
            numBytesToSkip = 1;
        }
        else if( symlink->target[stringCount] == '.' && 
                 (stringCount + 1 == targetLen || symlink->target[stringCount + 1] == '/') )
        /* current (.) */
        {
            numBytesNeeded += 2;
            numBytesToSkip = 2;
        }
        else if( symlink->target[stringCount] == '.' && 
                 stringCount + 1 < targetLen && symlink->target[stringCount + 1] == '.' )
        /* parent (..) */
        {
            numBytesNeeded += 2;
            numBytesToSkip = 3;
        }
        else
        /* regular filename */
        {
            nextSlash = strchr(symlink->target + stringCount, '/');
            if(nextSlash != NULL)
                numBytesToSkip = nextSlash - (symlink->target + stringCount);
            else
                numBytesToSkip = targetLen - stringCount;
            
            numBytesNeeded += 2 + numBytesToSkip;
            
            numBytesToSkip += 1;
        }
        
        stringCount += numBytesToSkip;
    }
    
    if(!doWrite)
        return 5 + numBytesNeeded;
    
    if(numBytesNeeded > NCHARS_SYMLINK_TARGET_MAX - 1)
        return BKERROR_SYMLINK_TARGET_TOO_LONG;
    
    record = malloc(5 + numBytesNeeded);
    if(record == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    record[0] = 'S';
    record[1] = 'L';
    record[2] = 5 + numBytesNeeded; /* length */
    record[3] = 1; /* version */
    record[4] = 0x00; /* flags */
    
    /* write SL */
    numBytesToSkip = 0;
    stringCount = 0;
    recordCount = 5;
    while(stringCount < targetLen)
    {
        int numBytesToSkip;
        char* nextSlash;
        
        if(symlink->target[stringCount] == '/')
        /* root (/) */
        {
            numBytesToSkip = 1;
            record[recordCount] = 0x08;
            record[recordCount + 1] = 0;
            recordCount += 2;
        }
        else if( symlink->target[stringCount] == '.' && 
                 (stringCount + 1 == targetLen || symlink->target[stringCount + 1] == '/') )
        /* current (.) */
        {
            numBytesToSkip = 2;
            record[recordCount] = 0x02;
            record[recordCount + 1] = 0;
            recordCount += 2;
        }
        else if( symlink->target[stringCount] == '.' && 
                 stringCount + 1 < targetLen && symlink->target[stringCount + 1] == '.' )
        /* parent (..) */
        {
            numBytesToSkip = 3;
            record[recordCount] = 0x04;
            record[recordCount + 1] = 0;
            recordCount += 2;
        }
        else
        /* regular filename */
        {
            nextSlash = strchr(symlink->target + stringCount, '/');
            if(nextSlash != NULL)
                numBytesToSkip = nextSlash - (symlink->target + stringCount);
            else
                numBytesToSkip = targetLen - stringCount;
            
            record[recordCount] = 0x00;
            record[recordCount + 1] = numBytesToSkip;
            strncpy((char*)record + recordCount + 2, symlink->target + stringCount, numBytesToSkip);
            recordCount += 2 + numBytesToSkip;
            
            numBytesToSkip += 1;
        }
        
        /* + separator */
        stringCount += numBytesToSkip;
    }
    
    if(recordCount != numBytesNeeded + 5)
    {
        free(record);
        return BKERROR_SANITY;
    }
    
    rc = wcWrite(volInfo, (char*)record, recordCount);
    if(rc <= 0)
    {
        free(record);
        return rc;
    }
    
    free(record);

    return 5 + numBytesNeeded;
}

/* This doesn't need support for CE because it's only written in one place,
* the root 'self' directory record. */
int writeRockSP(VolInfo* volInfo)
{
    int rc;
    unsigned char record[7];
    
    /* identification */
    record[0] = 'S';
    record[1] = 'P';
    
    /* record length */
    record[2] = 7;
    
    /* entry version */
    record[3] = 1;
    
    /* check bytes */
    record[4] = 0xBE;
    record[5] = 0xEF;
    
    /* bytes skipped */
    record[6] = 0;
    
    rc = wcWrite(volInfo, (char*)record, 7);
    if(rc <= 0)
        return rc;
    
    return 1;
}

int writeVdsetTerminator(VolInfo* volInfo)
{
    int rc;
    unsigned char byte;
    unsigned char aString[6];
    
    /* volume descriptor type */
    byte = 255;
    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    
    /* standard identifier */
    strcpy((char*)aString, "CD001");
    rc = wcWrite(volInfo, (char*)aString, 5);
    if(rc <= 0)
        return rc;
    
    /* volume descriptor version */
    byte = 1;
    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    
    rc = writeByteBlock(volInfo, 0, 2041);
    if(rc < 0)
        return rc;
    
    return 1;
}

/*
* -has to be called after the files were written so that the 
*  volume size is recorded properly
* -rootdr location, size are in bytes
* -note strings are not terminated on image
*/
int writeVolDescriptor(VolInfo* volInfo, off_t rootDrLocation,
                       unsigned rootDrSize, off_t lPathTableLoc, 
                       off_t mPathTableLoc, unsigned pathTableSize, 
                       time_t creationTime, bool isPrimary)
{
    int rc;
    int count;
    
    unsigned char byte;
    unsigned char aString[129];
    unsigned anUnsigned;
    unsigned short anUnsignedShort;
    size_t currPos;
    
    /* VOLUME descriptor type */
    if(isPrimary)
        byte = 1;
    else
        byte = 2;
    /* END VOLUME descriptor type */

    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    
    /* standard identifier */
    strcpy((char*)aString, "CD001");
    rc = wcWrite(volInfo, (char*)aString, 5);
    if(rc <= 0)
        return rc;
    
    /* volume descriptor version (always 1) */
    byte = 1;
    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    
    /* primary: unused field
    *  supplementary: volume flags, 0x00 */
    byte = 0;
    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    
    /* system identifier (32 spaces) */
    if(isPrimary)
    {
        strcpy((char*)aString, "                                ");
        rc = wcWrite(volInfo, (char*)aString, 32);
        if(rc <= 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(volInfo, "", 32);
        if(rc < 0)
            return rc;
    }
    
    /* VOLUME identifier */
    if(isPrimary)
    {
        strcpy((char*)aString, volInfo->volId);
        
        for(count = strlen((char*)aString); count < 32; count++)
            aString[count] = ' ';
        
        rc = wcWrite(volInfo, (char*)aString, 32);
        if(rc <= 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(volInfo, volInfo->volId, 32);
        if(rc < 0)
            return rc;
    }
    /* END VOLUME identifier */
    
    /* unused field */
    rc = writeByteBlock(volInfo, 0, 8);
    if(rc < 0)
        return rc;
    
    /* VOLUME space size (number of logical blocks, absolutely everything) */
    /* it's safe to not use wcSeek() here since everything is left as it is */
    currPos = lseek(volInfo->imageForWriting, 0, SEEK_CUR);
    
    lseek(volInfo->imageForWriting, 0, SEEK_END);
    anUnsigned = lseek(volInfo->imageForWriting, 0, SEEK_CUR) / 
                 NBYTES_LOGICAL_BLOCK;
    
    lseek(volInfo->imageForWriting, currPos, SEEK_SET);
    
    rc = write733(volInfo, anUnsigned);
    if(rc <= 0)
        return rc;
    /* END VOLUME space size (number of logical blocks, absolutely everything) */
    
    /* primary: unused field
    *  joliet: escape sequences */
    if(isPrimary)
    {
        rc = writeByteBlock(volInfo, 0, 32);
        if(rc < 0)
            return rc;
    }
    else
    {
        /* this is the only joliet field that's padded with 0x00 instead of ' ' */
        aString[0] = 0x25;
        aString[1] = 0x2F;
        aString[2] = 0x45;
        
        rc = wcWrite(volInfo, (char*)aString, 3);
        if(rc <= 0)
            return rc;
        
        rc = writeByteBlock(volInfo, 0, 29);
        if(rc < 0)
            return rc;
    }
    
    /* volume set size (always 1) */
    anUnsignedShort = 1;
    rc = write723(volInfo, anUnsignedShort);
    if(rc <= 0)
        return rc;
    
    /* volume sequence number (also always 1) */
    rc = write723(volInfo, anUnsignedShort);
    if(rc <= 0)
        return rc;
    
    /* logical block size (always 2048) */
    anUnsignedShort = NBYTES_LOGICAL_BLOCK;
    rc = write723(volInfo, anUnsignedShort);
    if(rc <= 0)
        return rc;
    
    /* path table size */
    anUnsigned = pathTableSize;
    rc = write733(volInfo, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* location of occurence of type l path table */
    anUnsigned = lPathTableLoc / NBYTES_LOGICAL_BLOCK;
    rc = write731(volInfo, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* location of optional occurence of type l path table */
    anUnsigned = 0;
    rc = write731(volInfo, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* location of occurence of type m path table */
    anUnsigned = mPathTableLoc / NBYTES_LOGICAL_BLOCK;
    rc = write732(volInfo, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* location of optional occurence of type m path table */
    anUnsigned = 0;
    rc = write732(volInfo, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* ROOT dr */
        /* record length (always 34 here) */
        byte = 34;
        rc = write711(volInfo, byte);
        if(rc <= 0)
            return rc;
        
        /* extended attribute record length (always none) */
        byte = 0;
        rc = write711(volInfo, byte);
        if(rc <= 0)
            return rc;
        
        /* location of extent */
        anUnsigned = rootDrLocation / NBYTES_LOGICAL_BLOCK;
        rc = write733(volInfo, anUnsigned);
        if(rc <= 0)
            return rc;
        
        /* data length */
        rc = write733(volInfo, rootDrSize);
        if(rc <= 0)
            return rc;
        
        /* recording time */
        epochToShortString(creationTime, (char*)aString);
        rc = wcWrite(volInfo, (char*)aString, 7);
        if(rc <= 0)
            return rc;
        
        /* file flags (always binary 00000010 here) */
        byte = 0x02;
        rc = write711(volInfo, byte);
        if(rc <= 0)
            return rc;
        
        /* file unit size (not in interleaved mode -> 0) */
        byte = 0;
        rc = write711(volInfo, byte);
        if(rc <= 0)
            return rc;
        
        /* interleave gap size (not in interleaved mode -> 0) */
        rc = write711(volInfo, byte);
        if(rc <= 0)
            return rc;
         
        /* volume sequence number */
        anUnsignedShort = 1;
        rc = write723(volInfo, anUnsignedShort);
        if(rc <= 0)
            return rc;
        
        /* length of file identifier */
        byte = 1;
        rc = write711(volInfo, byte);
        if(rc <= 0)
            return rc;
        
        /* file identifier */
        byte = 0;
        rc = write711(volInfo, byte);
        if(rc <= 0)
            return rc;
    /* END ROOT dr */
    
    /* volume set identidier */
    if(isPrimary)
    {
        rc = writeByteBlock(volInfo, ' ', 128);
        if(rc < 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(volInfo, "", 128);
        if(rc < 0)
            return rc;
    }
    
    /* PUBLISHER identifier */
    strcpy((char*)aString, volInfo->publisher);
    
    if(isPrimary)
    {
        for(count = strlen((char*)aString); count < 128; count++)
            aString[count] = ' ';
        
        rc = wcWrite(volInfo, (char*)aString, 128);
        if(rc <= 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(volInfo, (char*)aString, 128);
        if(rc < 0)
            return rc;
    }
    /* PUBLISHER identifier */
    
    /* DATA preparer identifier */
    if(isPrimary)
    {
        rc = wcWrite(volInfo, "ISO Master", 10);
        if(rc <= 0)
            return rc;
        
        rc = writeByteBlock(volInfo, ' ', 118);
        if(rc < 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(volInfo, "ISO Master", 128);
        if(rc < 0)
            return rc;
    }
    /* END DATA preparer identifier */
    
    /* application identifier, copyright file identifier, abstract file 
    * identifier, bibliographic file identifier (128 + 3*37) */
    if(isPrimary)
    {
        rc = writeByteBlock(volInfo, ' ', 239);
        if(rc < 0)
            return rc;
    }
    else
    {
        /* application id */
        rc = writeJolietStringField(volInfo, "", 128);
        if(rc < 0)
            return rc;
        
        /* 18 ucs2 spaces + 0x00 */
        for(count = 0; count < 3; count++)
        {
            rc = writeJolietStringField(volInfo, "", 36);
            if(rc < 0)
                return rc;
            
            byte = 0x00;
            rc = wcWrite(volInfo, (char*)&byte, 1);
            if(rc <= 0)
                return rc;
        }
    }
    
    /* VOLUME creation date */
    epochToLongString(creationTime, (char*)aString);
    
    rc = wcWrite(volInfo, (char*)aString, 17);
    if(rc <= 0)
        return rc;
    /* END VOLUME creation date */
    
    /* volume modification date (same as creation) */
    rc = wcWrite(volInfo, (char*)aString, 17);
    if(rc <= 0)
        return rc;
    
    /* VOLUME expiration date (none) */
    rc = writeByteBlock(volInfo, '0', 16);
    if(rc < 0)
        return rc;
    
    byte = 0;
    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    /* END VOLUME expiration date (none) */
    
    /* volume effective date (same as creation) */
    rc = wcWrite(volInfo, (char*)aString, 17);
    if(rc <= 0)
        return rc;
    
    /* file structure version */
    byte = 1;
    rc = write711(volInfo, byte);
    if(rc <= 0)
        return rc;
    
    /* reserved, applications use, reserved */
    rc = writeByteBlock(volInfo, 0, 1166);
    if(rc < 0)
        return rc;
    
    return 1;
}
