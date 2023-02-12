/**
 * @file watchdog_defines.h
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef WATCHDOG_DEFINES_H
#define WATCHDOG_DEFINES_H

/* C Library Includes */
#include "bpacket.h"
#include "utilities.h"
#include "datetime.h"
#include "ds18b20.h"
#include <stdio.h>

#define MOUNT_POINT_PATH ("/sdcard")

#define WATCHDOG_FOLDER_NAME               ("WATCHDOG")
#define WATCHDOG_FOLDER_PATH               ("/WATCHDOG")
#define WATCHDOG_FOLDER_PATH_START_AT_ROOT ("/sdcard/WATCHDOG")

#define SETTINGS_FOLDER_NAME               ("SETTINGS")
#define SETTINGS_FOLDER_PATH               ("/SETTINGS")
#define SETTINGS_FOLDER_PATH_START_AT_ROOT ("/sdcard/WATCHDOG/SETTINGS")

#define LOG_FOLDER_NAME               ("LOGS")
#define LOG_FOLDER_PATH               ("/LOGS")
#define LOG_FOLDER_PATH_START_AT_ROOT ("/sdcard/WATCHDOG/LOGS")

#define SETTINGS_FILE_NAME               ("s.wd")
#define SETTINGS_FILE_NAME_PATH          ("/s.wd")
#define SETTINGS_FILE_PATH_START_AT_ROOT ("/sdcard/WATCHDOG/SETTINGS/s.wd")

#define LOG_FILE_NAME               ("logs.txt")
#define LOG_FILE_NAME_PATH          ("/logs.txt")
#define LOG_FILE_PATH_START_AT_ROOT ("/sdcard/WATCHDOG/LOGS/logs.txt")

#define ERROR_FILE_NAME               ("err.txt")
#define ERROR_FILE_NAME_PATH          ("/err.txts")
#define ERROR_FILE_PATH_START_AT_ROOT ("/sdcard/WATCHDOG/LOGS/err.txt")

// Deprecated and should no longer be used
#define SYSTEM_LOG_FILE        ("logs.txt")
#define IMAGE_DATA_FOLDER      ("WATCHDOG/DATA")
#define ROOT_IMAGE_DATA_FOLDER ("/sdcard/WATCHDOG/DATA")
// #define WATCHDOG_FOLDER_PATH      ("/sdcard/WATCHDOG")
#define SETTINGS_FOLDER_ROOT_PATH ("/sdcard/SETTINGS")
#define ROOT_LOG_FOLDER_PATH      ("/sdcard/WATCHDOG/LOGS")

// The values of the #defines here have been taken
// from the enums of the framesizes defined in
// sensors.h
#define WD_CAM_RES_320x240   5  // FRAMESIZE_QVGA
#define WD_CAM_RES_352x288   6  // FRAMESIZE_CIF
#define WD_CAM_RES_640x480   8  // FRAMESIZE_VGA
#define WD_CAM_RES_800x600   9  // FRAMESIZE_SVGA
#define WD_CAM_RES_1024x768  10 // FRAMESIZE_XGA
#define WD_CAM_RES_1280x1024 12 // FRAMESIZE_SXGA
#define WD_CAM_RES_1600x1200 13 // FRAMESIZE_UXGA

// UART Packet codes
#define WATCHDOG_BPK_R_LIST_DIR              (BPACKET_SPECIFIC_R_OFFSET + 0)
#define WATCHDOG_BPK_R_COPY_FILE             (BPACKET_SPECIFIC_R_OFFSET + 1)
#define WATCHDOG_BPK_R_TAKE_PHOTO            (BPACKET_SPECIFIC_R_OFFSET + 2)
#define WATCHDOG_BPK_R_WRITE_TO_FILE         (BPACKET_SPECIFIC_R_OFFSET + 3)
#define WATCHDOG_BPK_R_RECORD_DATA           (BPACKET_SPECIFIC_R_OFFSET + 4)
#define WATCHDOG_BPK_R_LED_RED_ON            (BPACKET_SPECIFIC_R_OFFSET + 6)
#define WATCHDOG_BPK_R_LED_RED_OFF           (BPACKET_SPECIFIC_R_OFFSET + 7)
#define WATCHDOG_BPK_R_CAMERA_VIEW           (BPACKET_SPECIFIC_R_OFFSET + 8)
#define WATCHDOG_BPK_R_GET_DATETIME          (BPACKET_SPECIFIC_R_OFFSET + 9)
#define WATCHDOG_BPK_R_SET_DATETIME          (BPACKET_SPECIFIC_R_OFFSET + 10)
#define WATCHDOG_BPK_R_GET_CAMERA_RESOLUTION (BPACKET_SPECIFIC_R_OFFSET + 13)
#define WATCHDOG_BPK_R_SET_CAMERA_RESOLUTION (BPACKET_SPECIFIC_R_OFFSET + 14)
#define WATCHDOG_BPK_R_GET_STATUS            (BPACKET_SPECIFIC_R_OFFSET + 15)
#define WATCHDOG_BPK_R_GET_SETTINGS          (BPACKET_SPECIFIC_R_OFFSET + 16)
#define WATCHDOG_BPK_R_SET_SETTINGS          (BPACKET_SPECIFIC_R_OFFSET + 17)

#define WATCHDOG_PING_CODE_ESP32 23
#define WATCHDOG_PING_CODE_STM32 47

#define WATCHDOG_ERROR_OFFSET              2 // So error codes are never = TRUE/FALSE
#define WATCHDOG_INVALID_CAMERA_RESOLUTION (WATCHDOG_ERROR_OFFSET + 0)
#define WATCHDOG_INVALID_START_TIME        (WATCHDOG_ERROR_OFFSET + 1)
#define WATCHDOG_INVALID_END_TIME          (WATCHDOG_ERROR_OFFSET + 2)
#define WATCHDOG_INVALID_INTERVAL_TIME     (WATCHDOG_ERROR_OFFSET + 3)
#define WATCHDOG_INVALID_REQUEST           (WATCHDOG_ERROR_OFFSET + 4)
#define WATCHDOG_INVALID_DATE              (WATCHDOG_ERROR_OFFSET + 5)
#define WATCHDOG_INVALID_BPACKET_SIZE      (WATCHDOG_ERROR_OFFSET + 6)

/* Public Enumerations */

typedef struct wd_camera_settings_t {
    uint8_t resolution;
} wd_camera_settings_t;

typedef struct wd_camera_capture_time_settings_t {
    dt_time_t startTime;
    dt_time_t endTime;
    dt_time_t intervalTime;
} wd_camera_capture_time_settings_t;

typedef struct wd_status_t {
    uint8_t id;
    uint8_t status;
    uint8_t batteryCapacity;
    uint8_t numImages;
    uint8_t sdCardFreeSpaceMb;
} wd_status_t;

typedef struct wd_settings_t {
    wd_camera_settings_t cameraSettings;
    wd_camera_capture_time_settings_t captureTime;
    uint8_t id;
    uint8_t status;
} wd_settings_t;

#define WD_ASSERT_VALID_CAMERA_RESOLUTION(resolution)            \
    do {                                                         \
        if (wd_camera_resolution_is_valid(resolution) != TRUE) { \
            return BPACKET_ERR_INVALID_RECEIVER;                 \
        }                                                        \
    } while (0)

/* Function Prototypes */
uint8_t wd_datetime_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                               dt_datetime_t* datetime);
uint8_t wd_bpacket_to_datetime(bpacket_t* bpacket, dt_datetime_t* datetime);
// uint8_t dt_time_is_valid(dt_time_t* time);

uint8_t wd_bpacket_to_camera_settings(bpacket_t* bpacket, wd_camera_settings_t* cameraSettings);
uint8_t wd_camera_settings_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request,
                                      uint8_t code, wd_camera_settings_t* cameraSettings);
uint8_t wd_camera_resolution_is_valid(uint8_t cameraResolution);

uint8_t wd_status_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                             wd_status_t* status);
uint8_t wd_bpacket_to_status(bpacket_t* bpacket, wd_status_t* status);

uint8_t wd_settings_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                               wd_settings_t* wdSettings);
uint8_t wd_bpacket_to_settings(bpacket_t* bpacket, wd_settings_t* wdSettings);

uint8_t wd_bpacket_to_photo_data(bpacket_t* bpacket, dt_datetime_t* datetime, ds18b20_temp_t* temp1,
                                 ds18b20_temp_t* temp2);

uint8_t wd_photo_data_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                                 dt_datetime_t* datetime, ds18b20_temp_t* temp1, ds18b20_temp_t* temp2);

void wd_get_error(uint8_t wdError, char* errorMsg);

#ifdef WATCHDOG_FUNCTIONS

uint8_t wd_photo_data_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                                 dt_datetime_t* datetime, ds18b20_temp_t* temp1, ds18b20_temp_t* temp2) {

    // Confirm the bpacket has the correct request
    if (request != WATCHDOG_BPK_R_TAKE_PHOTO) {
        return WATCHDOG_INVALID_REQUEST;
    }

    // Assert the sender and receiver are valid
    BPACKET_ASSERT_VALID_RECEIVER(receiver);
    BPACKET_ASSERT_VALID_SENDER(sender);
    BPACKET_ASSERT_VALID_CODE(code);

    // Assert the date and time are valid
    DATETIME_ASSERT_VALID_TIME(&datetime->time, WATCHDOG_INVALID_START_TIME);
    DATETIME_ASSERT_VALID_DATE(&datetime->date, WATCHDOG_INVALID_DATE);

    // Assert the temperatures are valid
    DS18B20_ASSERT_VALID_TEMPERATURE(temp1);
    DS18B20_ASSERT_VALID_TEMPERATURE(temp2);

    bpacket->receiver  = receiver;
    bpacket->sender    = sender;
    bpacket->request   = request;
    bpacket->code      = code;
    bpacket->numBytes  = 14;
    bpacket->bytes[0]  = datetime->time.second;
    bpacket->bytes[1]  = datetime->time.minute;
    bpacket->bytes[2]  = datetime->time.hour;
    bpacket->bytes[3]  = datetime->date.day;
    bpacket->bytes[4]  = datetime->date.month;
    bpacket->bytes[5]  = ((datetime->date.year & 0xFF00) >> 8);
    bpacket->bytes[6]  = (datetime->date.year & 0x00FF);
    bpacket->bytes[7]  = temp1->sign;
    bpacket->bytes[8]  = temp1->decimal;
    bpacket->bytes[9]  = ((temp1->fraction & 0xFF00) >> 8);
    bpacket->bytes[10] = temp1->fraction & 0x00FF;
    bpacket->bytes[11] = temp2->decimal;
    bpacket->bytes[12] = ((temp2->fraction & 0xFF00) >> 8);
    bpacket->bytes[13] = temp2->fraction & 0x00FF;

    return TRUE;
}

uint8_t wd_bpacket_to_photo_data(bpacket_t* bpacket, dt_datetime_t* datetime, ds18b20_temp_t* temp1,
                                 ds18b20_temp_t* temp2) {

    // Assert the request is valid
    if (bpacket->request != WATCHDOG_BPK_R_TAKE_PHOTO) {
        return WATCHDOG_INVALID_REQUEST;
    }

    // Assert the bpacket length is valid
    if (bpacket->numBytes != 14) {
        return WATCHDOG_INVALID_BPACKET_SIZE;
    }

    // Assert the time is valid
    if (dt_time_valid(bpacket->bytes[0], bpacket->bytes[1], bpacket->bytes[2]) != TRUE) {
        return WATCHDOG_INVALID_START_TIME;
    }

    // Assert the date is valid
    if (dt_date_valid(bpacket->bytes[3], bpacket->bytes[4], (bpacket->bytes[5] << 8) | bpacket->bytes[6]) != TRUE) {
        return WATCHDOG_INVALID_DATE;
    }

    datetime->time.second = bpacket->bytes[0];
    datetime->time.minute = bpacket->bytes[1];
    datetime->time.hour   = bpacket->bytes[2];
    datetime->date.day    = bpacket->bytes[3];
    datetime->date.month  = bpacket->bytes[4];
    datetime->date.year   = ((bpacket->bytes[5] << 8) | bpacket->bytes[6]);
    temp1->sign           = bpacket->bytes[7];
    temp1->decimal        = bpacket->bytes[8];
    temp1->fraction       = ((bpacket->bytes[9] << 8) | bpacket->bytes[10]);
    temp2->decimal        = bpacket->bytes[11];
    temp2->fraction       = ((bpacket->bytes[12] << 8) | bpacket->bytes[13]);

    return TRUE;
}

uint8_t wd_settings_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                               wd_settings_t* wdSettings) {

    /* Assert all the settings are valid */
    WD_ASSERT_VALID_CAMERA_RESOLUTION(wdSettings->cameraSettings.resolution);

    DATETIME_ASSERT_VALID_TIME(&wdSettings->captureTime.startTime, WATCHDOG_INVALID_START_TIME);
    DATETIME_ASSERT_VALID_TIME(&wdSettings->captureTime.endTime, WATCHDOG_INVALID_START_TIME);
    DATETIME_ASSERT_VALID_TIME(&wdSettings->captureTime.intervalTime, WATCHDOG_INVALID_INTERVAL_TIME);

    /* All settings valid. Parse settings into bpacket*/
    bpacket->receiver = receiver;
    bpacket->sender   = sender;
    bpacket->request  = request;
    bpacket->code     = code;
    bpacket->numBytes = 9;
    bpacket->bytes[0] = wdSettings->cameraSettings.resolution;
    bpacket->bytes[1] = wdSettings->captureTime.startTime.minute;
    bpacket->bytes[2] = wdSettings->captureTime.startTime.hour;
    bpacket->bytes[3] = wdSettings->captureTime.endTime.minute;
    bpacket->bytes[4] = wdSettings->captureTime.endTime.hour;
    bpacket->bytes[5] = wdSettings->captureTime.intervalTime.minute;
    bpacket->bytes[6] = wdSettings->captureTime.intervalTime.hour;
    bpacket->bytes[7] = wdSettings->id;
    bpacket->bytes[8] = wdSettings->status;

    return TRUE;
}

uint8_t wd_bpacket_to_settings(bpacket_t* bpacket, wd_settings_t* wdSettings) {

    // Assert valid bpacket request
    if ((bpacket->request != WATCHDOG_BPK_R_GET_SETTINGS) && (bpacket->request != WATCHDOG_BPK_R_SET_SETTINGS)) {
        return WATCHDOG_INVALID_REQUEST;
    }

    // Assert valid camera resolution
    WD_ASSERT_VALID_CAMERA_RESOLUTION(bpacket->bytes[0]);

    // Assert valid start time
    if (dt_time_valid(0, bpacket->bytes[1], bpacket->bytes[2]) != TRUE) {
        return WATCHDOG_INVALID_START_TIME;
    }

    // Assert valid end time
    if (dt_time_valid(0, bpacket->bytes[3], bpacket->bytes[4]) != TRUE) {
        return WATCHDOG_INVALID_END_TIME;
    }

    // Assert valid interval time
    if (dt_time_valid(0, bpacket->bytes[5], bpacket->bytes[6]) != TRUE) {
        return WATCHDOG_INVALID_INTERVAL_TIME;
    }

    /* All settings valid. Parse settings into bpacket*/
    wdSettings->cameraSettings.resolution       = bpacket->bytes[0];
    wdSettings->captureTime.startTime.minute    = bpacket->bytes[1];
    wdSettings->captureTime.startTime.hour      = bpacket->bytes[2];
    wdSettings->captureTime.endTime.minute      = bpacket->bytes[3];
    wdSettings->captureTime.endTime.hour        = bpacket->bytes[4];
    wdSettings->captureTime.intervalTime.minute = bpacket->bytes[5];
    wdSettings->captureTime.intervalTime.hour   = bpacket->bytes[6];
    wdSettings->id                              = bpacket->bytes[7];
    wdSettings->status                          = bpacket->bytes[8];

    return TRUE;
}

uint8_t wd_datetime_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                               dt_datetime_t* datetime) {

    // Confirm the request is valid
    if ((request != WATCHDOG_BPK_R_GET_DATETIME) && (request != WATCHDOG_BPK_R_SET_DATETIME) &&
        (request != BPACKET_CODE_SUCCESS)) {
        return BPACKET_ERR_INVALID_REQUEST;
    }

    BPACKET_ASSERT_VALID_RECEIVER(receiver);
    BPACKET_ASSERT_VALID_SENDER(sender);
    BPACKET_ASSERT_VALID_CODE(code);

    // Confirm the time is valid
    if (dt_time_is_valid(&datetime->time) != TRUE) {
        return FALSE;
    }

    // Confirm the date is valid
    if (dt_date_is_valid(&datetime->date) != TRUE) {
        return FALSE;
    }

    bpacket->receiver = receiver;
    bpacket->sender   = sender;
    bpacket->request  = request;
    bpacket->code     = code;
    bpacket->numBytes = 7;
    bpacket->bytes[0] = datetime->time.second;
    bpacket->bytes[1] = datetime->time.minute;
    bpacket->bytes[2] = datetime->time.hour;
    bpacket->bytes[3] = datetime->date.day;
    bpacket->bytes[4] = datetime->date.month;
    bpacket->bytes[5] = datetime->date.year >> 8;
    bpacket->bytes[6] = datetime->date.year & 0xFF;

    return TRUE;
}

uint8_t wd_bpacket_to_datetime(bpacket_t* bpacket, dt_datetime_t* datetime) {

    // confirm bpacket has the right number of bytes
    if (bpacket->numBytes != 7) {
        return FALSE;
    }

    // Confirm the request is valid
    if ((bpacket->request != WATCHDOG_BPK_R_GET_DATETIME) && (bpacket->request != WATCHDOG_BPK_R_SET_DATETIME) &&
        (bpacket->request != BPACKET_CODE_SUCCESS)) {
        return FALSE;
    }

    // Confirm the time is valid
    if (dt_time_valid(bpacket->bytes[0], bpacket->bytes[1], bpacket->bytes[2]) != TRUE) {
        return FALSE;
    }

    uint16_t year = (bpacket->bytes[5] << 8) | bpacket->bytes[6];
    if (dt_date_valid(bpacket->bytes[3], bpacket->bytes[4], year) != TRUE) {
        return FALSE;
    }

    // Confirm year is valid
    if (year < 2023 || year > 2099) {
        return FALSE;
    }

    datetime->time.second = bpacket->bytes[0];
    datetime->time.minute = bpacket->bytes[1];
    datetime->time.hour   = bpacket->bytes[2];
    datetime->date.day    = bpacket->bytes[3];
    datetime->date.month  = bpacket->bytes[4];
    datetime->date.year   = year;

    return TRUE;
}

uint8_t wd_camera_resolution_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request,
                                        uint8_t code, wd_camera_settings_t* cameraSettings) {

    // Confirm the request is valid
    if ((request != WATCHDOG_BPK_R_GET_CAMERA_RESOLUTION) && (request != WATCHDOG_BPK_R_SET_CAMERA_RESOLUTION)) {
        return WATCHDOG_INVALID_REQUEST;
    }

    BPACKET_ASSERT_VALID_RECEIVER(receiver);
    BPACKET_ASSERT_VALID_SENDER(sender);
    BPACKET_ASSERT_VALID_CODE(code);

    // Confirm the resolution is valid
    if (wd_camera_resolution_is_valid(cameraSettings->resolution) != TRUE) {
        return WATCHDOG_INVALID_CAMERA_RESOLUTION;
    }

    bpacket->receiver = receiver;
    bpacket->sender   = sender;
    bpacket->request  = request;
    bpacket->code     = code;
    bpacket->numBytes = 1;
    bpacket->bytes[0] = cameraSettings->resolution;

    return TRUE;
}

uint8_t wd_bpacket_to_camera_resolution(bpacket_t* bpacket, wd_camera_settings_t* cameraSettings) {

    // Confirm the request is valid
    if ((bpacket->request != WATCHDOG_BPK_R_GET_CAMERA_RESOLUTION) &&
        (bpacket->request != WATCHDOG_BPK_R_SET_CAMERA_RESOLUTION)) {
        return WATCHDOG_INVALID_REQUEST;
    }

    WD_ASSERT_VALID_CAMERA_RESOLUTION(bpacket->bytes[0]);

    cameraSettings->resolution = bpacket->bytes[0];

    return TRUE;
}

uint8_t wd_camera_resolution_is_valid(uint8_t cameraResolution) {

    switch (cameraResolution) {
        case WD_CAM_RES_320x240:
        case WD_CAM_RES_352x288:
        case WD_CAM_RES_640x480:
        case WD_CAM_RES_800x600:
        case WD_CAM_RES_1024x768:
        case WD_CAM_RES_1280x1024:
        case WD_CAM_RES_1600x1200:
            return TRUE;
        default:
            return FALSE;
    }
}

uint8_t wd_status_to_bpacket(bpacket_t* bpacket, uint8_t receiver, uint8_t sender, uint8_t request, uint8_t code,
                             wd_status_t* status) {

    // Confirm the status is valid
    if (status->status != WATCHDOG_BPK_R_GET_STATUS) {
        return FALSE;
    }

    if (BPACKET_RECEIVER_IS_INVALID(receiver) || BPACKET_SENDER_IS_INVALID(sender) ||
        BPACKET_REQUEST_IS_INVALID(request) || BPACKET_CODE_IS_INVALID(code)) {
        return FALSE;
    }

    bpacket->receiver = receiver;
    bpacket->sender   = sender;
    bpacket->request  = request;
    bpacket->code     = code;
    bpacket->request  = WATCHDOG_BPK_R_GET_STATUS;
    bpacket->numBytes = 5;
    bpacket->bytes[0] = status->id;
    bpacket->bytes[1] = status->status;
    bpacket->bytes[2] = status->numImages;
    bpacket->bytes[3] = status->batteryCapacity;
    bpacket->bytes[4] = status->sdCardFreeSpaceMb;

    return TRUE;
}

uint8_t wd_bpacket_to_status(bpacket_t* bpacket, wd_status_t* status) {

    // Confirm the request is valid
    if ((bpacket->request != WATCHDOG_BPK_R_GET_STATUS) && (bpacket->request != BPACKET_CODE_SUCCESS)) {
        return FALSE;
    }

    // Confirm the number of bytes is valid
    if (bpacket->numBytes != 5) {
        return FALSE;
    }

    // Confirm the status is valid
    if (bpacket->bytes[1] != WATCHDOG_BPK_R_GET_STATUS) {
        return FALSE;
    }

    status->id                = bpacket->bytes[0];
    status->status            = bpacket->bytes[1];
    status->numImages         = bpacket->bytes[2];
    status->batteryCapacity   = bpacket->bytes[3];
    status->sdCardFreeSpaceMb = bpacket->bytes[4];

    return TRUE;
}

void wd_get_error(uint8_t wdError, char* errorMsg) {

    switch (wdError) {

        case WATCHDOG_INVALID_CAMERA_RESOLUTION:
            sprintf(errorMsg, "WD def err: Invalid camera resolution\r\n");
            break;

        case WATCHDOG_INVALID_START_TIME:
            sprintf(errorMsg, "WD def err: Invalid start time\r\n");
            break;

        case WATCHDOG_INVALID_END_TIME:
            sprintf(errorMsg, "WD def err: Invalid end time\r\n");
            break;

        case WATCHDOG_INVALID_INTERVAL_TIME:
            sprintf(errorMsg, "WD def err: Invalid interval minute or hour\r\n");
            break;

        case WATCHDOG_INVALID_REQUEST:
            sprintf(errorMsg, "WD def err: Invalid request\r\n");
            break;

        case WATCHDOG_INVALID_DATE:
            sprintf(errorMsg, "WD def err: Invalid date\r\n");
            break;

        case WATCHDOG_INVALID_BPACKET_SIZE:
            sprintf(errorMsg, "WD def err: Invalid bpacket size\r\n");
            break;

        default:
            sprintf(errorMsg, "WD def err: Unknown WD error code %i\r\n", wdError);
            break;
    }
}

#endif

#endif // WATCHDOG_DEFINES_H