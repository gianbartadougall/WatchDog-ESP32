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
wd_camera_settings_t cameraSettings;

/* Function Prototypes */
void watchdog_esp32_on(void);
void watchdog_esp32_off(void);

void bpacket_print(bpacket_t* bpacket) {
    char msg[BPACKET_BUFFER_LENGTH_BYTES + 2];
    sprintf(msg, "heelo R: %i ", bpacket->request);
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

    // By deafult, camera takes two photos per day. Once at 9am and once at 3pm
    dt_time_init(&cameraSettings.startTime, 0, 0, 9);
    dt_time_init(&cameraSettings.endTime, 0, 0, 15);
    cameraSettings.intervalHour = 6;
    cameraSettings.resolution   = WD_CAM_RES_1024x768;

    // Turn the ESP32 on
    watchdog_esp32_on();
}

void watchdog_send_string(char* string) {

    // Loop through string
    uint32_t numBytes = chars_get_num_bytes(string);

    bpacket_t bpacket;
    bpacket_buffer_t packetBuffer;
    bpacket.request = BPACKET_R_IN_PROGRESS;

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
            bpacket.request  = BPACKET_R_SUCCESS;
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
void watchdog_report_error(char* errorMsg) {

    bpacket_t bpacket;
    bpacket_create_sp(&bpacket, BPACKET_R_FAILED, errorMsg);

    bpacket_buffer_t packetBuffer;
    bpacket_to_buffer(&bpacket, &packetBuffer);

    comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
}

void watchdog_report_success(void) {

    bpacket_t bpacket;
    bpacket_create_p(&bpacket, BPACKET_R_SUCCESS, 0, NULL);

    bpacket_buffer_t packetBuffer;
    bpacket_to_buffer(&bpacket, &packetBuffer);

    comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
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
    bpacket_buffer_t packetBuffer;
    wd_camera_settings_t cameraSettings;

    while (1) {

        while (comms_process_rxbuffer(BUFFER_2_ID, &mapleBpacket) != TRUE) {};

        // bpacket_print(&mapleBpacket);
        // continue;

        switch (mapleBpacket.request) {
            case BPACKET_GEN_R_PING:;

                // Create bpacket with STM32 ping code and send back
                uint8_t pingCode = WATCHDOG_PING_CODE_STM32;
                bpacket_create_p(&mapleBpacket, BPACKET_R_SUCCESS, 1, &pingCode);
                bpacket_to_buffer(&mapleBpacket, &packetBuffer);
                comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
                break;
            case BPACKET_GET_R_STATUS:

                break;
            case BPACKET_GEN_R_HELP:
                watchdog_send_string(uartHelp);
                break;
            case WATCHDOG_BPK_R_SET_CAMERA_SETTINGS:;

                // Confirm the camera settings are valid
                wd_camera_settings_t camTempSettings;
                if (wd_bpacket_to_camera_settings(&mapleBpacket, &camTempSettings) != TRUE) {
                    watchdog_report_error("Invalid camera settings\0");
                } else {
                    // Send bpacket to esp32 to update camera resolution
                    bpacket_create_p(&mapleBpacket, WATCHDOG_BPK_R_SET_CAMERA_RESOLUTION, 1,
                                     &camTempSettings.resolution);
                    bpacket_to_buffer(&mapleBpacket, &packetBuffer);
                    comms_transmit(ESP32_UART, packetBuffer.buffer, packetBuffer.numBytes);

                    while (comms_process_rxbuffer(BUFFER_1_ID, &esp32Bpacket) != TRUE) {}

                    if (esp32Bpacket.request == BPACKET_R_FAILED) {
                        watchdog_report_error("Failed to set camera res\0");
                    } else {
                        cameraSettings.resolution = mapleBpacket.bytes[0];
                        watchdog_report_success();

                        cameraSettings.startTime.second = camTempSettings.startTime.second;
                        cameraSettings.startTime.minute = camTempSettings.startTime.minute;
                        cameraSettings.startTime.hour   = camTempSettings.startTime.hour;
                        cameraSettings.endTime.second   = camTempSettings.endTime.second;
                        cameraSettings.endTime.minute   = camTempSettings.endTime.minute;
                        cameraSettings.endTime.hour     = camTempSettings.endTime.hour;
                        cameraSettings.intervalHour     = camTempSettings.intervalHour;
                        cameraSettings.intervalMinute   = camTempSettings.intervalMinute;
                        cameraSettings.resolution       = camTempSettings.resolution;
                    }
                }

                break;
            case WATCHDOG_BPK_R_GET_CAMERA_SETTINGS:

                // Convert camera resolution settings to bpacket
                if (wd_camera_settings_to_bpacket(&mapleBpacket, BPACKET_R_SUCCESS, &cameraSettings) != TRUE) {
                    // TODO: Handle error
                } else {

                    // Transmit camera resolution settings
                    bpacket_to_buffer(&mapleBpacket, &packetBuffer);
                    comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
                }

                break;
            case WATCHDOG_BPK_R_GET_DATETIME:;

                // Get the current datetime of the RTC
                stm32_rtc_read_datetime(&datetime);
                if (wd_datetime_to_bpacket(&mapleBpacket, BPACKET_R_SUCCESS, &datetime) == TRUE) {
                    bpacket_to_buffer(&mapleBpacket, &packetBuffer);
                    comms_transmit(MAPLE_UART, packetBuffer.buffer, packetBuffer.numBytes);
                } else {
                    watchdog_report_error("Invalid RTC datetime\0");
                }

                break;
            case WATCHDOG_BPK_R_SET_DATETIME:

                // Convert the bacpacket to a datetime
                if (wd_bpacket_to_datetime(&mapleBpacket, &datetime) != TRUE) {
                    // TODO: Return error
                } else {
                    // Update the RTC with the current datetime
                    stm32_rtc_write_datetime(&datetime);
                }

                break;
            case WATCHDOG_BPK_R_LED_RED_ON:
            case WATCHDOG_BPK_R_LED_RED_OFF:
                watchdog_report_error("Here\0");
                // Pass message straight back to ESP32
                bpacket_to_buffer(&mapleBpacket, &packetBuffer);
                comms_transmit(ESP32_UART, packetBuffer.buffer, packetBuffer.numBytes);
                uint32_t currentTick = SysTick->VAL;
                uint8_t timeout      = 0;
                while (comms_process_rxbuffer(BUFFER_1_ID, &esp32Bpacket) != TRUE) {
                    if ((SysTick->VAL - currentTick) > 1000) {
                        timeout = 1;
                        break;
                    }
                }

                // if (timeout) {
                //     watchdog_report_error("Timeout from ESP32\0");
                // } else if (esp32Bpacket.request == BPACKET_R_FAILED) {
                //     watchdog_report_error("Failed to update LED\0");
                // } else {
                // }

                break;
            default:
                // TODO: Return failed unknown request

                watchdog_report_error("Unknown request\0");
                break;
        }
    }
}

void watchdog_esp32_on(void) {
    ESP32_POWER_PORT->BSRR |= (0x01 << ESP32_POWER_PIN);
}

void watchdog_esp32_off(void) {
    ESP32_POWER_PORT->BSRR |= (0x10000 << ESP32_POWER_PIN);
}