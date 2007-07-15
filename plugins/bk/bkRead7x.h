/*******************************************************************************
* bkRead7x
* functions to read simple variables as described in sections 7.x of iso9660
* not including filenames (7.4, 7.5, 7.6)
* 
* if they are stored in both byte orders, the appropriate one is read into
* the parameter but the return is 2x the size of that variable
*
* */

int read711(int image, unsigned char* value);
int read721(int image, unsigned short* value);
int read731(int image, unsigned* value);
int read733(int image, unsigned* value);
void read733FromCharArray(unsigned char* array, unsigned* value);
