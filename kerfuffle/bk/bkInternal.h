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

/********************************* PURPOSE ************************************
* bkInternal.h
* This header file is for #defines and structures only used by bkisofs
******************************** END PURPOSE *********************************/

#ifndef bkInternal_h
#define bkInternal_h

#include "bk.h"

/* number of logical sectors in system area (in practice always 16) */
#define NLS_SYSTEM_AREA 16
/* number of bytes in a logical block (in practice always 2048) */
#define NBYTES_LOGICAL_BLOCK 2048

/*******************************************************************************
* Joliet allows max 128 bytes
*     + 2 separator1 (9660, just in case)
*     + 2 separator2 (9660, just in case)
*     + 10 version (9660, just in case)
*     = 142 bytes (71 characters)
* Only a max of 64 characters of this will be stored. (plus '\0') */
#define NCHARS_FILE_ID_MAX_JOLIET 65

#define BASETW_PTR(item) ((BaseToWrite*)(item))
#define DIRTW_PTR(item) ((DirToWrite*)(item))
#define FILETW_PTR(item) ((FileToWrite*)(item))
#define SYMLINKTW_PTR(item) ((SymLinkToWrite*)(item))

#define WRITE_CACHE_SIZE 1048576

typedef struct
{
    unsigned numChildren;
    char** children;
    
} NewPath;

typedef struct BaseToWrite
{
    char name9660[15]; /* 8.3 + ";1" max */
    char nameRock[NCHARS_FILE_ID_MAX_STORE];
    char nameJoliet[NCHARS_FILE_ID_MAX_JOLIET];
    unsigned posixFileMode;
    off_t extentLocationOffset; /* where on image to write location of extent 
                                *  for this directory */
    unsigned extentNumber; /* extent number */
    unsigned dataLength; /* bytes, including blank */
    off_t extentLocationOffset2; /* for svd (joliet) */
    off_t offsetForCE; /* if the name won't fit inside the directory record */
    
    struct BaseToWrite* next;
    
} BaseToWrite;

typedef struct DirToWrite
{
    BaseToWrite base;
    
    unsigned extentNumber2; /* for svd (joliet) */
    unsigned dataLength2; /* for svd (joliet) */
    struct BaseToWrite* children;
    
} DirToWrite;

typedef struct FileToWrite
{
    BaseToWrite base;
    
    unsigned size; /* in bytes */
    BkHardLink* location; /* basically a copy of the following variables */
    bool onImage;
    unsigned offset; /* if on image, in bytes */
    char* pathAndName; /* if on filesystem, full path + filename
                       * is to be freed by whenever the File is freed */
    BkFile* origFile; /* this pointer only has one purpose: to potentially 
                      * identify this file as the boot record. it will never
                      * be dereferenced, just compared to. */
    
} FileToWrite;

typedef struct SymLinkToWrite
{
    BaseToWrite base;
    
    char target[NCHARS_SYMLINK_TARGET_MAX];
    
} SymLinkToWrite;

#endif
