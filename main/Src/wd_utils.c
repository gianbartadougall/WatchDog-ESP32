
/* Public Marcos */
#include <stdlib.h>

/* Private Macros */
#include "wd_utils.h"

int wd_utils_extract_image_number(char* imageName, int startIndex, char endCharacter) {

    // Create string to store number with enough room for number up to 999,999
    int length = 6;
    char strNum[length + 1];

    for (int i = 0; i < length; i++) {

        strNum[i] = imageName[i + startIndex];

        // Breka out of loop if end character found
        if (strNum[i] == endCharacter) {
            strNum[i] = '\0';
            break;
        }

        // Return lowest possible image number if the given string was invalid
        if ((strNum[i] < '0' || strNum[i] > '9')) {
            return 0;
        }
    }

    strNum[length + 1] = '\0';
    return atoi(strNum);
}