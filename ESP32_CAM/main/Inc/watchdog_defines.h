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
#include <stdio.h>

/* Personal Includes */
#include "utils.h"
#include "datetime.h"
#include "custom_data_types.h"
#include "bpacket.h"
#include "bpacket_utils.h"

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

// Deprecated and should no longer be used
#define SYSTEM_LOG_FILE        ("logs.txt")
#define IMAGE_DATA_FOLDER      ("watchdog/data")
#define ROOT_IMAGE_DATA_FOLDER ("/sdcard/watchdog/data")
// #define WATCHDOG_FOLDER_PATH      ("/sdcard/watchdog")
#define SETTINGS_FOLDER_ROOT_PATH ("/sdcard/settings")
#define ROOT_LOG_FOLDER_PATH      ("/sdcard/watchdog/logs")

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
#define WATCHDOG_INVALID_YEAR              (WATCHDOG_ERROR_OFFSET + 7)

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
} wd_settings_t;

#define WD_ASSERT_VALID_CAMERA_RESOLUTION(Bpacket, resolution)      \
    do {                                                            \
        if (wd_camera_resolution_is_valid(resolution) != TRUE) {    \
            Bpacket->ErrorCode = BPK_Err_Invalid_Camera_Resolution; \
            return FALSE;                                           \
        }                                                           \
    } while (0)

/* Function Prototypes */
uint8_t wd_bpk_to_camera_settings(bpk_packet_t* Bpacket, cdt_u8_t* CameraSettings);
uint8_t wd_camera_settings_to_bpk(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                  bpk_request_t Request, bpk_code_t Code, cdt_u8_t* CameraSettings);

uint8_t wd_datetime_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                               bpk_request_t Request, bpk_code_t Code, dt_datetime_t* datetime);
uint8_t wd_bpacket_to_datetime(bpk_packet_t* Bpacket, dt_datetime_t* datetime);
// uint8_t dt_time_is_valid(dt_time_t* time);

uint8_t wd_bpacket_to_camera_settings(bpk_packet_t* Bpacket, wd_camera_settings_t* cameraSettings);
uint8_t wd_camera_settings_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                      bpk_request_t Request, bpk_code_t Code, wd_camera_settings_t* cameraSettings);
uint8_t wd_camera_resolution_is_valid(uint8_t cameraResolution);

uint8_t wd_status_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                             bpk_request_t Request, bpk_code_t Code, wd_status_t* status);
uint8_t wd_bpacket_to_status(bpk_packet_t* Bpacket, wd_status_t* status);

uint8_t wd_capture_time_settings_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                            bpk_request_t Request, bpk_code_t Code,
                                            wd_camera_capture_time_settings_t* wdSettings);
uint8_t wd_bpacket_to_capture_time_settings(bpk_packet_t* Bpacket, wd_camera_capture_time_settings_t* wdSettings);

uint8_t wd_bpacket_to_photo_data(bpk_packet_t* Bpacket, dt_datetime_t* datetime, cdt_dbl_16_t* temp1,
                                 cdt_dbl_16_t* temp2);

uint8_t wd_photo_data_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                 bpk_request_t Request, bpk_code_t Code, dt_datetime_t* datetime, cdt_dbl_16_t* temp1,
                                 cdt_dbl_16_t* temp2);

void wd_get_error(uint8_t wdError, char* errorMsg);

#ifdef WATCHDOG_FUNCTIONS

uint8_t wd_photo_data_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                 bpk_request_t Request, bpk_code_t Code, dt_datetime_t* datetime, cdt_dbl_16_t* temp1,
                                 cdt_dbl_16_t* temp2) {

    // Confirm the bpacket has the correct request
    if (Request.val != BPK_Req_Take_Photo.val) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Request;
        return FALSE;
    }

    // Assert the date and time are valid
    DATETIME_ASSERT_VALID_TIME(&datetime->Time, WATCHDOG_INVALID_START_TIME);
    DATETIME_ASSERT_VALID_DATE(&datetime->Date, WATCHDOG_INVALID_DATE);

    Bpacket->Receiver       = Receiver;
    Bpacket->Sender         = Sender;
    Bpacket->Request        = Request;
    Bpacket->Code           = Code;
    Bpacket->Data.numBytes  = 14;
    Bpacket->Data.bytes[0]  = datetime->Time.second;
    Bpacket->Data.bytes[1]  = datetime->Time.minute;
    Bpacket->Data.bytes[2]  = datetime->Time.hour;
    Bpacket->Data.bytes[3]  = datetime->Date.day;
    Bpacket->Data.bytes[4]  = datetime->Date.month;
    Bpacket->Data.bytes[5]  = ((datetime->Date.year & 0xFF00) >> 8);
    Bpacket->Data.bytes[6]  = (datetime->Date.year & 0x00FF);
    Bpacket->Data.bytes[7]  = temp1->info;
    Bpacket->Data.bytes[8]  = temp1->integer;
    Bpacket->Data.bytes[9]  = ((temp1->decimal & 0xFF00) >> 8);
    Bpacket->Data.bytes[10] = temp1->decimal & 0x00FF;
    Bpacket->Data.bytes[11] = temp2->integer;
    Bpacket->Data.bytes[12] = ((temp2->decimal & 0xFF00) >> 8);
    Bpacket->Data.bytes[13] = temp2->decimal & 0x00FF;

    return TRUE;
}

uint8_t wd_bpacket_to_photo_data(bpk_packet_t* Bpacket, dt_datetime_t* datetime, cdt_dbl_16_t* temp1,
                                 cdt_dbl_16_t* temp2) {

    // Assert the request is valid
    if (Bpacket->Request.val != BPK_Req_Take_Photo.val) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Request;
        return FALSE;
    }

    // Assert the bpacket length is valid
    if (Bpacket->Data.numBytes != 14) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Bpacket_Size;
        return FALSE;
    }

    // Assert the time is valid
    if (dt_time_valid(Bpacket->Data.bytes[0], Bpacket->Data.bytes[1], Bpacket->Data.bytes[2]) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Start_Time;
        return FALSE;
    }

    // Assert the date is valid
    if (dt_date_valid(Bpacket->Data.bytes[3], Bpacket->Data.bytes[4],
                      (Bpacket->Data.bytes[5] << 8) | Bpacket->Data.bytes[6]) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Date;
        return FALSE;
    }

    datetime->Time.second = Bpacket->Data.bytes[0];
    datetime->Time.minute = Bpacket->Data.bytes[1];
    datetime->Time.hour   = Bpacket->Data.bytes[2];
    datetime->Date.day    = Bpacket->Data.bytes[3];
    datetime->Date.month  = Bpacket->Data.bytes[4];
    datetime->Date.year   = ((Bpacket->Data.bytes[5] << 8) | Bpacket->Data.bytes[6]);
    temp1->info           = Bpacket->Data.bytes[7];
    temp1->integer        = Bpacket->Data.bytes[8];
    temp1->decimal        = ((Bpacket->Data.bytes[9] << 8) | Bpacket->Data.bytes[10]);
    temp2->integer        = Bpacket->Data.bytes[11];
    temp2->decimal        = ((Bpacket->Data.bytes[12] << 8) | Bpacket->Data.bytes[13]);

    return TRUE;
}

uint8_t wd_capture_time_settings_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                            bpk_request_t Request, bpk_code_t Code,
                                            wd_camera_capture_time_settings_t* captureTime) {

    /* Assert all the settings are valid */
    DATETIME_ASSERT_VALID_TIME(&captureTime->startTime, WATCHDOG_INVALID_START_TIME);
    DATETIME_ASSERT_VALID_TIME(&captureTime->endTime, WATCHDOG_INVALID_START_TIME);
    DATETIME_ASSERT_VALID_TIME(&captureTime->intervalTime, WATCHDOG_INVALID_INTERVAL_TIME);

    /* All settings valid. Parse settings into bpacket*/
    Bpacket->Receiver      = Receiver;
    Bpacket->Sender        = Sender;
    Bpacket->Request       = Request;
    Bpacket->Code          = Code;
    Bpacket->Data.numBytes = 6;
    Bpacket->Data.bytes[0] = captureTime->startTime.minute;
    Bpacket->Data.bytes[1] = captureTime->startTime.hour;
    Bpacket->Data.bytes[2] = captureTime->endTime.minute;
    Bpacket->Data.bytes[3] = captureTime->endTime.hour;
    Bpacket->Data.bytes[4] = captureTime->intervalTime.minute;
    Bpacket->Data.bytes[5] = captureTime->intervalTime.hour;

    return TRUE;
}

uint8_t wd_bpacket_to_capture_time_settings(bpk_packet_t* Bpacket, wd_camera_capture_time_settings_t* captureTime) {

    // Assert valid bpacket request
    if ((Bpacket->Request.val != BPK_Req_Get_Camera_Capture_Times.val) &&
        (Bpacket->Request.val != BPK_Req_Set_Camera_Capture_Times.val)) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Request;
        return FALSE;
    }

    // Assert valid start time
    if (dt_time_valid(0, Bpacket->Data.bytes[0], Bpacket->Data.bytes[1]) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Start_Time;
        return FALSE;
    }

    // Assert valid end time
    if (dt_time_valid(0, Bpacket->Data.bytes[2], Bpacket->Data.bytes[3]) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_End_Time;
        return FALSE;
    }

    // Assert valid interval time
    if (dt_time_valid(0, Bpacket->Data.bytes[4], Bpacket->Data.bytes[5]) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Interval_Time;
        return FALSE;
    }

    /* All settings valid. Parse settings into bpacket*/
    captureTime->startTime.minute    = Bpacket->Data.bytes[0];
    captureTime->startTime.hour      = Bpacket->Data.bytes[1];
    captureTime->endTime.minute      = Bpacket->Data.bytes[2];
    captureTime->endTime.hour        = Bpacket->Data.bytes[3];
    captureTime->intervalTime.minute = Bpacket->Data.bytes[4];
    captureTime->intervalTime.hour   = Bpacket->Data.bytes[5];

    return TRUE;
}

uint8_t wd_datetime_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                               bpk_request_t Request, bpk_code_t Code, dt_datetime_t* datetime) {

    // Confirm the request is valid
    if ((Request.val != BPK_Req_Get_Datetime.val) && (Request.val != BPK_Req_Set_Datetime.val)) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Request;
        return FALSE;
    }

    // Confirm the time is valid
    if (dt_time_is_valid(&datetime->Time) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Start_Time;
        return FALSE;
    }

    // Confirm the date is valid
    if (dt_date_is_valid(&datetime->Date) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Date;
        return FALSE;
    }

    Bpacket->Receiver      = Receiver;
    Bpacket->Sender        = Sender;
    Bpacket->Request       = Request;
    Bpacket->Code          = Code;
    Bpacket->Data.numBytes = 7;
    Bpacket->Data.bytes[0] = datetime->Time.second;
    Bpacket->Data.bytes[1] = datetime->Time.minute;
    Bpacket->Data.bytes[2] = datetime->Time.hour;
    Bpacket->Data.bytes[3] = datetime->Date.day;
    Bpacket->Data.bytes[4] = datetime->Date.month;
    Bpacket->Data.bytes[5] = datetime->Date.year >> 8;
    Bpacket->Data.bytes[6] = datetime->Date.year & 0xFF;

    return TRUE;
}

uint8_t wd_bpacket_to_datetime(bpk_packet_t* Bpacket, dt_datetime_t* datetime) {

    // confirm bpacket has the right number of bytes
    if (Bpacket->Data.numBytes != 7) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Bpacket_Size;
        return FALSE;
    }

    // Confirm the request is valid
    if ((Bpacket->Request.val != BPK_Req_Get_Datetime.val) && (Bpacket->Request.val != BPK_Req_Set_Datetime.val)) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Request;
        return FALSE;
    }

    // Confirm the time is valid
    if (dt_time_valid(Bpacket->Data.bytes[0], Bpacket->Data.bytes[1], Bpacket->Data.bytes[2]) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Start_Time;
        return FALSE;
    }

    uint16_t year = (Bpacket->Data.bytes[5] << 8) | Bpacket->Data.bytes[6];
    if (dt_date_valid(Bpacket->Data.bytes[3], Bpacket->Data.bytes[4], year) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Date;
        return FALSE;
    }

    // Confirm year is valid
    if (year < 2023 || year > 2099) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Year;
        return FALSE;
    }

    datetime->Time.second = Bpacket->Data.bytes[0];
    datetime->Time.minute = Bpacket->Data.bytes[1];
    datetime->Time.hour   = Bpacket->Data.bytes[2];
    datetime->Date.day    = Bpacket->Data.bytes[3];
    datetime->Date.month  = Bpacket->Data.bytes[4];
    datetime->Date.year   = year;

    return TRUE;
}

uint8_t wd_camera_settings_to_bpk(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                  bpk_request_t Request, bpk_code_t Code, cdt_u8_t* CameraSettings) {

    // Confirm the request is
    switch (Request.val) {
        case BPK_REQ_GET_CAMERA_SETTINGS:
        case BPK_REQ_SET_CAMERA_SETTINGS:
            break;
        default:
            Bpacket->ErrorCode = BPK_Err_Invalid_Request;
            return FALSE;
    }

    // Confirm the resolution is valid
    if (wd_camera_resolution_is_valid(CameraSettings->value) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Camera_Resolution;
        return FALSE;
    }

    Bpacket->Receiver = Receiver;
    Bpacket->Sender   = Sender;
    Bpacket->Request  = Request;
    Bpacket->Code     = Code;
    bpk_utils_write_cdt_u8(Bpacket, CameraSettings);

    return TRUE;
}

uint8_t wd_bpk_to_camera_settings(bpk_packet_t* Bpacket, cdt_u8_t* CameraSettings) {

    switch (Bpacket->Request.val) {
        case BPK_REQ_GET_CAMERA_SETTINGS:
        case BPK_REQ_SET_CAMERA_SETTINGS:
            break;
        default:
            Bpacket->ErrorCode = BPK_Err_Invalid_Request;
            return FALSE;
    }

    WD_ASSERT_VALID_CAMERA_RESOLUTION(Bpacket, Bpacket->Data.bytes[0]);

    bpk_utils_read_cdt_u8(Bpacket, CameraSettings);

    return TRUE;
}

uint8_t wd_camera_settings_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                                      bpk_request_t Request, bpk_code_t Code, wd_camera_settings_t* cameraSettings) {

    // Confirm the request is valid
    if ((Request.val != BPK_Req_Get_Camera_Settings.val) && (Request.val != BPK_Req_Set_Camera_Settings.val)) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Request;
        return FALSE;
    }

    // Confirm the resolution is valid
    if (wd_camera_resolution_is_valid(cameraSettings->resolution) != TRUE) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Camera_Resolution;
        return FALSE;
    }

    Bpacket->Receiver      = Receiver;
    Bpacket->Sender        = Sender;
    Bpacket->Request       = Request;
    Bpacket->Code          = Code;
    Bpacket->Data.numBytes = 1;
    Bpacket->Data.bytes[0] = cameraSettings->resolution;

    return TRUE;
}

uint8_t wd_bpacket_to_camera_settings(bpk_packet_t* Bpacket, wd_camera_settings_t* cameraSettings) {

    // Confirm the request is valid
    if ((Bpacket->Request.val != BPK_Req_Get_Camera_Settings.val) &&
        (Bpacket->Request.val != BPK_Req_Set_Camera_Settings.val)) {
        Bpacket->ErrorCode = BPK_Err_Invalid_Request;
        return FALSE;
    }

    WD_ASSERT_VALID_CAMERA_RESOLUTION(Bpacket, Bpacket->Data.bytes[0]);

    cameraSettings->resolution = Bpacket->Data.bytes[0];

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

uint8_t wd_status_to_bpacket(bpk_packet_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender,
                             bpk_request_t Request, bpk_code_t Code, wd_status_t* status) {

    // Confirm the status is valid
    if (status->status != BPK_REQUEST_STATUS) {
        return FALSE;
    }

    Bpacket->Receiver      = Receiver;
    Bpacket->Sender        = Sender;
    Bpacket->Request       = Request;
    Bpacket->Code          = Code;
    Bpacket->Data.numBytes = 5;
    Bpacket->Data.bytes[0] = status->id;
    Bpacket->Data.bytes[1] = status->status;
    Bpacket->Data.bytes[2] = status->numImages;
    Bpacket->Data.bytes[3] = status->batteryCapacity;
    Bpacket->Data.bytes[4] = status->sdCardFreeSpaceMb;

    return TRUE;
}

uint8_t wd_bpacket_to_status(bpk_packet_t* Bpacket, wd_status_t* status) {

    // Confirm the request is valid
    if ((Bpacket->Request.val != BPK_REQUEST_STATUS) && (Bpacket->Request.val != BPK_Code_Success.val)) {
        return FALSE;
    }

    // Confirm the number of bytes is valid
    if (Bpacket->Data.numBytes != 5) {
        return FALSE;
    }

    // Confirm the status is valid
    if (Bpacket->Data.bytes[1] != BPK_REQUEST_STATUS) {
        return FALSE;
    }

    status->id                = Bpacket->Data.bytes[0];
    status->status            = Bpacket->Data.bytes[1];
    status->numImages         = Bpacket->Data.bytes[2];
    status->batteryCapacity   = Bpacket->Data.bytes[3];
    status->sdCardFreeSpaceMb = Bpacket->Data.bytes[4];

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

        case WATCHDOG_INVALID_YEAR:
            sprintf(errorMsg, "WD def err: Invalid year\r\n");
            break;

        default:
            sprintf(errorMsg, "WD def err: Unknown WD error code %i\r\n", wdError);
            break;
    }
}

#endif

#endif // WATCHDOG_DEFINES_H
