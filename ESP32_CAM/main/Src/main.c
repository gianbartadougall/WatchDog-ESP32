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
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

#include "sd_card.h"
#include "camera.h"
#include "hardware_config.h"
#include "led.h"
#include "wd_utils.h"
#include "bpacket.h"

#include "hardware_config.h"
#include "esp32_uart.h"

#define COB_LED HC_COB_LED
#define RED_LED HC_RED_LED

/* Private Function Declarations */

void generate_status_string(packet_t* response) {

    /**
     * @brief Status String is of the form
     *
     * Number of images: 34
     * SD Card size: 32000Mb
     * Errors: No
     */

    uint16_t numImages = 0;
    if (sd_card_search_num_images(&numImages, response) != WD_SUCCESS) {
        return;
    }

    response->request        = UART_REQUEST_SUCCEEDED;
    response->instruction[0] = '\0';
    sprintf(response->data, "Number of images: %i\r\n", numImages);
}

void record_data(packet_t* packet, packet_t* response) {

    camera_capture_and_save_image(response);
    if (response->request != UART_REQUEST_SUCCEEDED) {
        return;
    }

    sd_card_write_to_file(packet->instruction, packet->data, response);
}

void watchdog_system_start(void) {

    // Turn all the LEDs off
    led_off(COB_LED);
    led_off(RED_LED);

    uint8_t ping[1];
    ping[0] = 23;

    // char instruction[100];
    // char data[RX_BUF_SIZE];
    // packet_t packet, response;
    uint8_t bdata[BPACKET_BUFFER_LENGTH_BYTES];
    bpacket_t bpacket;

    while (1) {

        // Delay for second
        vTaskDelay(200 / portTICK_PERIOD_MS);

        // Read UART and wait for command.
        if (esp32_uart_read_bpacket(bdata) == 0) {
            continue;
        }

        bpacket_decode(&bpacket, bdata);

        switch (bpacket.request) {
            case BPACKET_R_LED_RED_ON:
                led_on(RED_LED);
                break;
            case BPACKET_R_LED_RED_OFF:
                led_off(RED_LED);
                break;
            case BPACKET_R_COPY_FILE:
                sd_card_copy_file(&bpacket);
                break;
            case BPACKET_R_LIST_DIR:
                sd_card_list_directory("\0", &bpacket);
                break;
            case BPACKET_R_PING:
                bpacket_create_p(&bpacket, BPACKET_R_SUCCESS, 1, ping);
                esp32_uart_send_bpacket(&bpacket);
                break;
            default:
                break;
        }

        continue;
    }
    // Read UART and wait for command.
    // const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);

    // if (rxBytes == 0) {
    //     continue;
    // }

    // get_command(data);

    // Validate incoming data
    //     if (string_to_packet(&packet, data) != WD_SUCCESS) {
    //         uart_comms_create_packet(&response, UART_ERROR_REQUEST_TRANSLATION_FAILED, "Failed to parse packet",
    //         data); send_packet(&response); sendData("\r\n\0"); // REMOVE WHEN GOING BACK TO USING THE STM32
    //         continue;
    //     }

    //     // Carry out instruction
    //     switch (packet.request) {
    //         case UART_REQUEST_LED_RED_ON:
    //             led_on(RED_LED);
    //             uart_comms_create_packet(&response, UART_REQUEST_SUCCEEDED, "LED has been turned on", "\0");
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_LED_RED_OFF:
    //             led_off(RED_LED);
    //             uart_comms_create_packet(&response, UART_REQUEST_SUCCEEDED, "LED has been turned off", "\0");
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_LED_COB_ON:
    //             led_on(COB_LED);
    //             uart_comms_create_packet(&response, UART_REQUEST_SUCCEEDED, "COB LED has been turned on", "\0");
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_LED_COB_OFF:
    //             led_off(COB_LED);
    //             uart_comms_create_packet(&response, UART_REQUEST_SUCCEEDED, "COB LED has been turned off", "\0");
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_LIST_DIRECTORY:
    //             sd_card_list_directory(packet.instruction, &response);
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_COPY_FILE:
    //             sd_card_copy_file(&packet, &response);
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_TAKE_PHOTO:
    //             camera_capture_and_save_image(&response);
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_WRITE_TO_FILE:
    //             sd_card_write_to_file(packet.instruction, packet.data, &response);
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_CREATE_PATH:
    //             sd_card_create_path(packet.instruction, &response);
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_STATUS:
    //             generate_status_string(&response);
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_RECORD_DATA:
    //             record_data(&packet, &response);
    //             send_packet(&response);
    //             break;
    //         case UART_REQUEST_PING:
    //             sendData("ESP32 Watchdog\0");
    //             break;
    //         default:; // comma here required because only statments can follow a label
    //             char errMsg[50];
    //             sprintf(errMsg, "Request %i is unkown", packet.request);
    //             uart_comms_create_packet(&response, UART_ERROR_REQUEST_UNKNOWN, errMsg, "\0");
    //             send_packet(&response);
    //     }

    //     sendData("\r\n\0"); // REMOVE WHEN GOING BACK TO USING THE STM32
    // }
}

uint8_t software_config(packet_t* response) {

    if (sd_card_init(response) != WD_SUCCESS) {
        return WD_ERROR;
    }

    return WD_SUCCESS;
}

void app_main(void) {

    packet_t status;

    /* Initialise all the hardware used */
    if (hardware_config(&status) == WD_SUCCESS && software_config(&status) == WD_SUCCESS) {
        watchdog_system_start();
    }

    if (sd_card_open() == WD_SUCCESS) {
        sd_card_log(SYSTEM_LOG_FILE, "Exited Watchdog System. Waiting for shutdown");
        sd_card_close();
    }

    while (1) {
        // esp32_uart_send_packet(&status);
        // esp32_uart_send_data("\r\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
