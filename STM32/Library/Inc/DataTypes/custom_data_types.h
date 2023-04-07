#ifndef CUSTOM_DATA_TYPES_H
#define CUSTOM_DATA_TYPES_H

/* C Library Includes */
#include <stdint.h>

typedef struct cdt_double8_t {
    uint8_t info; // Holds information such as the sign and any error codes
    uint8_t integer;
    uint8_t deicmal;
} cdt_double8_t;

typedef struct cdt_double16_t {
    uint8_t info; // Holds information such as the sign and any error codes
    uint8_t integer;
    uint16_t decimal;
} cdt_double16_t;

#endif // CUSTOM_DATA_TYPES_H