/**
 * @file watchdog.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-2
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

/* C Library Includes */

/* STM32 Library Includes */

/* Personal Includes */
#include "event_group.h"

/* Public Variables */
extern EventGroup_t gbl_EventsStm, gbl_EventsEsp;

typedef enum esp_event_e {
    EVENT_ESP_TAKE_PHOTO = EGB_0,
    EVENT_ESP_ON         = EGB_1,
    EVENT_ESP_OFF        = EGB_2,
    EVENT_ESP_READY      = EGB_3,
};

typedef enum stm_event_e {
    EVENT_STM_LED_GREEN_ON        = EGB_0,
    EVENT_STM_LED_GREEN_OFF       = EGB_1,
    EVENT_STM_LED_RED_ON          = EGB_2,
    EVENT_STM_LED_RED_OFF         = EGB_3,
    EVENT_STM_SYSTEM_INITIALISING = EGB_4,
    EVENT_STM_SLEEP               = EGB_5,
    EVENT_STM_USBC_CONNECTED      = EGB_6,
};

void wd_start(void);

#endif // WATCHDOG_H