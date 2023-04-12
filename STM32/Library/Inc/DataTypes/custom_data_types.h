#ifndef CUSTOM_DATA_TYPES_H
#define CUSTOM_DATA_TYPES_H

/* C Library Includes */
#include <stdint.h>

typedef struct cdt_u8_t {
    uint8_t value;
} cdt_u8_t;

typedef struct cdt_4_data_u8_t {
    uint8_t bytes[4];
} cdt_4_data_u8_t;

typedef struct cdt_8_data_u8_t {
    uint8_t bytes[8];
} cdt_8_data_u8_t;

typedef struct cdt_16_data_u8_t {
    uint8_t bytes[16];
} cdt_16_data_u8_t;

typedef struct cdt_32_data_u8_t {
    uint8_t bytes[32];
} cdt_32_data_u8_t;

typedef struct cdt_dbl_8_t {
    uint8_t info; // Holds information such as the sign and any error codes
    uint8_t integer;
    uint8_t deicmal;
} cdt_dbl_8_t;

typedef struct cdt_dbl_16_t {
    uint8_t info; // Holds information such as the sign and any error codes
    uint8_t integer;
    uint16_t decimal;
} cdt_dbl_16_t;

#endif // CUSTOM_DATA_TYPES_H