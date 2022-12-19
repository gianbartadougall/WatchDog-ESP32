#ifndef WD_UTILS_H
#define WD_UTILS_H

#define FALSE      0
#define TRUE       1
#define WD_ERROR   0
#define WD_SUCCESS 1

int wd_utils_extract_number(char* imageName, int startIndex, char endCharacter);

#endif // WD_UTILS_H