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

static const char* TAG = "example";
#include "camera.h"
#include "hardware_config.h"
#include "led.h"
#include "sd_card.h"
#include "uart_comms.h"
#include "wd_utils.h"

#include "hardware_config.h"

#define COB_LED  HC_COB_LED
#define RED_LED  HC_RED_LED
#define UART_NUM HC_UART_COMMS_UART_NUM

/* Private Function Declarations */
void esp32_led_on(void);
void esp32_led_off(void);

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

void watchdog_system_start(void) {

    // char instruction[100];
    char data[RX_BUF_SIZE];

    // while (1) {
    //     sendData("Sent some data to the STM32 to be recorded!!!\0");
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     led_toggle(RED_LED);
    // }

    while (1) {
        // Delay for second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led_toggle(RED_LED);
        // Transmit message

        // Read UART and wait for command.
        const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);

        if (rxBytes == 0) {
            sendData("Received nothing\0");
            continue;
        }

        char msg[200];
        sprintf(msg, "ESP32 received: '%s'", data);
        sendData(msg);

        // Validate incoming data
        // packet_t packet;
        // if (string_to_packet(&packet, data) != WD_SUCCESS) {
        //     sendData(data);
        //     continue;
        // }

        // // Send return message
        // packet_t response;
        // response.request = packet.request;
        // sprintf(response.instruction, "%s", packet.instruction);
        // sprintf(response.data, "%s", packet.data);
        // send_packet(&response);

        continue;

        // Carry out instruction
        // switch (packet.request) {
        //     case UART_REQUEST_LED_ON:
        //         led_on(RED_LED);
        //         sprintf(response.instruction, "turning the LED on");
        //         response.data[0] = '\0';
        //         break;
        //     case UART_REQUEST_LED_OFF:
        //         led_off(RED_LED);
        //         sprintf(response.instruction, "turning the LED off");
        //         response.data[0] = '\0';
        //         break;
        //     case UART_REQUEST_DATA_READ:
        //         sprintf(response.instruction, "Data read requested");
        //         sprintf(response.data, "%s", packet.instruction);
        //         // sd_card_data_copy(&response);
        //         break;
        //     default:
        //         // For some reason, sprintf didn't appear to be adding \0 on the end
        //         // may be a bug somewhere else but for the moment, leaving it on
        //         sprintf(response.instruction, "Request %i was not recognised", packet.request);
        //         response.data[0] = '\0';
        // }

        // char responseData[RX_BUF_SIZE];
        // packet_to_string(&response, responseData);
        // uart_write_bytes(UART_NUM, responseData, strlen(responseData));

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
        // xTaskCreate(watchdog_system_start, "watchdog_task", 1024 * 2, NULL, configMAX_PRIORITIES - 2, NULL);
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
