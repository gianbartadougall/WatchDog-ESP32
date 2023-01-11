#ifndef WD_UTILS_H
#define WD_UTILS_H

/* Public Includes */
#include "uart_comms.h"

/* Public Macros */
#define FALSE 0
#define TRUE 1
#define WD_ERROR 0
#define WD_SUCCESS 1

int wd_utils_extract_number(char *string, int *number, int startIndex, char endCharacter);

void wd_utils_split_string(char *string, char list[][RX_BUF_SIZE], int startIndex, char delimeter);

#endif // WD_UTILS_H