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

/* Private Variables */
dt_datetime_t datetime;
wd_settings_t wdSettings;

wd_settings_t deafultWdSettings = {
    .cameraSettings.resolution    = WD_CAM_RES_640x480,
    .captureTime.startTime.second = 0,
    .captureTime.startTime.minute = 0,
    .captureTime.startTime.hour   = 9,
    .captureTime.endTime.second   = 0,
    .captureTime.endTime.minute   = 0,
    .captureTime.endTime.hour     = 15,
};

/* Function Prototypes */
void watchdog_esp32_on(void);
void watchdog_esp32_off(void);
void watchdog_send_bpacket_to_esp32(bpacket_t* bpacket);
void watchdog_send_bpacket_to_maple(bpacket_t* bpacket);
void watchdog_create_and_send_bpacket_to_esp32(uint8_t request, uint8_t code, uint8_t numBytes, uint8_t* data);
void watchdog_create_and_send_bpacket_to_maple(uint8_t request, uint8_t code, uint8_t numBytes, uint8_t* data);

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
    datetime.date.month  = 1;
    datetime.date.day    = 1;
    datetime.time.hour   = 0;
    datetime.time.minute = 0;
    datetime.time.second = 0;
    stm32_rtc_write_datetime(&datetime);

    // Turn the ESP32 on
    watchdog_esp32_on();

    HAL_Delay(2000); // Wait for the ESP32 to startup

    // Get the watchdog settings from the ESP32
    watchdog_create_and_send_bpacket_to_esp32(WATCHDOG_BPK_R_READ_SETTINGS, BPACKET_CODE_EXECUTE, 0, NULL);
}

void watchdog_send_string(char* string) {

    // Loop through string
    uint32_t numBytes = chars_get_num_bytes(string);

    bpacket_t bpacket;
    bpacket_buffer_t packetBuffer;
    bpacket.request = BPACKET_CODE_IN_PROGRESS;

    int j = 0;
    for (int i = 0; i < numBytes; i++) {

        bpacket.bytes[j++] = (uint8_t)string[i];

        // Send bpacket if buffer is full or the end of the string
        // has been reached
        if (j == BPACKET_MAX_NUM_DATA_BYTES) {
            bpacket.numBytes = j;
            j                = 0;
            bpacket_to_buffer(&bpacket, &packetBuffer);
            comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
            continue;
        }

        if (i == (numBytes - 1)) {
            bpacket.numBytes = j;
            bpacket.request  = BPACKET_CODE_SUCCESS;
            bpacket_to_buffer(&bpacket, &packetBuffer);
            comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
        }
    }
}

/**
 * @brief Send message over uart to connected computer where the request is fail
 * and message is error message.
 *
 * @param errorMsg
 */
void watchdog_report_error(uint8_t request, char* errorMsg) {

    bpacket_t bpacket;
    uint8_t result = bpacket_create_sp(&bpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32, request,
                                       BPACKET_CODE_ERROR, errorMsg);

    if (result == TRUE) {
        bpacket_buffer_t packetBuffer;
        bpacket_to_buffer(&bpacket, &packetBuffer);

        comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
    } else {
        char msg[50];
        bpacket_get_error(result, msg);
        log_send_data(msg, sizeof(msg));
    }
}

void watchdog_report_success(uint8_t request) {

    bpacket_t bpacket;
    bpacket_create_p(&bpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32, request, BPACKET_CODE_SUCCESS, 0, NULL);

    bpacket_buffer_t packetBuffer;
    bpacket_to_buffer(&bpacket, &packetBuffer);

    comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_create_and_send_bpacket_to_maple(uint8_t request, uint8_t code, uint8_t numBytes, uint8_t* data) {
    bpacket_t bpacket;
    bpacket_buffer_t packetBuffer;
    bpacket_create_p(&bpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32, request, code, numBytes, data);
    bpacket_to_buffer(&bpacket, &packetBuffer);
    comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_create_and_send_bpacket_to_esp32(uint8_t request, uint8_t code, uint8_t numBytes, uint8_t* data) {
    bpacket_t bpacket;
    bpacket_buffer_t packetBuffer;
    bpacket_create_p(&bpacket, BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_STM32, request, code, numBytes, data);
    bpacket_to_buffer(&bpacket, &packetBuffer);
    comms_transmit(ESP32_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_send_bpacket_to_esp32(bpacket_t* bpacket) {
    bpacket_buffer_t bpacketBuffer;
    bpacket_to_buffer(bpacket, &bpacketBuffer);
    comms_transmit(ESP32_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);
}

void watchdog_send_bpacket_to_maple(bpacket_t* bpacket) {
    bpacket_buffer_t bpacketBuffer;
    bpacket_to_buffer(bpacket, &bpacketBuffer);
    comms_transmit(MAPLE_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);
}

void process_watchdog_stm32_request(bpacket_t* bpacket) {
    switch (bpacket->request) {
        case WATCHDOG_BPK_R_LED_RED_ON:
            watchdog_create_and_send_bpacket_to_esp32(WATCHDOG_BPK_R_LED_RED_ON, BPACKET_CODE_EXECUTE, 0, NULL);
            break;

        case WATCHDOG_BPK_R_LED_RED_OFF:
            watchdog_create_and_send_bpacket_to_esp32(WATCHDOG_BPK_R_LED_RED_OFF, BPACKET_CODE_EXECUTE, 0, NULL);
            break;

        case BPACKET_GEN_R_PING:;
            uint8_t ping = WATCHDOG_PING_CODE_STM32;
            watchdog_create_and_send_bpacket_to_maple(BPACKET_GEN_R_PING, BPACKET_CODE_SUCCESS, 1, &ping);
            break;

        case BPACKET_CODE_SUCCESS:
            watchdog_report_success(bpacket->request);
            break;

        case BPACKET_GET_R_MESSAGE:
            log_send_bdata(bpacket->bytes, bpacket->numBytes);
            break;

        default:
            watchdog_report_error(bpacket->request, "Unknown request");
    }
}

void watchdog_print_wd_error(uint8_t result) {
    char msg[40];
    wd_get_error(result, msg);
    log_error(msg);
}

uint8_t stm32_match_esp32_request(bpacket_t* bpacket) {
    // log_send_data("here 9", 6);
    switch (bpacket->request) {

        case WATCHDOG_BPK_R_READ_SETTINGS:

            // Log success to Maple
            if (bpacket->code == BPACKET_CODE_SUCCESS) {
                // // Store watchdog settings in struct
                uint8_t result = wd_bpacket_to_settings(bpacket, &wdSettings);

                if (result != TRUE) {
                    watchdog_print_wd_error(result);
                    break;
                }
                log_send_data(" CONV SUCC ", 11);
                // // watchdog_report_success(WATCHDOG_BPK_R_READ_SETTINGS);

                char k[52];
                sprintf(k, "Cam res: %i Start time: %i %i End time: %i %i", wdSettings.cameraSettings.resolution,
                        wdSettings.captureTime.startTime.minute, wdSettings.captureTime.startTime.hour,
                        wdSettings.captureTime.endTime.minute, wdSettings.captureTime.endTime.hour);
                log_send_data(k, chars_get_num_bytes(k));
                break;
            }

            // If reading the watchdog settings fails, write the default watchdog
            // settings to the ESP32
            if (bpacket->code == BPACKET_CODE_ERROR) {

                watchdog_report_error(WATCHDOG_BPK_R_READ_SETTINGS, "Failed to read");

                bpacket_t settings;
                uint8_t result =
                    wd_settings_to_bpacket(&settings, BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_STM32,
                                           WATCHDOG_BPK_R_WRITE_SETTINGS, BPACKET_CODE_EXECUTE, &deafultWdSettings);

                // Write camera settings to the esp32
                if (result == TRUE) {
                    watchdog_send_bpacket_to_esp32(&settings);
                    watchdog_report_error(WATCHDOG_BPK_R_READ_SETTINGS, "Wrote default settings");
                } else {
                    watchdog_report_error(WATCHDOG_BPK_R_READ_SETTINGS, "Failed to write default settings");
                }

                break;
            }

            break;

        case WATCHDOG_BPK_R_WRITE_SETTINGS:

            // Log success to Maple
            if (bpacket->code == BPACKET_CODE_SUCCESS) {
                // watchdog_report_success(WATCHDOG_BPK_R_WRITE_SETTINGS);
                watchdog_report_error(WATCHDOG_BPK_R_READ_SETTINGS, "Set new settings");
                break;
            }

            // Log failure to Maple
            if (bpacket->code == BPACKET_CODE_ERROR) {
                watchdog_report_error(WATCHDOG_BPK_R_WRITE_SETTINGS, "Failed to write settings to ESP32");
            }

            break;

        default:
            return FALSE;
    }

    return TRUE;
}

uint8_t stm32_match_maple_request(bpacket_t* bpacket) {

    bpacket_buffer_t bpacketBuffer;

    uint8_t request = bpacket->request;

    switch (bpacket->request) {
        case WATCHDOG_BPK_R_TAKE_PHOTO:
            // TODO: Implement
            break;
        case WATCHDOG_BPK_R_RECORD_DATA:
            // TODO: Implement
            break;
        case WATCHDOG_BPK_R_CAMERA_VIEW:
            // TODO: Implement
            break;
        case WATCHDOG_BPK_R_GET_DATETIME:

            // Get the datetime from the RTC
            stm32_rtc_read_datetime(&datetime);
            if (wd_datetime_to_bpacket(bpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_STM32, request,
                                       BPACKET_CODE_SUCCESS, &datetime) == TRUE) {
                bpacket_to_buffer(bpacket, &bpacketBuffer);
                comms_transmit(MAPLE_UART, bpacketBuffer.buffer, bpacketBuffer.numBytes);
            } else {
                watchdog_report_error(WATCHDOG_BPK_R_GET_DATETIME, "Failed to convert datetime to bpacket\0");
            }
            break;
        case WATCHDOG_BPK_R_SET_DATETIME:

            // Set the datetime using the information from the bpacket
            if (wd_bpacket_to_datetime(bpacket, &datetime) == TRUE) {
                stm32_rtc_write_datetime(&datetime);
                watchdog_report_success(WATCHDOG_BPK_R_SET_DATETIME);
            } else {
                watchdog_report_error(WATCHDOG_BPK_R_SET_DATETIME, "Failed to convert bpacket to datetime\0");
            }
            break;
        case WATCHDOG_BPK_R_GET_CAMERA_RESOLUTION:;
            // TODO: Implement
            break;
        case WATCHDOG_BPK_R_SET_CAMERA_RESOLUTION:;
            // TODO: Implement
            break;
        case WATCHDOG_BPK_R_GET_STATUS:

            // TODO: Implement
            break;
        default:;
            return FALSE;
    }

    return TRUE;
}

void watchdog_update(void) {

    /* TEMPERATURE CONVERSION CODE */
    // // Make the temp sensor record the temperature
    // if (ds18b20_read_temperature(DS18B20_SENSOR_ID_2) != TRUE) {
    //     log_error("Failed to record temperature\r\n");
    //     return;
    // }

    // ds18b20_temp_t temp;

    // if (ds18b20_copy_temperature(DS18B20_SENSOR_ID_2, &temp) != TRUE) {
    //     log_error("Failed to read temperature\r\n");
    //     return;
    // }

    // char msg[40];
    // sprintf(msg, "Temperature: %s%i.%i\r\n", (temp.sign == 0) ? "" : "-", temp.decimal, temp.fraction);
    // log_message(msg);
    /* TEMPERATURE CONVERSION CODE */

    // Wait for STM32 to receive a bpacket
    bpacket_t mapleBpacket, esp32Bpacket;

    while (1) {

        // Process anything the ESP32 sends to the STM32
        if (comms_process_rxbuffer(BUFFER_1_ID, &esp32Bpacket) == TRUE) {
            if (stm32_match_esp32_request(&esp32Bpacket) != TRUE) {
                process_watchdog_stm32_request(&esp32Bpacket);
            }
        }

        // Process anything Maple sends to the STM32
        if (comms_process_rxbuffer(BUFFER_2_ID, &mapleBpacket) == TRUE) {
            if (stm32_match_maple_request(&mapleBpacket) != TRUE) {
                process_watchdog_stm32_request(&mapleBpacket);
            }
        }
    }
}

void watchdog_esp32_on(void) {
    ESP32_POWER_PORT->BSRR |= (0x01 << ESP32_POWER_PIN);
}

void watchdog_esp32_off(void) {
    ESP32_POWER_PORT->BSRR |= (0x10000 << ESP32_POWER_PIN);
}