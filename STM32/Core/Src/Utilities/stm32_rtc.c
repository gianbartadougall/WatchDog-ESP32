/**
 * @file stm32_rtc.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @Date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <stdio.h>

/* Personal Includes */
#include "stm32_rtc.h"
#include "log.h"

/* Private Macros */
#define STM32_RTC_KEY_1 0xCA
#define STM32_RTC_KEY_2 0x53

/* Private Function Prototypes */
void stm32_rtc_unlock_registers(void);

void stm32_rtc_init(void) {

    // Enable DPB bit so the RCC->BDCR register can be modified
    PWR->CR1 |= PWR_CR1_DBP;

    RCC->BDCR |= RCC_BDCR_RTCEN;    // Enable the RTC
    RCC->BDCR |= RCC_BDCR_RTCSEL_0; // Choose the LSE oscillator clock for the RTC
    RCC->BDCR |= RCC_BDCR_LSEON;    // Turn the LSE oscillator on

    // Wait for the LSE to stabalise
    while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {}

    // Unlock the RTC registers
    stm32_rtc_unlock_registers();

    // Enter initialisation mode so the Datetime of the RTC can be updated
    STM32_RTC->ISR |= RTC_ISR_INIT;

    // Wait for the INTIF bit to be set. This is done automatically by the
    // mcu and confirms the RTC is ready to be configured
    while ((STM32_RTC->ISR & RTC_ISR_INITF) == 0) {}

    /****** START CODE BLOCK ******/
    // Description: Configure the Alarms for the RTC
    STM32_RTC->CR |= RTC_CR_ALRAE;  // Enable alarm A
    STM32_RTC->CR |= RTC_CR_ALRAIE; // Enable the interrupt for alarm A

    /****** END CODE BLOCK ******/

    // Generate 1Hz clock for the calendar counter. Assume freq = 32.768Khz
    RTC->PRER = 32768 - 1;

    // Set the Time format to 24 hour Time
    STM32_RTC->CR &= ~(RTC_CR_FMT);

    // Set the RTC Time to 0
    STM32_RTC->TR &= ~(RTC_TR_HT | RTC_TR_HU | RTC_TR_MNT | RTC_TR_MNU | RTC_TR_ST | RTC_TR_SU);

    // Set the RTC Date to 0
    STM32_RTC->DR &= ~(RTC_DR_YT | RTC_DR_YU | RTC_DR_MT | RTC_DR_MU | RTC_DR_DT | RTC_DR_DU);

    // Clear the initialisation to enable the RTC
    STM32_RTC->ISR &= ~(RTC_ISR_INIT);

    while ((STM32_RTC->ISR & RTC_ISR_INITF) != 0) {}

    // Confirm RSF flag is set
    while ((STM32_RTC->ISR & RTC_ISR_RSF) == 0) {}

    /* Configure the Interrupt of the RTC */

    // The RTC has two alarms which are both connected to EXTI line 18
    EXTI->RTSR1 |= (0x01 << 18); // Configure EXTI 18 to trigger on rising edge
    EXTI->IMR1 |= (0x01 << 18);  // Enable the interrupts

    // Configure and enable the RTC Alarm channel in the NVIC
    HAL_NVIC_SetPriority(STM32_RTC_ALARM_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(STM32_RTC_ALARM_IRQn);
}

void stm32_rtc_set_alarmA(dt_datetime_t* Datetime) {

    // Unlock the RTC registers
    stm32_rtc_unlock_registers();

    // Disable the alarm so that it can be updated
    STM32_RTC->CR &= ~(RTC_CR_ALRAE);

    STM32_RTC->ALRMAR &= ~(0x01 << 31); // Date must match for alarm to trigger
    STM32_RTC->ALRMAR &= ~(0x01 << 23); // Hour must match for alarm to trigger
    STM32_RTC->ALRMAR &= ~(0x01 << 15); // Minute must match for alarm to trigger
    STM32_RTC->ALRMAR &= ~(0x01 << 7);  // Second must match for alarm to trigger

    // Set Date to represent calendar day (i.e The 15th of every month) instead of week day (i.e Monday of everyweek)
    STM32_RTC->ALRMAR &= ~(0x01 << 30);

    /* Set the Date the calendar day the alarm should trigger on */

    // Calculate the day
    uint32_t dayTens = (Datetime->Date.day / 10);
    uint32_t dayOnes = (Datetime->Date.day % 10);

    // Calculate the Hour
    uint32_t hourTens = (Datetime->Time.hour / 10);
    uint32_t hourOnes = (Datetime->Time.hour % 10);

    // Calculate the Minute
    uint32_t minuteTens = (Datetime->Time.minute / 10);
    uint32_t minuteOnes = (Datetime->Time.minute % 10);

    // Calculate the Second
    uint32_t secondTens = (Datetime->Time.second / 10);
    uint32_t secondOnes = (Datetime->Time.second % 10);

    STM32_RTC->ALRMAR = (dayTens << 28) | (dayOnes << 24) | (hourTens << 20) | (hourOnes << 16) | (minuteTens << 12) |
                        (minuteOnes << 8) | (secondTens << 4) | (secondOnes);

    // Renable the alarm
    STM32_RTC->CR |= (RTC_CR_ALRAE);
}

void stm32_rtc_read_alarmA(dt_datetime_t* Datetime) {

    uint8_t dayTens = (STM32_RTC->ALRMAR & (0x03 << 28)) >> 28;
    uint8_t dayOnes = (STM32_RTC->ALRMAR & (0x0F << 24)) >> 24;

    uint8_t hourTens = (STM32_RTC->ALRMAR & (0x03 << 20)) >> 20;
    uint8_t hourOnes = (STM32_RTC->ALRMAR & (0x0F << 16)) >> 16;

    uint8_t minuteTens = (STM32_RTC->ALRMAR & (0x07 << 12)) >> 12;
    uint8_t minuteOnes = (STM32_RTC->ALRMAR & (0x0F << 8)) >> 8;

    uint8_t secondTens = (STM32_RTC->ALRMAR & (0x07 << 4)) >> 4;
    uint8_t secondOnes = (STM32_RTC->ALRMAR & 0x0F);

    Datetime->Date.day    = (dayTens * 10) + dayOnes;
    Datetime->Time.hour   = (hourTens * 10) + hourOnes;
    Datetime->Time.minute = (minuteTens * 10) + minuteOnes;
    Datetime->Time.second = (secondTens * 10) + secondOnes;
}

void stm32_rtc_set_alarmB(dt_datetime_t* Datetime) {

    STM32_RTC->ALRMBR &= ~(0x01 << 31); // Date must match for alarm to trigger
    STM32_RTC->ALRMBR &= ~(0x01 << 23); // Hour must match for alarm to trigger
    STM32_RTC->ALRMBR &= ~(0x01 << 15); // Minute must match for alarm to trigger
    STM32_RTC->ALRMBR &= ~(0x01 << 7);  // Second must match for alarm to trigger

    // Set Date to represent calendar day (i.e The 15th of every month) instead of week day (i.e Monday of every week)
    STM32_RTC->ALRMBR &= ~(0x01 << 30);

    /* Set the Date the calendar day the alarm should trigger on */

    // Calculate the day
    uint32_t dayTens = (Datetime->Date.day / 10);
    uint32_t dayOnes = (Datetime->Date.day % 10);

    // Calculate the Hour
    uint32_t hourTens = (Datetime->Date.day / 10);
    uint32_t hourOnes = (Datetime->Date.day % 10);

    // Calculate the Minute
    uint32_t minuteTens = (Datetime->Date.day / 10);
    uint32_t minuteOnes = (Datetime->Date.day % 10);

    // Calculate the Second
    uint32_t secondTens = (Datetime->Date.day / 10);
    uint32_t secondOnes = (Datetime->Date.day % 10);

    STM32_RTC->ALRMBR = (dayTens << 28) | (dayOnes << 24) | (hourTens << 20) | (hourOnes << 16) | (minuteTens << 12) |
                        (minuteOnes << 8) | (secondTens << 4) | (secondOnes);
}

void stm32_rtc_write_datetime(dt_datetime_t* Datetime) {

    uint16_t originalYear = Datetime->Date.year;
    if (Datetime->Date.year > 2000) {
        Datetime->Date.year -= 2000;
    }

    // Unlock the RTC registers
    stm32_rtc_unlock_registers();

    // Enter initialisation mode so the Datetime of the RTC can be updated
    STM32_RTC->ISR |= RTC_ISR_INIT;

    // Wait for the INTIF bit to be set. This is done automatically by the
    // mcu and confirms the RTC is ready to be configured
    while ((STM32_RTC->ISR & RTC_ISR_INITF) == 0) {}

    // Wait for the INTIF bit to be set. This is done automatically by the
    // mcu and confirms the RTC is ready to be configured
    while ((STM32_RTC->ISR & RTC_ISR_INITF) == 0) {}

    // Set the Date and Time registers to 0
    STM32_RTC->TR &= ~(RTC_TR_HT | RTC_TR_HU | RTC_TR_MNT | RTC_TR_MNU | RTC_TR_ST | RTC_TR_SU);
    STM32_RTC->DR &= ~(RTC_DR_YT | RTC_DR_YU | RTC_DR_MT | RTC_DR_MU | RTC_DR_DT | RTC_DR_DU);

    // Update the year
    uint32_t yearTens = (Datetime->Date.year / 10);
    uint32_t yearOnes = (Datetime->Date.year % 10);

    // Update the month
    uint32_t monthTens = (Datetime->Date.month / 10);
    uint32_t monthOnes = (Datetime->Date.month % 10);

    // Update the day
    uint32_t dayTens = (Datetime->Date.day / 10);
    uint32_t dayOnes = (Datetime->Date.day % 10);

    // Don't fully understand why this is the case but you cannot do |= on the DR or the TR registers
    // of the RTC. You can only use = thus need precomute everything and set the register in one go
    STM32_RTC->DR = ((yearTens << RTC_DR_YT_Pos) | (yearOnes << RTC_DR_YU_Pos) | (monthTens << RTC_DR_MT_Pos) |
                     (monthOnes << RTC_DR_MU_Pos) | (dayTens << RTC_DR_DT_Pos) | (dayOnes << RTC_DR_DU_Pos));

    // Update the hour
    uint32_t hourTens = (Datetime->Time.hour / 10);
    uint32_t hourOnes = (Datetime->Time.hour % 10);

    // Update the minute
    uint32_t minuteTens = (Datetime->Time.minute / 10);
    uint32_t minuteOnes = (Datetime->Time.minute % 10);

    // Update the hour
    uint32_t secondTens = (Datetime->Time.second / 10);
    uint32_t secondOnes = (Datetime->Time.second % 10);

    STM32_RTC->TR = ((hourTens << RTC_TR_HT_Pos) | (hourOnes << RTC_TR_HU_Pos) | (minuteTens << RTC_TR_MNT_Pos) |
                     (minuteOnes << RTC_TR_MNU_Pos) | (secondTens << RTC_TR_ST_Pos) | (secondOnes << RTC_TR_SU_Pos));

    STM32_RTC->ISR &= ~(RTC_ISR_INIT);

    while ((STM32_RTC->ISR & RTC_ISR_RSF) == 0) {}

    // Set the Datetime back to the original year
    Datetime->Date.year = originalYear;
}

void stm32_rtc_read_datetime(dt_datetime_t* Datetime) {

    // Only the tens and the ones place of the year is stored in the year register.
    // These are stored in BCD format thus 2023 would read 23 for example
    uint8_t yearTens    = (STM32_RTC->DR & RTC_DR_YT) >> RTC_DR_YT_Pos;
    uint8_t yearOnes    = (STM32_RTC->DR & RTC_DR_YU) >> RTC_DR_YU_Pos;
    Datetime->Date.year = (yearTens * 10) + yearOnes;

    uint8_t MonthTens    = (STM32_RTC->DR & RTC_DR_MT) >> RTC_DR_MT_Pos;
    uint8_t MonthOnes    = (STM32_RTC->DR & RTC_DR_MU) >> RTC_DR_MU_Pos;
    Datetime->Date.month = (MonthTens * 10) + MonthOnes;

    uint8_t dayTens    = (STM32_RTC->DR & RTC_DR_DT) >> RTC_DR_DT_Pos;
    uint8_t dayOnes    = (STM32_RTC->DR & RTC_DR_DU) >> RTC_DR_DU_Pos;
    Datetime->Date.day = (dayTens * 10) + dayOnes;

    uint8_t hourTens    = (STM32_RTC->TR & RTC_TR_HT) >> RTC_TR_HT_Pos;
    uint8_t hourOnes    = (STM32_RTC->TR & RTC_TR_HU) >> RTC_TR_HU_Pos;
    Datetime->Time.hour = (hourTens * 10) + hourOnes;

    uint8_t minuteTens    = ((STM32_RTC->TR & RTC_TR_MNT) >> RTC_TR_MNT_Pos);
    uint8_t minuteOnes    = ((STM32_RTC->TR & RTC_TR_MNU) >> RTC_TR_MNU_Pos);
    Datetime->Time.minute = (minuteTens * 10) + minuteOnes;

    uint8_t secondTens    = (STM32_RTC->TR & RTC_TR_ST) >> RTC_TR_ST_Pos;
    uint8_t secondOnes    = (STM32_RTC->TR & RTC_TR_SU) >> RTC_TR_SU_Pos;
    Datetime->Time.second = (secondTens * 10) + secondOnes;

    // The year of the date include just that tens digits so adding the extra 2000
    Datetime->Date.year += 2000;
}

void stm32_rtc_format_datetime(dt_datetime_t* Datetime, char* string) {
    sprintf(string, "%i:%i:%i %i/%i/%i\r\n", Datetime->Time.second, Datetime->Time.minute, Datetime->Time.hour,
            Datetime->Date.day, Datetime->Date.month, Datetime->Date.year);
}

void stm32_rtc_unlock_registers(void) {

    // To unlock the registers write 0xCA followed by 0x53 into the RTC_WPR
    STM32_RTC->WPR = 0xCA;
    STM32_RTC->WPR = 0x53;
}