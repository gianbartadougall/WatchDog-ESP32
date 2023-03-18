/**
 * @file stm32_rtc.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef STM32_RTC_H
#define STM32_RTC_H

/* C Library Includes */

/* Personal Includes */
#include "datetime.h"

/* STM32 Includes */
#include "stm32l432xx.h"
#include "stm32l4xx_hal.h"

#define STM32_RTC                 RTC
#define STM32_RTC_WKUP_IRQn       RTC_WKUP_IRQn
#define STM32_RTC_ALARM_IRQn      RTC_Alarm_IRQn
#define STM32_RTC_TIMER_FREQUENCY 1 // 1 Hz frequency for the RTC
#define STM32_RTC_CLK_EN()        __HAL_RCC_TSC_CLK_ENABLE()

void stm32_rtc_init(void);

void stm32_rtc_read_datetime(dt_datetime_t* datetime);

void stm32_rtc_write_datetime(dt_datetime_t* datetime);

void stm32_rtc_format_datetime(dt_datetime_t* datetime, char* string);

void stm32_rtc_set_alarmA(dt_datetime_t* datetime);

void stm32_rtc_read_alarmA(dt_datetime_t* datetime);

void stm32_rtc_set_alarmB(dt_datetime_t* datetime);

#endif // STM32_RTC_H