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
#include "utils.h"

/* Private Macros */
#define COB_LED HC_COB_LED
#define RED_LED HC_RED_LED

/* Private Function Declarations */

uint8_t esp3_match_stm32_request(bpk_packet_t* Bpacket) {

    switch (Bpacket->Request.val) {

        case BPK_WD_REQUEST_TAKE_PHOTO:
            camera_capture_and_save_image(Bpacket);
            break;

        case BPK_WD_REQUEST_RECORD_DATA:
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

uint8_t esp3_match_maple_request(bpk_packet_t* Bpacket) {

    bpacket_char_array_t bpacketCharArray;
    wd_camera_settings_t cameraSettings;
    uint8_t result;

    switch (Bpacket->Request.val) {

        case BPK_WD_REQUEST_LIST_DIR:
            bpacket_data_to_string(Bpacket, &bpacketCharArray);
            sd_card_list_directory(Bpacket, &bpacketCharArray);
            break;

        case BPK_WD_REQUEST_COPY_FILE:;
            bpacket_data_to_string(Bpacket, &bpacketCharArray);
            sd_card_copy_file(Bpacket, &bpacketCharArray);
            break;

        case BPK_WD_REQUEST_STREAM_IMAGE:
            camera_stream_image(Bpacket);
            break;

        case BPK_WD_REQUEST_SET_CAMERA_SETTINGS:;

            result = wd_bpacket_to_camera_settings(Bpacket, &cameraSettings);
            if (result != TRUE) {
                char errMsg[50];
                wd_get_error(result, errMsg);
                bp_create_string_response(Bpacket, BPK_Code_Error, errMsg);
                esp32_uart_send_bpacket(Bpacket);
                break;
            }

            if (camera_set_resolution(cameraSettings.resolution) != TRUE) {
                bp_create_string_response(Bpacket, BPK_Code_Error, "Invalid resolution\r\n\0");
                esp32_uart_send_bpacket(Bpacket);
                break;
            }

            if (sd_card_write_settings(Bpacket) == TRUE) {
                esp32_uart_send_bpacket(Bpacket);
            }

            break;

        case BPK_WD_REQUEST_GET_CAMERA_SETTINGS:;

            cameraSettings.resolution = camera_get_resolution();

            bpk_swap_address(Bpacket);
            if (wd_camera_settings_to_bpacket(Bpacket, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request,
                                              BPK_Code_Success, &cameraSettings) != TRUE) {
                esp32_uart_send_bpacket(Bpacket); // Send bpacket error
            }

            esp32_uart_send_bpacket(Bpacket);
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
    bpk_packet_t Bpacket;

    while (1) {

        // Delay for second
        vTaskDelay(200 / portTICK_PERIOD_MS);

        // Read UART and wait for command.
        if (esp32_uart_read_bpacket(bdata) == 0) {
            continue;
        }

        uint8_t result = bpacket_buffer_decode(&Bpacket, bdata);

        if (result != TRUE) {
            esp32_uart_send_bpacket(&Bpacket);
            continue;
        }

        if ((Bpacket.Sender.val == BPK_Addr_Send_Stm32.val) && (esp3_match_stm32_request(&Bpacket) == TRUE)) {
            continue;
        }

        if ((Bpacket.Sender.val == BPK_Addr_Send_Maple.val) && (esp3_match_maple_request(&Bpacket) == TRUE)) {
            continue;
        }

        switch (Bpacket.Request.val) {

            case BPK_REQUEST_PING:
                bp_convert_to_response(&Bpacket, BPK_Code_Success, 1, &ping);
                esp32_uart_send_bpacket(&Bpacket);
                break;

            case BPK_WD_REQUEST_LED_RED_ON:
                led_on(RED_LED);
                bp_convert_to_response(&Bpacket, BPK_Code_Success, 0, NULL);
                esp32_uart_send_bpacket(&Bpacket);
                break;

            case BPK_WD_REQUEST_LED_RED_OFF:
                led_off(RED_LED);
                bp_convert_to_response(&Bpacket, BPK_Code_Success, 0, NULL);
                esp32_uart_send_bpacket(&Bpacket);
                break;

            case BPK_WD_REQUEST_WRITE_TO_FILE:

                break;

            case BPK_WD_REQUEST_SET_CAPTURE_TIME_SETTINGS:

                if (sd_card_write_settings(&Bpacket) == TRUE) {
                    esp32_uart_send_bpacket(&Bpacket); // Send response back
                }
                break;

            case BPK_WD_REQUEST_GET_CAPTURE_TIME_SETTINGS:

                sd_card_read_settings(&Bpacket);
                esp32_uart_send_bpacket(&Bpacket); // Send response back
                break;

            default:; // No request was able to be matched. Send response back to sender
                bp_create_string_response(&Bpacket, BPK_Code_Unknown, "Unknown Request\r\n\0");
                esp32_uart_send_bpacket(&Bpacket);
                break;
        }
    }
}

uint8_t software_config(bpk_packet_t* Bpacket) {

    bpk_packet_t b1;

    if (sd_card_init(Bpacket) != TRUE) {
        return FALSE;
    }

    bp_create_packet(Bpacket, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple, BPK_WD_Request_Get_Camera_Settings,
                     BPK_Code_Execute, 0, NULL);

    if (sd_card_format_sd_card(Bpacket) != TRUE) {
        esp32_uart_send_bpacket(Bpacket);
        return FALSE;
    }

    // Get the camera resolution saved on the SD card
    wd_camera_settings_t cameraSettings;
    if (sd_card_get_camera_settings(&cameraSettings) == TRUE) {
        char o[40];
        sprintf(o, "Setting camera resolution to: %i\r\n", cameraSettings.resolution);
        bpacket_create_sp(&b1, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message, BPK_Code_Success, o);
        esp32_uart_send_bpacket(&b1);
        camera_set_resolution(cameraSettings.resolution);
    }

    // Initialise the camera
    if (camera_init() != TRUE) {
        sd_card_log(SYSTEM_LOG_FILE, "Camera failed to initialise\n\0");
        return FALSE;
    }

    bpacket_create_sp(&b1, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message, BPK_Code_Success,
                      "All software initialised");
    esp32_uart_send_bpacket(&b1);

    return TRUE;
}

void app_main(void) {

    bpk_packet_t status;

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
