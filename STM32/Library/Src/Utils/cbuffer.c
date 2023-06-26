/* C Library Includes */
#include <stdio.h>

/* Personal Includes */
#include "cbuffer.h"

void cbuffer_init(cbuffer_t* Cbuffer, void* buffer, uint16_t elementSize, uint16_t maxNumElements) {
    Cbuffer->elements       = buffer;
    Cbuffer->elementSize    = elementSize;
    Cbuffer->maxNumElements = maxNumElements;
    Cbuffer->wIndex         = 0;
    Cbuffer->rIndex         = 0;
}

void cbuffer_increment_read_index(cbuffer_t* Cbuffer) {
    Cbuffer->rIndex++;
    if (Cbuffer->rIndex == Cbuffer->maxNumElements) {
        Cbuffer->rIndex = 0;
    }
}

void cbuffer_increment_write_index(cbuffer_t* Cbuffer) {
    Cbuffer->wIndex++;
    if (Cbuffer->wIndex == Cbuffer->maxNumElements) {
        Cbuffer->wIndex = 0;
    }
}

uint8_t cbuffer_read_next_element(cbuffer_t* Cbuffer, void* element) {

    if (Cbuffer->rIndex == Cbuffer->wIndex) {
        return FALSE;
    }

    cbuffer_read_current_byte(Cbuffer, element);

    cbuffer_increment_read_index(Cbuffer);
    return TRUE;
}

void cbuffer_read_current_byte(cbuffer_t* Cbuffer, void* element) {

    // Cast the data pointer to the appropriate type
    uint8_t* ptr = (uint8_t*)Cbuffer->elements;
    ptr += Cbuffer->rIndex * Cbuffer->elementSize;

    // Copy the element from the buffer into the provided element pointer
    uint8_t* elementPtr = (uint8_t*)element;

    for (uint16_t i = 0; i < Cbuffer->elementSize; i++) {
        elementPtr[i] = ptr[i];
    }
}

void cbuffer_append_element(cbuffer_t* Cbuffer, void* element) {

    // Cast the data pointer to the appropriate type
    uint8_t* ptr = (uint8_t*)Cbuffer->elements;
    ptr += Cbuffer->wIndex * Cbuffer->elementSize;

    // Copy the element from the buffer into the provided element pointer
    uint8_t* elementPtr = (uint8_t*)element;
    for (uint16_t i = 0; i < Cbuffer->elementSize; i++) {
        ptr[i] = elementPtr[i];
    }

    cbuffer_increment_write_index(Cbuffer);
}

void cbuffer_reset_read_index(cbuffer_t* Cbuffer) {
    Cbuffer->rIndex = 0;
}

void cbuffer_reset_write_index(cbuffer_t* Cbuffer) {
    Cbuffer->wIndex = 0;
}

uint8_t cbuffer_is_empty(cbuffer_t* Cbuffer) {
    return Cbuffer->rIndex != Cbuffer->wIndex;
}