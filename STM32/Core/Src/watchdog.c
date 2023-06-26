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

/* STM32 Includes */
#include "usb_device.h"
#include "usbd_cdc_if.h"

/* Personal Includes */
#include "watchdog.h"
#include "ds18b20.h"
#include "hardware_config.h"
#include "log.h"
#include "comms_stm32.h"
#include "utils.h"
#include "chars.h"
#include "help.h"
#include "stm32_rtc.h"
#include "watchdog_defines.h"
#include "stm32_uart.h"
#include "cbuffer.h"

/* Private Macros */
#define USART_ESP   USART1
#define USART_MAPLE USART2
#define ESP32_UART  BUFFER_1_ID
#define MAPLE_UART  BUFFER_2_ID
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

cbuffer_t RxBufferMaple;
#define RX_BUF_MAPLE_MAX_NUM_BYTES (BPACKET_MAX_NUM_DATA_BYTES * 2)
uint8_t rxBufMapleBytes[RX_BUF_MAPLE_MAX_NUM_BYTES];

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

uint8_t state          = S1_INIT_RTC;
uint8_t sleeping       = FALSE;
uint8_t esp32Connected = FALSE;

/* Function Prototypes */
void watchdog_esp32_on(void);
void watchdog_esp32_off(void);
void watchdog_send_bpacket_to_esp32(bpk_t* Bpacket);
void watchdog_create_and_send_bpacket_to_esp32(const bpk_request_t Request, const bpk_code_t Code, uint8_t numBytes,
                                               uint8_t* data);
void watchdog_create_and_send_bpacket_to_maple(const bpk_request_t Request, const bpk_code_t Code, uint8_t numBytes,
                                               uint8_t* data);
uint8_t stm32_match_esp32_request(bpk_t* Bpacket);
uint8_t stm32_match_maple_request(bpk_t* Bpacket);
void process_watchdog_stm32_request(bpk_t* Bpacket);
void watchdog_report_success(const bpk_request_t Request);
void watchdog_message_maple(char* string, bpk_code_t Code);
void watchdog_calculate_rtc_alarm_time(void);
void watchdog_request_esp32_take_photo(void);

void bpacket_print(bpk_t* Bpacket) {
    char msg[BPACKET_BUFFER_LENGTH_BYTES + 2];
    sprintf(msg, "Receiver: %i Sender: %i Request: %i Code: %i Num bytes: %i", Bpacket->Receiver.val,
            Bpacket->Sender.val, Bpacket->Request.val, Bpacket->Code.val, Bpacket->Data.numBytes);
    log_message(msg);

    if (Bpacket->Data.numBytes > 0) {
        for (int i = 0; i < Bpacket->Data.numBytes; i++) {
            sprintf(msg, "%c", Bpacket->Data.bytes[i]);
            log_message(msg);
        }
    }
}

void watchdog_trasmit_byte_to_maple(uint8_t byte) {
    uart_transmit_data(USART_MAPLE, &byte, 1);
}

void watchdog_init(void) {

    // Initialise log
    log_init(uart_transmit_string, NULL);

    log_clear();

    // Initialise all the peripherals
    ds18b20_init(); // Temperature sensor
    uart_init();    // Bpacket communications between Maple and EPS32

    // Set the datetime to 12am 1 January 2023
    datetime.Date.year   = 23;
    datetime.Date.month  = 2;
    datetime.Date.day    = 10;
    datetime.Time.hour   = 7;
    datetime.Time.minute = 54;
    datetime.Time.second = 0;
    stm32_rtc_write_datetime(&datetime);

    // Turn the ESP32 on
    watchdog_esp32_on();

    // Get the capture time and resolution settings from the ESP32
    watchdog_message_maple("Reading watchdog settings\r\n", BPK_Code_Debug);
    watchdog_create_and_send_bpacket_to_esp32(BPK_Req_Get_Camera_Capture_Times, BPK_Code_Execute, 0, NULL);
}

void watchdog_rtc_alarm_triggered(void) {
    state = S4_RECORD_DATA;
}

void watchdog_enter_state_machine(void) {

    while (1) {

        switch (state) {

            case S1_INIT_RTC:;
                // watchdog_message_maple("Initialising RTC\r\n", BPK_Code_Debug);

                // Set the RTC time
                dt_datetime_t initDateTime;
                dt_date_init(&initDateTime.Date, 1, 1, 2023);
                dt_time_init(&initDateTime.Time, 0, 59, 8);
                stm32_rtc_write_datetime(&initDateTime);

                dt_datetime_t initAlarmDate;
                dt_date_init(&initAlarmDate.Date, 1, 1, 2023);
                dt_time_init(&initAlarmDate.Time, 0, 0, 9);
                stm32_rtc_set_alarmA(&initAlarmDate);

                state = S2_SET_ALARM;
                break;

            case S2_SET_ALARM:
                // watchdog_message_maple("Setting alarm\r\n", BPK_Code_Debug);

                // TODO: Set the alarm on the RTC

                // Check if there were any requests
                if (uart_buffer_not_empty(MAPLE_UART) == TRUE) {
                    state = S7_EXECUTE_REQUEST;
                } else {
                    state = S3_STM32_SLEEP;
                }

                break;

            case S3_STM32_SLEEP:
                if (sleeping != TRUE) {
                    // watchdog_message_maple("Sleeping\r\n", BPK_Code_Debug);
                    sleeping = TRUE;
                }

                // TODO: Set stm32 to sleep

                if ((uart_buffer_not_empty(MAPLE_UART) == TRUE) || (uart_buffer_not_empty(ESP32_UART) == TRUE)) {
                    state    = S7_EXECUTE_REQUEST;
                    sleeping = FALSE;
                    // watchdog_message_maple("Waking up\r\n", BPK_Code_Debug);
                }

                break;

            case S4_RECORD_DATA:

                watchdog_request_esp32_take_photo();

                state = S3_STM32_SLEEP;

                break;

            case S5_SET_RTC_ALARM:;

                watchdog_message_maple("Upating RTC Alarm\r\n", BPK_Code_Debug);
                watchdog_calculate_rtc_alarm_time();

                state = S6_CHECK_INCOMING_REQUEST;

                break;

            case S6_CHECK_INCOMING_REQUEST:
                // watchdog_message_maple("Checking incoming requests\r\n", BPK_Code_Debug);

                if (uart_buffer_not_empty(MAPLE_UART) == TRUE) {
                    state = S7_EXECUTE_REQUEST;
                } else {
                    state = S3_STM32_SLEEP;
                }

                break;

            case S7_EXECUTE_REQUEST:;
                // watchdog_message_maple("Executing request\r\n", BPK_Code_Debug);

                // Execute requests until none left
                uint32_t processNextPacket = TRUE;

                while (processNextPacket == TRUE) {

                    // Clear flag
                    processNextPacket = FALSE;

                    // Process anything Maple sends to the STM32
                    bpk_t bpacket1, bpacket2;
                    if (uart_process_rxbuffer(BUFFER_2_ID, &bpacket1) == TRUE) {
                        // log_message("Bpacket: %i %i %i %i %i %i\r\n", bpacket1.Receiver.val, bpacket1.Sender.val,
                        //             bpacket1.Request.val, bpacket1.Code.val, bpacket1.Data.numBytes,
                        //             bpacket1.Data.bytes[0]);

                        // watchdog_message_maple("Got request from maple\r\n", BPK_Code_Debug);
                        if (stm32_match_maple_request(&bpacket1) != TRUE) {
                            process_watchdog_stm32_request(&bpacket1);
                        }

                        // Set flag incase there is another incoming packet afterwards
                        processNextPacket = TRUE;
                    }

                    // Process anything the ESP32 sends to Maple
                    if (uart_process_rxbuffer(BUFFER_1_ID, &bpacket2) == TRUE) {
                        // watchdog_message_maple("Got request from ESP32\r\n", BPK_Code_Debug);
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
                // watchdog_message_maple("Starting count down\r\n", BPK_Code_Debug);

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
                    if ((uart_buffer_not_empty(MAPLE_UART) != TRUE) && (uart_buffer_not_empty(ESP32_UART) != TRUE)) {
                        continue;
                    }

                    state = S7_EXECUTE_REQUEST;
                    break;
                }

                if (state != S7_EXECUTE_REQUEST) {
                    state = S3_STM32_SLEEP;
                }
                // watchdog_message_maple("Exiting count down\r\n", BPK_Code_Debug);
                break;

            default:;
                char errMsg[50];
                sprintf(errMsg, "Invalid state found: %i\r\n", state);
                watchdog_message_maple(errMsg, BPK_Code_Error);
                break;
        }
    }
}

void watchdog_report_success(const bpk_request_t Request) {

    bpk_t Bpacket;
    bpk_create(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, Request, BPK_Code_Success, 0, NULL);

    bpk_buffer_t packetBuffer;
    bpk_to_buffer(&Bpacket, &packetBuffer);

    uart_write(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_create_and_send_bpacket_to_maple(const bpk_request_t Request, const bpk_code_t Code, uint8_t numBytes,
                                               uint8_t* data) {
    bpk_t Bpacket;
    bpk_buffer_t packetBuffer;
    bpk_create(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, Request, Code, numBytes, data);
    bpk_to_buffer(&Bpacket, &packetBuffer);
    uart_write(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_create_and_send_bpacket_to_esp32(const bpk_request_t Request, const bpk_code_t Code, uint8_t numBytes,
                                               uint8_t* data) {
    bpk_t Bpacket;
    bpk_buffer_t packetBuffer;
    bpk_create(&Bpacket, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Stm32, Request, Code, numBytes, data);
    bpk_to_buffer(&Bpacket, &packetBuffer);
    uart_write(ESP32_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_message_maple(char* string, bpk_code_t Code) {

    bpk_t Bpacket;
    bpk_buffer_t packetBuffer;

    switch (Code.val) {

        case BPK_CODE_TODO:
        case BPK_CODE_SUCCESS:
        case BPK_CODE_DEBUG:
        case BPK_CODE_ERROR:
            bpk_create_sp(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message, Code, string);
            break;

        default:
            bpk_create_sp(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Message, BPK_Code_Error,
                          "Failed to create watchdog message!\r\n");
    }

    bpk_to_buffer(&Bpacket, &packetBuffer);
    uart_write(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_send_bpacket_to_esp32(bpk_t* Bpacket) {
    bpk_buffer_t bpacketBuffer;
    bpk_to_buffer(Bpacket, &bpacketBuffer);
    uart_write(ESP32_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);
}

void process_watchdog_stm32_request(bpk_t* Bpacket) {

    switch (Bpacket->Request.val) {

        case BPK_REQ_LED_RED_ON:

            if (Bpacket->Code.val == BPK_Code_Execute.val) {
                watchdog_create_and_send_bpacket_to_esp32(BPK_Req_Led_Red_On, BPK_Code_Execute, 0, NULL);
                break;
            }

            if (Bpacket->Code.val == BPK_Code_Success.val) {
                watchdog_report_success(BPK_Req_Led_Red_On);
                break;
            }

            watchdog_message_maple("Invalid code for LED on!\r\n", BPK_Code_Error);

            break;

        case BPK_REQ_LED_RED_OFF:

            if (Bpacket->Code.val == BPK_Code_Execute.val) {
                watchdog_create_and_send_bpacket_to_esp32(BPK_Req_Led_Red_Off, BPK_Code_Execute, 0, NULL);
                break;
            }

            if (Bpacket->Code.val == BPK_Code_Success.val) {
                watchdog_report_success(BPK_Req_Led_Red_Off);
                break;
            }

            watchdog_message_maple("Invalid code for LED off!\r\n", BPK_Code_Error);

            break;

        case BPK_REQUEST_PING:

            if (Bpacket->Code.val == BPK_Code_Execute.val) {
                uint8_t ping = WATCHDOG_PING_CODE_STM32;
                watchdog_create_and_send_bpacket_to_maple(BPK_Request_Ping, BPK_Code_Success, 1, &ping);
                break;
            }

            watchdog_message_maple("Invalid code for Ping!\r\n", BPK_Code_Error);

            break;

        case BPK_REQUEST_MESSAGE:
            uart_transmit_data(USART_MAPLE, Bpacket->Data.bytes, Bpacket->Data.numBytes);
            break;

        default:;
            char bpacketInfo[80];
            bpk_get_info(Bpacket, bpacketInfo);
            char errorMsg[120];
            sprintf(errorMsg, "Error %s line %i. %s\r\n", __FILE__, __LINE__, bpacketInfo);
            watchdog_message_maple(errorMsg, BPK_Code_Error);
    }
}

uint8_t stm32_match_esp32_request(bpk_t* Bpacket) {

    switch (Bpacket->Request.val) {

        case BPK_REQUEST_PING:
            esp32Connected = TRUE;
            LED_GREEN_PORT->BSRR |= (0x01 << LED_GREEN_PIN); // Turn green LED on
            break;

        case BPK_REQ_TAKE_PHOTO: // ESP32 Response to request from STM32 to take a photo

            if (Bpacket->Code.val == BPK_Code_Success.val) { // Photo was succesfully taken

#ifdef DEBUG
                watchdog_report_success(BPK_Req_Take_Photo);
#endif

                // TODO: Need to update RTC Alarm
                watchdog_message_maple("Need to update RTC alarm\r\n\0", BPK_Code_Todo);
                break;
            }

            // If photo could not be taken, print error code to Maple
            if (Bpacket->Code.val == BPK_Code_Error.val) {
                watchdog_create_and_send_bpacket_to_maple(BPK_Req_Take_Photo, BPK_Code_Error, Bpacket->Data.numBytes,
                                                          Bpacket->Data.bytes);
                break;
            }

            break;

        case BPK_REQ_GET_CAMERA_CAPTURE_TIMES:

            // Log success to Maple
            if (Bpacket->Code.val == BPK_Code_Success.val) {

                // Store watchdog settings in struct
                uint8_t result = wd_bpacket_to_capture_time_settings(Bpacket, &captureTime);

                if (result != TRUE) {
                    watchdog_message_maple("Failed to convert Bpacket to settings\r\n", BPK_Code_Error);
                    break;
                }

                // // Update the RTC time
                watchdog_calculate_rtc_alarm_time();

                // Report success
                watchdog_report_success(BPK_Req_Get_Camera_Capture_Times);
                break;
            }

            // If reading the watchdog settings fails, keep default settings
            if (Bpacket->Code.val == BPK_Code_Error.val) {
                watchdog_message_maple("Failed to read Watchdog settings\r\n", BPK_Code_Error);
                break;
            }

            watchdog_message_maple("ESP32 returned unknown code when request to get wd settings", BPK_Code_Error);

            break;

        case BPK_REQ_SET_CAMERA_CAPTURE_TIMES:

            // Log success to Maple
            if (Bpacket->Code.val == BPK_Code_Success.val) {
                watchdog_message_maple("Received capture time response from ESP32", BPK_Code_Debug);
                watchdog_report_success(BPK_Req_Set_Camera_Capture_Times);

                // Request read of capture time from ESP32 to update the settings on the STM32
                watchdog_create_and_send_bpacket_to_esp32(BPK_Req_Get_Camera_Capture_Times, BPK_Code_Execute, 0, NULL);
                break;
            }

            // Log failure to Maple
            if (Bpacket->Code.val == BPK_Code_Error.val) {
                watchdog_message_maple("Failed to write settings to ESP32", BPK_Code_Error);
            }

            watchdog_message_maple("ESP32 returned invalid code when udpating wd settings", BPK_Code_Error);

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

    if (dt_time_t1_leq_t2(&currentDateTime.Time, &captureTime.startTime) == TRUE) {

        // Set the alarm to be the start time of the capture time
        dt_datetime_t alarmA = {
            .Time.second = 0,
            .Time.minute = captureTime.startTime.minute,
            .Time.hour   = captureTime.startTime.hour,
            .Date.day    = currentDateTime.Date.day,
            .Date.month  = currentDateTime.Date.month,
            .Date.year   = currentDateTime.Date.year,
        };
        stm32_rtc_set_alarmA(&alarmA);

    } else if (dt_time_t1_leq_t2(&captureTime.endTime, &currentDateTime.Time) == TRUE) {

        // Increment to the following day
        dt_datetime_increment_day(&currentDateTime);

        // Set the alarm to be the start time
        dt_datetime_t alarmA = {
            .Time.second = 0,
            .Time.minute = captureTime.startTime.minute,
            .Time.hour   = captureTime.startTime.hour,
            .Date.day    = currentDateTime.Date.day,
            .Date.month  = currentDateTime.Date.month,
            .Date.year   = currentDateTime.Date.year,
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
            if (dt_time_t1_leq_t2(&currentDateTime.Time, &alarmTime) == TRUE) {
                // Set the alarm time
                dt_datetime_t alarmA = {
                    .Time.second = 0,
                    .Time.minute = alarmTime.minute,
                    .Time.hour   = alarmTime.hour,
                    .Date.day    = currentDateTime.Date.day,
                    .Date.month  = currentDateTime.Date.month,
                    .Date.year   = currentDateTime.Date.year,
                };
                stm32_rtc_set_alarmA(&alarmA);
                break;
            }
        }
    }
}

uint8_t stm32_match_maple_request(bpk_t* Bpacket) {

    bpk_buffer_t bpacketBuffer;

    switch (Bpacket->Request.val) {

        case BPK_REQ_TAKE_PHOTO: // Send command to ESP32 to take a photo

            watchdog_request_esp32_take_photo();

            break;

        case BPK_REQ_GET_DATETIME:

            // Get the datetime from the RTC
            stm32_rtc_read_datetime(&datetime);
            if (wd_datetime_to_bpacket(Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, Bpacket->Request,
                                       BPK_Code_Success, &datetime) == TRUE) {

                bpk_to_buffer(Bpacket, &bpacketBuffer);
                uart_write(MAPLE_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);
            } else {
                watchdog_message_maple("Failed to convert datetime to Bpacket\0", BPK_Code_Error);
            }

            break;

        case BPK_REQ_SET_DATETIME:

            // Set the datetime using the information from the Bpacket
            if (wd_bpacket_to_datetime(Bpacket, &datetime) == TRUE) {

                stm32_rtc_write_datetime(&datetime);

                watchdog_report_success(BPK_Req_Set_Datetime);

                // Recalculate the RTC alarm
                watchdog_calculate_rtc_alarm_time();

            } else {
                watchdog_message_maple("Failed to convert Bpacket to datetime\r\n", BPK_Code_Error);
            }

            break;

        case BPK_REQUEST_STATUS:;

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

            bpk_t statusBpacket;
            bpk_create_sp(&statusBpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Status,
                          BPK_Code_Success, message);
            bpk_to_buffer(&statusBpacket, &bpacketBuffer);
            uart_write(MAPLE_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);

            break;

        case BPK_REQ_GET_CAMERA_CAPTURE_TIMES:;

            // Convert capture times to a bpacket
            bpk_data_t Data;
            Data.bytes[0] = captureTime.startTime.second;
            Data.bytes[1] = captureTime.startTime.minute;
            Data.bytes[2] = captureTime.startTime.hour;

            Data.bytes[3] = captureTime.endTime.second;
            Data.bytes[4] = captureTime.endTime.minute;
            Data.bytes[5] = captureTime.endTime.hour;

            Data.bytes[6] = captureTime.intervalTime.second;
            Data.bytes[7] = captureTime.intervalTime.minute;
            Data.bytes[8] = captureTime.intervalTime.hour;
            Data.numBytes = 9;

            bpk_t settingsPacket;
            if (bpk_create(&settingsPacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32,
                           BPK_Req_Get_Camera_Capture_Times, BPK_Code_Success, Data.numBytes, Data.bytes) != TRUE) {
                watchdog_message_maple("Failed to convert settings to packet", BPK_Code_Error);
                break;
            }

            // Send the Bpacket to maple
            bpk_to_buffer(&settingsPacket, &bpacketBuffer);
            uart_write(MAPLE_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);

            break;

        case BPK_REQ_SET_CAMERA_CAPTURE_TIMES:;

            // Validate the capture time received
            dt_time_t Start, End, Interval;
            if (dt_time_init(&Start, Bpacket->Data.bytes[0], Bpacket->Data.bytes[1], Bpacket->Data.bytes[2]) != TRUE) {
                watchdog_message_maple("Invalid Start Time\r\n", BPK_Code_Error);
                break;
            }

            if (dt_time_init(&End, Bpacket->Data.bytes[3], Bpacket->Data.bytes[4], Bpacket->Data.bytes[5]) != TRUE) {
                watchdog_message_maple("Invalid End Time\r\n", BPK_Code_Error);
                break;
            }

            if (dt_time_init(&Interval, Bpacket->Data.bytes[6], Bpacket->Data.bytes[7], Bpacket->Data.bytes[8]) !=
                TRUE) {
                watchdog_message_maple("Invalid Interval Time\r\n", BPK_Code_Error);
                break;
            }

            // Update the capture time
            captureTime.startTime.second = Start.second;
            captureTime.startTime.minute = Start.minute;
            captureTime.startTime.hour   = Start.hour;

            captureTime.endTime.second = End.second;
            captureTime.endTime.minute = End.minute;
            captureTime.endTime.hour   = End.hour;

            captureTime.intervalTime.second = Interval.second;
            captureTime.intervalTime.minute = Interval.minute;
            captureTime.intervalTime.hour   = Interval.hour;

            watchdog_report_success(Bpacket->Request);
            break;

        case BPK_REQ_ESP32_ON:
            watchdog_esp32_on();
            watchdog_create_and_send_bpacket_to_maple(BPK_Req_Esp32_On, BPK_Code_Success, 0, NULL);
            break;

        case BPK_REQ_ESP32_OFF:
            watchdog_esp32_off();
            watchdog_create_and_send_bpacket_to_maple(BPK_Req_Esp32_Off, BPK_Code_Success, 0, NULL);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

void watchdog_request_esp32_take_photo(void) {

    // Record the current temperature from both temperature sensors
    if (ds18b20_read_temperature(DS18B20_SENSOR_ID_1) != TRUE) {
        watchdog_message_maple("Failed to read temperature", BPK_Code_Error);
    }

    cdt_dbl_16_t temp1;
    if (ds18b20_copy_temperature(DS18B20_SENSOR_ID_1, &temp1) != TRUE) {
        watchdog_message_maple("Failed to copy temperature", BPK_Code_Error);
    }

    if (ds18b20_read_temperature(DS18B20_SENSOR_ID_2) != TRUE) {
        watchdog_message_maple("Failed to read temperature", BPK_Code_Error);
    }

    cdt_dbl_16_t temp2;
    if (ds18b20_copy_temperature(DS18B20_SENSOR_ID_2, &temp2) != TRUE) {
        watchdog_message_maple("Failed to copy temperature", BPK_Code_Error);
    }

    // Update the datetime struct
    stm32_rtc_read_datetime(&datetime);

    // Get the real time clock time and date. Put it in a packet and send to the ESP32
    bpk_t photoRequest;

    uint8_t result = wd_photo_data_to_bpacket(&photoRequest, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Stm32,
                                              BPK_Req_Take_Photo, BPK_Code_Execute, &datetime, &temp1, &temp2);

    if (result == TRUE) {
        watchdog_send_bpacket_to_esp32(&photoRequest);
    } else {
        char errMsg[50];
        wd_get_error(result, errMsg);
        char msg[130];
        sprintf(msg, "Failed to convert photo data to Bpacket with error: %s\r\n", errMsg);
        watchdog_message_maple(msg, BPK_Code_Error);
    }
}

void watchdog_esp32_on(void) {
    // Turn UART off for ESP32. This prevents STM32 from reading ESP32 UART which is in
    // non bpacket form
    comms_close_connection(ESP32_UART);

    ESP32_POWER_PORT->BSRR |= (0x01 << ESP32_POWER_PIN);

    // Delay for 1.5 seconds to let ESP32 startup
    HAL_Delay(1500);

    // Reopen the ESP32 uart connection
    uart_open_connection(ESP32_UART);

    // Ping the ESP32 to ensure it's communicating
    uint8_t pingCode = WATCHDOG_PING_CODE_ESP32;
    watchdog_create_and_send_bpacket_to_esp32(BPK_Request_Ping, BPK_Code_Execute, 1, &pingCode);
}

void watchdog_esp32_off(void) {
    ESP32_POWER_PORT->BSRR |= (0x10000 << ESP32_POWER_PIN);

    esp32Connected = FALSE;
    LED_GREEN_PORT->BSRR |= (0x10000 << LED_GREEN_PIN); // Turn green LED off
}