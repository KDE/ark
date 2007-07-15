bool charIsValid9660(char theChar);
bool charIsValidJoliet(char theChar);
unsigned hashString(const char *str, unsigned int length);
int mangleDir(const BkDir* origDir, DirToWrite* newDir, int filenameTypes);
void mangleNameFor9660(const char* origName, char* newName, bool isADir);
void mangleNameForJoliet(const char* origName, char* newName, bool appendHash);
void shortenNameFor9660(const char* origName, char* newName);
