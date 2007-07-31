/******************************************************************************
* errnum.h
* this file contains #defined ints for return codes (errors and warnings)
* */

/* error codes in between these numbers */
#define BKERROR_MAX_ID                           -1001
#define BKERROR_MIN_ID                           -10000

/* warning codes in between these numbers */
#define BKWARNING_MAX_ID                         -10001
#define BKWARNING_MIN_ID                         -20000

/* #define IS_ERROR(number)   ( (((number) >= BKERROR_MIN_ID) && ((number) <= BKERROR_MAX_ID)) ? true : false )
* #define IS_WARNING(number) ( (((number) >= BKWARNING_MIN_ID) && ((number) <= BKWARNING_MAX_ID)) ? true : false )*/

#define BKERROR_READ_GENERIC                     -1001
#define BKERROR_READ_GENERIC_TEXT                "Failed to read expected number of bytes"
#define BKERROR_DIR_NOT_FOUND_ON_IMAGE           -1002
#define BKERROR_DIR_NOT_FOUND_ON_IMAGE_TEXT      "Directory not found on image"
#define BKERROR_MAX_NAME_LENGTH_EXCEEDED         -1003
#define BKERROR_MAX_NAME_LENGTH_EXCEEDED_TEXT    "Maximum file/directory name length exceeded"
#define BKERROR_STAT_FAILED                      -1004
#define BKERROR_STAT_FAILED_TEXT                 "Failed to stat file/directory"
#define BKERROR_TARGET_NOT_A_DIR                 -1005
#define BKERROR_TARGET_NOT_A_DIR_TEXT            "Target not a directory (UI problem)"
#define BKERROR_OUT_OF_MEMORY                    -1006
#define BKERROR_OUT_OF_MEMORY_TEXT               "Out of memory"
#define BKERROR_OPENDIR_FAILED                   -1007
#define BKERROR_OPENDIR_FAILED_TEXT              "Failed to open directory for listing"
#define BKERROR_EXOTIC                           -1008
#define BKERROR_EXOTIC_TEXT                      "Some really exotic problem happened"                      
#define BKERROR_FIXME                            -1009
#define BKERROR_FIXME_TEXT                       "Incomplete/broken something that the author needs to fix, please report bug"
#define BKERROR_FILE_NOT_FOUND_ON_IMAGE          -1010
#define BKERROR_FILE_NOT_FOUND_ON_IMAGE_TEXT     "File not found on image"
#define BKERROR_MKDIR_FAILED                     -1011
#define BKERROR_MKDIR_FAILED_TEXT                "Failed to create directory on the filesystem"
#define BKERROR_OPEN_WRITE_FAILED                -1012
#define BKERROR_OPEN_WRITE_FAILED_TEXT           "Failed to open file on the filesystem for writing"
#define BKERROR_WRITE_GENERIC                    -1013
#define BKERROR_WRITE_GENERIC_TEXT               "Failed to write expected number of bytes (disk full?)"
#define BKERROR_MANGLE_TOO_MANY_COL              -1014
#define BKERROR_MANGLE_TOO_MANY_COL_TEXT         "Too many collisons while mangling filenames (too many files/directories with a similar name)"
#define BKERROR_MISFORMED_PATH                   -1015
#define BKERROR_MISFORMED_PATH_TEXT              "Misformed path"
#define BKERROR_INVALID_UCS2                     -1016
#define BKERROR_INVALID_UCS2_TEXT                "Invalid UCS-2 string"
#define BKERROR_UNKNOWN_FILENAME_TYPE            -1017
#define BKERROR_UNKNOWN_FILENAME_TYPE_TEXT       "Unknown filename type"
#define BKERROR_RR_FILENAME_MISSING              -1018
#define BKERROR_RR_FILENAME_MISSING_TEXT         "Rockridge filename missing when expected on image"
#define BKERROR_VD_NOT_PRIMARY                   -1019
#define BKERROR_VD_NOT_PRIMARY_TEXT              "First volume descriptor type not primary like ISO9660 requires"
#define BKERROR_SANITY                           -1020
#define BKERROR_SANITY_TEXT                      "Internal library failure (sanity check), please report bug"
#define BKERROR_OPEN_READ_FAILED                 -1021
#define BKERROR_OPEN_READ_FAILED_TEXT            "Failed to open file on the filesystem for reading"
#define BKERROR_DIRNAME_NEED_TRAILING_SLASH      -1022
#define BKERROR_DIRNAME_NEED_TRAILING_SLASH_TEXT "String specifying directory name must end with '/'"
#define BKERROR_EXTRACT_ROOT                     -1023
#define BKERROR_EXTRACT_ROOT_TEXT                "Extracting root of iso not allowed"
#define BKERROR_DELETE_ROOT                      -1024
#define BKERROR_DELETE_ROOT_TEXT                 "Deleting root of iso not allowed"
#define BKERROR_DUPLICATE_ADD                    -1025
#define BKERROR_DUPLICATE_ADD_TEXT               "Cannot add item because another item with the same name already exists in this directory"
#define BKERROR_DUPLICATE_EXTRACT                -1026
#define BKERROR_DUPLICATE_EXTRACT_TEXT           "Cannot extract item because another item with the same name already exists in this directory"
#define BKERROR_NO_SPECIAL_FILES                 -1027
#define BKERROR_NO_SPECIAL_FILES_TEXT            "Special files (device files and such) are not supported"
#define BKERROR_NO_POSIX_PRESENT                 -1028
#define BKERROR_NO_POSIX_PRESENT_TEXT            "No posix extentions found"
#define BKERROR_EXTRACT_ABSENT_BOOT_RECORD       -1029
#define BKERROR_EXTRACT_ABSENT_BOOT_RECORD_TEXT  "Cannot extract boot record because there isn't one one the image"
#define BKERROR_EXTRACT_UNKNOWN_BOOT_MEDIA       -1030
#define BKERROR_EXTRACT_UNKNOWN_BOOT_MEDIA_TEXT  "Unable to extract boot record of unknown media type"
#define BKERROR_ADD_UNKNOWN_BOOT_MEDIA           -1031
#define BKERROR_ADD_UNKNOWN_BOOT_MEDIA_TEXT      "Unable to add boot record with unknown media type"
#define BKERROR_ADD_BOOT_RECORD_WRONG_SIZE       -1032
#define BKERROR_ADD_BOOT_RECORD_WRONG_SIZE_TEXT  "Size of boot record on the filesystem does not match the size requested via the boot record type parameter"
#define BKERROR_WRITE_BOOT_FILE_4                -1033
#define BKERROR_WRITE_BOOT_FILE_4_TEXT           "Size of no emulation boot record visible on image must be divisible by 4 so i can do a checksum (invalid boot file?)"
#define BKERROR_DUPLICATE_CREATE_DIR             -1034
#define BKERROR_DUPLICATE_CREATE_DIR_TEXT        "Cannot create directory because another file or directory with the same name exists"
#define BKERROR_NAME_INVALID_CHAR                -1035
#define BKERROR_NAME_INVALID_CHAR_TEXT           "Name contains invalid character(s)"
#define BKERROR_BLANK_NAME                       -1036
#define BKERROR_BLANK_NAME_TEXT                  "Name cannot be blank"
#define BKERROR_ADD_FILE_TOO_BIG                 -1037
#define BKERROR_ADD_FILE_TOO_BIG_TEXT            "Cannot add file larger than 4294967295 bytes because the ISO filesystem does not support such large files"
#define BKERROR_SAVE_OVERWRITE                   -1038
#define BKERROR_SAVE_OVERWRITE_TEXT              "Cannot overwrite original image when saving"
#define BKERROR_OPER_CANCELED_BY_USER            -1039
#define BKERROR_OPER_CANCELED_BY_USER_TEXT       "You have canceled the operation"
#define BKERROR_NOT_DIR_IN_PATH                  -1040
#define BKERROR_NOT_DIR_IN_PATH_TEXT             "One of the names in the path is not a directory"
#define BKERROR_WRONG_EXTRACT_FILE               -1041
#define BKERROR_WRONG_EXTRACT_FILE_TEXT          "Tried to extract something that's not a file using the file extracting function"
#define BKERROR_NOT_REG_FILE_FOR_BR              -1042
#define BKERROR_NOT_REG_FILE_FOR_BR_TEXT         "Can only use a regular file as a boot record"
#define BKERROR_WRITE_CACHE_OVERFLOWED           -1043
#define BKERROR_WRITE_CACHE_OVERFLOWED_TEXT      "Write cache overflowed, please report bug"
#define BKERROR_CREATE_SYMLINK_FAILED            -1044
#define BKERROR_CREATE_SYMLINK_FAILED_TEXT       "Failed to create symbolic link"
#define BKERROR_SYMLINK_TARGET_TOO_LONG          -1045
#define BKERROR_SYMLINK_TARGET_TOO_LONG_TEXT     "Too many characters in target of a symbolic link"
#define BKERROR_HARD_LINK_CALL_PARAMS            -1046
#define BKERROR_HARD_LINK_CALL_PARAMS_TEXT       "Call to a hard link function with both a 0 offset and a NULL filename not allowed"
#define BKERROR_NAME_INVALID                     -1047
#define BKERROR_NAME_INVALID_TEXT                "Invalid file/directory name"
#define BKERROR_RENAME_ROOT                      -1048
#define BKERROR_RENAME_ROOT_TEXT                 "Cannot rename the root directory"
#define BKERROR_ITEM_NOT_FOUND_ON_IMAGE          -1049
#define BKERROR_ITEM_NOT_FOUND_ON_IMAGE_TEXT     "Item not found on image"
#define BKERROR_DUPLICATE_RENAME                 -1050
#define BKERROR_DUPLICATE_RENAME_TEXT            "Cannot rename item because another file or directory with the same name exists"
#define BKERROR_GET_PERM_BAD_PARAM               -1051
#define BKERROR_GET_PERM_BAD_PARAM_TEXT          "bk_get_permissions() called with NULL mode_t*"

#define BKWARNING_OPER_PARTLY_FAILED             -10001
#define BKWARNING_OPER_PARTLY_FAILED_TEXT        "Operation was only partially successful or perhaps completely unsuccessful"

/* do not make up #defines with numbers lower then this */
#define BKERROR_END                              -1000000
#define BKERROR_END_TEXT                         "Double oops, unusable error number used"
