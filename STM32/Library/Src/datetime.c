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

uint8_t dt_time_is_valid(dt_time_t* time) {

    // Confirm seconds are valid
    if (time->second > 59) {
        return FALSE;
    }

    // Confirm seconds are valid
    if (time->minute > 59) {
        return FALSE;
    }

    // Confirm hours are valid
    if (time->hour > 23) {
        return FALSE;
    }

    return TRUE;
}