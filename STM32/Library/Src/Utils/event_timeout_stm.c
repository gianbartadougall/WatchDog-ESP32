/* C Library Includes */

/* STM32 Includes */
#include "stm32l4xx_hal.h"

/* Personal Includes */
#include "event_timeout_stm.h"
#include "utils.h"

#define MAX_TIMER_COUNT 4294967295

void et_set_timeout(et_timeout_t* Et, enum et_timeout_e id, uint32_t delay) {
    uint32_t tick                = HAL_GetTick();
    uint32_t countsUntilOverflow = MAX_TIMER_COUNT - tick;
    if (countsUntilOverflow < delay) {
        Et->timeouts[id] = delay - countsUntilOverflow;
    }
    Et->timeouts[id] = tick + delay;
}

void et_clear_timeout(et_timeout_t* Et, enum et_timeout_e id) {
    Et->timeouts[id] = 0;
}

void et_clear_all_timeouts(et_timeout_t* Et) {
    for (uint8_t i = 0; i < ET_NUM_TIMEOUTS; i++) {
        Et->timeouts[i] = 0;
    }
}

uint8_t et_timeout_has_occured(et_timeout_t* Et, enum et_timeout_e id) {

    if (Et->timeouts[id] == 0) {
        return FALSE;
    }

    return HAL_GetTick() >= Et->timeouts[id] ? TRUE : FALSE;
}

uint32_t et_poll_timeout(et_timeout_t* Et, enum et_timeout_e id) {
    return Et->timeouts[id];
}