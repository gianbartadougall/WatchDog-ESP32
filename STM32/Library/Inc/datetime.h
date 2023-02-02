/**
 * @file time.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-01-30
 *
 * @copyright Copyright (c) 2023
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

uint8_t dt_time_is_valid(dt_time_t* time);
uint8_t dt_time_valid(uint8_t second, uint8_t minute, uint8_t hour);

uint8_t dt_date_is_valid(dt_date_t* date);
uint8_t dt_date_valid(uint8_t day, uint8_t month, uint16_t year);

#endif // DATETIME_H