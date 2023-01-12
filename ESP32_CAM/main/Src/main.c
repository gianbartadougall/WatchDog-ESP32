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
            case UART_REQUEST_FOLDER_STRUCTURE:
                sd_card_copy_folder_structure(&packet, &response);
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
            case UART_REQUEST_DATA_READ:
                uart_comms_create_packet(&response, UART_REQUEST_SUCCEEDED, "Data has been retrieved",
                                         "Temperature: 35.6 degrees");
                send_packet(&response);
                // sd_card_data_copy(&response);
                break;
            default:; // comma here required because only statments can follow a label

                char errMsg[50];
                sprintf(errMsg, "Request %i is unkown", packet.request);
                uart_comms_create_packet(&response, UART_ERROR_REQUEST_UNKNOWN, errMsg, "\0");
                send_packet(&response);
        }

        sendData("\r\n\0"); // REMOVE WHEN GOING BACK TO USING THE STM32

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

    if (sd_card_init() != WD_SUCCESS) {
        return WD_ERROR;
    }

    return WD_SUCCESS;
}

void app_main(void) {

    packet_t status;

    /* Initialise all the hardware used */
    if ((hardware_config(&status) == WD_SUCCESS) && (software_config() == WD_SUCCESS)) {
        // xTaskCreate(tx_task, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 2, NULL);
        // xTaskCreate(watchdog_system_start, "watchdog_task", 1024 * 2, NULL, configMAX_PRIORITIES - 2, NULL);
        watchdog_system_start();
    }

    if (sd_card_open() == WD_SUCCESS) {
        sd_card_log(SYSTEM_LOG_FILE, "Exited Watchdog System. Waiting for shutdown");
        sd_card_close();
    }

    while (1) {
        send_packet(&status);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
