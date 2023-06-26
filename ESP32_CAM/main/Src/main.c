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
#include "bpacket_utils.h"

/* Private Macros */
#define COB_LED HC_COB_LED
#define RED_LED HC_RED_LED

/* Private Function Declarations */

uint8_t esp3_match_stm32_request(bpk_t* Bpacket) {

    switch (Bpacket->Request.val) {

        case BPK_REQ_TAKE_PHOTO:
            camera_capture_and_save_image(Bpacket);
            break;

        case BPK_REQ_RECORD_DATA:
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

uint8_t esp3_match_maple_request(bpk_t* Bpacket) {

    bpacket_char_array_t bpacketCharArray;
    cdt_u8_t CameraSettings;

    switch (Bpacket->Request.val) {

        case BPK_REQ_LIST_DIR:
            bpk_data_to_string(Bpacket, &bpacketCharArray);
            sd_card_list_directory(Bpacket, &bpacketCharArray);
            break;

        case BPK_REQ_COPY_FILE:;
            bpk_data_to_string(Bpacket, &bpacketCharArray);
            sd_card_copy_file(Bpacket, &bpacketCharArray);
            break;

        case BPK_REQ_STREAM_IMAGE:
            camera_stream_image(Bpacket);
            break;

        case BPK_REQ_SET_CAMERA_SETTINGS:;

            // Read the camera settings from the bpacket
            bpk_utils_read_cdt_u8(Bpacket, &CameraSettings, 1);

            // Try to set the camera settings on the ESP32
            if (camera_set_resolution(CameraSettings.value) != TRUE) {
                bpk_create_string_response(Bpacket, BPK_Code_Error, "Invalid resolution\0");
                esp32_uart_send_bpacket(Bpacket);
                break;
            }

            // Currently the SD card will send a response back if it fails
            if (sd_card_write_settings(Bpacket) != TRUE) {
                break;
            }

            // Send response back to sender
            bpk_create_response(Bpacket, BPK_Code_Success);
            esp32_uart_send_bpacket(Bpacket);
            break;

        case BPK_REQ_GET_CAMERA_SETTINGS:;

            bpk_create_response(Bpacket, BPK_Code_Success);
            CameraSettings.value = camera_get_resolution();
            bpk_utils_write_cdt_u8(Bpacket, &CameraSettings, 1);
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

    // Take a photo in a loop
    bpk_t Pack;
    // Add dummy data
    dt_datetime_t Datetime;
    dt_datetime_init(&Datetime, 0, 0, 7, 1, 6, 2023);
    cdt_dbl_16_t Temp1, Temp2;
    Temp1.decimal = 15;
    Temp1.integer = 18;
    Temp2.decimal = 23;
    Temp2.integer = 32;
    wd_photo_data_to_bpacket(&Pack, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Stm32, BPK_Req_Take_Photo, BPK_Code_Execute,
                             &Datetime, &Temp1, &Temp2);
    while (1) {
        camera_capture_and_save_image(&Pack);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    uint8_t ping = WATCHDOG_PING_CODE_ESP32;

    uint8_t bdata[BPACKET_BUFFER_LENGTH_BYTES];
    bpk_t Bpacket;

    while (1) {

        // Delay for second
        vTaskDelay(200 / portTICK_PERIOD_MS);

        // Read UART and wait for command.
        if (esp32_uart_read_bpacket(bdata) == 0) {
            continue;
        }

        uint8_t result = bpk_buffer_decode(&Bpacket, bdata);

        if (result != TRUE) {
            bpk_utils_create_string_response(&Bpacket, BPK_Code_Debug, "Failed to decode");
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
                bpk_convert_to_response(&Bpacket, BPK_Code_Success, 1, &ping);
                esp32_uart_send_bpacket(&Bpacket);
                break;

            case BPK_REQ_LED_RED_ON:
                led_on(RED_LED);
                bpk_convert_to_response(&Bpacket, BPK_Code_Success, 0, NULL);
                esp32_uart_send_bpacket(&Bpacket);
                break;

            case BPK_REQ_LED_RED_OFF:
                led_off(RED_LED);
                bpk_convert_to_response(&Bpacket, BPK_Code_Success, 0, NULL);
                esp32_uart_send_bpacket(&Bpacket);
                break;

            case BPK_REQ_WRITE_TO_FILE:

                break;

            case BPK_REQ_SET_CAMERA_CAPTURE_TIMES:

                // SD Card write settings function currently sends messages back by itself
                // if an error occurs
                if (sd_card_write_settings(&Bpacket) != TRUE) {
                    break;
                }

                // Send success response
                bpk_create_response(&Bpacket, BPK_Code_Success);
                esp32_uart_send_bpacket(&Bpacket); // Send response back

                break;

            case BPK_REQ_GET_CAMERA_CAPTURE_TIMES:

                sd_card_read_settings(&Bpacket);
                esp32_uart_send_bpacket(&Bpacket); // Send response back
                break;

            default:; // No request was able to be matched. Send response back to sender
                char errMsg[100];
                sprintf(errMsg, "Unknown Request: %i %i %i %i %i", Bpacket.Receiver.val, Bpacket.Sender.val,
                        Bpacket.Code.val, Bpacket.Request.val, Bpacket.Data.numBytes);
                bpk_create_string_response(&Bpacket, BPK_Code_Unknown, errMsg);
                esp32_uart_send_bpacket(&Bpacket);
                break;
        }
    }
}

uint8_t software_config(bpk_t* Bpacket) {

    bpk_t b1;

    if (sd_card_init(Bpacket) != TRUE) {
        return FALSE;
    }

    bpk_create(Bpacket, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple, BPK_Req_Get_Camera_Settings, BPK_Code_Execute, 0,
               NULL);

    if (sd_card_format_sd_card(Bpacket) != TRUE) {
        esp32_uart_send_bpacket(Bpacket);
        return FALSE;
    }

    // Get the camera resolution saved on the SD card
    wd_camera_settings_t cameraSettings;
    if (sd_card_get_camera_settings(&cameraSettings) == TRUE) {
        char o[40];
        sprintf(o, "Setting camera resolution to: %i\r\n", cameraSettings.resolution);
        bpk_create_sp(&b1, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message, BPK_Code_Success, o);
        esp32_uart_send_bpacket(&b1);
        camera_set_resolution(cameraSettings.resolution);
    }

    // Initialise the camera
    if (camera_init() != TRUE) {
        sd_card_log(SYSTEM_LOG_FILE, "Camera failed to initialise\n\0");
        return FALSE;
    }

    bpk_create_sp(&b1, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message, BPK_Code_Success,
                  "All software initialised");
    esp32_uart_send_bpacket(&b1);

    return TRUE;
}

void app_main(void) {

    bpk_t status;

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
