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

char errorMsg[150];
uint8_t gbl_mapleConnected = FALSE;

/* Function Prototypes */
uint8_t usb_read_packet(bpk_t* Bpacket);
uint8_t wd_request_photo_capture(uint8_t errorCode[1]);

uint8_t wd_write_settings(capture_time_t* CaptureTime, camera_settings_t* CameraSettings);
void wd_read_settings(capture_time_t* CaptureTime, camera_settings_t* CameraSettings);

void wd_handle_esp_response(bpk_t* Bpacket);
void wd_handle_maple_request(bpk_t* Bpacket);
void wd_write_bpacket_maple(bpk_t* Bpacket);
void wd_write_bpacket_esp(bpk_t* Bpacket);
uint8_t wd_calculate_next_alarm_time(dt_datetime_t* AlarmDatetime);

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

void wd_error_handler1(char* fileName, uint32_t lineNumber, uint8_t code) {

    while (1) {
        GPIO_TOGGLE(LED_RED_PORT, LED_RED_PIN);
        log_usb_error("Error in %s on line %i. Code: %i\r\n", fileName, lineNumber, code);
        HAL_Delay(1000);
    }
}

void wd_esp_turn_on(void) {
    GPIO_SET_LOW(ESP32_POWER_PORT, ESP32_POWER_PIN);

    // Set the event bit to signify the ESP now has power
    event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE);

    // TODO: Enable the interrupt on the UART line for the ESP32
    HAL_NVIC_EnableIRQ(UART_ESP32_IRQn);

    /* Start timeout for ping */

    /* ###### START DEBUGGING BLOCK ###### */
    // Description:
    // log_usb_message("ESP now on");
    /* ####### END DEBUGGING BLOCK ####### */
}

void wd_esp_turn_off(void) {

    // Disable the initerrupt on the UART line for the ESP32
    HAL_NVIC_DisableIRQ(UART_ESP32_IRQn);

    GPIO_SET_HIGH(ESP32_POWER_PORT, ESP32_POWER_PIN);

    // Clear the esp ready and on flags
    event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE);
    event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_READY, EGT_ACTIVE);

    /* ###### START DEBUGGING BLOCK ###### */
    // Description:
    // log_usb_message("ESP now off");

    GPIO_SET_LOW(PA0_PORT, PA0_PIN);
    GPIO_SET_LOW(PA1_PORT, PA1_PIN);
    /* ####### END DEBUGGING BLOCK ####### */
}

void alarm_test() {
    /****** START CODE BLOCK ******/
    // Description: Testing. Can delete when finished with it
    dt_datetime_t AlarmDatetime;
    // rtc_set_alarm(&AlarmDatetime);
    // rtc_enable_alarm();
    dt_datetime_t RtcDatetime = {
        .Time.second = 0,
        .Time.minute = 0,
        .Time.hour   = 0,
        .Date.day    = 14,
        .Date.month  = 7,
        .Date.year   = 2023,
    };
    rtc_write_datetime(&RtcDatetime);

    dt_datetime_init(&gbl_CaptureTime.Start, 0, 0, 9, 1, 1, 2023);
    dt_datetime_init(&gbl_CaptureTime.End, 0, 30, 15, 1, 1, 2030);
    gbl_CaptureTime.intervalSecond = 1;
    gbl_CaptureTime.intervalMinute = 0;
    gbl_CaptureTime.intervalHour   = 0;
    gbl_CaptureTime.intervalDay    = 1;

    AlarmDatetime.Date.day   = gbl_CaptureTime.Start.Date.day;
    AlarmDatetime.Date.month = gbl_CaptureTime.Start.Date.month;
    AlarmDatetime.Date.year  = gbl_CaptureTime.Start.Date.year;
    HAL_Delay(2000);
    int i = 0;
    while (i < 40) {
        wd_calculate_next_alarm_time(&AlarmDatetime);

        log_usb_message("New alarm: %i:%i:%i %i/%i/%i\r\n", AlarmDatetime.Time.second, AlarmDatetime.Time.minute,
                        AlarmDatetime.Time.hour, AlarmDatetime.Date.day, AlarmDatetime.Date.month,
                        AlarmDatetime.Date.year);

        rtc_write_datetime(&AlarmDatetime);
        i++;
    }

    while (1) {}
    /****** END CODE BLOCK ******/
}

void software_config_init(void) {
    uart_init(); // Initialise COMMS with the esp32

    ds18b20_init(); // Initialise the temperature sensor
}

void wd_start(void) {

    // Initialise all the hardware
    hardware_clock_config();
    hardware_config_init();

    /* Initialise software */
    software_config_init();

    uint8_t code = rtc_init();
    if (code != TRUE) {
        wd_error_handler1(__FILE__, __LINE__, code);
    }

    // Read the watchdog settings from flash
    wd_read_settings(&gbl_CaptureTime, &lg_CameraSettings);

    /* Confirm the settings are valid */
    uint8_t errorFound = FALSE;

    /* TODO: Check the rest of the values */

    if (lg_CameraSettings.frameSize > WD_CR_UXGA_1600x1200) {
        lg_CameraSettings.frameSize = WD_CR_UXGA_1600x1200;
        errorFound                  = TRUE;
    }

    if (lg_CameraSettings.jpegCompression > 63) {
        lg_CameraSettings.jpegCompression = 8;
        errorFound                        = TRUE;
    }

    if (errorFound) {
        wd_write_settings(&gbl_CaptureTime, &lg_CameraSettings);
    }

    /* Confirm the start, end and interval times are valid */
    if (dt_time_is_valid(&gbl_CaptureTime.Start.Time) != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    if (dt_time_is_valid(&gbl_CaptureTime.End.Time) != TRUE) {
        wd_error_handler(__FILE__, __LINE__);
    }

    /* ###### START DEBUGGING BLOCK ###### */
    // Description:
    // log_usb_message("Start time: %i:%i:%i %i/%i/%i\r\n", gbl_CaptureTime.Start.Time.second,
    //                 gbl_CaptureTime.Start.Time.minute, gbl_CaptureTime.Start.Time.hour,
    //                 gbl_CaptureTime.Start.Date.day, gbl_CaptureTime.Start.Date.month,
    //                 gbl_CaptureTime.Start.Date.year);
    // log_usb_message("End time: %i:%i:%i %i/%i/%i\r\n", gbl_CaptureTime.End.Time.second,
    // gbl_CaptureTime.End.Time.minute,
    //                 gbl_CaptureTime.End.Time.hour, gbl_CaptureTime.End.Date.day, gbl_CaptureTime.End.Date.month,
    //                 gbl_CaptureTime.End.Date.year);
    // log_usb_message("Interval: sec: %i min: %i hour: %i day: %i\r\n", gbl_CaptureTime.intervalSecond,
    //                 gbl_CaptureTime.intervalMinute, gbl_CaptureTime.intervalHour, gbl_CaptureTime.intervalDay);

    // dt_datetime_t Rtc;
    // if (rtc_read_datetime(&Rtc) != TRUE) {
    //     wd_error_handler(__FILE__, __LINE__);
    // }
    // log_usb_message("RTC Time: %i:%i:%i %i/%i/%i\r\n", Rtc.Time.hour, Rtc.Time.minute, Rtc.Time.second, Rtc.Date.day,
    //                 Rtc.Date.month, Rtc.Date.year);

    /* Calculate when the next alarm should be and set the RTC alarm for that */

    dt_datetime_t AlarmTime1;
    wd_calculate_next_alarm_time(&AlarmTime1);
    rtc_set_alarm(&AlarmTime1);
    rtc_enable_alarm();

    /* ###### START DEBUGGING BLOCK ###### */
    // Description:
    // log_usb_message("Alarm time set to: %i:%i:%i %i/%i/%i\r\n", AlarmTime1.Time.hour, AlarmTime1.Time.minute,
    //                 AlarmTime1.Time.second, AlarmTime1.Date.day, AlarmTime1.Date.month, AlarmTime1.Date.year);
    /* ####### END DEBUGGING BLOCK ####### */

    // Initialise event groups
    event_group_clear(&gbl_EventsStm);
    event_group_clear(&gbl_EventsEsp);

    // Set bit to update the rtc alarm
    event_group_set_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE);

    /* DEBUGGING PURPOSES I HAVE COMMENTED THIS OUT BUT YOU WILL NEED TO COMMENT IT BACK IN*/
    // Set flag 'take photo' flag. In main loop below, STM32 will request ESP to
    // take a photo if this flag has been set. Getting ESP32 to take a photo on
    // startup to ensure the system is working correctly
    // event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);
    // event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_STM_REQUEST);

    // Enter main loop
    bpk_t BpkEspResponse, MapleBpacket;
    uint8_t lastRTCSecond = 0;

    /* ###### START DEBUGGING BLOCK ###### */
    // Description:
    GPIO_SET_LOW(PA0_PORT, PA0_PIN);
    GPIO_SET_LOW(PA1_PORT, PA1_PIN);
    /* ####### END DEBUGGING BLOCK ####### */

    while (1) {

        /* Turn the ESP on or off depending on if there are esp tasks that need to occur */

        // The conditions the ESP should turn on are
        uint8_t espTurnOn = FALSE;
        if ((event_group_get_bits(&gbl_EventsEsp, EGT_ACTIVE) &
             ~((0x01 << EVENT_ESP_ON) | (0x01 << EVENT_ESP_READY))) != 0) {
            espTurnOn = TRUE;
        }

        /* When the USB is connected, the ESP32 should turn on and be ready for any commands
            request by the computer */
        if (GPIO_PIN_IS_HIGH(USBC_CONN_PORT, USBC_CONN_PIN)) {
            espTurnOn = TRUE;
        }

        // TODO: Come up with a better way to do this because really we should not be turning the esp
        // on but we should be preventing it from being turned off
        if ((event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_EVENT_RUNNING)) == TRUE) {
            GPIO_SET_HIGH(PA1_PORT, PA1_PIN);
            espTurnOn = TRUE;
        }

        // Check if there are any esp flags that are set that are not ESP_ON or ESP_READY
        if (espTurnOn) {

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

            // Disable HAL tick
            // usb_denit();
            // HAL_SuspendTick();
            // HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

            // // Device now in sleep mode.Wakes up by interrupt from RTC HAL_Init();
            // hardware_clock_config();
            // hardware_config_init();
            // software_config_init();
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
            if (rtc_read_alarm_datetime(&AlarmTime, 0) != TRUE) {
                wd_error_handler(__FILE__, __LINE__);
            }

            // TODO: Calculate the next alarm time more efficiently
            wd_calculate_next_alarm_time(&AlarmTime);

            // Check if the system still needs to take photos
            if (dt1_greater_than_dt2(&AlarmTime, &gbl_CaptureTime.End) == TRUE) {
                rtc_disable_alarm();
                log_usb_message("Alarm disabled\r\n");
            } else {
                rtc_set_alarm(&AlarmTime);
                rtc_enable_alarm(); // Ensure the alarm is enabled

                /* Check if the USB is connected. If it is connected, we want to send the alarm time if it has
                changed since the last time it was sent */
                if (GPIO_PIN_IS_HIGH(USBC_CONN_PORT, USBC_CONN_PIN)) {

                    /* Packetise the RTC alarm and send to Maple */
                    uint8_t alarmData[7] = {
                        AlarmTime.Time.second, AlarmTime.Time.minute,    AlarmTime.Time.hour,        AlarmTime.Date.day,
                        AlarmTime.Date.month,  AlarmTime.Date.year >> 8, AlarmTime.Date.year & 0xFF,
                    };

                    // Commenting this out for debugging purposes
                    bpk_t BpkNextAlarm;
                    bpk_create(&BpkNextAlarm, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Get_Rtc_Alarm,
                               BPK_Code_Success, 7, alarmData);
                    wd_write_bpacket_maple(&BpkNextAlarm);
                }
            }
        }

        // /* Process all packets received from the ESP32 */
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

            // Uncommented for testing. Comment back in when ready
            uint8_t i = 0;
            GPIO_SET_HIGH(LED_GREEN_PORT, LED_GREEN_PIN);
            while (i < 5) {
                GPIO_TOGGLE(LED_GREEN_PORT, LED_GREEN_PIN);
                HAL_Delay(100);
                i++;
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
            // Uncommented for testing
            GPIO_SET_LOW(LED_GREEN_PORT, LED_GREEN_PIN);
        }

        /* Request STM to go to sleep if there is no current requests for the STM or the ESP */
        if ((event_group_get_bits(&gbl_EventsStm, EGT_ACTIVE) == 0) &&
            (event_group_get_bits(&gbl_EventsEsp, EGT_ACTIVE) == 0) &&
            (GPIO_PIN_IS_LOW(USBC_CONN_PORT, USBC_CONN_PIN)) == TRUE) {

            // event_group_set_bit(&gbl_EventsStm, EVENT_STM_SLEEP, EGT_ACTIVE);
        }
    }
}

void wd_handle_maple_request(bpk_t* Bpacket) {

    // Create a bpacket to store the response to send back to Maple
    static bpk_t BpkStmResponse;

    if (Bpacket->Request.val == BPK_REQ_COPY_FILE) {
        /****** START CODE BLOCK ******/
        // Description: Debugging. Can delete whenever
        bpk_t DebugMessage;
        bpk_create_sp(&DebugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message, BPK_Code_Success,
                      "STM ack Maple req Copy file");
        wd_write_bpacket_maple(&DebugMessage);
        /****** END CODE BLOCK ******/

        // Forward message to ESP32
        bpk_create(&BpkStmResponse, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Stm32, BPK_Req_Copy_File, BPK_Code_Execute,
                   Bpacket->Data.numBytes, Bpacket->Data.bytes);
        wd_write_bpacket_esp(&BpkStmResponse);

        return;
    }

    if (Bpacket->Request.val == BPK_REQ_DELETE_FILE) {
        /****** START CODE BLOCK ******/
        // Description: Debugging. Can delete whenever
        bpk_t DebugMessage;
        bpk_create_sp(&DebugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message, BPK_Code_Success,
                      "STM ack Maple req delete file");
        wd_write_bpacket_maple(&DebugMessage);
        /****** END CODE BLOCK ******/

        // Forward message to ESP32
        bpk_create(&BpkStmResponse, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Stm32, BPK_Req_Delete_File, BPK_Code_Execute,
                   Bpacket->Data.numBytes, Bpacket->Data.bytes);
        wd_write_bpacket_esp(&BpkStmResponse);

        return;
    }

    if (Bpacket->Request.val == BPK_REQ_TAKE_PHOTO) {

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

        return;
    }

    if (Bpacket->Request.val == BPK_REQUEST_PING) {
        // Create response
        static uint8_t stmPingCode = 47;
        if (bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Ping, BPK_Code_Success,
                       1, &stmPingCode) != TRUE) {
            wd_error_handler(__FILE__, __LINE__);
        }

        // Send bpacket to Maple
        wd_write_bpacket_maple(&BpkStmResponse);

        return;
    }

    switch (Bpacket->Request.val) {

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

            // Update the alarm time incase the alarm settings have changed
            event_group_set_bit(&gbl_EventsStm, EVENT_STM_RTC_UPDATE_ALARM, EGT_ACTIVE);

            break;

        case BPK_REQ_GET_WATCHDOG_SETTINGS:;

            /* Store current watchdog settings into bpacket and send to computer */
            uint8_t settingsData[WD_NUM_SETTINGS_BYTES];
            wd_utils_settings_to_array(settingsData, &gbl_CaptureTime, &lg_CameraSettings);

            /* Create bpacket and send to computer */
            bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Get_Watchdog_Settings,
                       BPK_Code_Success, WD_NUM_SETTINGS_BYTES, settingsData);
            wd_write_bpacket_maple(&BpkStmResponse);

            /* Maple requests the watchdog settings on startup. Currently the RTC gets sent automatically when
                it updates which most likely won't be when Maple connects so to ensure Maple always has the
                correct RTC alarm time, will also send it once here when Maple requests the watchdog settings */
            // TODO: Come up with a more elegant way to do this.
            dt_datetime_t AlarmTime;
            if (rtc_read_alarm_datetime(&AlarmTime, 0) != TRUE) {
                wd_error_handler(__FILE__, __LINE__);
            }

            uint8_t alarmData[7] = {
                AlarmTime.Time.second, AlarmTime.Time.minute,    AlarmTime.Time.hour,        AlarmTime.Date.day,
                AlarmTime.Date.month,  AlarmTime.Date.year >> 8, AlarmTime.Date.year & 0xFF,
            };

            bpk_t BpkNextAlarm;
            bpk_create(&BpkNextAlarm, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Req_Get_Rtc_Alarm,
                       BPK_Code_Success, 7, alarmData);
            wd_write_bpacket_maple(&BpkNextAlarm);

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
        // bpk_t debugMessage;
        // bpk_create_sp(&debugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
        // BPK_Code_Success,
        //               "ESP is ready");
        // wd_write_bpacket_maple(&debugMessage);
        log_usb_success("ESP now ready\r\n");
        GPIO_SET_HIGH(LED_RED_PORT, LED_RED_PIN);

        /****** END CODE BLOCK ******/
        return;
    }

    if (Bpacket->Request.val == BPK_REQ_DELETE_FILE) {
        // Forward the bpacket to maple
        Bpacket->Receiver = BPK_Addr_Receive_Maple;
        wd_write_bpacket_maple(Bpacket);

        return;
    }

    if (Bpacket->Request.val == BPK_REQ_COPY_FILE) {
        /****** START CODE BLOCK ******/
        // Description: Debugging. Can delete whenever
        // bpk_t DebugMessage;
        // bpk_create_sp(&DebugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
        // BPK_Code_Success,
        //               "STM received copy file data back");
        // wd_write_bpacket_maple(&DebugMessage);
        /****** END CODE BLOCK ******/

        // Forward the bpacket to maple
        Bpacket->Receiver = BPK_Addr_Receive_Maple;
        wd_write_bpacket_maple(Bpacket);
        return;
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
            while (1) {
                for (int i = 0; i < Bpacket->Data.numBytes; i++) {
                    log_usb_error("%c", Bpacket->Data.bytes[i]);
                }
                log_usb_message("\r\n");
                HAL_Delay(1000);
                GPIO_TOGGLE(LED_RED_PORT, LED_RED_PIN);
            }
            /****** END CODE BLOCK ******/
            wd_error_handler_2(__FILE__, __LINE__);
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
    uint8_t data[sizeof(dt_datetime_t) + sizeof(camera_settings_t) + sizeof(float)];
    wd_utils_photo_data_to_array(data, &Datetime, &lg_CameraSettings, Ds18b20.temp);

    bpk_t BpkTakePhoto;
    bpk_create(&BpkTakePhoto, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Stm32, BPK_Req_Take_Photo, BPK_Code_Execute,
               sizeof(dt_datetime_t) + sizeof(camera_settings_t) + sizeof(float), data);
    wd_write_bpacket_esp(&BpkTakePhoto);

    // Set the appropriate event bits
    event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_EVENT_RUNNING);

    // Start timeout
    et_set_timeout(&lg_Timeouts, EVENT_ESP_TAKE_PHOTO, 10000);
    /* ###### START DEBUGGING BLOCK ###### */
    // Description: Debugging. Can remove when not needed
    // bpk_t debugMessage;
    // if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_STM_REQUEST) == TRUE) {
    //     bpk_create_sp(&debugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
    //     BPK_Code_Success,
    //                   "STM32 Requesting photo");
    // } else {
    //     bpk_create_sp(&debugMessage, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message,
    //     BPK_Code_Success,
    //                   "Maple Requesting photo");
    // }
    // wd_write_bpacket_maple(&debugMessage);
    /* ####### END DEBUGGING BLOCK ####### */

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
            (((uint64_t)CameraSettings->jpegCompression) << 16) | (((uint64_t)CameraSettings->flashEnabled) << 8) |
            ((uint64_t)0xFF),
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
        log_usb_message("Setting default\r\n");
        /* Set the default settings such that system will take a photo twice a day */
        CaptureTime->Start.Date.day    = 1;
        CaptureTime->Start.Date.month  = 1;
        CaptureTime->Start.Date.year   = 2023;
        CaptureTime->Start.Time.second = 0;
        CaptureTime->Start.Time.minute = 0;
        CaptureTime->Start.Time.hour   = 5; // Start time set to 5am

        CaptureTime->End.Date.day    = 1;
        CaptureTime->End.Date.month  = 1;
        CaptureTime->End.Date.year   = 2030;
        CaptureTime->End.Time.second = 0;
        CaptureTime->End.Time.minute = 0;
        CaptureTime->End.Time.hour   = 18; // End time set to 6pm

        CaptureTime->intervalSecond = 0;
        CaptureTime->intervalMinute = 1;
        CaptureTime->intervalHour   = 0;
        CaptureTime->intervalDay    = 0;

        CameraSettings->frameSize       = WD_CR_UXGA_1600x1200;
        CameraSettings->jpegCompression = 8;
        CameraSettings->flashEnabled    = FALSE;

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
    CameraSettings->flashEnabled    = (uint8_t)((flashRead[3] >> 8) & 0xFF);
}

uint8_t wd_calculate_next_alarm_time(dt_datetime_t* AlarmDatetime) {

    // Read the rtc time
    dt_datetime_t Rtc;
    if (rtc_read_datetime(&Rtc) != TRUE) {
        return FALSE;
    }

    // Set the date of the alarm to be the current date on the real time clock
    AlarmDatetime->Date.day   = Rtc.Date.day;
    AlarmDatetime->Date.month = Rtc.Date.month;
    AlarmDatetime->Date.year  = Rtc.Date.year;

    // Set the time of the alarm to be the start time
    if (dt_time_init(&AlarmDatetime->Time, gbl_CaptureTime.Start.Time.second, gbl_CaptureTime.Start.Time.minute,
                     gbl_CaptureTime.Start.Time.hour) != TRUE) {
        return FALSE;
    }

    /****** START CODE BLOCK ******/
    // Description: Debugging. Can delete whenever
    // log_usb_message("RTC Time: %i:%i:%i %i/%i/%i\r\n", Rtc.Time.second, Rtc.Time.minute, Rtc.Time.hour, Rtc.Date.day,
    //                 Rtc.Date.month, Rtc.Date.year);
    // // log_usb_message("RTC Time: %i:%i:%i\r\n", RtcTime.second, RtcTime.minute, RtcTime.hour);
    /****** END CODE BLOCK ******/

    /* Need to check whether the alarm time should actually be later than the start time */
    while (1) {

        // RTC time less than current alarm time so the alarm time is set correctly
        if (dt1_less_than_dt2(&Rtc, AlarmDatetime) == TRUE) {
            // log_usb_message("Breaking\r\n");
            break;
        }

        // Rtc time >= current alarm time. Need to advance the current alarm time by the
        // given time interval
        dt_datetime_add_time(AlarmDatetime, gbl_CaptureTime.intervalSecond, gbl_CaptureTime.intervalMinute,
                             gbl_CaptureTime.intervalHour, gbl_CaptureTime.intervalDay);

        // If the current alarm time is now > the end time then need to set the alarm time
        // to the start time and exit
        if (dt_t1_greater_than_t2(&AlarmDatetime->Time, &gbl_CaptureTime.End.Time) == TRUE) {

            // Increment the alarm time day
            dt_datetime_add_time(AlarmDatetime, 0, 0, 0, 1);

            if (dt_time_init(&AlarmDatetime->Time, gbl_CaptureTime.Start.Time.second, gbl_CaptureTime.Start.Time.minute,
                             gbl_CaptureTime.Start.Time.hour) != TRUE) {
                return FALSE;
            }

            break;
        }
    }

    return TRUE;
}