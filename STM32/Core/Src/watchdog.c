/**
 * @file watchdog.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-03
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <stdarg.h>

/* STM32 Library Includes */
#include "usb_device.h"
#include "usbd_cdc_if.h"

/* Personal Includes */
#include "watchdog.h"
#include "event_group.h"
#include "datetime.h"
#include "utils.h"
#include "hardware_config.h"
#include "gpio.h"
#include "task_scheduler.h"
#include "task_scheduler_config.h"
#include "stm32_uart.h"
#include "log_usb.h"
#include "bpacket.h"
#include "comms_stm32.h"
#include "rtc_mcp7940n.h"
#include "watchdog_utils.h"

/* Private Enums */

// typedef struct camera_settings_t {
//     uint8_t resolution;
// } camera_settings_t;

/* Private Macros */
#define WD_FLASH_SETTINGS_START_ADDRESS 0x0803F800
#define WD_FLASH_SETTINGS_END_ADDRESS   0x0803F8BC

/* Public Variables */
event_group_t gbl_EventsStm, gbl_EventsEsp;

/* Private Variables */
capture_time_t gbl_CaptureTime;
camera_settings_t lg_CameraSettings;

/* Function Prototypes */
uint8_t usb_read_packet(bpk_t* Bpacket);
uint8_t wd_request_photo_capture(void);

uint8_t wd_write_settings(capture_time_t* CaptureTime, camera_settings_t* CameraSettings);
void wd_read_settings(capture_time_t* CaptureTime, camera_settings_t* CameraSettings);

void wd_handle_esp_response(bpk_t* Bpacket);
void wd_handle_maple_request(bpk_t* Bpacket);
void wd_write_bpacket_maple(bpk_t* Bpacket);

void i2c_test(void) {

    if (dt_datetime_init(&gbl_CaptureTime.Start, 0, 0, 6, 5, 7, 2023) != TRUE) {
        wd_error_handler(__FILE__, __LINE__, EC_DATETIME_CREATION);
    }

    if (dt_datetime_init(&gbl_CaptureTime.End, 0, 0, 6, 31, 12, 2023) != TRUE) {
        wd_error_handler(__FILE__, __LINE__, EC_DATETIME_CREATION);
    }

    gbl_CaptureTime.intervalSecond = 30;
    gbl_CaptureTime.intervalMinute = 0;
    gbl_CaptureTime.intervalHour   = 0;
    gbl_CaptureTime.intervalDay    = 0;

    if (rtc_init() != TRUE) {
        wd_error_handler(__FILE__, __LINE__, EC_RTC_INIT);
    }

    if (rtc_write_datetime(&gbl_CaptureTime.Start) != TRUE) {
        wd_error_handler(__FILE__, __LINE__, EC_DATETIME_CREATION);
    }

    dt_datetime_t Alarm;
    if (dt_datetime_init(&Alarm, 10, 0, 6, 5, 7, 2023) != TRUE) {
        wd_error_handler(__FILE__, __LINE__, EC_DATETIME_CREATION);
    }

    dt_datetime_t bTime;
    if (rtc_read_datetime(&bTime) != TRUE) {
        wd_error_handler(__FILE__, __LINE__, EC_RTC_READ_ALARM);
    }

    if (rtc_enable_alarm() != TRUE) {
        wd_error_handler(__FILE__, __LINE__, 0);
    }

    if (rtc_set_alarm(&Alarm) != TRUE) {
        wd_error_handler(__FILE__, __LINE__, EC_RTC_SET_ALARM);
    }

    dt_datetime_t aTime;
    if (rtc_read_alarm_datetime(&aTime) != TRUE) {
        wd_error_handler(__FILE__, __LINE__, EC_RTC_READ_ALARM);
    }

    log_usb_success("Alarm set to %i:%i:%i %i/%i/%i\tTime set to %i:%i:%i %i/%i/%i \r\n", aTime.Time.hour,
                    aTime.Time.minute, aTime.Time.second, aTime.Date.day, aTime.Date.month, aTime.Date.year,
                    bTime.Time.hour, bTime.Time.minute, bTime.Time.second, bTime.Date.day, bTime.Date.month,
                    bTime.Date.year);

    dt_datetime_t Datetime;
    uint8_t lastSecond = 0;

    while (1) {

        // if (rtc_read_datetime(&Datetime) != TRUE) {
        //     wd_error_handler(__FILE__, __LINE__, EC_RTC_READ_DATETIME);
        // }

        // if (Datetime.Time.second != lastSecond) {
        //     lastSecond = Datetime.Time.second;
        //     log_usb_success("%i:%i:%i %i/%i/%i\r\n", Datetime.Time.hour, Datetime.Time.minute, Datetime.Time.second,
        //                     Datetime.Date.day, Datetime.Date.month, Datetime.Date.year);
        // }

        if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);

            if (rtc_read_datetime(&Datetime) != TRUE) {
                wd_error_handler(__FILE__, __LINE__, EC_RTC_READ_DATETIME);
            }

            log_usb_success("Photo Captured! @ %i:%i:%i %i/%i/%i \r\n", Datetime.Time.hour, Datetime.Time.minute,
                            Datetime.Time.second, Datetime.Date.day, Datetime.Date.month, Datetime.Date.year);
        }

        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE);

            // Read the the current alarm time
            dt_datetime_t AlarmTime;
            if (rtc_read_alarm_datetime(&AlarmTime) != TRUE) {
                wd_error_handler(__FILE__, __LINE__, EC_RTC_READ_ALARM);
            }

            dt_datetime_add_time(&AlarmTime, gbl_CaptureTime.intervalSecond, gbl_CaptureTime.intervalMinute,
                                 gbl_CaptureTime.intervalHour, gbl_CaptureTime.intervalDay);

            if (dt1_greater_than_dt2(&AlarmTime, &gbl_CaptureTime.Start) == TRUE) {
                rtc_disable_alarm();
            } else {
                rtc_set_alarm(&AlarmTime);
            }
        }

        HAL_Delay(10);
    }
}

void flash_test(void) {

    capture_time_t CPSettings = {
        .Start.Time.second = 00,
        .Start.Time.minute = 30,
        .Start.Time.hour   = 9,
        .Start.Date.day    = 1,
        .Start.Date.month  = 1,
        .Start.Date.year   = 2023,
        .End.Time.second   = 00,
        .End.Time.minute   = 30,
        .End.Time.hour     = 7,
        .End.Date.day      = 31,
        .End.Date.month    = 12,
        .End.Date.year     = 2024,
        .intervalDay       = 0,
        .intervalSecond    = 0,
        .intervalMinute    = 30,
        .intervalHour      = 2,
    };

    camera_settings_t CS = {
        .resolution = 5,
    };

    if (wd_write_settings(&CPSettings, &CS) != TRUE) {
        wd_error_handler(__FILE__, __LINE__, 0);
    }

    camera_settings_t ncs;
    capture_time_t nct;
    wd_read_settings(&nct, &ncs);

    log_usb_message("Start.Time.second %i\r\n", nct.Start.Time.second);
    log_usb_message("Start.Time.minute %i\r\n", nct.Start.Time.minute);
    log_usb_message("Start.Time.hour   %i\r\n", nct.Start.Time.hour);
    log_usb_message("Start.Date.day    %i\r\n", nct.Start.Date.day);
    log_usb_message("Start.Date.month  %i\r\n", nct.Start.Date.month);
    log_usb_message("Start.Date.year   %i\r\n", nct.Start.Date.year);
    log_usb_message("End.Time.second   %i\r\n", nct.End.Time.second);
    log_usb_message("End.Time.minute   %i\r\n", nct.End.Time.minute);
    log_usb_message("End.Time.hour     %i\r\n", nct.End.Time.hour);
    log_usb_message("End.Date.day      %i\r\n", nct.End.Date.day);
    log_usb_message("End.Date.month    %i\r\n", nct.End.Date.month);
    log_usb_message("End.Date.year     %i\r\n", nct.End.Date.year);
    log_usb_message("intervalDay       %i\r\n", nct.intervalDay);
    log_usb_message("intervalSecond    %i\r\n", nct.intervalSecond);
    log_usb_message("intervalMinute    %i\r\n", nct.intervalMinute);
    log_usb_message("intervalHour      %i\r\n", nct.intervalHour);
    log_usb_message("resolution        %i\r\n", ncs.resolution);

    log_usb_message("Done\r\n");

    while (1) {}
}

void wd_error_handler_2(char* format, ...) {

    static char msg[100];
    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    while (1) {
        GPIO_TOGGLE(LED_RED_PORT, LED_RED_PIN);
        log_usb_error(msg);
        HAL_Delay(1000);
    }
}

void wd_start(void) {

    // Initialise all the hardware
    hardware_config_init();

    /* Initialise software */
    task_scheduler_init(); // Reset Recipes list

    if (rtc_init() != TRUE) {
        wd_error_handler(__FILE__, __LINE__, EC_RTC_INIT);
    }

    // Initialise event groups
    event_group_clear(&gbl_EventsStm);
    event_group_clear(&gbl_EventsEsp);

    /* Initialise the settings for watchdog (Capture time and camera settings) */
    wd_read_settings(&gbl_CaptureTime, &lg_CameraSettings);

    // while (1) {
    //     HAL_Delay(1000);
    //     log_usb_message("Cam res: %i\r\n", lg_CameraSettings.resolution);
    // }

    // TODO: Confirm the start, end and interval times just read are valid

    // TODO: Calcaulte when the next alarm should be and set the RTC alarm for that

    // if (wd_flash_read_camera_settings(&lg_CameraSettings) != TRUE) {
    //     wd_error_handler(__FILE__, __LINE__, EC_READ_CAMERA_SETTINGS);
    // }

    // TODO: Confirm the camera settings are valid

    // Set flag 'take photo' flag. In main loop below, STM32 will request ESP to
    // take a photo if this flag has been set. Getting ESP32 to take a photo on
    // startup to ensure the system is working correctly
    event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);

    // Set flag 'system initialising' flag. In main loop where stm32 waits for
    // responses back from the ESP, if it gets a response saying a photo was taken
    // and the system init flag has been set then it knows that the photo was a test
    // photo as this flag is only set once on startup. The STM32 can then blink an
    // led to let the user know that the system has been initialised
    event_group_set_bit(&gbl_EventsStm, EVENT_STM_SYSTEM_INITIALISING, EGT_ACTIVE);

    // Enter main loop
    bpk_t BpkEspResponse, MapleBpacket;
    uint8_t lastRTCSecond = 0;

    while (1) {

        /* Turn the ESP on or off depending on if there are esp tasks that need to occur */

        // Check if there are any esp flags that are set that are not ESP_ON or ESP_READY
        if ((event_group_get_bits(&gbl_EventsEsp, EGT_ACTIVE) & ~(EVENT_ESP_ON | EVENT_ESP_READY)) != 0) {

            // There is an esp event that needs to occur. Ensure the ESP has been turned on
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE) != TRUE) {
                // TODO: Turn the esp on
            }
        } else {

            // There are no pending esp events. Turn the ESP off if it is not already off
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE) == TRUE) {
                // TODO: Turn the esp off
            }
        }

        /* Put the STM32 to sleep if required */
        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_SLEEP, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_SLEEP, EGT_ACTIVE);

            // TODO: Put the STM32 to sleep
        }

        /* Request ESP32 to take a photo if the 'take photo' flag has been set */
        if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE) == TRUE) {

            // Don't request a photo until the ESP is in the ready state. The ESP takes
            // a second or two to startup after it has been turned on. The ESP_READY flag
            // will be set automatically by the STM when the ESP is ready to receive commands
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_READY, EGT_ACTIVE) == TRUE) {
                event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);

                if (wd_request_photo_capture() != TRUE) {
                    wd_error_handler(__FILE__, __LINE__, EC_REQUEST_PHOTO_CAPTURE);
                }

                /****** START CODE BLOCK ******/
                // Description: Debugging. Can delete when not needed
                dt_datetime_t Dt;
                if (rtc_read_datetime(&Dt) != TRUE) {
                    wd_error_handler(__FILE__, __LINE__, EC_RTC_READ_DATETIME);
                }

                log_usb_success("Photo Requested [%i:%i:%i %i/%i/%i]\r\n", Dt.Time.hour, Dt.Time.minute, Dt.Time.second,
                                Dt.Date.day, Dt.Date.month, Dt.Date.year);
                /****** END CODE BLOCK ******/
            }
        }

        /* Check whether the RTC alarm needs to be updated. This flag is set in the ISR that is triggered
            whenever the alarm time in the RTC matches the RTC time */
        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE);

            /* Calculate the next alarm time */
            dt_datetime_t AlarmTime;
            if (rtc_read_alarm_datetime(&AlarmTime) != TRUE) {
                wd_error_handler(__FILE__, __LINE__, EC_RTC_READ_ALARM);
            }

            dt_datetime_add_time(&AlarmTime, gbl_CaptureTime.intervalSecond, gbl_CaptureTime.intervalMinute,
                                 gbl_CaptureTime.intervalHour, gbl_CaptureTime.intervalDay);

            // Check if the system still needs to take photos
            if (dt1_greater_than_dt2(&AlarmTime, &gbl_CaptureTime.Start) == TRUE) {
                rtc_disable_alarm();
            } else {
                rtc_set_alarm(&AlarmTime);
            }
        }

        /* Process all packets received from the ESP32 */
        if (uart_read_bpacket(0, &BpkEspResponse) == TRUE) {
            wd_handle_esp_response(&BpkEspResponse);
        }

        // TODO: Process the button being pressed

        // Polling the USBC connection pin. The reason it is being polled instead of having an
        // interrupt is because if the system is not powered by batteries and only powered by
        // a USB C connector, the pin will go high before the circuit starts running in which
        // case the interrupt would never fire. Polling ensures that no matter how the system
        // is powered, it will always know if the USBC is connected or not
        if (GPIO_PIN_IS_HIGH(USBC_CONN_PORT, USBC_CONN_PIN)) {

            GPIO_SET_HIGH(LED_GREEN_PORT, LED_GREEN_PIN);

            // USBC is connected, check for any incoming bpackets and process them if any come
            if (usb_read_packet(&MapleBpacket) == TRUE) {
                wd_handle_maple_request(&MapleBpacket);
            }

            /* Send the datetime to the computer to display on GUI */
            dt_datetime_t dt;
            if (rtc_read_datetime(&dt) != TRUE) {
                wd_error_handler(__FILE__, __LINE__, EC_RTC_READ_DATETIME);
            }

            if (lastRTCSecond != dt.Time.second) {
                lastRTCSecond = dt.Time.second;

                uint8_t datetimeData[7] = {
                    dt.Time.second, dt.Time.minute,    dt.Time.hour,        dt.Date.day,
                    dt.Date.month,  dt.Date.year >> 8, dt.Date.year & 0xFF,
                };

                bpk_t BpkDatetime;
                bpk_create(&BpkDatetime, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Get_Datetime,
                           BPK_Code_Success, 7, datetimeData);

                wd_write_bpacket_maple(&BpkDatetime);
            }

        } else {
            GPIO_SET_LOW(LED_GREEN_PORT, LED_GREEN_PIN);
        }

        // TODO: Check whether the STM32 needs to go to sleep
    }
}

void wd_handle_maple_request(bpk_t* Bpacket) {

    // Create a bpacket to store the response to send back to Maple
    static bpk_t BpkStmResponse;

    switch (Bpacket->Request.val) {

        case BPK_REQUEST_PING:;

            // Create response
            static uint8_t stmPingCode = 47;
            if (bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Ping,
                           BPK_Code_Success, 1, &stmPingCode) != TRUE) {
                wd_error_handler(__FILE__, __LINE__, EC_BPACKET_CREATE);
                break;
            }

            // Send bpacket to Maple
            wd_write_bpacket_maple(&BpkStmResponse);

            break;

        case BPK_REQ_SET_DATETIME:;

            // Extract the datetime
            dt_datetime_t Datetime;
            if (dt_datetime_init(&Datetime, Bpacket->Data.bytes[0], Bpacket->Data.bytes[1], Bpacket->Data.bytes[2],
                                 Bpacket->Data.bytes[3], Bpacket->Data.bytes[4],
                                 (((uint16_t)Bpacket->Data.bytes[5]) << 8) | Bpacket->Data.bytes[6]) != TRUE) {
                // TODO: Handle error
                break;
            }

            /* Update the RTC with the new datetime */
            if (rtc_write_datetime(&Datetime) != TRUE) {
                // TODO: Handle the error

                break;
            }

            bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Set_Datetime,
                       BPK_Code_Success, 0, NULL);
            wd_write_bpacket_maple(&BpkStmResponse);
            break;

        case BPK_REQ_SET_WATCHDOG_SETTINGS:;

            /* Extract the settings from the bpacket and convert them to structs */
            capture_time_t NewCaptureTimeSettings;
            camera_settings_t NewCamSettings;
            wd_utils_bpk_to_settings(Bpacket, &NewCaptureTimeSettings, &NewCamSettings);

            /* Write new settings to flash */
            if (wd_write_settings(&NewCaptureTimeSettings, &NewCamSettings) != TRUE) {
                wd_error_handler(__FILE__, __LINE__, EC_FLASH_WRITE);
            }

            // Update the camera settings structs by reading the flash
            wd_read_settings(&gbl_CaptureTime, &lg_CameraSettings);

            /* Send success response */
            bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Set_Watchdog_Settings,
                       BPK_Code_Success, 0, NULL);
            wd_write_bpacket_maple(&BpkStmResponse);

            break;

        case BPK_REQ_GET_WATCHDOG_SETTINGS:;

            /* Store current watchdog settings into bpacket and send to computer */
            uint8_t settingsData[19];
            wd_utils_settings_to_array(settingsData, &gbl_CaptureTime, &lg_CameraSettings);

            /* Create bpacket and send to computer */
            bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Get_Watchdog_Settings,
                       BPK_Code_Success, 19, settingsData);
            wd_write_bpacket_maple(&BpkStmResponse);

            break;

        default:
            wd_error_handler_2("Error %s on line %i. Unexpected request %i\r\n", __FILE__, __LINE__,
                               Bpacket->Request.val);
    }

    // TODO: implement
}

void wd_write_bpacket_maple(bpk_t* Bpacket) {
    static bpk_buffer_t Buffer;
    bpk_to_buffer(Bpacket, &Buffer);
    log_usb_bytes(Buffer.buffer, Buffer.numBytes);
}

void wd_handle_esp_response(bpk_t* Bpacket) {
    // TODO: Implement
}

void wd_error_handler(char* fileName, uint16_t lineNumber, enum error_code_e errorCode) {

    // TODO: Turn the ESP off

    while (1) {

        HAL_Delay(1000);
        GPIO_TOGGLE(LED_RED_PORT, LED_RED_PIN);

        log_usb_error("Error code in %s on line %i: %i\r\n", fileName, lineNumber, errorCode);
    }
}

uint8_t wd_request_photo_capture(void) {
    // TODO: Implement

    return TRUE;
}

uint8_t wd_write_settings(capture_time_t* CaptureTime, camera_settings_t* CameraSettings) {

    uint32_t* startAddress = (uint32_t*)WD_FLASH_SETTINGS_START_ADDRESS;

    // Loop through each address and look for the first 2 32 bit address that have no data
    // in them
    while ((*startAddress != 0xFFFFFFFF) || ((*(startAddress + 1)) != 0xFFFFFFFF)) {
        // Increment by 2 because flash is comprised of 64 bit values
        startAddress += 2;
    }

    // Compact capture time data into a uin64_t array. This is required because only 64 bit
    // data can be written to the flash
    uint64_t settingsData[4] = {
        (((uint64_t)CaptureTime->Start.Time.second) << 56) | (((uint64_t)CaptureTime->Start.Time.minute) << 48) |
            (((uint64_t)CaptureTime->Start.Time.hour) << 40) | (((uint64_t)CaptureTime->Start.Date.day) << 32) |
            (((uint64_t)CaptureTime->Start.Date.month) << 24) | (((uint64_t)CaptureTime->Start.Date.year) << 8),
        (((uint64_t)CaptureTime->End.Time.second) << 56) | (((uint64_t)CaptureTime->End.Time.minute) << 48) |
            (((uint64_t)CaptureTime->End.Time.hour) << 40) | (((uint64_t)CaptureTime->End.Date.day) << 32) |
            (((uint64_t)CaptureTime->End.Date.month) << 24) | (((uint64_t)CaptureTime->End.Date.year) << 8),
        (((uint64_t)CaptureTime->intervalSecond) << 48) | (((uint64_t)CaptureTime->intervalMinute) << 32) |
            (((uint64_t)CaptureTime->intervalHour) << 16) | (((uint64_t)CaptureTime->intervalSecond) << 0),
        ((uint64_t)0xFFFFFFFF << 32) | (((uint64_t)CameraSettings->resolution) << 24) | 0x00FFFFFF,
    };

    // Check whether data has already been written to the page or not
    if (startAddress >= (uint32_t*)(WD_FLASH_SETTINGS_START_ADDRESS + (8 * 4))) {
        // Assume that the data needs to be written
        uint8_t writeRequired = FALSE;

        // Compare the data to write with the last bit of data that was written and see if they
        // are the same
        for (uint8_t i = 0; i < 4; i++) {

            uint64_t upperBytes = (uint64_t) * (startAddress - (8 - (i * 2))) << 32;
            uint32_t lowerBytes = (uint64_t) * (startAddress - (7 - (i * 2)));

            if (settingsData[i] != (upperBytes | lowerBytes)) {
                writeRequired = TRUE;
                break;
            }
        }

        if (writeRequired == FALSE) {
            return TRUE;
        }

        // Check whether the 32 bit address is at an odd or even address. If the
        // address is currently odd then we need to increment one to make it even
        // that way a full 64 bit value can be written to it otherwise writing to
        // the flash will fail when doing so with an odd address
        if (((((uint32_t)startAddress) / 4) % 2) != 0) {
            startAddress++;
        }

        // Calculate the correct address for the new data
        if ((startAddress + 8) > (uint32_t*)WD_FLASH_SETTINGS_END_ADDRESS) {

            startAddress = (uint32_t*)WD_FLASH_SETTINGS_START_ADDRESS;

            /* Erase the page to start again */
            if (HAL_FLASH_Unlock() != HAL_OK) {
                wd_error_handler(__FILE__, __LINE__, EC_FLASH_UNLOCK);
            }

            FLASH_EraseInitTypeDef eraseConfig;
            eraseConfig.TypeErase = FLASH_TYPEERASE_PAGES;
            eraseConfig.Page      = 127;
            eraseConfig.NbPages   = 1; // Number of pages to be erased

            uint32_t pageError = 0;
            if (HAL_FLASHEx_Erase(&eraseConfig, &pageError) != HAL_OK) {
                wd_error_handler(__FILE__, __LINE__, 0);
            }

            if (HAL_FLASH_Lock() != HAL_OK) {
                wd_error_handler(__FILE__, __LINE__, EC_FLASH_UNLOCK);
            }
        }
    }

    /* Write data to memory */
    if (HAL_FLASH_Unlock() != HAL_OK) {
        wd_error_handler(__FILE__, __LINE__, EC_FLASH_UNLOCK);
    }

    for (uint8_t i = 0; i < 4; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)startAddress, settingsData[i]) != HAL_OK) {
            wd_error_handler(__FILE__, __LINE__, EC_FLASH_WRITE);
        }

        // Increment by 2 because we just wrote two 32 bit values to the flash
        startAddress += 2;
    }

    if (HAL_FLASH_Lock() != HAL_OK) {
        wd_error_handler(__FILE__, __LINE__, EC_FLASH_LOCK);
    }

    return TRUE;
}

void wd_read_settings(capture_time_t* CaptureTime, camera_settings_t* CameraSettings) {

    uint32_t* startAddress = (uint32_t*)WD_FLASH_SETTINGS_START_ADDRESS;

    while ((*startAddress != 0xFFFFFFFF) || ((*(startAddress + 1)) != 0xFFFFFFFF)) {
        startAddress += 2;
    }

    // If there are no settings currently set, startAddress will not have incremented
    if (startAddress == (uint32_t*)WD_FLASH_SETTINGS_START_ADDRESS) {

        /* Set the default settings such that system will take a photo twice a day */
        CaptureTime->Start.Date.day    = 1;
        CaptureTime->Start.Date.month  = 1;
        CaptureTime->Start.Date.year   = 2023;
        CaptureTime->Start.Time.second = 0;
        CaptureTime->Start.Time.minute = 0;
        CaptureTime->Start.Time.hour   = 0;

        CaptureTime->End.Date.day    = 1;
        CaptureTime->End.Date.month  = 1;
        CaptureTime->End.Date.year   = 2030;
        CaptureTime->End.Time.second = 0;
        CaptureTime->End.Time.minute = 0;
        CaptureTime->End.Time.hour   = 0;

        CaptureTime->intervalSecond = 0;
        CaptureTime->intervalMinute = 0;
        CaptureTime->intervalHour   = 2;
        CaptureTime->intervalDay    = 0;

        CameraSettings->resolution = 5;

        /* These are now the default settings. Write them to the flash */
        if (wd_write_settings(CaptureTime, CameraSettings) != TRUE) {
            wd_error_handler(__FILE__, __LINE__, EC_FLASH_WRITE);
        }

        return;
    }

    // Data to read in the settings. The startAddress is pointing to the address after the
    // last bit of data. Need to go back 8 addresses
    startAddress -= 8;

    // Read data into buffer
    uint64_t flashRead[4];
    for (uint8_t i = 0; i < 4; i++) {

        // The way the flash is written in memory you need to read it out like this
        flashRead[i] = (((uint64_t)(*(startAddress + 1))) << 32) | ((uint64_t)(*startAddress));

        // Increment by 2 as we are reading 64 bits at a time
        startAddress += 2;
    }

    // Copy buffer across to structs
    CaptureTime->Start.Time.second = (uint8_t)((flashRead[0] >> 56) & 0xFF);
    CaptureTime->Start.Time.minute = (uint8_t)((flashRead[0] >> 48) & 0xFF);
    CaptureTime->Start.Time.hour   = (uint8_t)((flashRead[0] >> 40) & 0xFF);
    CaptureTime->Start.Date.day    = (uint8_t)((flashRead[0] >> 32) & 0xFF);
    CaptureTime->Start.Date.month  = (uint8_t)((flashRead[0] >> 24) & 0xFF);
    CaptureTime->Start.Date.year   = (uint16_t)((flashRead[0] >> 8) & 0xFFFF);

    CaptureTime->End.Time.second = (uint8_t)((flashRead[1] >> 56) & 0xFF);
    CaptureTime->End.Time.minute = (uint8_t)((flashRead[1] >> 48) & 0xFF);
    CaptureTime->End.Time.hour   = (uint8_t)((flashRead[1] >> 40) & 0xFF);
    CaptureTime->End.Date.day    = (uint8_t)((flashRead[1] >> 32) & 0xFF);
    CaptureTime->End.Date.month  = (uint8_t)((flashRead[1] >> 24) & 0xFF);
    CaptureTime->End.Date.year   = (uint16_t)((flashRead[1] >> 8) & 0xFFFF);

    CaptureTime->intervalSecond = (uint16_t)(flashRead[2] >> 48);
    CaptureTime->intervalMinute = (uint16_t)((flashRead[2] >> 32) & 0xFFFF);
    CaptureTime->intervalHour   = (uint16_t)((flashRead[2] >> 16) & 0xFFFF);
    CaptureTime->intervalDay    = (uint16_t)(flashRead[2] & 0xFFFF);

    CameraSettings->resolution = (uint8_t)((flashRead[3] >> 24) & 0xFF);
}