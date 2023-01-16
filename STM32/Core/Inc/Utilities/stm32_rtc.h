/**
 * @file stm32_rtc.h
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef STM32_RTC_H
#define STM32_RTC_H

/* Public Includes */
#include <stdint.h>

typedef struct date_time_t {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} date_time_t;

void stm32_rtc_init(void);

void stm32_rtc_write_datetime(date_time_t* datetime);

void stm32_rtc_read_datetime(date_time_t* datetime);

void stm32_rtc_print_datetime(date_time_t* datetime);

#endif // STM32_RTC_H