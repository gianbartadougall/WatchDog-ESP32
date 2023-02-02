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
/* */

/* Personal Includes */
#include "datetime.h"
#include "utilities.h"

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