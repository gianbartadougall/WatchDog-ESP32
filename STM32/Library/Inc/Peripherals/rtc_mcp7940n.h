/**
 * @file rtc_mcp7940n.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef RTC_MCP7940N
#define RTC_MCP7940N

/* C Library Includes */
#include <stdint.h>

/* STM32 Library Includes */

/* Personal Includes */
#include "datetime.h"

/* Public Structs and Enums */

// Do not change the values of these enums
// They map to bits for registers
enum rtc_alarm_e {
    RTC_ALARM_0 = 0x0D,
    RTC_ALARM_1 = 0x14,
};

// Do not change the values of these enums
// They map to bits for registers
enum rtc_alarm_mode_e {
    RTC_ALRM_MODE_SECONDS = 0x00,
    RTC_ALRM_MODE_MINUTES = 0x01,
    RTC_ALRM_MODE_HOURS   = 0x02,
    RTC_ALRM_MODE_ALL     = 0x07,
};

uint8_t rtc_init(void);

uint8_t rtc_set_alarm_settings(enum rtc_alarm_e alarmAddress, enum rtc_alarm_mode_e mode);

uint8_t rtc_read_time(dt_time_t* Time);

#endif // RTC_MCP7940N