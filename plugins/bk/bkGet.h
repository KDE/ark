#ifndef BKGET_H
#define BKGET_H
off_t estimateIsoSize(const BkDir* tree, int filenameTypes);
int getDirFromString(const BkDir* tree, const char* pathStr, 
                     BkDir** dirFoundPtr);
#endif
