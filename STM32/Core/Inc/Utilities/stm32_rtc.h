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

/* C Library Includes */
#include <stdint.h>

/* Personal Includes */
#include "datetime.h"

void stm32_rtc_write_datetime(dt_datetime_t* datetime);

void stm32_rtc_read_datetime(dt_datetime_t* datetime);

void stm32_rtc_print_datetime(dt_datetime_t* datetime);

#endif // STM32_RTC_H