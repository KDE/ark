int appendStringIfHaveRoom(char* dest, char* src, int destMaxLen, 
                           int destCharsAlreadyUsed, int maxSrcLen);
bool dirDrFollows(int image);
bool haveNextRecordInSector(int image);
int readDir(VolInfo* volInfo, BkDir* dir, int filenameType, 
            bool keepPosixPermissions);
int readDirContents(VolInfo* volInfo, BkDir* dir, unsigned size, 
                    int filenameType, bool keepPosixPermissions);
int readFileInfo(VolInfo* volInfo, BkFile* file, int filenameType, 
                 bool keepPosixPermissions, BkFileBase** specialFile);
unsigned char readNextRecordLen(int image);
int readPosixFileMode(VolInfo* volInfo, unsigned* posixPermissions, 
                         unsigned lenSU);
int readRockridgeFilename(VolInfo* volInfo, char* dest, unsigned lenSU, 
                          unsigned numCharsReadAlready);
int readRockridgeSymlink(VolInfo* volInfo, BkSymLink** dest, unsigned lenSU);
void removeCrapFromFilename(char* filename, int length);
int skipDR(int image);
void stripSpacesFromEndOfString(char* str);
