/**
 * @file watchdog_utils.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-08
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef WATCHDOG_UTILS_H
#define WATCHDOG_UTILS_H

/* C Library Includes */

/* Personal Includes */
#include "bpacket.h"
#include "datetime.h"

/* Public Macros */

/* Public Enums and Structs */
typedef struct camera_settings_t {
    uint8_t resolution;
} camera_settings_t;

typedef struct capture_time_t {
    dt_datetime_t Start;
    dt_datetime_t End;
    uint8_t intervalSecond;
    uint8_t intervalMinute;
    uint8_t intervalHour;
    uint8_t intervalDay;
} capture_time_t;

void wd_utils_settings_to_array(uint8_t array[19], capture_time_t* CaptureTime, camera_settings_t* CameraSettings);
void wd_utils_bpk_to_settings(bpk_t* Bpk, capture_time_t* CaptureTime, camera_settings_t* CameraSettings);

#endif // WATCHDOG_UTILS_H