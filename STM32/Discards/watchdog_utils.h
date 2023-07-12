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
#define WD_PING_CODE_ESP32 23
#define WD_PING_CODE_STM32 47

#define MOUNT_POINT_PATH ("/sdcard")

#define WATCHDOG_FOLDER_NAME               ("watchdog")
#define WATCHDOG_FOLDER_PATH               ("/watchdog")
#define WATCHDOG_FOLDER_PATH_START_AT_ROOT ("/sdcard/watchdog")

#define SETTINGS_FOLDER_NAME               ("settings")
#define SETTINGS_FOLDER_PATH               ("/watchdog/settings")
#define SETTINGS_FOLDER_PATH_START_AT_ROOT ("/sdcard/watchdog/settings")

#define DATA_FOLDER_NAME               ("data")
#define DATA_FOLDER_PATH               ("/watchdog/data")
#define DATA_FOLDER_PATH_START_AT_ROOT ("/sdcard/watchdog/data")

#define LOG_FOLDER_NAME               ("logs")
#define LOG_FOLDER_PATH               ("/watchdog/logs")
#define LOG_FOLDER_PATH_START_AT_ROOT ("/sdcard/watchdog/logs")

#define SETTINGS_FILE_NAME               ("s.wd")
#define SETTINGS_FILE_NAME_PATH          ("/watchdog/settings/s.wd")
#define SETTINGS_FILE_PATH_START_AT_ROOT ("/sdcard/watchdog/settings/s.wd")

#define DATA_FILE_NAME               ("data.txt")
#define DATA_FILE_NAME_PATH          ("/watchdog/data/data.txt")
#define DATA_FILE_PATH_START_AT_ROOT ("/sdcard/watchdog/data/data.txt")

#define LOG_FILE_NAME               ("logs.txt")
#define LOG_FILE_NAME_PATH          ("/watchdog/logs/logs.txt")
#define LOG_FILE_PATH_START_AT_ROOT ("/sdcard/watchdog/logs/logs.txt")

#define ERROR_FILE_NAME               ("err.txt")
#define ERROR_FILE_NAME_PATH          ("/err.txts")
#define ERROR_FILE_PATH_START_AT_ROOT ("/sdcard/watchdog/logs/err.txt")

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

void wd_utils_settings_to_array(uint8_t array[20], capture_time_t* CaptureTime, camera_settings_t* CameraSettings);
void wd_utils_bpk_to_settings(bpk_t* Bpk, capture_time_t* CaptureTime, camera_settings_t* CameraSettings);
void wd_utils_photo_data_to_array(uint8_t data[sizeof(dt_datetime_t) + sizeof(float)], dt_datetime_t* Datetime,
                                  float temperature);
void wd_utils_array_to_photo_data(uint8_t data[BPACKET_MAX_NUM_DATA_BYTES], dt_datetime_t* Datetime,
                                  float* temperature);
#endif // WATCHDOG_UTILS_H