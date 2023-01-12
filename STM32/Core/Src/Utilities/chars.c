#include "chars.h"

uint16_t chars_get_length(char* str) {

    uint16_t i = 0;
    while (str[i] != '\0') {
        i++;
    }

    return i;
}

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

uint8_t chars_to_int(char* str, int* number) {

    if (chars_get_length(str) > 9) {
        return FALSE;
    }

    int i = 0;
    while (str[i] != '\0') {

        if (str[i] < '0' || str[i] > '9') {
            return FALSE;
        }

        i++;
    }

    *number = atoi(str);

    return TRUE;
}