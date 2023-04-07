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
#include "utils.h"
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

uint8_t dt_time_is_valid(dt_time_t* Time) {
    return dt_time_valid(Time->second, Time->minute, Time->hour);
}

uint8_t dt_time_init(dt_time_t* Time, uint8_t second, uint8_t minute, uint8_t hour) {

    if (dt_time_valid(second, minute, hour) != TRUE) {
        return FALSE;
    }

    Time->second = second;
    Time->minute = minute;
    Time->hour   = hour;

    return TRUE;
}

uint8_t dt_date_init(dt_date_t* Date, uint8_t day, uint8_t month, uint16_t year) {

    if (dt_date_valid(day, month, year) != TRUE) {
        return FALSE;
    }

    Date->day   = day;
    Date->month = month;
    Date->year  = year;

    return TRUE;
}

uint8_t dt_date_is_valid(dt_date_t* Date) {
    return dt_date_valid(Date->day, Date->month, Date->year);
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

uint8_t dt_time_t1_leq_t2(dt_time_t* t1, dt_time_t* t2) {

    if (t1->hour < t2->hour) {
        return TRUE;
    }

    if ((t1->hour == t2->hour) && (t1->minute < t2->minute)) {
        return TRUE;
    }

    if ((t1->hour == t2->hour) && (t1->minute == t2->minute) && (t1->second <= t2->second)) {
        return TRUE;
    }

    return FALSE;
}

uint8_t dt_time_add_time(dt_time_t* Time, dt_time_t timeToAdd) {

    Time->second += timeToAdd.second;
    if (Time->second > 60) {
        Time->second -= 60;
        Time->minute++;
    }

    Time->minute += timeToAdd.minute;
    if (Time->minute > 60) {
        Time->minute -= 60;
        Time->hour++;
    }

    Time->hour += timeToAdd.hour;
    if (Time->minute > 23) {
        return FALSE;
    }

    return TRUE;
}

void dt_datetime_increment_day(dt_datetime_t* datetime) {

    uint8_t nextMonth = FALSE;

    if (datetime->Date.month == FEBRUARY && YEAR_IS_LEAP_YEAR(datetime->Date.year) == TRUE) {
        if (datetime->Date.day == 29) {
            nextMonth = TRUE;
        }
    }

    if (daysInMonth[datetime->Date.month] == datetime->Date.day) {
        nextMonth = TRUE;
    }

    if (nextMonth == TRUE) {
        datetime->Date.day = 1;
        datetime->Date.month++;
    } else {
        datetime->Date.day++;
    }

    // Correct for decemeber
    if (datetime->Date.month > DECEMBER) {
        datetime->Date.month = JANUARY;
    }
}

uint8_t dt_datetime_set_time(dt_datetime_t* datetime, dt_time_t Time) {

    // Confirm the time is valid
    if (dt_time_is_valid(&Time) != TRUE) {
        return FALSE;
    }

    datetime->Time.second = Time.second;
    datetime->Time.minute = Time.minute;
    datetime->Time.hour   = Time.hour;

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
    int isSpace   = 0;
    char period[3];
    int num;
    if ((num = sscanf(time, "%d:%d %2s", &hour, &min, period)) != 3) {
        return FALSE;
    }
    if (time[0] == '0' || hour > 9) {
        firstZero = 1;
    }
    if (time[1 + firstZero] != ':') {
        return FALSE;
    }
    if (time[4 + firstZero] == ' ') {
        isSpace = 1;
    }
    if (chars_get_num_bytes(time) > (6 + isSpace + firstZero)) {
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

void dt_datetime_to_string(dt_datetime_t* datetime, char* string) {
    sprintf(string, "%i:%i:%i %i/%i/%i", datetime->Time.hour, datetime->Time.minute, datetime->Time.second,
            datetime->Date.day, datetime->Date.month, datetime->Date.year);
}

void dt_time_to_string(char* timeString, dt_time_t timeStruct, uint8_t hasPeriod) {
    uint8_t minute;
    minute = timeStruct.minute;
    uint8_t hour;
    hour = timeStruct.hour;
    char period[3];
    if (hour >= 12) {
        sprintf(period, "pm");
    } else {
        sprintf(period, "am");
    }
    if (hour > 12) {
        hour -= 12;
    }
    if (hour == 0) {
        hour += 12;
    }
    if (hasPeriod == TRUE) {
        sprintf(timeString, "%i:%s%i %s", hour, minute < 10 ? "0" : "", minute, period);
    } else if (hasPeriod == FALSE) {
        sprintf(timeString, "%i:%s%i", hour, minute < 10 ? "0" : "", minute);
    }
}