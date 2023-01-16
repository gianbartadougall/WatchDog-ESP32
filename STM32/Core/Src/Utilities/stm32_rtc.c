/**
 * @file stm32_rtc.c
 * @author Giavanni Avila
 * @brief
 * @version 0.1
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
 *
 */

/* STM32 Includes */
#include "stm32l432xx.h"

/* Private Includes */
#include "stm32_rtc.h"
#include "hardware_config.h"
#include "log.h"

/* Private Macros */

void stm32_rtc_write_datetime(date_time_t* datetime) {

    // Most of the RTC registers are write protected. To unlock the registers
    // write 0xCA followed by 0x53 into the RTC_WPR
    STM32_RTC->WPR = 0xCA;
    STM32_RTC->WPR = 0x53;

    // Enter initialisation mode so the datetime of the RTC can be updated
    STM32_RTC->ISR |= RTC_ISR_INIT;

    // Wait for the INTIF bit to be set. This is done automatically by the
    // mcu and confirms the RTC is ready to be configured
    while ((STM32_RTC->ISR & RTC_ISR_INITF) == 0) {}

    // Wait for the INTIF bit to be set. This is done automatically by the
    // mcu and confirms the RTC is ready to be configured
    while ((STM32_RTC->ISR & RTC_ISR_INITF) == 0) {}

    // Set the date and time registers to 0
    STM32_RTC->TR &= ~(RTC_TR_HT | RTC_TR_HU | RTC_TR_MNT | RTC_TR_MNU | RTC_TR_ST | RTC_TR_SU);
    STM32_RTC->DR &= ~(RTC_DR_YT | RTC_DR_YU | RTC_DR_MT | RTC_DR_MU | RTC_DR_DT | RTC_DR_DU);

    // Update the year
    uint32_t yearTens = (datetime->year / 10);
    uint32_t yearOnes = (datetime->year % 10);
    // Update the month
    uint32_t monthTens = (datetime->month / 10);
    uint32_t monthOnes = (datetime->month % 10);

    // Update the day
    uint32_t dayTens = (datetime->day / 10);
    uint32_t dayOnes = (datetime->day % 10);

    // Don't fully understand why this is the case but you cannot do |= on the DR or the TR registers
    // of the RTC. You can only use = thus need precomute everything and set the register in one go
    STM32_RTC->DR = ((yearTens << RTC_DR_YT_Pos) | (yearOnes << RTC_DR_YU_Pos) | (monthTens << RTC_DR_MT_Pos) |
                     (monthOnes << RTC_DR_MU_Pos) | (dayTens << RTC_DR_DT_Pos) | (dayOnes << RTC_DR_DU_Pos));

    // Update the hour
    uint32_t hourTens = (datetime->hour / 10);
    uint32_t hourOnes = (datetime->hour % 10);

    // Update the minute
    uint32_t minuteTens = (datetime->minute / 10);
    uint32_t minuteOnes = (datetime->minute % 10);

    // Update the hour
    uint32_t secondTens = (datetime->second / 10);
    uint32_t secondOnes = (datetime->second % 10);

    STM32_RTC->TR = ((hourTens << RTC_TR_HT_Pos) | (hourOnes << RTC_TR_HU_Pos) | (minuteTens << RTC_TR_MNT_Pos) |
                     (minuteOnes << RTC_TR_MNU_Pos) | (secondTens << RTC_TR_ST_Pos) | (secondOnes << RTC_TR_SU_Pos));

    STM32_RTC->ISR &= ~(RTC_ISR_INIT);

    while ((STM32_RTC->ISR & RTC_ISR_RSF) == 0) {}

    // char msg[50];
    // sprintf(msg, "TR: %lu\r\nDR: %lu\r\n%i %i %i %i %i %i", STM32_RTC->TR, STM32_RTC->DR, datetime->second,
    //         datetime->minute, datetime->hour, datetime->day, datetime->month, datetime->year);
    // log_message(msg);
}

void stm32_rtc_read_datetime(date_time_t* datetime) {

    // Only the tens and the ones place of the year is stored in the year register.
    // These are stored in BCD format thus 2023 would read 23 for example
    uint8_t yearTens = (STM32_RTC->DR & RTC_DR_YT) >> RTC_DR_YT_Pos;
    uint8_t yearOnes = (STM32_RTC->DR & RTC_DR_YU) >> RTC_DR_YU_Pos;
    datetime->year   = (yearTens * 10) + yearOnes;

    uint8_t MonthTens = (STM32_RTC->DR & RTC_DR_MT) >> RTC_DR_MT_Pos;
    uint8_t MonthOnes = (STM32_RTC->DR & RTC_DR_MU) >> RTC_DR_MU_Pos;
    datetime->month   = (MonthTens * 10) + MonthOnes;

    uint8_t dayTens = (STM32_RTC->DR & RTC_DR_DT) >> RTC_DR_DT_Pos;
    uint8_t dayOnes = (STM32_RTC->DR & RTC_DR_DU) >> RTC_DR_DU_Pos;
    datetime->day   = (dayTens * 10) + dayOnes;

    uint8_t hourTens = (STM32_RTC->TR & RTC_TR_HT) >> RTC_TR_HT_Pos;
    uint8_t hourOnes = (STM32_RTC->TR & RTC_TR_HU) >> RTC_TR_HU_Pos;
    datetime->hour   = (hourTens * 10) + hourOnes;

    uint8_t minuteTens = ((STM32_RTC->TR & RTC_TR_MNT) >> RTC_TR_MNT_Pos);
    uint8_t minuteOnes = ((STM32_RTC->TR & RTC_TR_MNU) >> RTC_TR_MNU_Pos);
    datetime->minute   = (minuteTens * 10) + minuteOnes;

    uint8_t secondTens = (STM32_RTC->TR & RTC_TR_ST) >> RTC_TR_ST_Pos;
    uint8_t secondOnes = (STM32_RTC->TR & RTC_TR_SU) >> RTC_TR_SU_Pos;
    datetime->second   = (secondTens * 10) + secondOnes;
}

void stm32_rtc_print_datetime(date_time_t* datetime) {

    char dt[100];
    sprintf(dt, "%i:%i:%i %i/%i/%i\r\n", datetime->second, datetime->minute, datetime->hour, datetime->day,
            datetime->month, datetime->year);
    log_prints(dt);
}