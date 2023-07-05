/**
 * @file rtc_mcp7940n.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-04
 *
 * @copyright Copyright (c) 2023
 *
 */
/* C Library Includes */

/* STM32 Library Includes */

/* Personal Includes */
#include "rtc_mcp7940n.h"
#include "hardware_config.h"

/* Private Macros */
#define RTC_I2C_ADDRESS (0x6F << 1)
#define RTC_I2C_READ    (RTC_I2C_ADDRESS | 0x01)
#define RTC_I2C_WRITE   RTC_I2C_ADDRESS

#define RTC_SECONDS_REGISTER_TO_SECONDS(register) ((((register >> 4) & 0x07) * 10) + (register & 0x0F))
#define RTC_MINUTES_REGISTER_TO_MINUTES(register) ((((register >> 4) & 0x07) * 10) + (register & 0x0F))
#define RTC_HOURS_REGISTER_TO_HOURS(register)     ((((register >> 4) & 0x03) * 10) + (register & 0x0F))

#define RTC_ADDR_SECONDS           0x00
#define RTC_ADDR_MINUTES           0x01
#define RTC_ADDR_HOURS             0x02
#define RTC_ADDR_WEEK_DAYS         0x03
#define RTC_ADDR_DAYS              0x04
#define RTC_ADDR_MONTHS            0x05
#define RTC_ADDR_YEARS             0x06
#define RTC_ADDR_CONTROL           0x07
#define RTC_ADDR_OSC_TRIM          0x08
#define RTC_ADDR_ALARM_0_SECONDS   0x0A
#define RTC_ADDR_ALARM_0_MINUTES   0x0B
#define RTC_ADDR_ALARM_0_HOURS     0x0C
#define RTC_ADDR_ALARM_0_WEEK_DAYS 0x0D
#define RTC_ADDR_ALARM_0_DAYS      0x0E
#define RTC_ADDR_ALARM_0_MONTHS    0x0F
#define RTC_ADDR_ALARM_1_SECONDS   0x11
#define RTC_ADDR_ALARM_1_MINUTES   0x12
#define RTC_ADDR_ALARM_1_HOURS     0x13
#define RTC_ADDR_ALARM_1_WEEK_DAYS 0x14
#define RTC_ADDR_ALARM_1_DAYS      0x15
#define RTC_ADDR_ALARM_1_MONTHS    0x16

/* Function Prototypes */

uint8_t rtc_init(void) {

    // The start oscillator bit is in the seconds register. Reading the seconds register to keep
    // their values the same and then writing a 1 in the ST bit to enable the oscillator
    uint8_t secondAddr = RTC_ADDR_SECONDS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &secondAddr, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    uint8_t secondsRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &secondsRead, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    uint8_t secondsWrite[2] = {secondAddr, secondsRead | (0x01 << 7)};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, secondsWrite, 2, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    /* Set the RTC to 24 hr format */

    // The time format bit is ini the Hour register. Read the hour register then set the format
    // bit to 24hr mode so none of the hour values are changed
    uint8_t hoursAddr = RTC_ADDR_HOURS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &hoursAddr, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    uint8_t hoursRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &hoursRead, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    uint8_t hoursWrite[2] = {hoursAddr, hoursRead & 0x3F};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, hoursWrite, 2, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    /* Confirm the oscillator is running */
    // The oscillator running bit is in the weekday register
    uint8_t weekDaysAddr = RTC_ADDR_WEEK_DAYS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &weekDaysAddr, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    uint8_t weekDaysRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &weekDaysRead, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    // Check that the oscillator running bit is set
    if ((weekDaysRead & (0x01 << 5)) == 0) {
        return FALSE;
    }

    /* Set the alarm mode to trigger on the every register. Only need one alarm. Using alarm 0 */
    if (rtc_set_alarm_settings(RTC_ALARM_0, RTC_ALRM_MODE_ALL) != TRUE) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_set_alarm_settings(enum rtc_alarm_e alarmAddress, enum rtc_alarm_mode_e mode) {

    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &alarmAddress, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    static uint8_t alarmRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &alarmRead, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    // Also clearing the alarm interrupt flag here and setting the alarm interrupt output to logic high
    uint8_t alarmWrite[2] = {alarmAddress, (0x01 << 7) | (mode << 4) | (alarmRead & 0x07)};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, alarmWrite, 2, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_read_time(dt_time_t* Time) {

    /* Read the seconds from the RTC */
    uint8_t secondsAddr = RTC_ADDR_SECONDS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &secondsAddr, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    uint8_t secondsRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &secondsRead, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    Time->second = RTC_SECONDS_REGISTER_TO_SECONDS(secondsRead);

    /* Read the minutes from the RTC */
    uint8_t minutesAddr = RTC_ADDR_MINUTES;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &minutesAddr, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    uint8_t minutesRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &minutesRead, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    Time->minute = RTC_MINUTES_REGISTER_TO_MINUTES(minutesRead);

    /* Read the hours from the RTC */
    uint8_t hoursAddr = RTC_ADDR_HOURS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &hoursAddr, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    uint8_t hoursRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &hoursRead, 1, HAL_MAX_DELAY) != HAL_OK) {
        return FALSE;
    }

    Time->hour = RTC_HOURS_REGISTER_TO_HOURS(hoursRead);

    return TRUE;
}