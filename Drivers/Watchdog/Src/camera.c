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
#include "camera.h"
#include "wd_utils.h"
#include "sd_card.h"

/* Private Macros */
#define BOARD_ESP32CAM_AITHINKER

static const char* CAMERA_TAG = "CAMERA:";

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
        FRAMESIZE_QVGA, // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the
                        // ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, // 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count     = 1,  // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
};

int cameraInitalised = 0;

esp_err_t camera_init(void) {

    // initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        cameraInitalised = FALSE;
        ESP_LOGE(CAMERA_TAG, "Camera Init Failed");
        return WD_ERROR;
    }

    cameraInitalised = TRUE;
    return ESP_OK;
}

void camera_capture_and_save_image(void) {

    // Confirm camera has been initialised
    if (cameraInitalised == FALSE) {
        return;
    }

    // Confirm the SD card can be mounted
    if (sd_card_open() != WD_SUCCESS) {
        return;
    }

    // Take a photo
    sd_card_log(SYSTEM_LOG_FILE, "Taking image");
    camera_fb_t* pic = esp_camera_fb_get();

    // Return error if picture could not be taken
    if (pic == NULL) {
        sd_card_log(SYSTEM_LOG_FILE, "Camera failed to take image");
        return;
    }

    char msg[100];
    sprintf(msg, "Image captured. Size was %zu bytes", pic->len);
    sd_card_log(SYSTEM_LOG_FILE, msg);

    // Save the image
    sd_card_save_image(pic->buf, pic->len);

    sd_card_close();
}