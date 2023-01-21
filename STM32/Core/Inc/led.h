/**
 * @file led.h
 * @author Gian Barta-Dougall
 * @brief System file for led
 * @version 0.1
 * @date --
 *
 * @copyright Copyright (c)
 *
 */
#ifndef LED_H
#define LED_H

/* Public Includes */

/* Public STM Includes */
#include "stm32l4xx.h"

/* Public #defines */
#define LED_ID_OFFSET 60
#define LED_GREEN_ID  (0 + LED_ID_OFFSET)

/* Public Structures and Enumerations */

/* Public Variable Declarations */

/* Public Function Prototypes */

/**
 * @brief Turns the LED corresponding to the given ID on. If the ID
 * is invalid, nothing will happen.
 *
 * @param ledId The id of the LED to turn on
 */
void led_on(uint8_t ledId);

/**
 * @brief Turns the LED corresponding to the given ID off. If the ID
 * is invalid, nothing will happen.
 *
 * @param ledId The id of the LED to turn off
 */
void led_off(uint8_t ledId);

/**
 * @brief Toggles the state of the LED corresponding to the given ID.
 * If the ID is invalid, nothing will happen.
 *
 * @param ledId The id of the LED to toggle
 */
void led_toggle(uint8_t ledId);

#endif // LED_H
