#ifndef bkDelete_h
#define bkDelete_h

#include "bkInternal.h"

void deleteDirContents(VolInfo* volInfo, BkDir* dir);
void deleteNode(VolInfo* volInfo, BkDir* parentDir, char* nodeToDeleteName);
void deleteRegFileContents(VolInfo* volInfo, BkFile* file);

#endif
