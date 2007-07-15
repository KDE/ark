int add(VolInfo* volInfo, const char* srcPathAndName, BkDir* destDir);
int addDirContents(VolInfo* volInfo, const char* srcPath, BkDir* destDir);
bool itemIsInDir(const char* name, const BkDir* dir);
