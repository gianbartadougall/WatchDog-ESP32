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
#include "ds18b20.h"
#include "integer.h"
#include "float.h"
#include "event_timeout_stm.h"

/* Private Enums */

/* Private Macros */
#define WD_FLASH_SETTINGS_START_ADDRESS 0x0803F800
#define WD_FLASH_SETTINGS_END_ADDRESS   0x0803F8BC

/* Public Variables */
event_group_t gbl_EventsStm, gbl_EventsEsp;
et_timeout_t lg_Timeouts;

/* Private Variables */
capture_time_t gbl_CaptureTime;
camera_settings_t lg_CameraSettings;

ds18b20_t Ds18b20 = {
    .rom  = DS18B20_ROM_1,
    .temp = 0,
};

/* Function Prototypes */
uint8_t usb_read_packet(bpk_t* Bpacket);
uint8_t wd_request_photo_capture(uint8_t errorCode[1]);

uint8_t wd_write_settings(capture_time_t* CaptureTime, camera_settings_t* CameraSettings);
void wd_read_settings(capture_time_t* CaptureTime, camera_settings_t* CameraSettings);

void wd_handle_esp_response(bpk_t* Bpacket);
void wd_handle_maple_request(bpk_t* Bpacket);
void wd_write_bpacket_maple(bpk_t* Bpacket);
void wd_write_bpacket_esp(bpk_t* Bpacket);

void i2c_test(void) {

    if (dt_datetime_init(&gbl_CaptureTime.Start, 0, 0, 6, 5, 7, 2023) != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    if (dt_datetime_init(&gbl_CaptureTime.End, 0, 0, 6, 31, 12, 2023) != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    gbl_CaptureTime.intervalSecond = 30;
    gbl_CaptureTime.intervalMinute = 0;
    gbl_CaptureTime.intervalHour   = 0;
    gbl_CaptureTime.intervalDay    = 0;

    if (rtc_init() != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    if (rtc_write_datetime(&gbl_CaptureTime.Start) != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    dt_datetime_t Alarm;
    if (dt_datetime_init(&Alarm, 10, 0, 6, 5, 7, 2023) != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    dt_datetime_t bTime;
    if (rtc_read_datetime(&bTime) != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    if (rtc_enable_alarm() != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    if (rtc_set_alarm(&Alarm) != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    dt_datetime_t aTime;
    if (rtc_read_alarm_datetime(&aTime) != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    log_usb_success("Alarm set to %i:%i:%i %i/%i/%i\tTime set to %i:%i:%i %i/%i/%i \r\n", aTime.Time.hour,
                    aTime.Time.minute, aTime.Time.second, aTime.Date.day, aTime.Date.month, aTime.Date.year,
                    bTime.Time.hour, bTime.Time.minute, bTime.Time.second, bTime.Date.day, bTime.Date.month,
                    bTime.Date.year);

    dt_datetime_t Datetime;
    uint8_t lastSecond = 0;

    while (1) {

        // if (rtc_read_datetime(&Datetime) != TRUE) {
        //     wd_error_handler(__FILE__, __LINE__);
        // }

        // if (Datetime.Time.second != lastSecond) {
        //     lastSecond = Datetime.Time.second;
        //     log_usb_success("%i:%i:%i %i/%i/%i\r\n", Datetime.Time.hour, Datetime.Time.minute, Datetime.Time.second,
        //                     Datetime.Date.day, Datetime.Date.month, Datetime.Date.year);
        // }

        if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);

            if (rtc_read_datetime(&Datetime) != TRUE) {
                wd_error_handler(__FILE__, __LINE__);
            }

            log_usb_success("Photo Captured! @ %i:%i:%i %i/%i/%i \r\n", Datetime.Time.hour, Datetime.Time.minute,
                            Datetime.Time.second, Datetime.Date.day, Datetime.Date.month, Datetime.Date.year);
        }

        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE);

            // Read the the current alarm time
            dt_datetime_t AlarmTime;
            if (rtc_read_alarm_datetime(&AlarmTime) != TRUE) {
                wd_error_handler(__FILE__, __LINE__);
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

void wd_handle_timeouts(void) {

    bpk_t BpkStm32Response;

    if (et_timeout_has_occured(&lg_Timeouts, EVENT_ESP_TAKE_PHOTO) == TRUE) {

        if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_STM_REQUEST) == FALSE) {
            bpk_create_sp(&BpkStm32Response, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
                          BPK_Code_Success, "ESP Take photo Timed out");
            wd_write_bpacket_maple(&BpkStm32Response);
        } else {
            log_usb_error("ESP Take photo timeout occured!\r\n");
        }

        // Clear all necessary event bits
        et_clear_timeout(&lg_Timeouts, EVENT_ESP_TAKE_PHOTO);
        event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);
        event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_STM_REQUEST);
        event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_EVENT_RUNNING);
    }
}

void photo_test(void) {

    event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_STM_REQUEST);

    lg_CameraSettings.frameSize = WD_CR_QVGA_320x240;
    bpk_t bpk3;

    while (1) {

        if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_READY, EGT_ACTIVE) == TRUE) {
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_EVENT_RUNNING) == FALSE) {
                // Update the resolution
                lg_CameraSettings.frameSize++;
                if (lg_CameraSettings.frameSize > WD_CR_UXGA_1600x1200) {
                    lg_CameraSettings.frameSize = WD_CR_QVGA_320x240;
                }

                uint8_t errorCode[1];
                if (wd_request_photo_capture(errorCode) != TRUE) {
                    log_usb_error("Failed to request photo. Error %i\r\n", errorCode[0]);
                } else {
                    log_usb_message("Requesting another photo\r\n");
                }
            }
        }

        if (uart_read_bpacket(0, &bpk3) == TRUE) {
            wd_handle_esp_response(&bpk3);
        }

        wd_handle_timeouts();

        // if (usb_read_packet(&bpk2) == TRUE) {
        //     if (bpk2.Request.val == BPK_REQUEST_PING) {
        //         // Create response
        //         static uint8_t stmPingCode = 47;
        //         if (bpk_create(&bpk3, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Ping,
        //         BPK_Code_Success,
        //                        1, &stmPingCode) != TRUE) {
        //             wd_error_handler(__FILE__, __LINE__);
        //             break;
        //         }

        //         // Send bpacket to Maple
        //         wd_write_bpacket_maple(&bpk3);
        //     }
        // }
    }
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

void wd_esp_turn_on(void) {
    GPIO_SET_LOW(ESP32_POWER_PORT, ESP32_POWER_PIN);

    // Set the event bit to signify the ESP now has power
    event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE);

    /* Start timeout for ping */
}

void wd_esp_turn_off(void) {
    GPIO_SET_HIGH(ESP32_POWER_PORT, ESP32_POWER_PIN);

    // Clear the esp ready and on flags
    event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE);
    event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_READY, EGT_ACTIVE);
}

void wd_start(void) {

    // Initialise all the hardware
    hardware_config_init();

    /* Initialise software */
    uart_init();    // Initialise COMMS with the esp32
    ds18b20_init(); // Initialise the temperature sensor

    // TODO: Change the task scheduler timer. TIM15 is used for the ds18b20 so it cannot
    // be used for the task scheduler. Will need to find a different one. Reenable timer
    // in hardware config.c as I commented it out
    // task_scheduler_init(); // Reset Recipes list

    if (rtc_init() != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    // Read the watchdog settings from flash
    wd_read_settings(&gbl_CaptureTime, &lg_CameraSettings);

    /****** START CODE BLOCK ******/
    // Description: Debugging. Remove when uneeded
    // photo_test();
    /****** END CODE BLOCK ******/

    // Initialise event groups
    event_group_clear(&gbl_EventsStm);
    event_group_clear(&gbl_EventsEsp);

    // TODO: Confirm the start, end and interval times just read are valid

    // TODO: Calcaulte when the next alarm should be and set the RTC alarm for that

    // TODO: Confirm the camera settings are valid

    // Set flag 'take photo' flag. In main loop below, STM32 will request ESP to
    // take a photo if this flag has been set. Getting ESP32 to take a photo on
    // startup to ensure the system is working correctly
    event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);
    event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_STM_REQUEST);

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
        if ((event_group_get_bits(&gbl_EventsEsp, EGT_ACTIVE) &
             ~((0x01 << EVENT_ESP_ON) | (0x01 << EVENT_ESP_READY))) != 0) {

            // There is an esp event that needs to occur. Ensure the ESP has been turned on
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE) != TRUE) {
                wd_esp_turn_on();
            }
        } else {

            // Only turn the ESP off is the USB is not connected
            if (GPIO_PIN_IS_HIGH(USBC_CONN_PORT, USBC_CONN_PIN) == FALSE) {
                // There are no pending esp events. Turn the ESP off if it is not already off
                if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE) == TRUE) {
                    wd_esp_turn_off();
                }
            }
        }

        /* Put the STM32 to sleep if required */
        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_SLEEP, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_SLEEP, EGT_ACTIVE);

            // TODO: Put the STM32 to sleep
        }

        /* Request ESP32 to take a photo if the 'take photo' flag has been set */
        if ((event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE) == TRUE) &&
            (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_EVENT_RUNNING) == FALSE)) {

            // Don't request a photo until the ESP is in the ready state. The ESP takes
            // a second or two to startup after it has been turned on. The ESP_READY flag
            // will be set automatically by the STM when the ESP is ready to receive commands
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_READY, EGT_ACTIVE) == TRUE) {
                event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);

                uint8_t errorCode[1];
                if (wd_request_photo_capture(errorCode) != TRUE) {
                    wd_error_handler_2("Error %s on line %i. Code %i\r\n", __FILE__, __LINE__, errorCode[0]);
                }
            }
        }

        /* Check whether the RTC alarm needs to be updated. This flag is set in the ISR that is triggered
            whenever the alarm time in the RTC matches the RTC time */
        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE);

            /* Calculate the next alarm time */
            dt_datetime_t AlarmTime;
            if (rtc_read_alarm_datetime(&AlarmTime) != TRUE) {
                wd_error_handler(__FILE__, __LINE__);
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

            /* When the USB is connected, the ESP32 should turn on and be ready for any commands
                request by the computer */
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE) == FALSE) {

                // Turn the ESP on. This will also set the EVENT_ESP_ON flag
                wd_esp_turn_on();

                /****** START CODE BLOCK ******/
                // Description: For the moment, the ESP is always on because the PCB for the ESP
                // doesn't work. Given this, we will send a ping request instead of turning the
                // esp on (as it is always on). The response from the ping request will be the
                // same as if the ESP has just turned on and the EVENT_ESP_ON and EVENT_ESP_READY
                // flags should be set
                bpk_t PingRequest;
                bpk_create(&PingRequest, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Stm32, BPK_Request_Ping,
                           BPK_Code_Execute, 0, NULL);
                wd_write_bpacket_esp(&PingRequest);
                /****** END CODE BLOCK ******/
            }

            // USBC is connected, check for any incoming bpackets and process them if any come
            if (usb_read_packet(&MapleBpacket) == TRUE) {
                wd_handle_maple_request(&MapleBpacket);
            }

            /* Send the datetime to the computer to display on GUI */
            dt_datetime_t dt;
            if (rtc_read_datetime(&dt) != TRUE) {
                wd_error_handler(__FILE__, __LINE__);
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
                wd_error_handler(__FILE__, __LINE__);
                break;
            }

            // Send bpacket to Maple
            wd_write_bpacket_maple(&BpkStmResponse);

            break;

        case BPK_REQ_LIST_DIR:;

            /****** START CODE BLOCK ******/
            // Description: Debugging. Can delete whenever
            bpk_t DebugMessage;
            bpk_create_sp(&DebugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
                          BPK_Code_Success, "STM ack Maple req list dir");
            wd_write_bpacket_maple(&DebugMessage);
            /****** END CODE BLOCK ******/

            bpk_create(&BpkStmResponse, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Stm32, BPK_Req_List_Dir, BPK_Code_Execute,
                       0, NULL);
            wd_write_bpacket_esp(&BpkStmResponse);
            break;

        case BPK_REQ_TAKE_PHOTO:;

            /****** START CODE BLOCK ******/
            // Description: Testing this bit of code. Can delete if necessary
            uint8_t occured = FALSE;
            bpk_t debugMessage;
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE) == FALSE) {
                bpk_create_sp(&debugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
                              BPK_Code_Success, "Maple request but ESP is not on");
                wd_write_bpacket_maple(&debugMessage);
                occured = TRUE;
            }
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_READY, EGT_ACTIVE) == FALSE) {
                bpk_create_sp(&debugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
                              BPK_Code_Success, "Maple request but ESP is not ready");
                wd_write_bpacket_maple(&debugMessage);
                occured = TRUE;
            }

            if (occured == TRUE) {
                return;
            }
            /****** END CODE BLOCK ******/

            /* Set the required event group bits */
            event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);
            event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_STM_REQUEST);
            event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_EVENT_RUNNING);

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
                wd_error_handler(__FILE__, __LINE__);
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
            uint8_t settingsData[20];
            wd_utils_settings_to_array(settingsData, &gbl_CaptureTime, &lg_CameraSettings);

            /* Create bpacket and send to computer */
            bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Get_Watchdog_Settings,
                       BPK_Code_Success, 20, settingsData);
            wd_write_bpacket_maple(&BpkStmResponse);

            break;

        case BPK_REQ_GET_TEMPERATURE:;

            if (ds18b20_read_temperature(&Ds18b20) != TRUE) {
                wd_error_handler(__FILE__, __LINE__);
            }

            uint8_t tempData[sizeof(float)];
            float_to_u8(tempData, Ds18b20.temp);
            bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Get_Temperature,
                       BPK_Code_Success, sizeof(float), tempData);
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

void wd_write_bpacket_esp(bpk_t* Bpacket) {
    static bpk_buffer_t Buffer;
    bpk_to_buffer(Bpacket, &Buffer);
    uart_transmit_data(UART_ESP32, Buffer.buffer, Buffer.numBytes);
}

void wd_handle_esp_response(bpk_t* Bpacket) {

    bpk_t BpkStmResponse;

    if (Bpacket->Request.val == BPK_REQUEST_PING) {

        /* Clear timeout */

        // Ping from ESP received => ESP ready to take requests. Settings both event
        // bits just incase ESP_ON hasn't been set for some reason
        event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE);
        event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_READY, EGT_ACTIVE);

        /****** START CODE BLOCK ******/
        // Description: Debugging purposes. Can delete when not required
        bpk_t debugMessage;
        bpk_create_sp(&debugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message, BPK_Code_Success,
                      "ESP is ready");
        wd_write_bpacket_maple(&debugMessage);
        /****** END CODE BLOCK ******/

        log_usb_success("Received ESP ping!\r\n");
    }

    if (Bpacket->Request.val == BPK_REQ_TAKE_PHOTO) {

        // Clear the timeout as a response has been received
        et_clear_timeout(&lg_Timeouts, EVENT_ESP_TAKE_PHOTO);

        // Clear necessary event bits
        event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_EVENT_RUNNING);

        // Determine whether the STM triggered the request or not
        if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_STM_REQUEST) == FALSE) {

            if (Bpacket->Code.val != BPK_CODE_SUCCESS) {
                bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Take_Photo,
                           BPK_Code_Error, 0, NULL);
            } else {
                bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Take_Photo,
                           BPK_Code_Success, 0, NULL);
            }

            wd_write_bpacket_maple(&BpkStmResponse);
            return;
        }

        if (Bpacket->Code.val != BPK_CODE_SUCCESS) {
            /****** START CODE BLOCK ******/
            // Description: Debug. Can delete when done
            for (int i = 0; i < Bpacket->Data.numBytes; i++) {
                log_usb_success("%c", Bpacket->Data.bytes[i]);
            }
            log_usb_message("\r\n");
            /****** END CODE BLOCK ******/

            wd_error_handler(__FILE__, __LINE__);
        }

        /****** START CODE BLOCK ******/
        // Description: Debug messaging. Can delete when confident everything is working correctly
        log_usb_success("Took a photo\r\n!");
        for (int i = 0; i < Bpacket->Data.numBytes; i++) {
            log_usb_success("%c", Bpacket->Data.bytes[i]);
        }
        log_usb_message("\r\n");

        /****** END CODE BLOCK ******/

        return;
    }

    if (Bpacket->Request.val == BPK_REQ_LIST_DIR) {

        /****** START CODE BLOCK ******/
        // Description: Debugging. Can delete whenever
        bpk_t DebugMessage;
        bpk_create_sp(&DebugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message, BPK_Code_Success,
                      "STM received dir list back");
        wd_write_bpacket_maple(&DebugMessage);
        /****** END CODE BLOCK ******/

        // Forward the bpacket to maple
        Bpacket->Receiver = BPK_Addr_Receive_Maple;
        wd_write_bpacket_maple(Bpacket);
        return;
    }
}

void wd_error_handler(char* fileName, uint16_t lineNumber) {

    // TODO: Turn the ESP off

    while (1) {

        HAL_Delay(1000);
        GPIO_TOGGLE(LED_RED_PORT, LED_RED_PIN);

        log_usb_error("Error code in %s on line %i\r\n", fileName, lineNumber);
    }
}

uint8_t wd_request_photo_capture(uint8_t errorCode[1]) {

    if (ds18b20_read_temperature(&Ds18b20) != TRUE) {
        errorCode[0] = bpkErrRecordTemp[0];
        return FALSE;
    }

    dt_datetime_t Datetime;
    if (rtc_read_datetime(&Datetime) != TRUE) {
        errorCode[0] = bpkErrReadDatetime[0];
        return FALSE;
    }

    /* Store temperature and datetime into bpacket and send request to the ESP */
    uint8_t data[sizeof(dt_datetime_t) + sizeof(float)];
    wd_utils_photo_data_to_array(data, &Datetime, &lg_CameraSettings, Ds18b20.temp);

    bpk_t BpkTakePhoto;
    bpk_create(&BpkTakePhoto, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Stm32, BPK_Req_Take_Photo, BPK_Code_Execute,
               sizeof(dt_datetime_t) + sizeof(float), data);
    wd_write_bpacket_esp(&BpkTakePhoto);

    // Set the appropriate event bits
    event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_EVENT_RUNNING);

    // Start timeout
    et_set_timeout(&lg_Timeouts, EVENT_ESP_TAKE_PHOTO, 6000);

    /****** START CODE BLOCK ******/
    // Description: Debugging. Can remove when not needed
    bpk_t debugMessage;
    if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_STM_REQUEST) == TRUE) {
        bpk_create_sp(&debugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message, BPK_Code_Success,
                      "STM32 Requesting photo");
    } else {
        bpk_create_sp(&debugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message, BPK_Code_Success,
                      "Maple Requesting photo");
    }
    wd_write_bpacket_maple(&debugMessage);
    /****** END CODE BLOCK ******/

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
        ((uint64_t)0xFFFFFFFF << 32) | (((uint64_t)CameraSettings->frameSize) << 24) |
            (((uint64_t)CameraSettings->jpegCompression) << 16) | 0x0000FFFF,
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
                wd_error_handler(__FILE__, __LINE__);
            }

            FLASH_EraseInitTypeDef eraseConfig;
            eraseConfig.TypeErase = FLASH_TYPEERASE_PAGES;
            eraseConfig.Page      = 127;
            eraseConfig.NbPages   = 1; // Number of pages to be erased

            uint32_t pageError = 0;
            if (HAL_FLASHEx_Erase(&eraseConfig, &pageError) != HAL_OK) {
                wd_error_handler(__FILE__, __LINE__);
            }

            if (HAL_FLASH_Lock() != HAL_OK) {
                wd_error_handler(__FILE__, __LINE__);
            }
        }
    }

    /* Write data to memory */
    if (HAL_FLASH_Unlock() != HAL_OK) {
        wd_error_handler(__FILE__, __LINE__);
    }

    for (uint8_t i = 0; i < 4; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)startAddress, settingsData[i]) != HAL_OK) {
            wd_error_handler(__FILE__, __LINE__);
        }

        // Increment by 2 because we just wrote two 32 bit values to the flash
        startAddress += 2;
    }

    if (HAL_FLASH_Lock() != HAL_OK) {
        wd_error_handler(__FILE__, __LINE__);
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

        CameraSettings->frameSize       = WD_CR_UXGA_1600x1200;
        CameraSettings->jpegCompression = 0;

        /* These are now the default settings. Write them to the flash */
        if (wd_write_settings(CaptureTime, CameraSettings) != TRUE) {
            wd_error_handler(__FILE__, __LINE__);
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

    CameraSettings->frameSize       = (uint8_t)((flashRead[3] >> 24) & 0xFF);
    CameraSettings->jpegCompression = (uint8_t)((flashRead[3] >> 16) & 0xFF);
}