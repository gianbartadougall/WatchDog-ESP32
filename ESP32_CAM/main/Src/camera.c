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
#include "utils.h"
#include "datetime.h"
#include "watchdog_utils.h"

/* Private Macros */
#define BOARD_ESP32CAM_AITHINKER

#define ESP_CR_QVGA_320x240   5  // FRAMESIZE_QVGA
#define ESP_CR_CIF_352x288    6  // FRAMESIZE_CIF
#define ESP_CR_VGA_640x480    8  // FRAMESIZE_VGA
#define ESP_CR_SVGA_800x600   9  // FRAMESIZE_SVGA
#define ESP_CR_XGA_1024x768   10 // FRAMESIZE_XGA
#define ESP_CR_SXGA_1280x1024 12 // FRAMESIZE_SXGA
#define ESP_CR_UXGA_1600x1200 13 // FRAMESIZE_UXGA

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

    .pixel_format = PIXFORMAT_JPEG,      // YUV422,GRAYSCALE,RGB565,JPEG - This was the deafultPIXFORMAT_RGB565
    .frame_size   = ESP_CR_QVGA_320x240, // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The
                                         // performance of the ESP32-S series has improved a lot, but JPEG mode always
                                         // gives better frame rates. Last was svga

    .jpeg_quality = 8, // 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count     = 1, // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
};

int cameraInitalised = 0;

/* Function Prototypes */
uint8_t camera_capture_image(camera_fb_t** image);

uint8_t camera_init(void) {

    if ((cameraInitalised != TRUE) && (esp_camera_init(&camera_config) != ESP_OK)) {
        return FALSE;
    }

    cameraInitalised = TRUE;
    return TRUE;
}

uint8_t camera_deinit(void) {

    if ((cameraInitalised != FALSE) && (esp_camera_deinit() != ESP_OK)) {
        return FALSE;
    }

    cameraInitalised = FALSE;
    return TRUE;
}

uint8_t camera_get_resolution(void) {
    return camera_config.frame_size;
}

uint8_t camera_set_settings(camera_settings_t* CameraSettings, char* msg) {

    /* Update the cameras frame size */
    switch (CameraSettings->frameSize) {
        case WD_CR_QVGA_320x240:
            camera_config.frame_size = ESP_CR_QVGA_320x240;
            break;
        case WD_CR_CIF_352x288:
            camera_config.frame_size = ESP_CR_CIF_352x288;
            break;
        case WD_CR_VGA_640x480:
            camera_config.frame_size = ESP_CR_VGA_640x480;
            break;
        case WD_CR_SVGA_800x600:
            camera_config.frame_size = ESP_CR_SVGA_800x600;
            break;
        case WD_CR_XGA_1024x768:
            camera_config.frame_size = ESP_CR_XGA_1024x768;
            break;
        case WD_CR_SXGA_1280x1024:
            camera_config.frame_size = ESP_CR_SXGA_1280x1024;
            break;
        case WD_CR_UXGA_1600x1200:
            camera_config.frame_size = ESP_CR_UXGA_1600x1200;
            break;
        default:
            sprintf(msg, "Invalid framesize");
            return FALSE;
    }

    /* Update the JPEG quality */
    if (CameraSettings->jpegCompression > 63) {
        sprintf(msg, "Invalid jpeg compression");
        return FALSE;
    }

    camera_config.jpeg_quality = CameraSettings->jpegCompression;

    return TRUE;
}

void camera_stream_image(bpk_t* Bpacket) {

    camera_fb_t* image = NULL;

    if (camera_capture_image(&image) != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Camera could not taken photo\r\n\0");
        esp32_uart_send_bpacket(Bpacket);
        return;
    }

    // Image was able to be taken. Send image back to sender
    bpk_addr_receive_t Receiver = {.val = Bpacket->Sender.val};
    bpk_addr_send_t Sender      = {.val = Bpacket->Receiver.val};
    if (bpk_send_data(esp32_uart_send_data, Receiver, Sender, Bpacket->Request, image->buf, image->len) != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Failed to send image\r\n\0");
        esp32_uart_send_bpacket(Bpacket);
    }

    // Free the image
    esp_camera_fb_return(image);
}

uint8_t camera_capture_and_save_image1(dt_datetime_t* Datetime, float temperature, char* msg) {

    bpk_t Bpacket;
    // Confirm camera has been initialised
    if (camera_init() != TRUE) {
        sprintf(msg, "Error %s on line %i. Camera could not init", __FILE__, __LINE__);
        return FALSE;
    }

    camera_fb_t* pic = esp_camera_fb_get();

    // Return error if picture could not be taken
    uint8_t success = FALSE;
    if (pic == NULL) {
        sprintf(msg, "Error %s on line %i. Failed to take a photo", __FILE__, __LINE__);
    } else if (sd_card_save_image(pic->buf, pic->len, Datetime, temperature, msg) != TRUE) {
        sprintf(msg, "Error %s on line %i. Image could not be saved", __FILE__, __LINE__);
    } else {
        sprintf(msg, "Image was %zu bytes", pic->len);
        success = TRUE;
    }

    esp_camera_fb_return(pic);

    camera_deinit();

    return success;
}

void camera_capture_and_save_image(bpk_t* Bpacket) {

    // Confirm camera has been initialised
    if (cameraInitalised != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Camera was unitailised\0");
        esp32_uart_send_bpacket(Bpacket);
        return;
    }

    // Confirm the SD card can be mounted
    if (sd_card_open() != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "SD card could not open\0");
        esp32_uart_send_bpacket(Bpacket);
        return;
    }

    // Take a photo
    sd_card_log(LOG_FILE_NAME, "Taking image");
    camera_fb_t* pic = esp_camera_fb_get();

    // Return error if picture could not be taken
    if (pic == NULL) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Failed to take a photo\0");
        esp32_uart_send_bpacket(Bpacket);
        sd_card_log(LOG_FILE_NAME, "Camera failed to take image");
    } else if (sd_card_save_image1(pic->buf, pic->len, Bpacket) != TRUE) {
        sd_card_log(LOG_FILE_NAME, "Image could not be saved");
    } else {
        char msg[100];
        sprintf(msg, "Image was %zu bytes", pic->len);
        sd_card_log(LOG_FILE_NAME, msg);
        bpk_create_string_response(Bpacket, BPK_Code_Success, msg);
        esp32_uart_send_bpacket(Bpacket);
    }

    esp_camera_fb_return(pic);

    sd_card_close();
}

uint8_t camera_capture_image(camera_fb_t** image) {
    bpk_t Bpacket;

    // Initialise the camera
    if (camera_init() != TRUE) {
        bp_create_string_packet(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message,
                                BPK_Code_Error, "Camera Init failed\r\n\0");
        esp32_uart_send_bpacket(&Bpacket);
        return FALSE;
    }

    *image = esp_camera_fb_get();

    if (*image == NULL) {
        esp_camera_fb_return(*image);
        bp_create_string_packet(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message,
                                BPK_Code_Error, "Camera failed to take photo\r\n\0");
        esp32_uart_send_bpacket(&Bpacket);
        return FALSE;
    }

    // Deinitialise the camera
    if (camera_deinit() != TRUE) {
        bp_create_string_packet(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message,
                                BPK_Code_Error, "Camera deInit failed\r\n\0");
        esp32_uart_send_bpacket(&Bpacket);
        return FALSE;
    }

    return TRUE;
}