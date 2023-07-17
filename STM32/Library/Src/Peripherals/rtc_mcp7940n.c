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
#include "utils.h"

/* Private Macros */
#define RTC_I2C_ADDRESS (0x6F << 1)
#define RTC_I2C_READ    (RTC_I2C_ADDRESS | 0x01)
#define RTC_I2C_WRITE   RTC_I2C_ADDRESS

#define RTC_REGISTER_TO_SECONDS(register) ((((register >> 4) & 0x07) * 10) + (register & 0x0F))
#define RTC_REGISTER_TO_MINUTES(register) ((((register >> 4) & 0x07) * 10) + (register & 0x0F))
#define RTC_REGISTER_TO_HOURS(register)   ((((register >> 4) & 0x03) * 10) + (register & 0x0F))

#define RTC_REGISTER_TO_DAYS(register)   ((((register >> 4) & 0x03) * 10) + (register & 0x0F))
#define RTC_REGISTER_TO_MONTHS(register) ((((register >> 4) & 0x01) * 10) + (register & 0x0F))
#define RTC_REGISTER_TO_YEARS(register)  (2000 + (((register >> 4) * 10) + (register & 0x0F)))

#define RTC_SECONDS_TO_REGISTER(seconds) (((seconds / 10) << 4) | (seconds % 10))
#define RTC_MINUTES_TO_REGISTER(minutes) (((minutes / 10) << 4) | (minutes % 10))
#define RTC_HOURS_TO_REGISTER(hours)     (((hours / 10) << 4) | (hours % 10))

#define RTC_DAYS_TO_REGISTER(days)     (((days / 10) << 4) | (days % 10))
#define RTC_MONTHS_TO_REGISTER(months) (((months / 10) << 4) | (months % 10))
#define RTC_YEARS_TO_REGISTER(years)   ((((years % 2000) / 10) << 4) | ((years % 2000) % 10))

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

#define ALARM_SETTINGS 0x70

/* Function Prototypes */
uint8_t rtc_set_alarm_settings(void);
uint8_t rtc_enable_ext_oscillator(void);
uint8_t rtc_disable_ext_oscillator(void);

uint8_t rtc_init(void) {

    // The external oscillator needs to be enabled for the rtc to run
    if (rtc_enable_ext_oscillator() != TRUE) {
        return EXIT_CODE_2;
    }

    /* Set the RTC to 24 hr format */

    // The time format bit is ini the Hour register. Read the hour register then set the format
    // bit to 24hr mode so none of the hour values are changed
    uint8_t hoursAddr = RTC_ADDR_HOURS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &hoursAddr, 1, 3000) != HAL_OK) {
        return EXIT_CODE_5;
    }

    uint8_t hoursRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &hoursRead, 1, 3000) != HAL_OK) {
        return EXIT_CODE_6;
    }

    uint8_t hoursWrite[2] = {hoursAddr, hoursRead & 0x3F};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, hoursWrite, 2, 3000) != HAL_OK) {
        return EXIT_CODE_7;
    }

    /* Set the alarm mode to trigger on the every register. Only need one alarm. Using alarm 0 */
    if (rtc_set_alarm_settings() != TRUE) {
        return EXIT_CODE_3;
    }

    return TRUE;
}

uint8_t rtc_set_alarm_settings(void) {

    // Only intend to use one alarm so this is why its hardcoded to alarm 0. The settings for
    // alarm 0 are in the week days register
    uint8_t alarmAddress = RTC_ADDR_ALARM_0_WEEK_DAYS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &alarmAddress, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    static uint8_t alarmRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &alarmRead, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    // Also clearing the alarm interrupt flag here and setting the alarm interrupt output to logic high.
    // Only planning on using the alarm in mode where it checks everything so thats why its hardcoded as such
    uint8_t alarmWrite[2] = {alarmAddress, ALARM_SETTINGS | (alarmRead & 0x07)};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, alarmWrite, 2, 3000) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_read_alarm_settings(uint8_t* alarmSettings) {

    // Only intend to use one alarm so this is why its hardcoded to alarm 0. The settings for
    // alarm 0 are in the week days register
    uint8_t alarmAddress = RTC_ADDR_ALARM_0_WEEK_DAYS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &alarmAddress, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, alarmSettings, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_clear_alarm(void) {
    return rtc_set_alarm_settings();
}

uint8_t rtc_set_alarm(dt_datetime_t* Datetime) {

    // Clear the interrupt. Because the settings are hardcoded for this, I can just reset
    // the settings because I also clear the interrupt in this function when I set the settings
    rtc_clear_alarm();

    // Create the buffer to write to the alarm
    uint8_t buffer[7] = {
        RTC_ADDR_ALARM_0_SECONDS,
        RTC_SECONDS_TO_REGISTER(Datetime->Time.second),
        RTC_MINUTES_TO_REGISTER(Datetime->Time.minute),
        RTC_HOURS_TO_REGISTER(Datetime->Time.hour),
        ALARM_SETTINGS | dt_calculate_day_of_week(&Datetime->Date),
        RTC_DAYS_TO_REGISTER(Datetime->Date.day),
        RTC_MONTHS_TO_REGISTER(Datetime->Date.month),
    };

    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, buffer, 7, 3000) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_enable_alarm(void) {

    // Read the control register so none of the other values are changed
    uint8_t controlAddr = RTC_ADDR_CONTROL;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &controlAddr, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    static uint8_t controlRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &controlRead, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t controlWrite[2] = {controlAddr, controlRead | (0x01 << 4)};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, controlWrite, 2, 3000) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_disable_alarm(void) {

    // Read the control register so none of the other values are changed
    uint8_t controlAddr = RTC_ADDR_CONTROL;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &controlAddr, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    static uint8_t controlRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &controlRead, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t controlWrite[2] = {controlAddr, controlRead & ~(0x01 << 4)};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, controlWrite, 2, 3000) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_alarm_clear(void) {

    // Read the control register so none of the other values are changed
    uint8_t weekDaysAddr = RTC_ADDR_ALARM_0_WEEK_DAYS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &weekDaysAddr, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t weekDaysRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &weekDaysRead, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t weekDaysWrite[2] = {weekDaysAddr, weekDaysRead & ~(0x01 << 3)};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, weekDaysWrite, 2, 3000) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_read_alarm_datetime(dt_datetime_t* Datetime, uint8_t alarmNum) {

    if (rtc_read_alarm_time(&Datetime->Time, alarmNum) != TRUE) {
        return FALSE;
    }

    if (rtc_read_alarm_date(&Datetime->Date, alarmNum) != TRUE) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_read_alarm_time(dt_time_t* Time, uint8_t alarmNum) {

    uint8_t startAddress = alarmNum == 0 ? RTC_ADDR_ALARM_0_SECONDS : RTC_ADDR_ALARM_1_SECONDS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &startAddress, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t buffer[3] = {0};
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, buffer, 3, 3000) != HAL_OK) {
        return FALSE;
    }

    Time->second = RTC_REGISTER_TO_SECONDS(buffer[0]);
    Time->minute = RTC_REGISTER_TO_MINUTES(buffer[1]);
    Time->hour   = RTC_REGISTER_TO_HOURS(buffer[2]);

    return TRUE;
}

uint8_t rtc_read_alarm_date(dt_date_t* Date, uint8_t alarmNum) {

    uint8_t startAddress = alarmNum == 0 ? RTC_ADDR_ALARM_0_DAYS : RTC_ADDR_ALARM_1_DAYS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &startAddress, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t buffer[2] = {0};
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, buffer, 3, 3000) != HAL_OK) {
        return FALSE;
    }

    /* The alarm does not check the year. Set the year to be the current year on the RTC */
    uint8_t yearsAddr = RTC_ADDR_YEARS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &yearsAddr, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t yearsRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &yearsRead, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    Date->day   = RTC_REGISTER_TO_DAYS(buffer[0]);
    Date->month = RTC_REGISTER_TO_MONTHS(buffer[1]);
    Date->year  = RTC_REGISTER_TO_YEARS(yearsRead);

    return TRUE;
}

uint8_t rtc_write_datetime(dt_datetime_t* Datetime) {

    /* Disable the oscillator (datasheet reccommends this to avoid rollover issues ) */
    // if (rtc_disable_ext_oscillator() != TRUE) {
    //     return FALSE;
    // }

    if (rtc_write_time(&Datetime->Time) != TRUE) {
        return FALSE;
    }

    if (rtc_write_date(&Datetime->Date) != TRUE) {
        return FALSE;
    }

    // Reenable oscillator. Try fix this but some reason enable and disable makes writing the time not correct.
    // Maybe I am writing over values or something
    // if (rtc_enable_ext_oscillator() != TRUE) {
    //     return FALSE;
    // }

    return TRUE;
}

uint8_t rtc_write_time(dt_time_t* Time) {

    /* Read the start oscillator bit so that value is not altered */
    uint8_t secondsAddr = RTC_ADDR_SECONDS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &secondsAddr, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t secondsRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &secondsRead, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    /* Computer all the values time register values to write to the RTC */

    uint8_t buffer[4] = {
        RTC_ADDR_SECONDS,
        (secondsRead & 0x80) | RTC_SECONDS_TO_REGISTER(Time->second),
        RTC_MINUTES_TO_REGISTER(Time->minute),
        0x3F & RTC_HOURS_TO_REGISTER(Time->hour),
    };

    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, buffer, 4, 3000) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_write_date(dt_date_t* Date) {

    /* Read the OSCRUN, PWRFAIL and VBATEN bits so those values are not altered */
    uint8_t weekDayAddr = RTC_ADDR_WEEK_DAYS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &weekDayAddr, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t weekDaysRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &weekDaysRead, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    /* Computer all the values date register values to write to the RTC */

    uint8_t buffer[5] = {
        RTC_ADDR_WEEK_DAYS,
        (weekDaysRead & 0x38) | dt_calculate_day_of_week(Date),
        RTC_DAYS_TO_REGISTER(Date->day),
        RTC_MONTHS_TO_REGISTER(Date->month),
        RTC_YEARS_TO_REGISTER(Date->year),
    };

    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, buffer, 5, 3000) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_read_datetime(dt_datetime_t* Datetime) {

    if (rtc_read_time(&Datetime->Time) != TRUE) {
        return FALSE;
    }

    if (rtc_read_date(&Datetime->Date) != TRUE) {
        return FALSE;
    }

    return TRUE;
}

uint8_t rtc_read_time(dt_time_t* Time) {

    uint8_t startAddress = RTC_ADDR_SECONDS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &startAddress, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t buffer[3] = {0};
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, buffer, 3, 3000) != HAL_OK) {
        return FALSE;
    }

    Time->second = RTC_REGISTER_TO_SECONDS(buffer[0]);
    Time->minute = RTC_REGISTER_TO_MINUTES(buffer[1]);
    Time->hour   = RTC_REGISTER_TO_HOURS(buffer[2]);

    return TRUE;
}

uint8_t rtc_read_date(dt_date_t* Date) {

    uint8_t startAddress = RTC_ADDR_DAYS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &startAddress, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t buffer[3] = {0};
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, buffer, 3, 3000) != HAL_OK) {
        return FALSE;
    }

    Date->day   = RTC_REGISTER_TO_DAYS(buffer[0]);
    Date->month = RTC_REGISTER_TO_MONTHS(buffer[1]);
    Date->year  = RTC_REGISTER_TO_YEARS(buffer[2]);

    return TRUE;
}

uint8_t rtc_enable_ext_oscillator(void) {

    // Enable the oscillator
    uint8_t secondAddr = RTC_ADDR_SECONDS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &secondAddr, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t secondsRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &secondsRead, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t secondsWrite[2] = {secondAddr, secondsRead | (0x01 << 7)};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, secondsWrite, 2, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t weekDaysAddr = RTC_ADDR_WEEK_DAYS;
    uint8_t weekDaysRead = 0;

    // Wait for the oscillator to start
    while ((weekDaysRead & (0x01 << 5)) == 0) {

        if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &weekDaysAddr, 1, 3000) != HAL_OK) {
            return FALSE;
        }

        if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &weekDaysRead, 1, 3000) != HAL_OK) {
            return FALSE;
        }
    }

    return TRUE;
}

uint8_t rtc_disable_ext_oscillator(void) {

    uint8_t secondAddr = RTC_ADDR_SECONDS;
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_READ, &secondAddr, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t secondsRead;
    if (HAL_I2C_Master_Receive(&hi2c1, RTC_I2C_ADDRESS, &secondsRead, 1, 3000) != HAL_OK) {
        return FALSE;
    }

    uint8_t secondsWrite[2] = {secondAddr, secondsRead & ~(0x01 << 7)};
    if (HAL_I2C_Master_Transmit(&hi2c1, RTC_I2C_WRITE, secondsWrite, 2, 3000) != HAL_OK) {
        return FALSE;
    }

    return TRUE;
}