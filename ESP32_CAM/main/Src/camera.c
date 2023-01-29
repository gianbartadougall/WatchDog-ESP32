/**
 * @file camera.c
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2022-12-19
 *
 * @copyright Copyright (c) 2022
 *
 */

/* Personal Includes */
#include "camera.h"
#include "sd_card.h"
#include "esp32_uart.h"
#include "utilities.h"

/* Private Macros */
#define BOARD_ESP32CAM_AITHINKER

// support IDF 5.x
#ifndef portTICK_RATE_MS
    #define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

    #define CAM_PIN_PWDN  32
    #define CAM_PIN_RESET -1 // software reset will be performed
    #define CAM_PIN_XCLK  0
    #define CAM_PIN_SIOD  26
    #define CAM_PIN_SIOC  27

    #define CAM_PIN_D7    35
    #define CAM_PIN_D6    34
    #define CAM_PIN_D5    39
    #define CAM_PIN_D4    36
    #define CAM_PIN_D3    21
    #define CAM_PIN_D2    19
    #define CAM_PIN_D1    18
    #define CAM_PIN_D0    5
    #define CAM_PIN_VSYNC 25
    #define CAM_PIN_HREF  23
    #define CAM_PIN_PCLK  22

#endif

/* Private Variable Declarations */

static camera_config_t camera_config = {
    .pin_pwdn     = CAM_PIN_PWDN,
    .pin_reset    = CAM_PIN_RESET,
    .pin_xclk     = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7    = CAM_PIN_D7,
    .pin_d6    = CAM_PIN_D6,
    .pin_d5    = CAM_PIN_D5,
    .pin_d4    = CAM_PIN_D4,
    .pin_d3    = CAM_PIN_D3,
    .pin_d2    = CAM_PIN_D2,
    .pin_d1    = CAM_PIN_D1,
    .pin_d0    = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href  = CAM_PIN_HREF,
    .pin_pclk  = CAM_PIN_PCLK,

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG - This was the deafultPIXFORMAT_RGB565
    .frame_size =
        FRAMESIZE_WQXGA, // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of
                         // the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 8, // 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count     = 1, // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
};

int cameraInitalised = 0;

uint8_t camera_init(void) {

    // Initialize the camera
    if (esp_camera_init(&camera_config) != ESP_OK) {
        cameraInitalised = FALSE;
        return FALSE;
    }

    cameraInitalised = TRUE;
    return TRUE;
}

uint8_t camera_get_resolution(void) {
    return camera_config.frame_size;
}

uint8_t change_camera_resolution(uint8_t cam_res) {

    // Changes the resolution of the camera
    esp_camera_deinit();
    switch (cam_res) {
        case FRAMESIZE_QVGA:
            camera_config.frame_size = FRAMESIZE_QVGA;
            break;
        case FRAMESIZE_CIF:
            camera_config.frame_size = FRAMESIZE_CIF;
            break;
        case FRAMESIZE_VGA:
            camera_config.frame_size = FRAMESIZE_VGA;
            break;
        case FRAMESIZE_SVGA:
            camera_config.frame_size = FRAMESIZE_SVGA;
            break;
        case FRAMESIZE_XGA:
            camera_config.frame_size = FRAMESIZE_XGA;
            break;
        case FRAMESIZE_SXGA:
            camera_config.frame_size = FRAMESIZE_SXGA;
            break;
        case FRAMESIZE_UXGA:
            camera_config.frame_size = FRAMESIZE_UXGA;
            break;
        default:
            // Should never get here, should only ever recieve one of the above cases
            return FALSE;
    }

    // Configure Camera
    sd_card_log(SYSTEM_LOG_FILE, "Reconfiguring Camera\0");
    if (camera_init() != TRUE) {
        bpacket_create_sp(bpacket, BPACKET_R_FAILED, "Camera failed to reinitalise\0");
        return FALSE;
    }

    // Successfully changed the camera resolution
    return TRUE;
}

void camera_capture_and_save_image(bpacket_t* bpacket) {

    // Confirm camera has been initialised
    if (cameraInitalised != TRUE) {
        bpacket_create_sp(bpacket, BPACKET_R_FAILED, "Camera was unitailised\0");
        esp32_uart_send_bpacket(bpacket);
        return;
    }

    // Confirm the SD card can be mounted
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, BPACKET_R_FAILED, "SD card could not open\0");
        esp32_uart_send_bpacket(bpacket);
        return;
    }

    // Take a photo
    sd_card_log(SYSTEM_LOG_FILE, "Taking image");
    camera_fb_t* pic = esp_camera_fb_get();

    // Return error if picture could not be taken
    if (pic == NULL) {
        bpacket_create_sp(bpacket, BPACKET_R_FAILED, "Failed to take a photo\0");
        esp32_uart_send_bpacket(bpacket);
        sd_card_log(SYSTEM_LOG_FILE, "Camera failed to take image");
    } else if (sd_card_save_image(pic->buf, pic->len, bpacket) != TRUE) {
        sd_card_log(SYSTEM_LOG_FILE, "Image could not be saved");
    } else {
        char msg[100];
        sprintf(msg, "Image was %zu bytes", pic->len);
        sd_card_log(SYSTEM_LOG_FILE, msg);
        bpacket_create_sp(bpacket, BPACKET_R_SUCCESS, msg);
        esp32_uart_send_bpacket(bpacket);
    }

    esp_camera_fb_return(pic);

    sd_card_close();
}