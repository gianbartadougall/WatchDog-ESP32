#ifndef EVENT_GROUP_H
#define EVENT_GROUP_H

/* C Library Includes */
#include <stdint.h>

/* Public Macros */
#define EG_NUM_TRAITS 2

typedef enum eg_trait_e {
    EGT_ACTIVE = 0,
    EGT_STM_REQUEST,
} eg_trait_e;

typedef enum eg_bit_e {
    EGB_0  = (0x01 << 0),
    EGB_1  = (0x01 << 1),
    EGB_2  = (0x01 << 2),
    EGB_3  = (0x01 << 3),
    EGB_4  = (0x01 << 4),
    EGB_5  = (0x01 << 5),
    EGB_6  = (0x01 << 6),
    EGB_7  = (0x01 << 7),
    EGB_8  = (0x01 << 8),
    EGB_9  = (0x01 << 9),
    EGB_10 = (0x01 << 10),
    EGB_11 = (0x01 << 11),
    EGB_12 = (0x01 << 12),
    EGB_13 = (0x01 << 13),
    EGB_14 = (0x01 << 14),
    EGB_15 = (0x01 << 15),
    EGB_16 = (0x01 << 16),
    EGB_17 = (0x01 << 17),
    EGB_18 = (0x01 << 18),
    EGB_19 = (0x01 << 19),
    EGB_20 = (0x01 << 20),
    EGB_21 = (0x01 << 21),
    EGB_22 = (0x01 << 22),
    EGB_23 = (0x01 << 23),
    EGB_24 = (0x01 << 24),
    EGB_25 = (0x01 << 25),
    EGB_26 = (0x01 << 26),
    EGB_27 = (0x01 << 27),
    EGB_28 = (0x01 << 28),
    EGB_29 = (0x01 << 29),
    EGB_30 = (0x01 << 30),
    EGB_31 = (0x01 << 31)
} eg_bit_e;

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

uint32_t event_group_get_bits(EventGroup_t* Eg, enum eg_trait_e trait);

#endif // EVENT_GROUP_H