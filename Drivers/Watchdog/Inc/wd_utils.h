#ifndef WD_UTILS_H
#define WD_UTILS_H

#define FALSE      0
#define TRUE       1
#define WD_ERROR   0
#define WD_SUCCESS 1

int wd_utils_extract_number(char imageName[100], int* number, int startIndex, char endCharacter);

void wd_utils_split_string(char* string, char list[][100], int startIndex, char delimeter);

#endif // WD_UTILS_H