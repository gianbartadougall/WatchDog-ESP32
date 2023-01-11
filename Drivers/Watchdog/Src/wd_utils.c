
/* Public Marcos */
#include <stdlib.h>

/* Private Macros */
#include "uart_comms.h"
#include "wd_utils.h"

int wd_utils_extract_number(char* string, int* number, int startIndex, char endCharacter) {

    // Create string to store number with enough room for number up to 999,999
    int strNumLength = 6;
    char strNum[strNumLength + 1];

    for (int i = 0; i < strNumLength; i++) {

        strNum[i] = string[i + startIndex];

        // Break out of loop if end character found
        if (strNum[i] == endCharacter || strNum[i] == '\0') {
            strNum[i] = '\0';
            break;
        }

        // Return lowest possible image number if the given string was invalid
        if ((strNum[i] < '0' || strNum[i] > '9')) {
            return UART_ERROR_INVALID_REQUEST;
        }
    }

    strNum[strNumLength + 1] = '\0';
    *number                  = atoi(strNum);

    return WD_SUCCESS;
}

void wd_utils_split_string(char* string, char list[][RX_BUF_SIZE], int startIndex, char delimeter) {

    int listIndex = 0;
    int charIndex = 0;
    int si        = startIndex;

    while (string[si] != '\0') {

        if (string[si] == delimeter) {
            list[listIndex][charIndex] = '\0';
            listIndex++;
            charIndex = 0;
            si++;
            continue;
        }

        list[listIndex][charIndex] = string[si];
        si++;
        charIndex++;
    }

    list[listIndex][charIndex] = '\0';
}