/**
 * @file chars.c
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-15
 *
 * @copyright Copyright (c) 2023
 *
 */

/* Public Includes */
#include <stdlib.h>

/* Private Includes */
#include "chars.h"

uint8_t chars_contains(char* string, char* substring) {

    uint16_t numBytes       = chars_get_num_bytes(string);
    uint16_t subStrNumBytes = chars_get_num_bytes(substring);

    for (int i = 0; i <= numBytes - subStrNumBytes; i++) {

        for (int j = 0; j < subStrNumBytes; j++) {

            if (string[i + j] != substring[j]) {
                break;
            }

            if (j == (subStrNumBytes - 1)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

uint16_t chars_get_num_bytes(char* str) {

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

    if (str1[i] != str2[i]) {
        return FALSE;
    }

    return TRUE;
}

uint8_t chars_to_int(char* str, int* number) {

    if (chars_get_num_bytes(str) > 9) {
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