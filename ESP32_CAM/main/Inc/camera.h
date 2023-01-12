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
#include "uart_comms.h"

uint8_t camera_init(void);

void camera_capture_and_save_image(packet_t* responsePacket);

#endif // CAMERA_H