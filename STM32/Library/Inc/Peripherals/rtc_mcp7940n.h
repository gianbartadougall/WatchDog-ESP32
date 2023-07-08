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

uint8_t rtc_init(void);

uint8_t rtc_read_time(dt_time_t* Time);

uint8_t rtc_read_datetime(dt_datetime_t* Datetime);

uint8_t rtc_read_date(dt_date_t* Date);

uint8_t rtc_read_time(dt_time_t* Time);

uint8_t rtc_write_datetime(dt_datetime_t* Datetime);

uint8_t rtc_write_date(dt_date_t* Date);

uint8_t rtc_write_time(dt_time_t* Time);

uint8_t rtc_set_alarm(dt_datetime_t* Datetime);

uint8_t rtc_enable_alarm(void);

uint8_t rtc_disable_alarm(void);

uint8_t rtc_read_alarm_datetime(dt_datetime_t* Datetime);

uint8_t rtc_read_alarm_time(dt_time_t* Time);

uint8_t rtc_read_alarm_date(dt_date_t* Date);

uint8_t rtc_alarm_clear(void);

#endif // RTC_MCP7940N