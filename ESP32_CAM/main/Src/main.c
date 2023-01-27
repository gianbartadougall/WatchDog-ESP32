/**
 * @file main.c
 * @author Gian Barta-Dougall, Yash Pandey
 * @brief
 * @version 0.1
 * @date 2022-12-19
 *
 * @copyright Copyright (c) 2022
 *
 */

/* Library includes */
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

/* Personal Includes */
#include "sd_card.h"
#include "camera.h"
#include "hardware_config.h"
#include "led.h"
#include "bpacket.h"
#include "esp32_uart.h"
#include "help.h"
#include "utilities.h"

/* Private Macros */
#define COB_LED HC_COB_LED
#define RED_LED HC_RED_LED

/* Private Function Declarations */

void watchdog_send_status(void) {

    uint8_t id               = 1;
    uint8_t cameraResolution = camera_get_resolution();
    uint16_t numImages;
    bpacket_t response;
    if (sd_card_search_num_images(&numImages, &response) != TRUE) {
        numImages = 5000;
    }
    uint8_t status = 0;
    bpacket_t bpacket;
    uint8_t data[5];
    data[0] = id;
    data[1] = cameraResolution;
    data[2] = numImages >> 8;
    data[3] = numImages & 0xFF;
    data[4] = status;
    bpacket_create_p(&bpacket, BPACKET_R_SUCCESS, 5, data);
    esp32_uart_send_bpacket(&bpacket);
}

void watchdog_system_start(void) {

    // Turn all the LEDs off
    led_off(COB_LED);
    led_off(RED_LED);

    uint8_t ping[1];
    ping[0] = 23;

    uint8_t bdata[BPACKET_BUFFER_LENGTH_BYTES];
    bpacket_t bpacket;
    bpacket_char_array_t bpacketCharArray;

    while (1) {

        // Delay for second
        vTaskDelay(200 / portTICK_PERIOD_MS);

        // Read UART and wait for command.
        if (esp32_uart_read_bpacket(bdata) == 0) {
            continue;
        }

        bpacket_decode(&bpacket, bdata);

        switch (bpacket.request) {
            case BPACKET_GEN_R_HELP:
                esp32_uart_send_string(uartHelp);
                break;
            case BPACKET_GEN_R_PING:
                bpacket_create_p(&bpacket, BPACKET_R_SUCCESS, 1, ping);
                esp32_uart_send_bpacket(&bpacket);
                break;
            case BPACKET_GET_R_STATUS:
                watchdog_send_status();
                break;
            case WATCHDOG_BPK_R_LIST_DIR:
                bpacket_data_to_string(&bpacket, &bpacketCharArray);
                sd_card_list_directory(&bpacket, &bpacketCharArray);
                break;
            case WATCHDOG_BPK_R_COPY_FILE:
                bpacket_data_to_string(&bpacket, &bpacketCharArray);
                sd_card_copy_file(&bpacket, &bpacketCharArray);
                break;
            case WATCHDOG_BPK_R_TAKE_PHOTO:
                camera_capture_and_save_image(&bpacket);
                break;
            case WATCHDOG_BPK_R_WRITE_TO_FILE:
                // sd_card_write_to_file();
                break;
            case WATCHDOG_BPK_R_RECORD_DATA:
                break;
            case WATCHDOG_BPK_R_LED_RED_ON:
                led_on(RED_LED);
                break;
            case WATCHDOG_BPK_R_LED_RED_OFF:
                led_off(RED_LED);
                break;
            default:
                break;
        }
    }
}

uint8_t software_config(bpacket_t* bpacket) {

    if (sd_card_init(bpacket) != TRUE) {
        return FALSE;
    }

    return TRUE;
}

void app_main(void) {

    bpacket_t status;

    /* Initialise all the hardware used */
    if (hardware_config(&status) == TRUE && software_config(&status) == TRUE) {
        watchdog_system_start();
    }

    if (sd_card_open() == TRUE) {
        sd_card_log(SYSTEM_LOG_FILE, "Exited Watchdog System. Waiting for shutdown");
        sd_card_close();
    }

    while (1) {
        // esp32_uart_send_packet(&status);
        // esp32_uart_send_data("\r\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
