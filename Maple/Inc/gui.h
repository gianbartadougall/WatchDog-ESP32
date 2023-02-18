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
#include <windows.h>
#include <wingdi.h>
#include <CommCtrl.h>
#include <stdint.h>

/* C Private Includes */
#include "bpacket.h"
#include "watchdog_defines.h"

/* Public Marcos */
#define SYSTEM_STATUS_ERROR 0
#define SYSTEM_STATUS_OK    1

#define MAX_CHARACTER_INPUT_TO_TEXT_BOX 20
#define GUI_TURN_RED_LED_ON             (0x01 << 0)
#define GUI_TURN_RED_LED_OFF            (0x01 << 1)
#define GUI_CLOSE                       (0x01 << 2)
#define GUI_CAMERA_VIEW_STATE           (0x01 << 3)
#define GUI_UPDATE_CAMERA_VIEW          (0x01 << 4)
#define CAMERA_VIEW_FILENAME            ("streamImage.jpg\0")

#define GUI_BPK_R_UPDATE_STREAM_IMAGE (WATCHDOG_BPK_OFFSET + 0)

typedef struct watchdog_info_t {
    uint8_t cameraResolution;
    uint16_t id;
    uint16_t numImages;
    uint8_t status;
    char datetime[50];
} watchdog_info_t;

typedef struct gui_initalisation_t {
    watchdog_info_t* watchdog;
    uint32_t* flags;
    bpacket_circular_buffer_t* guiToMain;
    bpacket_circular_buffer_t* mainToGui;
} gui_initalisation_t;

void gui_init();

void gui_update();

DWORD WINAPI gui(void* arg);