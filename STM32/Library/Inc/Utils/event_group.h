#ifndef EVENT_GROUP_H
#define EVENT_GROUP_H

/* C Library Includes */
#include <stdint.h>

/* Public Macros */
#define EG_NUM_TRAITS 2

enum eg_trait_e {
    EGT_ACTIVE = 0,
    EGT_STM_REQUEST,
};

enum eg_bit_e {
    EGB_TAKE_PHOTO = 0,
    EGB_GREEN_LED_ON,
    EGB_GREEN_LED_OFF,
    EGB_RED_LED_ON,
    EGB_RED_LED_OFF,
    EGB_5,
    EGB_6,
    EGB_7,
    EGB_8,
    EGB_9,
    EGB_10,
    EGB_11,
    EGB_12,
    EGB_13,
    EGB_14,
    EGB_15,
    EGB_16,
    EGB_17,
    EGB_18,
    EGB_19,
    EGB_20,
    EGB_21,
    EGB_22,
    EGB_23,
    EGB_24,
    EGB_25,
    EGB_26,
    EGB_27,
    EGB_28,
    EGB_29,
    EGB_30,
    EGB_31
};

typedef struct EventGroup_t {
    uint32_t bits[EG_NUM_TRAITS];
} EventGroup_t;

/* Function Prototypes */
void event_group_set_bit(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e trait);

void event_group_set_bits(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e* traits, uint8_t numTraits);

void event_group_clear_bit(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e trait);

void event_group_clear_bits(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e* traits, uint8_t numTraits);

uint8_t event_group_poll_bit(EventGroup_t* Eg, enum eg_bit_e bit, enum eg_trait_e trait);

void event_group_clear(EventGroup_t* Eg);

#endif // EVENT_GROUP_H