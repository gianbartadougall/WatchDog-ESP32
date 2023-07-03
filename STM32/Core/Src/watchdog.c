/**
 * @file watchdog.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-03
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */

/* STM32 Library Includes */
#include "usb_device.h"
#include "usbd_cdc_if.h"

/* Personal Includes */
#include "event_group.h"
#include "datetime.h"
#include "utils.h"
#include "hardware_config.h"
#include "gpio.h"
#include "task_scheduler.h"
#include "task_scheduler_config.h"
#include "stm32_uart.h"
#include "log_usb.h"

/* Private Enums */
enum error_code_e {
    EC_READ_CAPTURE_TIME,
    EC_READ_CAMERA_SETTINGS,
};

typedef struct camera_settings_t {
    uint8_t resolution;
} camera_settings_t;

typedef struct capture_time_t {
    dt_time_t Start;
    dt_time_t End;
    dt_time_t Interval;
} capture_time_t;

/* Private Macros */

/* Public Variables */
EventGroup_t gbl_EventsStm, gbl_EventsEsp;

/* Private Variables */
capture_time_t lg_CaptureTime;
camera_settings_t lg_CameraSettings;

/* Function Prototypes */
void wd_error_handler(enum error_code_e errorCode);
uint8_t wd_flash_read_camera_settings(camera_settings_t* CameraSettings);
uint8_t wd_flash_read_capture_time(dt_time_t* Start, dt_time_t* End, dt_time_t* Interval);

void test(void) {

    // Initialise all the hardware
    hardware_config_init();

    log_usb_warning("--- STARTING NEW SESSIONS ---\r\n");
    HAL_Delay(1000);
    GPIO_SET_HIGH(LED_GREEN_PORT, LED_GREEN_PIN);
    HAL_Delay(1000);
    GPIO_SET_LOW(LED_GREEN_PORT, LED_GREEN_PIN);
    HAL_Delay(3000);

    ts_init();

    ts_capture_state();

    if (ts_add_recipe(&BlinkGreenLED) != TRUE) {
        log_usb_error("Failed to add recipe\r\n");
    } else {
        log_usb_success("Added recipe\r\n");
    }

    if (ts_add_recipe(&BlinkRedLED) != TRUE) {
        log_usb_error("Failed to add recipe\r\n");
    } else {
        log_usb_success("Added recipe\r\n");
    }

    ts_capture_state();
    uint32_t lastCount = 0;
    while (1) {

        if (event_group_poll_bit(&gbl_EventsStm, EGB_GREEN_LED_ON, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EGB_GREEN_LED_ON, EGT_ACTIVE);
            GPIO_SET_HIGH(LED_GREEN_PORT, LED_GREEN_PIN);
            log_usb_message("--- Green LED set high --- \r\n");
            ts_capture_state();
        }

        if (event_group_poll_bit(&gbl_EventsStm, EGB_GREEN_LED_OFF, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EGB_GREEN_LED_OFF, EGT_ACTIVE);
            GPIO_SET_LOW(LED_GREEN_PORT, LED_GREEN_PIN);
            log_usb_message("--- Green LED set low --- \r\n");
            ts_capture_state();
        }

        if (event_group_poll_bit(&gbl_EventsStm, EGB_RED_LED_ON, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EGB_RED_LED_ON, EGT_ACTIVE);
            GPIO_SET_HIGH(LED_RED_PORT, LED_RED_PIN);
            log_usb_message("--- Red LED set high --- \r\n");
            ts_capture_state();
        }

        if (event_group_poll_bit(&gbl_EventsStm, EGB_RED_LED_OFF, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EGB_RED_LED_OFF, EGT_ACTIVE);
            GPIO_SET_LOW(LED_RED_PORT, LED_RED_PIN);
            log_usb_message("--- Red LED set low --- \r\n");
            ts_capture_state();
        }
    }
}

void wd_start(void) {

    test();

    // Initialise all the hardware
    hardware_config_init();

    // Initialise event groups
    event_group_clear(&gbl_EventsStm);
    event_group_clear(&gbl_EventsEsp);

    /* Initialise the software for watchdog (Capture time and camera settings) */
    if (wd_flash_read_capture_time(&lg_CaptureTime.Start, &lg_CaptureTime.End, &lg_CaptureTime.Interval) != TRUE) {
        wd_error_handler(EC_READ_CAPTURE_TIME);
    }

    // TODO: Confirm the start, end and interval times just read are valid

    // TODO: Set the RTC alarm for this capture time

    if (wd_flash_read_camera_settings(&lg_CameraSettings) != TRUE) {
        wd_error_handler(EC_READ_CAMERA_SETTINGS);
    }

    // TODO: Confirm the camera settings are valid

    // Request ESP to take a photo to ensure system is working correctly
    event_group_set_bit(&gbl_EventsEsp, EGB_TAKE_PHOTO, EGT_ACTIVE);

    // TODO: Blink green LED in x seconds to inidcate everything is good

    // Enter main loop
    while (1) {}
}

void wd_error_handler(enum error_code_e errorCode) {

    // TODO: Turn the ESP off

    while (1) {
        HAL_Delay(1000);

        // TODO: Toggle the LED

        // TODO: Send message across USB detailing the error
    }
}

uint8_t wd_flash_read_capture_time(dt_time_t* Start, dt_time_t* End, dt_time_t* Interval) {
    // TODO: Implement

    return TRUE;
}

uint8_t wd_flash_read_camera_settings(camera_settings_t* CameraSettings) {
    // TODO: Implement

    return TRUE;
}
