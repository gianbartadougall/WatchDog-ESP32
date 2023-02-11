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

// void watchdog_send_status(void) {

//     uint8_t id               = 1;
//     uint8_t cameraResolution = camera_get_resolution();
//     uint16_t numImages;
//     bpacket_t response;

//     if (sd_card_search_num_images(&numImages, &response) != TRUE) {
//         numImages = 5000;
//     }

//     uint8_t status = 0;
//     bpacket_t bpacket;
//     uint8_t data[5];
//     data[0] = id;
//     data[1] = cameraResolution;
//     data[2] = numImages >> 8;
//     data[3] = numImages & 0xFF;
//     data[4] = status;
//     bpacket_create_p(&bpacket, BPACKET_R_SUCCESS, 5, data);
//     esp32_uart_send_bpacket(&bpacket);
// }

uint8_t esp3_match_stm32_request(bpacket_t* bpacket) {

    // bpacket_t bpacket1;
    // char j[30];
    // sprintf(j, "Request: [%i]", bpacket->request);
    // bpacket_create_sp(&bpacket1, BPACKET_ADDRESS_STM32, BPACKET_ADDRESS_ESP32, BPACKET_GET_R_MESSAGE,
    //                   BPACKET_CODE_SUCCESS, j);
    // esp32_uart_send_bpacket(&bpacket1);

    // Save the address
    uint8_t request  = bpacket->request;
    uint8_t receiver = bpacket->receiver;
    uint8_t sender   = bpacket->sender;

    switch (bpacket->request) {

        case WATCHDOG_BPK_R_TAKE_PHOTO:
            camera_capture_and_save_image(bpacket);
            break;

        case WATCHDOG_BPK_R_SET_CAMERA_RESOLUTION:

            if (camera_set_resolution(bpacket->bytes[0]) != TRUE) {
                bpacket_create_p(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, 0, NULL);
            } else {
                bpacket_create_p(bpacket, sender, receiver, request, BPACKET_CODE_SUCCESS, 0, NULL);
            }

            esp32_uart_send_bpacket(bpacket);
            break;

        case WATCHDOG_BPK_R_GET_CAMERA_RESOLUTION:;

            uint8_t resolution = camera_get_resolution();
            bpacket_create_p(bpacket, sender, receiver, request, BPACKET_CODE_SUCCESS, 1, &resolution);
            esp32_uart_send_bpacket(bpacket);
            break;

        case WATCHDOG_BPK_R_WRITE_SETTINGS:

            sd_card_write_watchdog_settings(bpacket);
            esp32_uart_send_bpacket(bpacket); // Send response back
            break;

        case WATCHDOG_BPK_R_READ_SETTINGS:

            sd_card_read_watchdog_settings(bpacket);
            esp32_uart_send_bpacket(bpacket); // Send response back
            break;

        case WATCHDOG_BPK_R_RECORD_DATA:
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

uint8_t esp3_match_maple_request(bpacket_t* bpacket) {

    bpacket_char_array_t bpacketCharArray;

    switch (bpacket->request) {
        case WATCHDOG_BPK_R_LIST_DIR:
            bpacket_data_to_string(bpacket, &bpacketCharArray);
            sd_card_list_directory(bpacket, &bpacketCharArray);
            break;
        case WATCHDOG_BPK_R_COPY_FILE:;
            bpacket_data_to_string(bpacket, &bpacketCharArray);
            sd_card_copy_file(bpacket, &bpacketCharArray);
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

void watchdog_system_start(void) {

    // Turn all the LEDs off
    led_off(COB_LED);
    led_off(RED_LED);

    uint8_t ping = WATCHDOG_PING_CODE_ESP32;

    uint8_t bdata[BPACKET_BUFFER_LENGTH_BYTES];
    bpacket_t bpacket;

    while (1) {

        // Delay for second
        vTaskDelay(200 / portTICK_PERIOD_MS);

        // Read UART and wait for command.
        if (esp32_uart_read_bpacket(bdata) == 0) {
            continue;
        }

        led_toggle(RED_LED);

        uint8_t result = bpacket_buffer_decode(&bpacket, bdata);

        if (result != TRUE) {
            esp32_uart_send_bpacket(&bpacket);
            led_on(RED_LED);
            continue;
        }

        // Confirm valid receiver
        // if (bpacket.receiver != BPACKET_ADDRESS_ESP32) {
        //     char m[50];
        //     sprintf(m, "Invalid receiver. Expected %i but got %i", BPACKET_ADDRESS_ESP32, bpacket.receiver);
        //     bpacket_create_sp(&bpacket, bpacket.sender, bpacket.receiver, bpacket.request, BPACKET_CODE_ERROR, m);
        //     esp32_uart_send_bpacket(&bpacket);
        // } else {
        //     char m[50];
        //     sprintf(m, "Sender: %i Receiver: %i", bpacket.sender, bpacket.receiver);
        //     bpacket_create_sp(&bpacket, bpacket.sender, bpacket.receiver, BPACKET_GET_R_MESSAGE,
        //     BPACKET_CODE_SUCCESS,
        //                       m);
        //     esp32_uart_send_bpacket(&bpacket);
        // }

        if ((bpacket.sender == BPACKET_ADDRESS_STM32) && (esp3_match_stm32_request(&bpacket) == TRUE)) {
            continue;
        }

        if ((bpacket.sender == BPACKET_ADDRESS_MAPLE) && (esp3_match_maple_request(&bpacket) == TRUE)) {
            continue;
        }

        char m[50];
        bpacket_t h;
        sprintf(m, "Generic request [%i][%i][%i]", bpacket.receiver, bpacket.sender, bpacket.request);
        bpacket_create_sp(&h, bpacket.sender, bpacket.receiver, bpacket.request, BPACKET_CODE_ERROR, m);
        esp32_uart_send_bpacket(&h);

        // The request was a generic request that could have been sent from the stm32
        // or from maple
        uint8_t request  = bpacket.request;
        uint8_t receiver = bpacket.receiver;
        uint8_t sender   = bpacket.sender;

        switch (bpacket.request) {
            case BPACKET_GEN_R_PING:
                bpacket_create_p(&bpacket, sender, receiver, request, BPACKET_CODE_SUCCESS, 1, &ping);
                esp32_uart_send_bpacket(&bpacket);
                break;
            case WATCHDOG_BPK_R_LED_RED_ON:
                led_on(RED_LED);
                bpacket_create_p(&bpacket, sender, receiver, request, BPACKET_CODE_SUCCESS, 0, NULL);
                esp32_uart_send_bpacket(&bpacket);
                break;
            case WATCHDOG_BPK_R_LED_RED_OFF:
                led_off(RED_LED);
                bpacket_create_p(&bpacket, sender, receiver, request, BPACKET_CODE_SUCCESS, 0, NULL);
                esp32_uart_send_bpacket(&bpacket);
                break;
            case WATCHDOG_BPK_R_WRITE_TO_FILE:

                break;
            default: // No request was able to be matched. Send response back to sender
                bpacket_create_sp(&bpacket, sender, receiver, request, BPACKET_CODE_UNKNOWN,
                                  "ESP32 could not recnognise the request\0");
                esp32_uart_send_bpacket(&bpacket);
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
        led_toggle(RED_LED);
    }
}
