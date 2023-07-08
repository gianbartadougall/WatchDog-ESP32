/**
 * @file time.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-01-30
 *
 * @copyright Copyright (c) 2023
 *
 *
 */

#ifndef DATETIME_H
#define DATETIME_H

/* C Library Includes */
#include "stdint.h"

/* Personal Includes */

typedef struct dt_time_t {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
} dt_time_t;

typedef struct dt_date_t {
    uint8_t day;
    uint8_t month;
    uint16_t year;
} dt_date_t;

typedef struct dt_datetime_t {
    dt_time_t Time;
    dt_date_t Date;
} dt_datetime_t;

uint8_t dt_time_init(dt_time_t* Time, uint8_t second, uint8_t minute, uint8_t hour);

uint8_t dt_date_init(dt_date_t* Date, uint8_t day, uint8_t month, uint16_t year);

uint8_t dt_is_valid_hour_min_period(char* date);

uint8_t dt_is_valid_hour_min(char* time);

uint8_t dt_is_valid_date(char* date);

uint8_t dt_time_is_valid(dt_time_t* Time);
uint8_t dt_time_valid(uint8_t second, uint8_t minute, uint8_t hour);

uint8_t dt_date_is_valid(dt_date_t* Date);
uint8_t dt_date_valid(uint8_t day, uint8_t month, uint16_t year);

void dt_datetime_increment_day(dt_datetime_t* datetime);
uint8_t dt_datetime_set_time(dt_datetime_t* datetime, dt_time_t Time);

uint8_t dt_time_add_time(dt_time_t* Time, dt_time_t timeToAdd);

uint8_t dt_time_t1_leq_t2(dt_time_t* t1, dt_time_t* t2);

void dt_datetime_to_string(dt_datetime_t* datetime, char* string);

uint8_t dt_time_format_is_valid(char* time);

void dt_time_to_string(char* timeString, dt_time_t timeStruct, uint8_t hasPeriod);

uint8_t dt_datetime_confirm_values(dt_datetime_t* datetime, uint8_t seconds, uint8_t minutes, uint8_t hours,
                                   uint8_t days, uint8_t months, uint16_t years);

uint8_t dt_datetime_init(dt_datetime_t* datetime, uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t days,
                         uint8_t months, uint16_t years);

uint8_t dt_calculate_day_of_week(dt_date_t* Date);

void dt_datetime_add_time(dt_datetime_t* Datetime, uint16_t seconds, uint16_t minutes, uint16_t hours, uint16_t days);

uint8_t dt1_greater_than_dt2(dt_datetime_t* Dt1, dt_datetime_t* Dt2);

uint8_t dt_datetimes_are_equal(dt_datetime_t* dt1, dt_datetime_t* dt2);

#define DATETIME_ASSERT_VALID_TIME(time, error) \
    do {                                        \
        if (dt_time_is_valid(time) != TRUE) {   \
            return error;                       \
        }                                       \
    } while (0)

#define DATETIME_ASSERT_VALID_DATE(date, error) \
    do {                                        \
        if (dt_date_is_valid(date) != TRUE) {   \
            return error;                       \
        }                                       \
    } while (0)

#endif // DATETIME_H