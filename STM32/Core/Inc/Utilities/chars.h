#ifndef CHARS_H
#define CHARS_H

/* Public Includes */
#include "stdint.h"

/* Private Includes */
#include "utilities.h"

uint16_t chars_get_num_chars(char* str);

uint8_t chars_same(char* str1, char* str2);

uint8_t chars_to_int(char* str, int* number);

uint8_t chars_contains(char* string, char* substring);

#endif // CHAR_H