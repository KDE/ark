#ifndef BKADD_H
#define BKADD_H
int add(VolInfo* volInfo, const char* srcPathAndName, BkDir* destDir, 
        const char* nameToUse);
int addDirContents(VolInfo* volInfo, const char* srcPath, BkDir* destDir);
#endif
