
/**
 * @file main.c
 * @author Gian Barta-Dougall
 * @brief Program File
 * @version 0.1
 * @date 2022-04-15
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef MAIN_H
#define MAIN_H

#include "stm32l4xx_hal.h"

/* Private #defines */
#define LD3_PIN GPIO_PIN_3
#define LD3_PORT GPIOB
#define LD3_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();

#endif // MAIN_H 
