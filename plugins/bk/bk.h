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
* bk.h
* This header file is the public interface to bkisofs.
******************************** END PURPOSE *********************************/

#ifndef bk_h
#define bk_h

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include "bkError.h"

/* can be |ed */
#define FNTYPE_9660 1
#define FNTYPE_ROCKRIDGE 2
#define FNTYPE_JOLIET 4

/* many library functions rely on this being at least 256 */
#define NCHARS_FILE_ID_MAX_STORE 256

/* maximum length of the target of a symbolic link
* !! this is used for both the number of characters in the path and the number
* of bytes in the SL record, that should probably be fixed */
#define NCHARS_SYMLINK_TARGET_MAX 251

/* maximum number of bytes to read from a file for comparing it quickly
* with others (is it likely to be a hard link or not) */
#define MAX_NBYTES_HARDLINK_HEAD 32

/* options for VolInfo.bootMediaType */
#define BOOT_MEDIA_NONE 0
#define BOOT_MEDIA_NO_EMULATION 1
#define BOOT_MEDIA_1_2_FLOPPY 2 /* 1228800 byte floppy disk image */
#define BOOT_MEDIA_1_44_FLOPPY 3 /* 1474560 byte floppy disk image */
#define BOOT_MEDIA_2_88_FLOPPY 4 /* 2949120 byte floppy disk image */
#define BOOT_MEDIA_HARD_DISK 5

#define READ_WRITE_BUFFER_SIZE 102400

/* warning message string lengths in VolInfo */
#define BK_WARNING_MAX_LEN 512

#define IS_DIR(posix)      ((posix & 0770000) == 0040000)
#define IS_REG_FILE(posix) ((posix & 0770000) == 0100000)
#define IS_SYMLINK(posix)  ((posix & 0770000) == 0120000)

#define BK_BASE_PTR(item) ((BkFileBase*)(item))
#define BK_DIR_PTR(item) ((BkDir*)(item))
#define BK_FILE_PTR(item) ((BkFile*)(item))
#define BK_SYMLINK_PTR(item) ((BkSymLink*)(item))

/*******************************************************************************
* BkFileBase
* Linked list node.
* All files, directories, links need this. */
typedef struct BkFileBase
{
    char original9660name[15]; /* 8.3 + ";1" max */
    char name[NCHARS_FILE_ID_MAX_STORE]; /* '\0' terminated */
    unsigned posixFileMode; /* file type and permissions */
    
    struct BkFileBase* next;
    
} BkFileBase;

/*******************************************************************************
* BkDir
* Linked list node.
* Information about a directory and it's contents. */
typedef struct BkDir
{
    BkFileBase base; /* intended to be accessed using a cast */
    
    BkFileBase* children; /* child directories, files, etc. */
    
} BkDir;

/*******************************************************************************
* BkHardLink
* Linked list node.
* Information about a hard link (where to find a certain file).
* This is for internal use but is defined here because BkFile references it.
* You don't need to use this structure, please ignore it. */
typedef struct BkHardLink
{
    bool onImage;
    off_t position; /* if on image */
    char* pathAndName; /* if on filesystem, full path + filename
                       * is to be freed whenever the BkHardLink is freed */
    unsigned size; /* size of the file being pointed to */
    int headSize;
    unsigned char head[MAX_NBYTES_HARDLINK_HEAD];
    bool alreadyCounted; /* for estimateIsoSize() */
    
    unsigned extentNumberWrittenTo; /* only set once one file is written */
    
    struct BkHardLink* next;
    
} BkHardLink;

/*******************************************************************************
* BkFile
* Linked list node.
* Information about a file, whether on the image or on the filesystem. */
typedef struct BkFile
{
    BkFileBase base; /* intended to be accessed using a cast */
    
    unsigned size; /* in bytes, don't need off_t because it's stored 
                   * in a 32bit unsigned int on the iso */
    BkHardLink* location; /* basically a copy of the following variables */
    bool onImage;
    off_t position; /* if on image, in bytes */
    char* pathAndName; /* if on filesystem, full path + filename
                       * is to be freed whenever the File is freed */
    
} BkFile;

/*******************************************************************************
* BkSymLink
* Linked list node.
* Information about a symbolic link. */
typedef struct BkSymLink
{
    BkFileBase base; /* intended to be accessed using a cast */
    
    char target[NCHARS_SYMLINK_TARGET_MAX];
    
} BkSymLink;

/*******************************************************************************
* VolInfo
* Information about a volume (one image).
* Strings are '\0' terminated. */
typedef struct VolInfo
{
    /* private bk use  */
    unsigned filenameTypes;
    off_t pRootDrOffset; /* primary (9660 and maybe rockridge) */
    off_t sRootDrOffset; /* secondary (joliet), 0 if does not exist */
    off_t bootRecordSectorNumberOffset;
    int imageForReading;
    ino_t imageForReadingInode; /* to know which file was open for reading
                                * (filename is not reliable) */
    const BkFile* bootRecordOnImage; /* if visible, pointer to the file in the 
                                     *  directory tree */
    char warningMessage[BK_WARNING_MAX_LEN];
    bool rootRead; /* did i read the root record inside volume descriptor? */
    bool stopOperation; /* cancel current opertion */
    int imageForWriting;
    void(*progressFunction)(struct VolInfo*);
    void(*writeProgressFunction)(struct VolInfo*, double);
    time_t lastTimeCalledProgress;
    off_t estimatedIsoSize;
    BkHardLink* fileLocations; /* list of where to find regular files */
    char readWriteBuffer[READ_WRITE_BUFFER_SIZE];
    char readWriteBuffer2[READ_WRITE_BUFFER_SIZE];
    
    /* public use, read only */
    time_t creationTime;
    BkDir dirTree;
    unsigned char bootMediaType;
    unsigned bootRecordSize;       /* in bytes */
    bool bootRecordIsOnImage;      /* unused if visible (flag below) */
    off_t bootRecordOffset;     /* if on image */
    char* bootRecordPathAndName;   /* if on filesystem */
    bool bootRecordIsVisible;      /* whether boot record is a visible file 
                                   *  on the image */
    bool scanForDuplicateFiles;    /* whether to check every file for uniqueness
                                   * to decide is it a hard link or not */
    
    /* public use, read/write */
    char volId[33];
    char publisher[129];
    char dataPreparer[129];
    unsigned posixFileDefaults;    /* for extracting */
    unsigned posixDirDefaults;     /* for extracting or creating on iso */
    bool(*warningCbk)(const char*);
    bool followSymLinks;           /* whether to stat the link itself rather 
                                   *  than the file it's pointing to */
    
} VolInfo;

/* public bkisofs functions */

/* adding */
int bk_add_boot_record(VolInfo* volInfo, const char* srcPathAndName, 
                       int bootMediaType);
int bk_add(VolInfo* volInfo, const char* srcPathAndName, 
           const char* destPathStr, void(*progressFunction)(VolInfo*));
int bk_add_as(VolInfo* volInfo, const char* srcPathAndName, 
              const char* destPathStr, const char* nameToUse, 
              void(*progressFunction)(VolInfo*));
int bk_create_dir(VolInfo* volInfo, const char* destPathStr, 
                  const char* newDirName);

/* deleting */
void bk_delete_boot_record(VolInfo* volInfo);
int bk_delete(VolInfo* volInfo, const char* pathAndName);

/* extracting */
int bk_extract_boot_record(VolInfo* volInfo, const char* destPathAndName,
                           unsigned destFilePerms);
int bk_extract(VolInfo* volInfo, const char* srcPathAndName, 
               const char* destDir, bool keepPermissions, 
               void(*progressFunction)(VolInfo*));
int bk_extract_as(VolInfo* volInfo, const char* srcPathAndName, 
                  const char* destDir, const char* nameToUse,
                  bool keepPermissions, void(*progressFunction)(VolInfo*));

/* getters */
off_t bk_estimate_iso_size(const VolInfo* volInfo, int filenameTypes);
time_t bk_get_creation_time(const VolInfo* volInfo);
int bk_get_dir_from_string(const VolInfo* volInfo, const char* pathStr, 
                           BkDir** dirFoundPtr);
int bk_get_permissions(VolInfo* volInfo, const char* pathAndName, 
                       mode_t* permissions);
const char* bk_get_publisher(const VolInfo* volInfo);
const char* bk_get_volume_name(const VolInfo* volInfo);
const char* bk_get_error_string(int errorId);

/* setters */
void bk_cancel_operation(VolInfo* volInfo);
void bk_destroy_vol_info(VolInfo* volInfo);
int bk_init_vol_info(VolInfo* volInfo, bool scanForDuplicateFiles);
int bk_rename(VolInfo* volInfo, const char* srcPathAndName, 
              const char* newName);
int bk_set_boot_file(VolInfo* volInfo, const char* srcPathAndName);
void bk_set_follow_symlinks(VolInfo* volInfo, bool doFollow);
int bk_set_permissions(VolInfo* volInfo, const char* pathAndName, 
                       mode_t permissions);
void bk_set_publisher(VolInfo* volInfo, const char* publisher);
void bk_set_vol_name(VolInfo* volInfo, const char* volName);

/* reading */
int bk_open_image(VolInfo* volInfo, const char* filename);
int bk_read_dir_tree(VolInfo* volInfo, int filenameType, 
                     bool keepPosixPermissions, 
                     void(*progressFunction)(VolInfo*));
int bk_read_vol_info(VolInfo* volInfo);

/* writing */
int bk_write_image(const char* newImagePathAndName, VolInfo* volInfo, 
                   time_t creationTime, int filenameTypes, 
                   void(*progressFunction)(VolInfo*, double));

#ifdef __cplusplus
}
#endif

#endif
