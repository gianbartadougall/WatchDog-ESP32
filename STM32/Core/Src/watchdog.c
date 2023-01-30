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

/* Private Variables */
dt_datetime_t datetime;

/* Function Prototypes */
void watchdog_esp32_on(void);
void watchdog_esp32_off(void);

void bpacket_print(bpacket_t* bpacket) {
    char msg[BPACKET_BUFFER_LENGTH_BYTES + 2];
    sprintf(msg, "R: %i ", bpacket->request);
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
    // watchdog_esp32_on();
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
            comms_transmit(USART2, packetBuffer.buffer, packetBuffer.numBytes);
            continue;
        }

        if (i == (numBytes - 1)) {
            bpacket.numBytes = j;
            bpacket.request  = BPACKET_R_SUCCESS;
            bpacket_to_buffer(&bpacket, &packetBuffer);
            comms_transmit(USART2, packetBuffer.buffer, packetBuffer.numBytes);
        }
    }
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

    /* TURN ESP32 ON AND OFF USING BJT */
    // Power test
    // Turn the BJT on
    // ESP32_POWER_PORT->ODR |= (0x01 << ESP32_POWER_PIN);
    // LED_GREEN_PORT->ODR |= (0x01 << LED_GREEN_PIN);
    // log_message("Power on\r\n");

    // char msg[50];
    // // Wait for 30 seconds
    // for (int i = 30; i > 0; i--) {
    //     HAL_Delay(1000);
    //     sprintf(msg, "%i seconds left before shutdown\r\n", i);
    //     log_message(msg);
    // }

    // // Turn the BJT off
    // ESP32_POWER_PORT->ODR &= ~(0x01 << ESP32_POWER_PIN);
    // LED_GREEN_PORT->ODR &= ~(0x01 << LED_GREEN_PIN);

    // log_message("Power off\r\n");

    // // Wait for 30 seconds
    // for (int i = 30; i > 0; i--) {
    //     HAL_Delay(1000);
    //     sprintf(msg, "%i seconds left before Restart\r\n", i);
    //     log_message(msg);
    // }
    /* TURN ESP32 ON AND OFF USING BJT */

    // Wait for STM32 to receive a bpacket
    bpacket_t bpacket;
    bpacket_buffer_t packetBuffer;
    char text[] = {"Hello!\0"};
    while (1) {

        while (comms_process_rxbuffer(&bpacket) != TRUE) {};

        switch (bpacket.request) {
            case BPACKET_GEN_R_PING:;

                // Create bpacket with STM32 ping code and send back
                uint8_t pingCode = WATCHDOG_PING_CODE_STM32;
                bpacket_create_p(&bpacket, BPACKET_R_SUCCESS, 1, &pingCode);
                bpacket_to_buffer(&bpacket, &packetBuffer);
                comms_transmit(USART2, packetBuffer.buffer, packetBuffer.numBytes);
                break;
            case BPACKET_GET_R_STATUS:
                break;
            case BPACKET_GEN_R_HELP:
                watchdog_send_string(text);
                break;
            case WATCHDOG_BPK_R_UPDATE_CAMERA_SETTINGS:
                break;
            case WATCHDOG_BPK_R_GET_DATETIME:;

                // Get the current datetime of the RTC
                stm32_rtc_read_datetime(&datetime);
                if (wd_datetime_to_bpacket(&bpacket, BPACKET_R_SUCCESS, &datetime) == TRUE) {
                    bpacket_to_buffer(&bpacket, &packetBuffer);
                    comms_transmit(USART2, packetBuffer.buffer, packetBuffer.numBytes);
                } else {
                    bpacket_create_sp(&bpacket, BPACKET_R_FAILED, "Invalid RTC datetime\0");
                    bpacket_to_buffer(&bpacket, &packetBuffer);
                    comms_transmit(USART2, packetBuffer.buffer, packetBuffer.numBytes);
                }

                break;
            case WATCHDOG_BPK_R_SET_DATETIME:
                break;
            default:
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