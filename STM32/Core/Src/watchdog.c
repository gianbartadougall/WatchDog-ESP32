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
#include "watchdog.h"
#include "event_group.h"
#include "datetime.h"
#include "utils.h"
#include "hardware_config.h"
#include "gpio.h"
#include "task_scheduler.h"
#include "task_scheduler_config.h"
#include "stm32_uart.h"
#include "log_usb.h"
#include "bpacket.h"
#include "comms_stm32.h"
#include "rtc_mcp7940n.h"

/* Private Enums */
enum error_code_e {
    EC_READ_CAPTURE_TIME,
    EC_READ_CAMERA_SETTINGS,
    EC_REQUEST_PHOTO_CAPTURE,
    EC_BPACKET_CREATE,
    EC_UNKNOWN_BPACKET_REQUEST,
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
uint8_t wd_flash_read_camera_settings(camera_settings_t* CameraSettings);
uint8_t wd_flash_read_capture_time(dt_time_t* Start, dt_time_t* End, dt_time_t* Interval);
uint8_t usb_read_packet(bpk_t* Bpacket);
uint8_t wd_request_photo_capture(void);

void wd_error_handler(enum error_code_e errorCode);
void wd_handle_esp_response(bpk_t* Bpacket);
void wd_handle_maple_request(bpk_t* Bpacket);
void wd_write_bpacket_maple(bpk_t* Bpacket);

void test(void) {

    // Initialise all the hardware
    hardware_config_init();

    task_scheduler_init();

    task_scheduler_capture_state();

    if (task_scheduler_add_recipe(&BlinkGreenLED) != TRUE) {
        log_usb_error("Failed to add recipe\r\n");
    } else {
        log_usb_success("Added recipe\r\n");
    }

    if (task_scheduler_add_recipe(&BlinkRedLED) != TRUE) {
        log_usb_error("Failed to add recipe\r\n");
    } else {
        log_usb_success("Added recipe\r\n");
    }

    while (1) {

        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_LED_GREEN_ON, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_LED_GREEN_ON, EGT_ACTIVE);
            GPIO_SET_HIGH(LED_GREEN_PORT, LED_GREEN_PIN);
            // log_usb_message("--- Green LED set high --- \r\n");
            // task_scheduler_capture_state();
        }

        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_LED_GREEN_OFF, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_LED_GREEN_OFF, EGT_ACTIVE);
            GPIO_SET_LOW(LED_GREEN_PORT, LED_GREEN_PIN);
            // log_usb_message("--- Green LED set low --- \r\n");
            // task_scheduler_capture_state();
        }

        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_LED_RED_ON, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_LED_RED_ON, EGT_ACTIVE);
            GPIO_SET_HIGH(LED_RED_PORT, LED_RED_PIN);
            // log_usb_message("--- Red LED set high --- \r\n");
            // task_scheduler_capture_state();
        }

        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_LED_RED_OFF, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_LED_RED_OFF, EGT_ACTIVE);
            GPIO_SET_LOW(LED_RED_PORT, LED_RED_PIN);
            // log_usb_message("--- Red LED set low --- \r\n");
            // task_scheduler_capture_state();
        }
    }
}

void i2c_test(void) {

    if (rtc_init() != TRUE) {
        GPIO_SET_HIGH(LED_RED_PORT, LED_RED_PIN);
        while (1) {}
    }

    dt_time_t Time;
    uint8_t lastSecond = 0;
    while (1) {

        if (rtc_read_time(&Time) != TRUE) {
            GPIO_SET_HIGH(LED_RED_PORT, LED_RED_PIN);
        }

        if (Time.second != lastSecond) {
            lastSecond = Time.second;
            log_usb_success("%i:%i:%i\r\n", Time.hour, Time.minute, Time.second);
        }

        HAL_Delay(10);
    }
}

void wd_start(void) {

    // Initialise all the hardware
    hardware_config_init();

    i2c_test();

    /* Initialise software */
    task_scheduler_init(); // Reset Recipes list

    // Initialise event groups
    event_group_clear(&gbl_EventsStm);
    event_group_clear(&gbl_EventsEsp);

    /* Initialise the software for watchdog (Capture time and camera settings) */
    // if (wd_flash_read_capture_time(&lg_CaptureTime.Start, &lg_CaptureTime.End, &lg_CaptureTime.Interval) != TRUE) {
    //     wd_error_handler(EC_READ_CAPTURE_TIME);
    // }

    // TODO: Confirm the start, end and interval times just read are valid

    // TODO: Set the RTC alarm for this capture time

    // if (wd_flash_read_camera_settings(&lg_CameraSettings) != TRUE) {
    //     wd_error_handler(EC_READ_CAMERA_SETTINGS);
    // }

    // TODO: Confirm the camera settings are valid

    // Set flag 'take photo' flag. In main loop below, STM32 will request ESP to
    // take a photo if this flag has been set. Getting ESP32 to take a photo on
    // startup to ensure the system is working correctly
    event_group_set_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);

    // Set flag 'system initialising' flag. In main loop where stm32 waits for
    // responses back from the ESP, if it gets a response saying a photo was taken
    // and the system init flag has been set then it knows that the photo was a test
    // photo as this flag is only set once on startup. The STM32 can then blink an
    // led to let the user know that the system has been initialised
    event_group_set_bit(&gbl_EventsStm, EVENT_STM_SYSTEM_INITIALISING, EGT_ACTIVE);

    // Enter main loop
    bpk_t BpkEspResponse, MapleBpacket;

    while (1) {

        /* Turn the ESP on or off depending on if there are esp tasks that need to occur */

        // Check if there are any esp flags that are set that are not ESP_ON or ESP_READY
        if ((event_group_get_bits(&gbl_EventsEsp, EGT_ACTIVE) & ~(EVENT_ESP_ON | EVENT_ESP_READY)) != 0) {

            // There is an esp event that needs to occur. Ensure the ESP has been turned on
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE) != TRUE) {
                // TODO: Turn the esp on
            }
        } else {

            // There are no pending esp events. Turn the ESP off if it is not already off
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_ON, EGT_ACTIVE) == TRUE) {
                // TODO: Turn the esp off
            }
        }

        /* Put the STM32 to sleep if required */
        if (event_group_poll_bit(&gbl_EventsStm, EVENT_STM_SLEEP, EGT_ACTIVE) == TRUE) {
            event_group_clear_bit(&gbl_EventsStm, EVENT_STM_SLEEP, EGT_ACTIVE);

            // TODO: Put the STM32 to sleep
        }

        /* Request ESP32 to take a photo if the 'take photo' flag has been set */
        if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE) == TRUE) {

            // Don't request a photo until the ESP is in the ready state. The ESP takes
            // a second or two to startup after it has been turned on. The ESP_READY flag
            // will be set automatically by the STM when the ESP is ready to receive commands
            if (event_group_poll_bit(&gbl_EventsEsp, EVENT_ESP_READY, EGT_ACTIVE) == TRUE) {
                event_group_clear_bit(&gbl_EventsEsp, EVENT_ESP_TAKE_PHOTO, EGT_ACTIVE);

                if (wd_request_photo_capture() != TRUE) {
                    wd_error_handler(EC_REQUEST_PHOTO_CAPTURE);
                }
            }
        }

        /* Process all packets received from the ESP32 */
        if (uart_read_bpacket(0, &BpkEspResponse) == TRUE) {
            wd_handle_esp_response(&BpkEspResponse);
        }

        // TODO: Process the button being pressed

        // Polling the USBC connection pin. The reason it is being polled instead of having an
        // interrupt is because if the system is not powered by batteries and only powered by
        // a USB C connector, the pin will go high before the circuit starts running in which
        // case the interrupt would never fire. Polling ensures that no matter how the system
        // is powered, it will always know if the USBC is connected or not
        if (GPIO_PIN_IS_HIGH(USBC_CONN_PORT, USBC_CONN_PIN)) {
            // GPIO_SET_HIGH(LED_GREEN_PORT, LED_GREEN_PIN);

            // USBC is connected, check for any incoming bpackets and process them if any come
            if (usb_read_packet(&MapleBpacket) == TRUE) {
                wd_handle_maple_request(&MapleBpacket);
            }
        } else {
            // GPIO_SET_LOW(LED_GREEN_PORT, LED_GREEN_PIN);
        }

        // TODO: Check whether the STM32 needs to go to sleep
    }
}

void wd_handle_maple_request(bpk_t* Bpacket) {

    // Create a bpacket to store the response to send back to Maple
    static bpk_t BpkStmResponse;

    switch (Bpacket->Request.val) {

        case BPK_REQUEST_PING:;

            // Create response
            static uint8_t stmPingCode = 47;
            if (bpk_create(&BpkStmResponse, BPK_Addr_Receive_Maple, BPK_Addr_Send_Stm32, BPK_Request_Ping,
                           BPK_Code_Success, 1, &stmPingCode) != TRUE) {
                wd_error_handler(EC_BPACKET_CREATE);
                break;
            }

            // Send bpacket to Maple
            wd_write_bpacket_maple(&BpkStmResponse);

            break;

        default:
            wd_error_handler(EC_UNKNOWN_BPACKET_REQUEST);
    }

    // TODO: implement
}

void wd_write_bpacket_maple(bpk_t* Bpacket) {
    static bpk_buffer_t Buffer;
    bpk_to_buffer(Bpacket, &Buffer);
    log_usb_bytes(Buffer.buffer, Buffer.numBytes);
}

void wd_handle_esp_response(bpk_t* Bpacket) {
    // TODO: Implement
}

void wd_error_handler(enum error_code_e errorCode) {

    // TODO: Turn the ESP off

    while (1) {

        HAL_Delay(1000);
        GPIO_TOGGLE(LED_RED_PORT, LED_RED_PIN);

        log_usb_error("Error code: %i\r\n", errorCode);
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

uint8_t wd_request_photo_capture(void) {
    // TODO: Implement

    return TRUE;
}