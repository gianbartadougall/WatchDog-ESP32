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
#include <driver/uart.h>

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
#include "watchdog_utils.h"

/* Private Macros */
#define COB_LED HC_COB_LED
#define RED_LED HC_RED_LED

void esp_handle_request(bpk_t* Bpacket) {

    static char msg[150];
    static bpk_t BpkEspResponse;

    // Swap the sender and receiver
    uint8_t sender        = Bpacket->Sender.val;
    Bpacket->Sender.val   = Bpacket->Receiver.val;
    Bpacket->Receiver.val = sender;

    if (Bpacket->Request.val == BPK_REQ_TAKE_PHOTO) {

        /* Extract the Datetime and the temperature from the bpacket */
        dt_datetime_t Datetime;
        float temperature;
        camera_settings_t CameraSettings;
        wd_utils_array_to_photo_data(Bpacket->Data.bytes, &Datetime, &CameraSettings, &temperature);

        // Set the camera resolution
        if (camera_set_settings(&CameraSettings, msg) != TRUE) {
            bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request, BPK_Code_Error,
                          "Invalid camera resolution");
            esp32_uart_send_bpacket(&BpkEspResponse);
            return;
        }
        if (camera_capture_and_save_image1(&Datetime, temperature, msg) != TRUE) {
            bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request, BPK_Code_Error, msg);
        } else {
            bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request, BPK_Code_Success, msg);
        }

        esp32_uart_send_bpacket(&BpkEspResponse);
        return;
    }

    if (Bpacket->Request.val == BPK_REQ_COPY_FILE) {

        /* Extract the file name from the bpacket */
        char fileName[100];
        uint8_t i;
        for (i = 0; i < Bpacket->Data.numBytes; i++) {
            fileName[i] = Bpacket->Data.bytes[i];
        }
        fileName[i] = '\0';

        /****** START CODE BLOCK ******/
        // Description: Debugging. Can delete whenever
        // bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, BPK_Req_Copy_File, BPK_Code_Success,
        //               "ESP copying file!");
        // esp32_uart_send_bpacket(&BpkEspResponse);
        /****** END CODE BLOCK ******/

        if (sd_card_copy_file(fileName, msg) != TRUE) {
            bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request, BPK_Code_Error, msg);
            esp32_uart_send_bpacket(&BpkEspResponse);
            return;
        }

        /****** START CODE BLOCK ******/
        // Description: Debugging. Can delete whenever
        // bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, BPK_Req_Copy_File, BPK_Code_Success,
        //               "ESP has copied file!");
        // esp32_uart_send_bpacket(&BpkEspResponse);
        /****** END CODE BLOCK ******/

        /* No need to send Success code as this will be done by the SD card function */
        return;
    }

    if (Bpacket->Request.val == BPK_REQ_LIST_DIR) {

        /****** START CODE BLOCK ******/
        // Description: Debugging. Can delete whenever
        // bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, BPK_Req_List_Dir, BPK_Code_Success,
        //               "Starting!");
        // esp32_uart_send_bpacket(&BpkEspResponse);
        /****** END CODE BLOCK ******/

        if (sd_card_list_dir(msg) != TRUE) {
            bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request, BPK_Code_Error, msg);
            esp32_uart_send_bpacket(&BpkEspResponse);
            return;
        }

        /****** START CODE BLOCK ******/
        // Description: Debugging. Can delete whenever
        // bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, BPK_Req_List_Dir, BPK_Code_Success,
        //               "img1:img2:img3");
        // esp32_uart_send_bpacket(&BpkEspResponse);
        /****** END CODE BLOCK ******/

        /* No need to send Success code as this will be done by the SD card function */
        return;
    }

    if (Bpacket->Request.val == BPK_REQ_LED_RED_ON) {
        led_on(RED_LED);
        bpk_create(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request, BPK_Code_Success, 0, NULL);
        esp32_uart_send_bpacket(&BpkEspResponse);
        return;
    }

    if (Bpacket->Request.val == BPK_REQ_LED_RED_OFF) {
        led_off(RED_LED);
        bpk_create(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request, BPK_Code_Success, 0, NULL);
        esp32_uart_send_bpacket(&BpkEspResponse);
        return;
    }

    if (Bpacket->Request.val == BPK_REQUEST_PING) {
        bpk_create(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request, BPK_Code_Success, 0, NULL);
        esp32_uart_send_bpacket(&BpkEspResponse);
        return;
    }

    sprintf(msg, "Unknown request %i\r\n", Bpacket->Request.val);
    bpk_create_sp(&BpkEspResponse, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request, BPK_Code_Success, msg);
    esp32_uart_send_bpacket(&BpkEspResponse);
}

void watchdog_system_start(void) {

    // Turn all the LEDs off
    led_off(COB_LED);
    led_off(RED_LED);

    /* Send a ping out over UART to let Watchdog know the ESP is ready for requests */
    bpk_t Bpacket;
    bpk_create(&Bpacket, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Esp32, BPK_Request_Ping, BPK_Code_Success, 0, NULL);
    esp32_uart_send_bpacket(&Bpacket);

    bpk_reset(&Bpacket);
    int i = 0;
    while (1) {
        i++;
        // Delay for second
        vTaskDelay(200 / portTICK_PERIOD_MS);

        if (esp32_read_uart(&Bpacket) == TRUE) {
            esp_handle_request(&Bpacket);
        }

        if (i == 25) {
            i = 0;
            led_on(COB_LED);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            led_off(COB_LED);
        }
    }
}

uint8_t software_config(bpk_t* Bpacket) {

    // bpk_t b1;

    if (sd_card_init(Bpacket) != TRUE) {
        return FALSE;
    }

    // bpk_create(Bpacket, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple, BPK_Req_Get_Camera_Settings, BPK_Code_Execute,
    // 0,
    //            NULL);

    if (sd_card_format_sd_card(Bpacket) != TRUE) {
        esp32_uart_send_bpacket(Bpacket);
        return FALSE;
    }

    // Get the camera resolution saved on the SD card
    // wd_camera_settings_t cameraSettings;
    // if (sd_card_get_camera_settings(&cameraSettings) == TRUE) {
    //     char o[40];
    //     sprintf(o, "Setting camera resolution to: %i\r\n", cameraSettings.frameSize);
    //     bpk_create_sp(&b1, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message, BPK_Code_Success, o);
    //     esp32_uart_send_bpacket(&b1);
    //     camera_set_resolution(cameraSettings.frameSize);
    // }

    // Initialise the camera
    if (camera_init() != TRUE) {
        sd_card_log(LOG_FILE_NAME, "Camera failed to initialise\n\0");
        return FALSE;
    }

    // bpk_create_sp(&b1, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message, BPK_Code_Success,
    //               "All software initialised");
    // esp32_uart_send_bpacket(&b1);

    return TRUE;
}

void app_main(void) {

    bpk_t status;

    /* Initialise all the hardware used */
    if (hardware_config(&status) == TRUE) {
        watchdog_system_start();
    }

    /* Initialise all the hardware used */
    // if (hardware_config(&status) == TRUE && software_config(&status) == TRUE) {
    //     watchdog_system_start();
    // }

    // if (sd_card_open() == TRUE) {
    //     sd_card_log(LOG_FILE_NAME, "Exited Watchdog System. Waiting for shutdown");
    //     sd_card_close();
    // }

    uint8_t i = 0;
    while (1) {
        // esp32_uart_send_packet(&status);
        // esp32_uart_send_data("\r\n");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        if (i == 1) {
            led_on(RED_LED);
            i = 0;
        } else {
            led_off(RED_LED);
            i = 1;
        }
    }
}

/* Gabrage code */

/* Private Function Declarations */

// uint8_t esp3_match_stm32_request(bpk_t* Bpacket) {

//     switch (Bpacket->Request.val) {

//         case BPK_REQ_TAKE_PHOTO:
//             camera_capture_and_save_image(Bpacket);
//             break;

//         case BPK_REQ_RECORD_DATA:
//             break;

//         default:
//             return FALSE;
//     }

//     return TRUE;
// }

// uint8_t esp3_match_maple_request(bpk_t* Bpacket) {

//     bpacket_char_array_t bpacketCharArray;
//     cdt_u8_t CameraSettings;

//     switch (Bpacket->Request.val) {

//         case BPK_REQ_LIST_DIR:
//             bpk_data_to_string(Bpacket, &bpacketCharArray);
//             sd_card_list_directory(Bpacket, &bpacketCharArray);
//             break;

//         case BPK_REQ_COPY_FILE:;
//             bpk_data_to_string(Bpacket, &bpacketCharArray);
//             sd_card_copy_file(Bpacket, &bpacketCharArray);
//             break;

//         case BPK_REQ_STREAM_IMAGE:
//             camera_stream_image(Bpacket);
//             break;

//             // case BPK_REQ_SET_CAMERA_SETTINGS:;

//             //     // Read the camera settings from the bpacket
//             //     bpk_utils_read_cdt_u8(Bpacket, &CameraSettings, 1);

//             //     // Try to set the camera settings on the ESP32
//             //     if (camera_set_resolution(CameraSettings.value) != TRUE) {
//             //         bpk_create_string_response(Bpacket, BPK_Code_Error, "Invalid resolution\0");
//             //         esp32_uart_send_bpacket(Bpacket);
//             //         break;
//             //     }

//             //     // Currently the SD card will send a response back if it fails
//             //     if (sd_card_write_settings(Bpacket) != TRUE) {
//             //         break;
//             //     }

//             //     // Send response back to sender
//             //     bpk_create_response(Bpacket, BPK_Code_Success);
//             //     esp32_uart_send_bpacket(Bpacket);
//             //     break;

//             // case BPK_REQ_GET_CAMERA_SETTINGS:;

//             //     bpk_create_response(Bpacket, BPK_Code_Success);
//             //     CameraSettings.value = camera_get_resolution();
//             //     bpk_utils_write_cdt_u8(Bpacket, &CameraSettings, 1);
//             //     esp32_uart_send_bpacket(Bpacket);
//             //     break;

//         default:
//             return FALSE;
//     }

//     return TRUE;
// }

// Read UART and wait for command.
// if (esp32_uart_read_bpacket(bdata) == 0) {
//     continue;
// }

// uint8_t result = bpk_buffer_decode(&Bpacket, bdata);

// if (result != TRUE) {
//     bpk_utils_create_string_response(&Bpacket, BPK_Code_Debug, "Failed to decode");
//     esp32_uart_send_bpacket(&Bpacket);
//     continue;
// }

// if ((Bpacket.Sender.val == BPK_Addr_Send_Stm32.val) && (esp3_match_stm32_request(&Bpacket) == TRUE)) {
//     continue;
// }

// if ((Bpacket.Sender.val == BPK_Addr_Send_Maple.val) && (esp3_match_maple_request(&Bpacket) == TRUE)) {
//     continue;
// }

// switch (Bpacket.Request.val) {

//     case BPK_REQUEST_PING:
//         bpk_convert_to_response(&Bpacket, BPK_Code_Success, 1, &ping);
//         esp32_uart_send_bpacket(&Bpacket);
//         break;

//     case BPK_REQ_LED_RED_ON:
//         led_on(RED_LED);
//         bpk_convert_to_response(&Bpacket, BPK_Code_Success, 0, NULL);
//         esp32_uart_send_bpacket(&Bpacket);
//         break;

//     case BPK_REQ_LED_RED_OFF:
//         led_off(RED_LED);
//         bpk_convert_to_response(&Bpacket, BPK_Code_Success, 0, NULL);
//         esp32_uart_send_bpacket(&Bpacket);
//         break;

//     case BPK_REQ_WRITE_TO_FILE:

//         break;

//         // case BPK_REQ_SET_CAMERA_CAPTURE_TIMES:

//         //     // SD Card write settings function currently sends messages back by itself
//         //     // if an error occurs
//         //     if (sd_card_write_settings(&Bpacket) != TRUE) {
//         //         break;
//         //     }

//         //     // Send success response
//         //     bpk_create_response(&Bpacket, BPK_Code_Success);
//         //     esp32_uart_send_bpacket(&Bpacket); // Send response back

//         //     break;

//         // case BPK_REQ_GET_CAMERA_CAPTURE_TIMES:

//         //     sd_card_read_settings(&Bpacket);
//         //     esp32_uart_send_bpacket(&Bpacket); // Send response back
//         //     break;

//     default:; // No request was able to be matched. Send response back to sender
//         char errMsg[100];
//         sprintf(errMsg, "Unknown Request: %i %i %i %i %i", Bpacket.Receiver.val, Bpacket.Sender.val,
//                 Bpacket.Code.val, Bpacket.Request.val, Bpacket.Data.numBytes);
//         bpk_create_string_response(&Bpacket, BPK_Code_Unknown, errMsg);
//         esp32_uart_send_bpacket(&Bpacket);
//         break;
// }