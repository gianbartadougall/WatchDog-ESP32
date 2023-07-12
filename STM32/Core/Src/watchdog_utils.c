/* Personal Includes */
#include "watchdog_utils.h"
#include "datetime.h"
#include "float.h"

void wd_utils_settings_to_array(uint8_t array[19], capture_time_t* CaptureTime, camera_settings_t* CameraSettings) {

    array[0] = CaptureTime->Start.Time.second;
    array[1] = CaptureTime->Start.Time.minute;
    array[2] = CaptureTime->Start.Time.hour;
    array[3] = CaptureTime->Start.Date.day;
    array[4] = CaptureTime->Start.Date.month;
    array[5] = CaptureTime->Start.Date.year >> 8;
    array[6] = CaptureTime->Start.Date.year & 0xFF;

    array[7]  = CaptureTime->End.Time.second;
    array[8]  = CaptureTime->End.Time.minute;
    array[9]  = CaptureTime->End.Time.hour;
    array[10] = CaptureTime->End.Date.day;
    array[11] = CaptureTime->End.Date.month;
    array[12] = CaptureTime->End.Date.year >> 8;
    array[13] = CaptureTime->End.Date.year & 0xFF;

    array[14] = CaptureTime->intervalSecond;
    array[15] = CaptureTime->intervalMinute;
    array[16] = CaptureTime->intervalHour;
    array[17] = CaptureTime->intervalDay;

    array[18] = CameraSettings->resolution;
}

void wd_utils_bpk_to_settings(bpk_t* Bpk, capture_time_t* CaptureTime, camera_settings_t* CameraSettings) {

    CaptureTime->Start.Time.second = Bpk->Data.bytes[0];
    CaptureTime->Start.Time.minute = Bpk->Data.bytes[1];
    CaptureTime->Start.Time.hour   = Bpk->Data.bytes[2];
    CaptureTime->Start.Date.day    = Bpk->Data.bytes[3];
    CaptureTime->Start.Date.month  = Bpk->Data.bytes[4];
    CaptureTime->Start.Date.year   = (Bpk->Data.bytes[5] << 8) | Bpk->Data.bytes[6];
    CaptureTime->End.Time.second   = Bpk->Data.bytes[7];
    CaptureTime->End.Time.minute   = Bpk->Data.bytes[8];
    CaptureTime->End.Time.hour     = Bpk->Data.bytes[9];
    CaptureTime->End.Date.day      = Bpk->Data.bytes[10];
    CaptureTime->End.Date.month    = Bpk->Data.bytes[11];
    CaptureTime->End.Date.year     = (Bpk->Data.bytes[12] << 8) | Bpk->Data.bytes[13];
    CaptureTime->intervalSecond    = Bpk->Data.bytes[14];
    CaptureTime->intervalMinute    = Bpk->Data.bytes[15];
    CaptureTime->intervalHour      = Bpk->Data.bytes[16];
    CaptureTime->intervalDay       = Bpk->Data.bytes[17];

    CameraSettings->resolution = Bpk->Data.bytes[18];
}

void wd_utils_photo_data_to_array(uint8_t data[sizeof(dt_datetime_t) + sizeof(float)], dt_datetime_t* Datetime,
                                  float temperature) {

    data[0] = Datetime->Time.second;
    data[1] = Datetime->Time.minute;
    data[2] = Datetime->Time.hour;
    data[3] = Datetime->Date.day;
    data[4] = Datetime->Date.month;
    data[5] = Datetime->Date.year >> 8;
    data[6] = Datetime->Date.year & 0xFF;

    uint8_t* ptr = (uint8_t*)&temperature;
    for (uint8_t i = 0; i < sizeof(float); i++) {
        data[7 + i] = ptr[i];
    }
}