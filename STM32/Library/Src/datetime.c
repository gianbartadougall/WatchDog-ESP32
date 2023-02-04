/**
 * @file datetime.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-01-30
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <stdio.h>

/* Personal Includes */
#include "datetime.h"
#include "utilities.h"
#include "chars.h"

#define JANUARY   1
#define FEBRUARY  2
#define MARCH     3
#define APRIL     4
#define MAY       5
#define JUNE      6
#define JULY      7
#define AUGUST    8
#define SEPTEMBER 9
#define OCTOBER   10
#define NOVEMEBER 11
#define DECEMBER  12
#define MIN_YEAR  2022
#define MAX_YEAR  2100

#define YEAR_IS_LEAP_YEAR(year) ((year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0)))

const uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* Function Prototypes */

uint8_t dt_time_is_valid(dt_time_t* time) {
    return dt_time_valid(time->second, time->minute, time->hour);
}

uint8_t dt_time_init(dt_time_t* time, uint8_t second, uint8_t minute, uint8_t hour) {

    if (dt_time_valid(second, minute, hour) != TRUE) {
        return FALSE;
    }

    time->second = second;
    time->minute = minute;
    time->hour   = hour;

    return TRUE;
}

uint8_t dt_date_init(dt_date_t* date, uint8_t day, uint8_t month, uint16_t year) {

    if (dt_date_valid(day, month, year) != TRUE) {
        return FALSE;
    }

    date->day   = day;
    date->month = month;
    date->year  = year;

    return TRUE;
}

uint8_t dt_date_is_valid(dt_date_t* date) {
    return dt_date_valid(date->day, date->month, date->year);
}

uint8_t dt_date_valid(uint8_t day, uint8_t month, uint16_t year) {

    if ((day == 0) || (month == 0) || (year == 0) || (month > 12)) {
        return FALSE;
    }

    uint8_t maxDays = daysInMonth[month - 1]; // Minus 1 because index starts from 0
    if ((month == FEBRUARY) && YEAR_IS_LEAP_YEAR(year) == TRUE) {
        maxDays = 29;
    }

    if (day > maxDays) {
        return FALSE;
    }

    return TRUE;
}

uint8_t dt_time_valid(uint8_t second, uint8_t minute, uint8_t hour) {

    // Confirm seconds are valid
    if (second > 59) {
        return FALSE;
    }

    // Confirm minuytes are valid
    if (minute > 59) {
        return FALSE;
    }

    // Confirm hours are valid
    if (hour > 23) {
        return FALSE;
    }

    return TRUE;
}
uint8_t is_leap_year(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
}

uint8_t dt_is_valid_date(char* date) {
    int day, month, year;
    if (chars_get_num_bytes(date) > 10) {
        return FALSE;
    }
    if (sscanf(date, "%d/%d/%d", &day, &month, &year) != 3) {
        return FALSE;
    }
    if (month < 1 || month > 12) {
        return FALSE;
    }
    if (day < 1 || day > 31) {
        return FALSE;
    }
    if ((month == 2) && (day > 29)) {
        return FALSE;
    }
    if ((month == 2) && (day == 29) && (!is_leap_year(year))) {
        return FALSE;
    }
    if ((month == 4 || month == 6 || month == 9 || month == 11) && (day > 30)) {
        return FALSE;
    }
    if (year < MIN_YEAR || year > MAX_YEAR) {
        return FALSE;
    }
    return TRUE;
}

uint8_t dt_is_valid_hour_min(char* time) {

    if (dt_time_format_is_valid(time) != TRUE) {
        return FALSE;
    }

    int hour, min;
    if (sscanf(time, "%d:%d", &hour, &min) != 2) {
        return FALSE;
    }

    if ((min < 0) || min >= 60) {
        return FALSE;
    }
    if ((hour <= 0) || hour > 23) {
        return FALSE;
    }
    return TRUE;
}

uint8_t dt_time_format_is_valid(char* time) {

    int colonCount = 0;
    int i;
    for (i = 0; time[i] != '\0'; i++) {

        if (i > 4) {
            return FALSE;
        }

        if (time[i] == ':' && colonCount < 1) {
            colonCount++;
            if (time[i + 1] == '\0' || time[i + 2] == '\0') {
                return FALSE;
            }
            continue;
        }

        if (time[i] < '0' || time[i] > '9') {
            return FALSE;
        }
    }

    return TRUE;
}

uint8_t dt_is_valid_hour_min_period(char* time) {
    if (chars_get_num_bytes(time) > 8) {
        return FALSE;
    }
    int hour, min;
    int firstZero = 0;
    char period[3];
    if (sscanf(time, "%d:%d %2s", &hour, &min, period) != 3) {
        return FALSE;
    }
    if (time[0] == '0' || hour > 9) {
        firstZero = 1;
    }
    if (time[4 + firstZero] != ' ' || time[1 + firstZero] != ':') {
        printf("firstzero: %i, 4 + zero: %c, 1 + zero: %c:\n", firstZero, time[4 + firstZero], time[1 + firstZero]);
        return FALSE;
    }
    if ((min < 0) || min >= 60) {
        return FALSE;
    }
    if ((hour <= 0) || hour > 12) {
        return FALSE;
    }
    if (chars_same(period, "am\0") != TRUE && chars_same(period, "pm\0") != TRUE) {
        return FALSE;
    }
    return TRUE;
}