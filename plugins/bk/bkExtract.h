int copyByteBlock(VolInfo* volInfo, int src, int dest, unsigned numBytes);
int extract(VolInfo* volInfo, BkDir* parentDir, char* nameToExtract, 
            const char* destDir, const char* nameToUse, bool keepPermissions);
int extractDir(VolInfo* volInfo, BkDir* srcDir, const char* destDir, 
               const char* nameToUse, bool keepPermissions);
int extractFile(VolInfo* volInfo, BkFile* srcFileInTree, const char* destDir, 
                const char* nameToUse, bool keepPermissions);
int extractSymlink(BkSymLink* srcLink, const char* destDir, 
                   const char* nameToUse);
