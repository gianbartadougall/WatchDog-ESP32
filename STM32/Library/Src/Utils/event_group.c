/**
 * @file watchdog.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-03
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */

/* STM32 Library Includes */

/* Personal Includes */
#include "event_group.h"
#include "utils.h"

/* Private Macros */

/* Private Variables */

/* Function Prototypes */

void event_group_set_bit(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e trait) {
    Eg->bits[trait] |= (0x01 << bit);
}

void event_group_set_bits(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e* traits, uint8_t numTraits) {
    for (uint8_t i = 0; i < numTraits; i++) {
        Eg->bits[traits[i]] |= (0x01 << bit);
    }
}

void event_group_clear_bit(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e trait) {
    Eg->bits[trait] &= ~(0x01 << bit);
}

void event_group_clear_bits(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e* traits, uint8_t numTraits) {
    for (uint8_t i = 0; i < numTraits; i++) {
        Eg->bits[traits[i]] &= ~(0x01 << bit);
    }
}

uint8_t event_group_poll_bit(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e trait) {
    return (Eg->bits[trait] & (0x01 << bit)) == 0 ? FALSE : TRUE;
}

void event_group_clear(EventGroup_t* Eg) {
    for (uint8_t i = 0; i < EG_NUM_TRAITS; i++) {
        Eg->bits[i] = 0;
    }
}