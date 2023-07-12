#ifndef TIMEOUT_H
#define TIMEOUT_H

#ifdef __cplusplus
extern "C" {
#endif

/* C Library Includes */
#include <stdint.h>
#include <time.h>

/* Personal Includes */

/* Private Macros */
#define ET_NUM_TIMEOUTS 32

enum et_timeout_e {
    ET_TIMEOUT_ID_0,
    ET_TIMEOUT_ID_1,
    ET_TIMEOUT_ID_2,
    ET_TIMEOUT_ID_3,
    ET_TIMEOUT_ID_4,
    ET_TIMEOUT_ID_5,
    ET_TIMEOUT_ID_6,
    ET_TIMEOUT_ID_7,
    ET_TIMEOUT_ID_8,
    ET_TIMEOUT_ID_9,
    ET_TIMEOUT_ID_10,
    ET_TIMEOUT_ID_11,
    ET_TIMEOUT_ID_12,
    ET_TIMEOUT_ID_13,
    ET_TIMEOUT_ID_14,
    ET_TIMEOUT_ID_15,
    ET_TIMEOUT_ID_16,
    ET_TIMEOUT_ID_17,
    ET_TIMEOUT_ID_18,
    ET_TIMEOUT_ID_19,
    ET_TIMEOUT_ID_20,
    ET_TIMEOUT_ID_21,
    ET_TIMEOUT_ID_22,
    ET_TIMEOUT_ID_23,
    ET_TIMEOUT_ID_24,
    ET_TIMEOUT_ID_25,
    ET_TIMEOUT_ID_26,
    ET_TIMEOUT_ID_27,
    ET_TIMEOUT_ID_28,
    ET_TIMEOUT_ID_29,
    ET_TIMEOUT_ID_30,
    ET_TIMEOUT_ID_31,
};

typedef struct et_timeout_t {
    uint32_t timeouts[ET_NUM_TIMEOUTS];
} et_timeout_t;

void et_set_timeout(et_timeout_t* Et, enum et_timeout_e id, uint32_t delay);

void et_clear_timeout(et_timeout_t* Et, enum et_timeout_e id);

void et_clear_all_timeouts(et_timeout_t* Et);

uint8_t et_timeout_has_occured(et_timeout_t* Et, enum et_timeout_e id);

uint32_t et_poll_timeout(et_timeout_t* Et, enum et_timeout_e id);

#ifdef __cplusplus
}
#endif

#endif // TIMEOUT_H