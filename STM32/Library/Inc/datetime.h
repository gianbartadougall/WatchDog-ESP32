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
    dt_time_t time;
    dt_date_t date;
} dt_datetime_t;

uint8_t dt_time_init(dt_time_t* time, uint8_t second, uint8_t minute, uint8_t hour);

uint8_t dt_date_init(dt_date_t* date, uint8_t day, uint8_t month, uint16_t year);

uint8_t dt_is_valid_hour_min_period(char* date);

uint8_t dt_time_is_valid(dt_time_t* time);
uint8_t dt_time_valid(uint8_t second, uint8_t minute, uint8_t hour);

uint8_t dt_date_is_valid(dt_date_t* date);
uint8_t dt_date_valid(uint8_t day, uint8_t month, uint16_t year);

void dt_datetime_increment_day(dt_datetime_t* datetime);
uint8_t dt_datetime_set_time(dt_datetime_t* datetime, dt_time_t time);

uint8_t dt_time_add_time(dt_time_t* time, dt_time_t timeToAdd);

uint8_t dt_time_t1_leq_t2(dt_time_t* t1, dt_time_t* t2);

void dt_datetime_to_string(dt_datetime_t* datetime, char* string);

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