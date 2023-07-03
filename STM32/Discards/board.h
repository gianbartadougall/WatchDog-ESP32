#ifndef BOARD_H
#define BOARD_H

#include "stm32l432xx.h"
#include "stm32l4xx_hal.h"

/* Function Prototypes */

/**
 * @brief Initialises the onboard LED
 *
 */
void board_init(void);

/**
 * @brief Turns the onboard LED on
 *
 */
void brd_led_on(void);

/**
 * @brief Turns the onboard LED off
 *
 */
void brd_led_off(void);

/**
 * @brief Toggles the state of the onboard LED
 *
 */
void brd_led_toggle(void);

#endif // BOARD_H
