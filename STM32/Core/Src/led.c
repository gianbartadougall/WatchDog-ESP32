/**
 * @file led.c
 * @author Gian Barta-Dougall
 * @brief System file for led
 * @version 0.1
 * @date --
 *
 * @copyright Copyright (c)
 *
 */
/* Public Includes */

/* Private Includes */
#include "led_config.h"

/* Private STM Includes */

/* Private #defines */
#define SET_LED_ON(ledIndex)  (leds[ledIndex].port->BSRR |= (0x01 << leds[ledIndex].pin))
#define SET_LED_OFF(ledIndex) (leds[ledIndex].port->BSRR |= (0x10000 << leds[ledIndex].pin))
#define ID_INVALID(id)        ((id < LED_ID_OFFSET) || (id > (NUM_LEDS - 1 + LED_ID_OFFSET)))

/* Private Structures and Enumerations */

/* Private Variable Declarations */

/* Private Function Prototypes */

/* Public Functions */

/* Private Functions */

void led_on(uint8_t ledId) {

    if (ID_INVALID(ledId)) {
        return;
    }

    uint8_t ledIndex = ledId - LED_ID_OFFSET;
    SET_LED_ON(ledIndex);
}

void led_off(uint8_t ledId) {

    if (ID_INVALID(ledId)) {
        return;
    }

    uint8_t ledIndex = ledId - LED_ID_OFFSET;
    SET_LED_OFF(ledIndex);
}

void led_toggle(uint8_t ledId) {

    if (ID_INVALID(ledId)) {
        return;
    }

    uint8_t ledIndex = ledId - LED_ID_OFFSET;
    if ((leds[ledIndex].port->ODR & (0x01 << leds[ledIndex].pin)) == 0) {
        SET_LED_ON(ledIndex);
    } else {
        SET_LED_OFF(ledIndex);
    }
}