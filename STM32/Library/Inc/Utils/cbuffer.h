#ifndef CBUFFER_H
#define CBUFFER_H

/* C Library Includes */
#include <stdint.h>

/* Personal Includes */

#define TRUE  1
#define FALSE 0

typedef struct cbuffer_t {
    void* elements;          // Pointer to a buffer that can hold arbitrary data
    uint16_t elementSize;    // The size of each element in the circular buffer
    uint16_t maxNumElements; // Maximum number of elements the buffer can hold
    uint16_t wIndex;         // Index of buffer where next byte will be written
    uint16_t rIndex;         // Index of buffer where next byte will be read from
} cbuffer_t;

void cbuffer_init(cbuffer_t* Cbuffer, void* buffer, uint16_t elementSize, uint16_t maxNumElements);

void cbuffer_increment_read_index(cbuffer_t* Cbuffer);

void cbuffer_increment_write_index(cbuffer_t* Cbuffer);

uint8_t cbuffer_read_next_element(cbuffer_t* Cbuffer, void* element);

void cbuffer_read_current_byte(cbuffer_t* Cbuffer, void* element);

void cbuffer_write_element(cbuffer_t* Cbuffer, void* element);

void cbuffer_reset_read_index(cbuffer_t* Cbuffer);

void cbuffer_reset_write_index(cbuffer_t* Cbuffer);

uint8_t cbuffer_is empty(cbuffer_t* Cbuffer);

#endif // CBUFFER_H