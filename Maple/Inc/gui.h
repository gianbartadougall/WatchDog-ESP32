/**
 * @file gui.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-01-27
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <stdint.h>

/* Public Marcos */
#define SYSTEM_STATUS_ERROR 0
#define SYSTEM_STATUS_OK    1

#define GUI_TURN_RED_LED_ON  (0x01 << 0)
#define GUI_TURN_RED_LED_OFF (0x01 << 1)
#define GUI_CLOSE            (0x01 << 2)

typedef struct watchdog_info_t {
    uint8_t cameraResolution;
    uint16_t id;
    uint16_t numImages;
    uint8_t status;
    char datetime[50];
} watchdog_info_t;

void gui_init();

void gui_update();