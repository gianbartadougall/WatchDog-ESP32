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

static const char *TAG = "example";
#include "camera.h"
#include "hardware_config.h"
#include "led.h"
#include "sd_card.h"
#include "uart_comms.h"
#include "wd_utils.h"

#include "hardware_config.h"

#define COB_LED HC_COB_LED
#define RED_LED HC_RED_LED
#define UART_NUM HC_UART_COMMS_UART_NUM

/* Private Function Declarations */
void esp32_led_on(void);
void esp32_led_off(void);

int sendData(const char *logName, const char *data) {
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM, data, len);
    // ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

// static void tx_task(void *arg) {
//     static const char *TX_TASK_TAG = "TX_TASK";
//     esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
//     while (1) {
//         sendData(TX_TASK_TAG, "Hello world");
//         // gpio_set_level(LED, 0);
//         vTaskDelay(2000 / portTICK_PERIOD_MS);
//     }
// }

void watchdog_system_start(void) {

    // char instruction[100];
    char data[RX_BUF_SIZE];

    while (1) {
        // Delay for second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Transmit message

        // Read UART and wait for command.
        const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);

        if (rxBytes == 0) {
            continue;
        }

        // Validate incoming data
        packet_t packet;
        if (string_to_packet(&packet, data) != WD_SUCCESS) {

            // Create new packet to reply
            packet_t responsePacket;
            responsePacket.request = UART_REQUEST_PARSE_ERROR;
            responsePacket.instruction[0] = '\0';
            responsePacket.data[0] = '\0';

            char response[RX_BUF_SIZE];
            if (packet_to_string(&responsePacket, response) != WD_SUCCESS) {
                continue;
            }

            uart_write_bytes(UART_NUM, response, strlen(response));
            continue;
        }

        // Send return message
        packet_t response;
        response.request = UART_REQUEST_ACKNOWLEDGED;

        // Carry out instruction
        switch (packet.request) {
        case UART_REQUEST_LED_ON:
            led_on(RED_LED);
            sprintf(response.instruction, "turning the LED on");
            response.data[0] = '\0';
            break;
        case UART_REQUEST_LED_OFF:
            led_off(RED_LED);
            sprintf(response.instruction, "turning the LED off");
            response.data[0] = '\0';
            break;
        default:
            // Should never enter here because this is checked when converting string to packet
            sprintf(response.instruction, "Request was not recognised");
            response.data[0] = '\0';
        }

        char responseData[RX_BUF_SIZE];
        packet_to_string(&response, responseData);
        uart_write_bytes(UART_NUM, responseData, strlen(responseData));
        // if (action == UC_SAVE_DATA) {

        //     // Extract the file path
        //     char data[2][100]; // data[0] = filePath, data[1] = fileName
        //     wd_utils_split_string(instruction, data, 0, UC_DATA_DELIMETER);

        //     // Save data to SD card
        //     sd_card_open();
        //     sd_card_write(data[0], data[1], message);
        //     sd_card_close();
        // }

        // if (action == UC_ACTION_CAPTURE_IMAGE) {
        //     if (camera_take_image() != WD_SUCCESS) {
        //         break;
        //     }
        // }

        // Reset the action back to NONE
        // action = UC_COMMAND_NONE;
    }
}

uint8_t software_config(void) {

    // if (sd_card_init() != WD_SUCCESS) {
    //     return WD_ERROR;
    // }

    return WD_SUCCESS;
}

void app_main(void) {

    /* Initialise all the hardware used */
    if ((hardware_config() == WD_SUCCESS) && (software_config() == WD_SUCCESS)) {
        // xTaskCreate(tx_task, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 2, NULL);
        watchdog_system_start();
    }

    // if (sd_card_open() == WD_SUCCESS) {
    //     sd_card_log(SYSTEM_LOG_FILE, "Exited Watchdog System. Waiting for shutdown");
    //     sd_card_close();
    // }

    while (1) {
        ESP_LOGE(TAG, "Blink");
        vTaskDelay(300 / portTICK_PERIOD_MS);
        led_toggle(RED_LED);
    }
}
