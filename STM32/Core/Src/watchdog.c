/**
 * @file watchdog.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-01-30
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <stdio.h>

/* Personal Includes */
#include "watchdog.h"
#include "ds18b20.h"
#include "hardware_config.h"
#include "log.h"
#include "comms_stm32.h"
#include "utilities.h"
#include "chars.h"
#include "help.h"
#include "stm32_rtc.h"
#include "watchdog_defines.h"

/* Private Macros */
#define ESP32_UART BUFFER_1_ID
#define MAPLE_UART BUFFER_2_ID
#define DEBUG

#define S0_READ_WATCHDOG_SETTINGS 1
#define S1_INIT_RTC               3
#define S2_SET_ALARM              4
#define S3_STM32_SLEEP            5
#define S4_RECORD_DATA            6
#define S5_SET_RTC_ALARM          7
#define S6_CHECK_INCOMING_REQUEST 8
#define S7_EXECUTE_REQUEST        9
#define S8_START_COUNT_DOWN       10
#define S9_STM32_WAKE             11

#define TIMEOUT         5000
#define COUNT_DOWN_TIME 5000

#define WATCHDOG_CODE_MESSAGE 0
#define WATCHDOG_CODE_TODO    1
#define WATCHDOG_CODE_ERROR   2

/* Private Variables */
dt_datetime_t datetime;

wd_camera_capture_time_settings_t captureTime = {
    .startTime.second    = 0,
    .startTime.minute    = 0,
    .startTime.hour      = 9,
    .endTime.second      = 0,
    .endTime.minute      = 0,
    .endTime.hour        = 15,
    .intervalTime.minute = 15,
    .intervalTime.hour   = 1,
};

uint8_t state    = S1_INIT_RTC;
uint8_t sleeping = FALSE;

/* Function Prototypes */
void watchdog_esp32_on(void);
void watchdog_esp32_off(void);
void watchdog_send_bpacket_to_esp32(bpacket_t* bpacket);
void watchdog_create_and_send_bpacket_to_esp32(const bp_request_t Request, const bp_code_t Code, uint8_t numBytes,
                                               uint8_t* data);
void watchdog_create_and_send_bpacket_to_maple(const bp_request_t Request, const bp_code_t Code, uint8_t numBytes,
                                               uint8_t* data);
uint8_t stm32_match_esp32_request(bpacket_t* bpacket);
uint8_t stm32_match_maple_request(bpacket_t* bpacket);
void process_watchdog_stm32_request(bpacket_t* bpacket);
void watchdog_report_success(const bp_request_t Request);
void watchdog_message_maple(char* string, uint8_t bpacketCode);
void watchdog_calculate_rtc_alarm_time(void);
void watchdog_request_esp32_take_photo(void);

void bpacket_print(bpacket_t* bpacket) {
    char msg[BPACKET_BUFFER_LENGTH_BYTES + 2];
    sprintf(msg, "Receiver: %i Sender: %i Request: %i Code: %i Num bytes: %i", bpacket->receiver, bpacket->sender,
            bpacket->request, bpacket->code, bpacket->numBytes);
    log_message(msg);

    if (bpacket->numBytes > 0) {
        for (int i = 0; i < bpacket->numBytes; i++) {
            sprintf(msg, "%c", bpacket->bytes[i]);
            log_message(msg);
        }
    }
}

void watchdog_init(void) {

    log_clear();

    // Initialise all the peripherals
    ds18b20_init();     // Temperature sensor
    comms_stm32_init(); // Bpacket communications between Maple and EPS32

    // Set the datetime to 12am 1 January 2023
    datetime.date.year   = 23;
    datetime.date.month  = 2;
    datetime.date.day    = 10;
    datetime.time.hour   = 7;
    datetime.time.minute = 54;
    datetime.time.second = 0;
    stm32_rtc_write_datetime(&datetime);

    // Turn the ESP32 on
    watchdog_esp32_on();

    // Get the capture time and resolution settings from the ESP32
    watchdog_message_maple("Reading watchdog settings\r\n", BPACKET_CODE_DEBUG);
    watchdog_create_and_send_bpacket_to_esp32(WD_BPK_R_GET_CAPTURE_TIME_SETTINGS, BP_CODE_EXECUTE, 0, NULL);
}

void watchdog_rtc_alarm_triggered(void) {
    state = S4_RECORD_DATA;
}

void watchdog_enter_state_machine(void) {

    while (1) {

        switch (state) {

            case S1_INIT_RTC:;
                // watchdog_message_maple("Initialising RTC\r\n", BPACKET_CODE_DEBUG);

                // Set the RTC time
                dt_datetime_t initDateTime;
                dt_date_init(&initDateTime.date, 1, 1, 2023);
                dt_time_init(&initDateTime.time, 0, 59, 8);
                stm32_rtc_write_datetime(&initDateTime);

                dt_datetime_t initAlarmDate;
                dt_date_init(&initAlarmDate.date, 1, 1, 2023);
                dt_time_init(&initAlarmDate.time, 0, 0, 9);
                stm32_rtc_set_alarmA(&initAlarmDate);

                state = S2_SET_ALARM;
                break;

            case S2_SET_ALARM:
                // watchdog_message_maple("Setting alarm\r\n", BPACKET_CODE_DEBUG);

                // TODO: Set the alarm on the RTC

                // Check if there were any requests
                if (comms_stm32_request_pending(MAPLE_UART) == TRUE) {
                    state = S7_EXECUTE_REQUEST;
                } else {
                    state = S3_STM32_SLEEP;
                }

                break;

            case S3_STM32_SLEEP:
                if (sleeping != TRUE) {
                    // watchdog_message_maple("Sleeping\r\n", BPACKET_CODE_DEBUG);
                    sleeping = TRUE;
                }

                // TODO: Set stm32 to sleep

                if ((comms_stm32_request_pending(MAPLE_UART) == TRUE) ||
                    (comms_stm32_request_pending(ESP32_UART) == TRUE)) {
                    state    = S7_EXECUTE_REQUEST;
                    sleeping = FALSE;
                    // watchdog_message_maple("Waking up\r\n", BPACKET_CODE_DEBUG);
                }

                break;

            case S4_RECORD_DATA:

                watchdog_request_esp32_take_photo();

                state = S3_STM32_SLEEP;

                break;

            case S5_SET_RTC_ALARM:;

                watchdog_message_maple("Upating RTC Alarm\r\n", BPACKET_CODE_DEBUG);
                watchdog_calculate_rtc_alarm_time();

                state = S6_CHECK_INCOMING_REQUEST;

                break;

            case S6_CHECK_INCOMING_REQUEST:
                // watchdog_message_maple("Checking incoming requests\r\n", BPACKET_CODE_DEBUG);

                if (comms_stm32_request_pending(MAPLE_UART) == TRUE) {
                    state = S7_EXECUTE_REQUEST;
                } else {
                    state = S3_STM32_SLEEP;
                }

                break;

            case S7_EXECUTE_REQUEST:;
                // watchdog_message_maple("Executing request\r\n", BPACKET_CODE_DEBUG);

                // Execute requests until none left
                uint32_t processNextPacket = TRUE;

                while (processNextPacket == TRUE) {

                    // Clear flag
                    processNextPacket = FALSE;

                    // Process anything Maple sends to the STM32
                    bpacket_t bpacket1, bpacket2;
                    if (comms_process_rxbuffer(BUFFER_2_ID, &bpacket1) == TRUE) {
                        watchdog_message_maple("Got request from maple\r\n", BPACKET_CODE_DEBUG);
                        if (stm32_match_maple_request(&bpacket1) != TRUE) {
                            process_watchdog_stm32_request(&bpacket1);
                        }

                        // Set flag incase there is another incoming packet afterwards
                        processNextPacket = TRUE;
                    }

                    // Process anything the ESP32 sends to Maple
                    if (comms_process_rxbuffer(BUFFER_1_ID, &bpacket2) == TRUE) {
                        watchdog_message_maple("Got request from ESP32\r\n", BPACKET_CODE_DEBUG);
                        if (stm32_match_esp32_request(&bpacket2) != TRUE) {
                            process_watchdog_stm32_request(&bpacket2);
                        }

                        // Set flag incase there is another incoming packet afterwards
                        processNextPacket = TRUE;
                    }
                }

                state = S8_START_COUNT_DOWN;
                break;

            case S8_START_COUNT_DOWN:;
                // watchdog_message_maple("Starting count down\r\n", BPACKET_CODE_DEBUG);

                uint32_t countDownEnd;
                uint8_t overflow1 = FALSE;
                if ((UINT_32_BIT_MAX_VALUE - SysTick->VAL) < COUNT_DOWN_TIME) {
                    countDownEnd = COUNT_DOWN_TIME - (UINT_32_BIT_MAX_VALUE - SysTick->VAL);
                    overflow1    = TRUE;
                } else {
                    countDownEnd = HAL_GetTick() + COUNT_DOWN_TIME;
                }

                while ((HAL_GetTick() <= countDownEnd) || (overflow1 == TRUE && countDownEnd < HAL_GetTick())) {

                    if (overflow1 == TRUE && countDownEnd <= HAL_GetTick()) {
                        overflow1 = FALSE;
                    }

                    // Continue waiting if there is no request from ESP32 or Maple
                    if ((comms_stm32_request_pending(MAPLE_UART) != TRUE) &&
                        (comms_stm32_request_pending(ESP32_UART) != TRUE)) {
                        continue;
                    }

                    state = S7_EXECUTE_REQUEST;
                    break;
                }

                if (state != S7_EXECUTE_REQUEST) {
                    state = S3_STM32_SLEEP;
                }
                // watchdog_message_maple("Exiting count down\r\n", BPACKET_CODE_DEBUG);
                break;

            default:;
                char errMsg[50];
                sprintf(errMsg, "Invalid state found: %i\r\n", state);
                watchdog_message_maple(errMsg, BPACKET_CODE_ERROR);
                break;
        }
    }
}

void watchdog_report_success(const bp_request_t Request) {

    bpacket_t bpacket;
    bp_create_packet(&bpacket, BP_ADDRESS_R_MAPLE, BP_ADDRESS_S_STM32, Request, BP_CODE_SUCCESS, 0, NULL);

    bpacket_buffer_t packetBuffer;
    bpacket_to_buffer(&bpacket, &packetBuffer);

    comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_create_and_send_bpacket_to_maple(const bp_request_t Request, const bp_code_t Code, uint8_t numBytes,
                                               uint8_t* data) {
    bpacket_t bpacket;
    bpacket_buffer_t packetBuffer;
    bp_create_packet(&bpacket, BP_ADDRESS_R_MAPLE, BP_ADDRESS_S_STM32, Request, Code, numBytes, data);
    bpacket_to_buffer(&bpacket, &packetBuffer);
    comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_create_and_send_bpacket_to_esp32(const bp_request_t Request, const bp_code_t Code, uint8_t numBytes,
                                               uint8_t* data) {
    bpacket_t bpacket;
    bpacket_buffer_t packetBuffer;
    bp_create_packet(&bpacket, BP_ADDRESS_R_ESP32, BP_ADDRESS_S_STM32, Request, Code, numBytes, data);
    bpacket_to_buffer(&bpacket, &packetBuffer);
    comms_transmit(ESP32_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_message_maple(char* string, uint8_t bpacketCode) {

    bpacket_t bpacket;
    bpacket_buffer_t packetBuffer;

    switch (bpacketCode) {

        case BPACKET_CODE_TODO:
        case BPACKET_CODE_SUCCESS:
        case BPACKET_CODE_DEBUG:
        case BPACKET_CODE_ERROR:
            bpacket_create_sp(&bpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32, BPACKET_GEN_R_MESSAGE,
                              bpacketCode, string);
            break;

        default:
            bpacket_create_sp(&bpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32, BPACKET_GEN_R_MESSAGE,
                              BPACKET_CODE_ERROR, "Failed to create watchdog message!\r\n");
    }

    bpacket_to_buffer(&bpacket, &packetBuffer);
    comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_send_bpacket_to_esp32(bpacket_t* bpacket) {
    bpacket_buffer_t bpacketBuffer;
    bpacket_to_buffer(bpacket, &bpacketBuffer);
    comms_transmit(ESP32_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);
}

void process_watchdog_stm32_request(bpacket_t* bpacket) {

    switch (bpacket->request) {

        case WATCHDOG_BPK_R_LED_RED_ON:

            if (bpacket->code == BPACKET_CODE_EXECUTE) {
                watchdog_create_and_send_bpacket_to_esp32(WD_BPK_R_LED_RED_ON, BP_CODE_EXECUTE, 0, NULL);
                break;
            }

            if (bpacket->code == BPACKET_CODE_SUCCESS) {
                watchdog_report_success(WD_BPK_R_LED_RED_ON);
                break;
            }

            watchdog_message_maple("Invalid code for LED on!\r\n", BPACKET_CODE_ERROR);

            break;

        case WATCHDOG_BPK_R_LED_RED_OFF:

            if (bpacket->code == BPACKET_CODE_EXECUTE) {
                watchdog_create_and_send_bpacket_to_esp32(WD_BPK_R_LED_RED_OFF, BP_CODE_EXECUTE, 0, NULL);
                break;
            }

            if (bpacket->code == BPACKET_CODE_SUCCESS) {
                watchdog_report_success(WD_BPK_R_LED_RED_OFF);
                break;
            }

            watchdog_message_maple("Invalid code for LED off!\r\n", BPACKET_CODE_ERROR);

            break;

        case BPACKET_GEN_R_PING:

            if (bpacket->code == BPACKET_CODE_EXECUTE) {
                uint8_t ping = WATCHDOG_PING_CODE_STM32;
                watchdog_create_and_send_bpacket_to_maple(BP_GEN_R_PING, BP_CODE_SUCCESS, 1, &ping);
                break;
            }

            watchdog_message_maple("Invalid code for Ping!\r\n", BPACKET_CODE_ERROR);

            break;

        case BPACKET_GEN_R_MESSAGE:
            log_send_bdata(bpacket->bytes, bpacket->numBytes);
            break;

        default:;
            char bpacketInfo[80];
            bpacket_get_info(bpacket, bpacketInfo);
            char errorMsg[100];
            sprintf(errorMsg, "Unknown request: %s\r\n", bpacketInfo);
            watchdog_message_maple(errorMsg, BPACKET_CODE_ERROR);
    }
}

uint8_t stm32_match_esp32_request(bpacket_t* bpacket) {

    switch (bpacket->request) {

        case WATCHDOG_BPK_R_TAKE_PHOTO: // ESP32 Response to request from STM32 to take a photo

            if (bpacket->code == BPACKET_CODE_SUCCESS) { // Photo was succesfully taken

#ifdef DEBUG
                watchdog_report_success(WD_BPK_R_TAKE_PHOTO);
#endif

                // TODO: Need to update RTC Alarm
                watchdog_message_maple("Need to update RTC alarm\r\n\0", BPACKET_CODE_TODO);
                break;
            }

            // If photo could not be taken, print error code to Maple
            if (bpacket->code == BPACKET_CODE_ERROR) {
                watchdog_create_and_send_bpacket_to_maple(WD_BPK_R_TAKE_PHOTO, BP_CODE_ERROR, bpacket->numBytes,
                                                          bpacket->bytes);
                break;
            }

            break;

        case WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS:

            // Log success to Maple
            if (bpacket->code == BPACKET_CODE_SUCCESS) {

                // Store watchdog settings in struct
                uint8_t result = wd_bpacket_to_capture_time_settings(bpacket, &captureTime);

                if (result != TRUE) {
                    watchdog_message_maple("Failed to convert bpacket to settings\r\n", BPACKET_CODE_ERROR);
                    break;
                }

                // // Update the RTC time
                watchdog_calculate_rtc_alarm_time();

                // Report success
                watchdog_report_success(WD_BPK_R_GET_CAPTURE_TIME_SETTINGS);
                break;
            }

            // If reading the watchdog settings fails, keep default settings
            if (bpacket->code == BPACKET_CODE_ERROR) {
                watchdog_message_maple("Failed to read Watchdog settings\r\n", BPACKET_CODE_ERROR);
                break;
            }

            watchdog_message_maple("ESP32 returned unknown code when request to get wd settings", BPACKET_CODE_ERROR);

            break;

        case WATCHDOG_BPK_R_SET_CAPTURE_TIME_SETTINGS:

            // Log success to Maple
            if (bpacket->code == BPACKET_CODE_SUCCESS) {
                watchdog_report_success(WD_BPK_R_SET_CAPTURE_TIME_SETTINGS);

                // Request read of capture time from ESP32 to update the settings on the STM32
                watchdog_create_and_send_bpacket_to_esp32(WD_BPK_R_GET_CAPTURE_TIME_SETTINGS, BP_CODE_EXECUTE, 0, NULL);
                break;
            }

            // Log failure to Maple
            if (bpacket->code == BPACKET_CODE_ERROR) {
                watchdog_message_maple("Failed to write settings to ESP32", BPACKET_CODE_ERROR);
            }

            watchdog_message_maple("ESP32 returned invalid code when udpating wd settings", BPACKET_CODE_ERROR);

            break;

        default:
            return FALSE;
    }

    return TRUE;
}

void watchdog_calculate_rtc_alarm_time(void) {

    // Check whether the current RTC time is before or after the current capture time state time
    dt_datetime_t currentDateTime;
    stm32_rtc_read_datetime(&currentDateTime);

    if (dt_time_t1_leq_t2(&currentDateTime.time, &captureTime.startTime) == TRUE) {

        // Set the alarm to be the start time of the capture time
        dt_datetime_t alarmA = {
            .time.second = 0,
            .time.minute = captureTime.startTime.minute,
            .time.hour   = captureTime.startTime.hour,
            .date.day    = currentDateTime.date.day,
            .date.month  = currentDateTime.date.month,
            .date.year   = currentDateTime.date.year,
        };
        stm32_rtc_set_alarmA(&alarmA);

    } else if (dt_time_t1_leq_t2(&captureTime.endTime, &currentDateTime.time) == TRUE) {

        // Increment to the following day
        dt_datetime_increment_day(&currentDateTime);

        // Set the alarm to be the start time
        dt_datetime_t alarmA = {
            .time.second = 0,
            .time.minute = captureTime.startTime.minute,
            .time.hour   = captureTime.startTime.hour,
            .date.day    = currentDateTime.date.day,
            .date.month  = currentDateTime.date.month,
            .date.year   = currentDateTime.date.year,
        };

        stm32_rtc_set_alarmA(&alarmA);

    } else {

        dt_time_t alarmTime;
        dt_time_init(&alarmTime, captureTime.startTime.second, captureTime.startTime.minute,
                     captureTime.startTime.hour);

        // The next alarm is somewhere in between the start time and the end time
        while (1) {

            // Add interval time to the start time
            dt_time_add_time(&alarmTime, captureTime.intervalTime);

            // Check whether the time is now after the current RTC time
            if (dt_time_t1_leq_t2(&currentDateTime.time, &alarmTime) == TRUE) {
                // Set the alarm time
                dt_datetime_t alarmA = {
                    .time.second = 0,
                    .time.minute = alarmTime.minute,
                    .time.hour   = alarmTime.hour,
                    .date.day    = currentDateTime.date.day,
                    .date.month  = currentDateTime.date.month,
                    .date.year   = currentDateTime.date.year,
                };
                stm32_rtc_set_alarmA(&alarmA);
                break;
            }
        }
    }
}

uint8_t stm32_match_maple_request(bpacket_t* bpacket) {

    bpacket_buffer_t bpacketBuffer;
    uint8_t request = bpacket->request;
    uint8_t result;

    switch (bpacket->request) {

        case WATCHDOG_BPK_R_TAKE_PHOTO: // Send command to ESP32 to take a photo

            watchdog_request_esp32_take_photo();

            break;

        case WATCHDOG_BPK_R_GET_DATETIME:

            // Get the datetime from the RTC
            stm32_rtc_read_datetime(&datetime);
            if (wd_datetime_to_bpacket(bpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32, request,
                                       BPACKET_CODE_SUCCESS, &datetime) == TRUE) {
                bpacket_to_buffer(bpacket, &bpacketBuffer);
                comms_transmit(MAPLE_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);
            } else {
                watchdog_message_maple("Failed to convert datetime to bpacket\0", BPACKET_CODE_ERROR);
            }

            break;

        case WATCHDOG_BPK_R_SET_DATETIME:

            // Set the datetime using the information from the bpacket
            if (wd_bpacket_to_datetime(bpacket, &datetime) == TRUE) {

                stm32_rtc_write_datetime(&datetime);

                watchdog_report_success(WD_BPK_R_SET_DATETIME);

                // Recalculate the RTC alarm
                watchdog_calculate_rtc_alarm_time();

            } else {
                watchdog_message_maple("Failed to convert bpacket to datetime\r\n", BPACKET_CODE_ERROR);
            }

            break;

        case WATCHDOG_BPK_R_GET_STATUS:;

            // Currently what the status does is prints the gets the current date time and the current
            // RTC alarm time and sends that back
            dt_datetime_t currentDateTime, alarmATime;
            stm32_rtc_read_datetime(&currentDateTime);
            stm32_rtc_read_alarmA(&alarmATime);

            char cdtString[40];
            char atStr[40];
            stm32_rtc_format_datetime(&currentDateTime, cdtString);
            stm32_rtc_format_datetime(&alarmATime, atStr);

            // Combine the strings together
            char message[110];
            sprintf(message, "Current Time: %s, RTC Alarm: %s\r\n", cdtString, atStr);

            bpacket_t statusBpacket;
            bpacket_create_sp(&statusBpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32, WATCHDOG_BPK_R_GET_STATUS,
                              BPACKET_CODE_SUCCESS, message);
            bpacket_to_buffer(&statusBpacket, &bpacketBuffer);
            comms_transmit(MAPLE_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);

            break;

        case WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS:;

            // Send the watchdog settings back to maple
            bpacket_t settingsPacket;
            result = wd_capture_time_settings_to_bpacket(&settingsPacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32,
                                                         WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, BPACKET_CODE_SUCCESS,
                                                         &captureTime);

            if (result != TRUE) {
                watchdog_message_maple("Failed to convert settings to bpacket\r\n", BPACKET_CODE_ERROR);
                break;
            } else {
                watchdog_message_maple("Converted settings to bpacket. Sending to maple\r\n", BPACKET_CODE_DEBUG);
            }

            // Send the bpacket to maple
            bpacket_to_buffer(&settingsPacket, &bpacketBuffer);
            comms_transmit(MAPLE_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);

            break;

        case WATCHDOG_BPK_R_SET_CAPTURE_TIME_SETTINGS:;

            // Update the bpacket addresses to redirect the request to the ESP32
            bpacket->receiver = BPACKET_ADDRESS_ESP32;
            bpacket->sender   = BPACKET_ADDRESS_STM32;

            watchdog_send_bpacket_to_esp32(bpacket);

            break;

        case WATCHDOG_BPK_R_TURN_ON:
            watchdog_esp32_on();
            watchdog_create_and_send_bpacket_to_maple(WD_BPK_R_TURN_ON, BP_CODE_SUCCESS, 0, NULL);
            break;

        case WATCHDOG_BPK_R_TURN_OFF:
            watchdog_esp32_off();
            watchdog_create_and_send_bpacket_to_maple(WD_BPK_R_TURN_OFF, BP_CODE_SUCCESS, 0, NULL);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

void watchdog_request_esp32_take_photo(void) {

    // Record the current temperature from both temperature sensors
    if (ds18b20_read_temperature(DS18B20_SENSOR_ID_1) != TRUE) {
        watchdog_message_maple("Failed to read temperature", BPACKET_CODE_ERROR);
    }

    ds18b20_temp_t temp1;
    if (ds18b20_copy_temperature(DS18B20_SENSOR_ID_1, &temp1) != TRUE) {
        watchdog_message_maple("Failed to copy temperature", BPACKET_CODE_ERROR);
    }

    if (ds18b20_read_temperature(DS18B20_SENSOR_ID_2) != TRUE) {
        watchdog_message_maple("Failed to read temperature", BPACKET_CODE_ERROR);
    }

    ds18b20_temp_t temp2;
    if (ds18b20_copy_temperature(DS18B20_SENSOR_ID_2, &temp2) != TRUE) {
        watchdog_message_maple("Failed to copy temperature", BPACKET_CODE_ERROR);
    }

    // Update the datetime struct
    stm32_rtc_read_datetime(&datetime);

    // Get the real time clock time and date. Put it in a packet and send to the ESP32
    bpacket_t photoRequest;

    uint8_t result =
        wd_photo_data_to_bpacket(&photoRequest, BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_STM32, WATCHDOG_BPK_R_TAKE_PHOTO,
                                 BPACKET_CODE_EXECUTE, &datetime, &temp1, &temp2);

    if (result == TRUE) {
        watchdog_send_bpacket_to_esp32(&photoRequest);
    } else {
        char errMsg[50];
        wd_get_error(result, errMsg);
        char msg[130];
        sprintf(msg, "Failed to convert photo data to bpacket with error: %s\r\n", errMsg);
        watchdog_message_maple(msg, BPACKET_CODE_ERROR);
    }
}

void watchdog_update(void) {

    // // Process anything the ESP32 sends to the STM32
    // if (comms_process_rxbuffer(BUFFER_1_ID, &esp32Bpacket) == TRUE) {
    //     if (stm32_match_esp32_request(&esp32Bpacket) != TRUE) {
    //         process_watchdog_stm32_request(&esp32Bpacket);
    //     }
    // }

    // // Process anything Maple sends to the STM32
    // if (comms_process_rxbuffer(BUFFER_2_ID, &mapleBpacket) == TRUE) {
    //     if (stm32_match_maple_request(&mapleBpacket) != TRUE) {
    //         process_watchdog_stm32_request(&mapleBpacket);
    //     }
    // }
}

void watchdog_esp32_on(void) {
    ESP32_POWER_PORT->BSRR |= (0x01 << ESP32_POWER_PIN);

    // Delay for 1.5 seconds to let ESP32 startup
    HAL_Delay(1500);
}

void watchdog_esp32_off(void) {
    ESP32_POWER_PORT->BSRR |= (0x10000 << ESP32_POWER_PIN);
}