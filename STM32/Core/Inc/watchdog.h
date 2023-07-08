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
#include "datetime.h"
#include "watchdog_utils.h"

enum esp_event_e {
    EVENT_ESP_TAKE_PHOTO = EGB_0,
    EVENT_ESP_ON         = EGB_1,
    EVENT_ESP_OFF        = EGB_2,
    EVENT_ESP_READY      = EGB_3,
};

enum stm_event_e {
    EVENT_STM_LED_GREEN_ON        = EGB_0,
    EVENT_STM_LED_GREEN_OFF       = EGB_1,
    EVENT_STM_LED_RED_ON          = EGB_2,
    EVENT_STM_LED_RED_OFF         = EGB_3,
    EVENT_STM_SYSTEM_INITIALISING = EGB_4,
    EVENT_STM_SLEEP               = EGB_5,
    EVENT_STM_USBC_CONNECTED      = EGB_6,
    EVENT_STM_RTC_UPDATE_ALARM    = EGB_7,
};

enum error_code_e {
    EC_READ_WATCHDOG_SETTINGS,
    EC_READ_CAMERA_SETTINGS,
    EC_REQUEST_PHOTO_CAPTURE,
    EC_BPACKET_CREATE,
    EC_UNKNOWN_BPACKET_REQUEST,
    EC_DATETIME_CREATION,
    EC_RTC_SET_ALARM,
    EC_RTC_READ_DATETIME,
    EC_RTC_READ_ALARM,
    EC_RTC_INIT,
    EC_FLASH_UNLOCK,
    EC_FLASH_WRITE,
    EC_FLASH_LOCK,
};

// typedef struct capture_time_t {
//     dt_datetime_t Start;
//     dt_datetime_t End;
//     uint16_t intervalSecond;
//     uint16_t intervalMinute;
//     uint16_t intervalHour;
//     uint16_t intervalDay;
// } capture_time_t;

/* Public Variables */
extern event_group_t gbl_EventsStm, gbl_EventsEsp;

extern capture_time_t gbl_CaptureTime;

void wd_start(void);

void wd_error_handler(char* fileName, uint16_t lineNumber, enum error_code_e errorCode);

#endif // WATCHDOG_H