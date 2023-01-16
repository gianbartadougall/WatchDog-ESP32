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
#include "uart_comms.h"
#include "wd_utils.h"

#include "hardware_config.h"

#define COB_LED  HC_COB_LED
#define RED_LED  HC_RED_LED
#define UART_NUM HC_UART_COMMS_UART_NUM

/* Private Function Declarations */
void sd_card_copy_folder_structure(packet_t* requestPacket, packet_t* responsePacket);
void sd_card_copy_file(packet_t* requestPacket, packet_t* responsePacket);

int sendData(const char* data) {

    // Because this data is being sent to a master MCU, the NULL character
    // needs to be appended on if it is not to ensure the master MCU knows
    // when the end of the sent data is. Thus add 1 to the length to ensure
    // the null character is also sent
    const int len     = strlen(data) + 1;
    const int txBytes = uart_write_bytes(UART_NUM, data, len);

    return txBytes;
}

void send_packet(packet_t* packet) {
    char string[RX_BUF_SIZE];
    packet_to_string(packet, string);
    sendData(string);
}

// static void tx_task(void* arg) {
//     static const char* TX_TASK_TAG = "TX_TASK";
//     esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
//     while (1) {
//         sendData("Hello world");
//         // gpio_set_level(LED, 0);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }

void get_command(char msg[200]) {

    int i = 0;
    char data[5];
    while (1) {

        // Read data from the UART quick enough to get a single character
        int len = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, 20 / portTICK_RATE_MS);

        // Write data back to the UART
        if (len == 0) {
            continue;
        }

        if (len == 1) {
            uart_write_bytes(UART_NUM, (const char*)data, len);

            // Check for enter key
            if (data[0] == 0x0D) {
                msg[i++] = '\r';
                msg[i++] = '\n';
                msg[i]   = '\0';
                sendData(msg);

                msg[i - 2] = '\0';
                return;
            }

            if (data[0] >= '!' || data[0] <= '~') {
                msg[i++] = data[0];
            }
        }
    }
}

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

void watchdog_system_start(void) {

    // Turn all the LEDs off
    led_off(COB_LED);
    led_off(RED_LED);

    // char instruction[100];
    char data[RX_BUF_SIZE];

    while (1) {

        // Delay for second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Read UART and wait for command.
        // const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, 10000 / portTICK_PERIOD_MS);

        // if (rxBytes == 0) {
        //     continue;
        // }
        get_command(data);

        // Validate incoming data
        packet_t packet;
        if (string_to_packet(&packet, data) != WD_SUCCESS) {
            sendData("ESP32 failed to parse packet\0");
            sendData("\r\n\0"); // REMOVE WHEN GOING BACK TO USING THE STM32
            continue;
        }

        // Carry out instruction
        packet_t response;
        switch (packet.request) {
            case UART_REQUEST_LED_RED_ON:
                led_on(RED_LED);
                uart_comms_create_packet(&response, UART_REQUEST_SUCCEEDED, "LED has been turned on", "\0");
                send_packet(&response);
                break;
            case UART_REQUEST_LED_RED_OFF:
                led_off(RED_LED);
                uart_comms_create_packet(&response, UART_REQUEST_SUCCEEDED, "LED has been turned off", "\0");
                send_packet(&response);
                break;
            case UART_REQUEST_LED_COB_ON:
                led_on(COB_LED);
                uart_comms_create_packet(&response, UART_REQUEST_SUCCEEDED, "COB LED has been turned on", "\0");
                send_packet(&response);
                break;
            case UART_REQUEST_LED_COB_OFF:
                led_off(COB_LED);
                uart_comms_create_packet(&response, UART_REQUEST_SUCCEEDED, "COB LED has been turned off", "\0");
                send_packet(&response);
                break;
            case UART_REQUEST_LIST_DIRECTORY:
                sd_card_list_directory(packet.instruction, &response);
                send_packet(&response);
                break;
            case UART_REQUEST_COPY_FILE:
                sd_card_copy_file(&packet, &response);
                send_packet(&response);
                break;
            case UART_REQUEST_TAKE_PHOTO:
                camera_capture_and_save_image(&response);
                send_packet(&response);
                break;
            case UART_REQUEST_WRITE_TO_FILE:
                sd_card_write_to_file(packet.instruction, packet.data, &response);
                send_packet(&response);
                break;
            case UART_REQUEST_CREATE_PATH:
                sd_card_create_path(packet.instruction, &response);
                send_packet(&response);
                break;
            case UART_REQUEST_STATUS:
                generate_status_string(&response);
                send_packet(&response);
                break;
            default:; // comma here required because only statments can follow a label
                char errMsg[50];
                sprintf(errMsg, "Request %i is unkown", packet.request);
                uart_comms_create_packet(&response, UART_ERROR_REQUEST_UNKNOWN, errMsg, "\0");
                send_packet(&response);
        }

        sendData("\r\n\0"); // REMOVE WHEN GOING BACK TO USING THE STM32
    }
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
        send_packet(&status);
        sendData("\r\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
