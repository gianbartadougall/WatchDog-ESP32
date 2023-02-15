/**
 * @file camera.h
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef CAMERA_H
#define CAMERA_H

/* Public Includes */
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"

/* Private Includes */
#include "bpacket.h"

uint8_t camera_init(void);

void camera_capture_and_save_image(bpacket_t* bpacket);

uint8_t camera_get_resolution(void);

// This function changes the camera resolution, possible inputs are
// the FRAMESIZE_SETTINGS, the settings are QVGA, CIF, VGA, SVGA, XGA, SXGA, UXGA, WQXGA
uint8_t camera_set_resolution(uint8_t cam_res);

void camera_stream_image(bpacket_t* bpacket);

#endif // CAMERA_H