#include "chars.h"
#include "log.h"

uint8_t chars_same(char* str1, char* str2) {

    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {

        if (str1[i] != str2[i]) {
            return FALSE;
        }

        i++;
    }

    return TRUE;
}