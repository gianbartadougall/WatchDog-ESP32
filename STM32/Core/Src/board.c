/**
 * @file board.c
 * @author Utility functions for STM32 boards
 * @brief
 * @version 0.1
 * @date 2022-06-20
 *
 * @copyright Copyright (c) 2022
 *
 */
/* Public Includes */

/* Private Includes */
#include "board.h"

/* STM32 Includes */

/* Private #defines */
#define LD3_PIN_RAW      3
#define LD3_PIN_POS      (LD3_PIN_RAW * 2)
#define LD3_PIN          GPIO_PIN_3
#define LD3_PORT         GPIOB
#define LD3_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

/* Variable Declarations */

/* Function prototypes */
void brd_onboard_led_init(void);

/* Function prototpes */

void board_init(void) {
    brd_onboard_led_init();
}

/**
 * @brief Initialises the onboard LED (LD3)
 *
 */
void brd_onboard_led_init(void) {
    GPIO_InitTypeDef led;

    // Configure the LED
    led.Pin   = LD3_PIN;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_PULLUP;
    led.Speed = GPIO_SPEED_FREQ_HIGH;

    // Enable clock for LED
    LD3_CLK_ENABLE();

    // Initialise the LED
    HAL_GPIO_Init(LD3_PORT, &led);
}

void brd_led_on(void) {
    HAL_GPIO_WritePin(LD3_PORT, LD3_PIN, GPIO_PIN_SET);
}

void brd_led_off(void) {
    HAL_GPIO_WritePin(LD3_PORT, LD3_PIN, GPIO_PIN_RESET);
}

void brd_led_toggle(void) {
    HAL_GPIO_TogglePin(LD3_PORT, LD3_PIN);
}