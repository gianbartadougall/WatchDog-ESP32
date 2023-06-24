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

/* C Library includes */

/* Personal includes */
#include "main.h"
#include "hardware_config.h"
#include "utils.h"
#include "watchdog.h"
#include "log.h"
#include "led.h"
#include "datetime.h"
#include "stm32_rtc.h"
#include "bpacket.h"
#include "watchdog_defines.h"
#include "stm32_uart.h"
#include "bpacket_utils.h"
#include "stm32_flash.h"

/* STM32 Includes */
#include "stm32l4xx_hal.h"

/* Variable Declarations */

/* Private #defines */

/* Variable Declarations */

/* Function prototypes */
void error_handler(void);
void SystemClock_Config(void);

void rtc_testing(void) {
    dt_datetime_t datetime;
    datetime.Date.year   = 23;
    datetime.Date.month  = 1;
    datetime.Date.day    = 1;
    datetime.Time.hour   = 0;
    datetime.Time.minute = 0;
    datetime.Time.second = 0;

    // date_time_t dt;
    stm32_rtc_write_datetime(&datetime);

    char msg[50];
    uint8_t halTick    = 0;
    uint8_t halSecond  = 0;
    uint8_t halMinute  = 0;
    uint8_t halHour    = 0;
    uint8_t lastSecond = 0;

    while (1) {

        if (SysTick->VAL > ((halTick + 1) * 1000)) {
            halTick++;
            halSecond++;
            if (halSecond > 59) {
                halSecond = 0;
                halMinute++;
            }

            if (halMinute > 59) {
                halMinute = 0;
                halHour++;
            }

            sprintf(msg, "%i:%i:%i\n", halSecond, halMinute, halHour);
            log_message(msg);
        }

        stm32_rtc_read_datetime(&datetime);

        if (lastSecond != datetime.Time.second) {
            lastSecond = datetime.Time.second;
            // stm32_rtc_print_datetime(&datetime);
        }
    }
}

void log_p(char* msg) {

    // Transmit over uart if using a micrcontroller
    uint16_t i = 0;

    // Transmit until end of message reached
    while (msg[i] != '\0') {
        while ((USART2->ISR & USART_ISR_TXE) == 0) {};

        USART2->TDR = msg[i];
        i++;
    }
}

void flash_test(void) {

    log_init(uart_transmit_string, NULL);
    log_clear();
    uint32_t data = 0;

    // Erase the entire flash
    RCC->AHB1ENR |= RCC_AHB1ENR_FLASHEN;
    // FLASH->ACR |= 0x01;

    log_message("FLASH->SR: %x\r\n", FLASH->SR);

    // log_message("Flash 1: %x\r\n", FLASH->SR);
    if (stm32_flash_erase_page(0) != TRUE) {
        log_message("Failed to erase flash\r\n");
    } else {
        log_message("Flash 2: %x\r\n", FLASH->SR);
    }

    if (stm32_flash_read(STM32_FLASH_ADDR_START, &data) != TRUE) {
        log_message("Read failed\r\n");
    } else {
        log_message("Read flash\r\n");
    }
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

    // Reset all peripherals, initialise the flash interface and the systick
    HAL_Init();
    SystemClock_Config();

    // Initialise hardware
    hardware_config_init();
    watchdog_init();

    // SERVO_TIMER->CR1 |= 0x01; // Start the time
    watchdog_enter_state_machine();

    while (1) {
        // watchdog_update();
    }

    return 0;
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {

    RCC_OscInitTypeDef RCC_OscInitStruct   = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct   = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Configure LSE Drive Capability
     */
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.LSEState            = RCC_LSE_ON;
    RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_6;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM            = 1;
    RCC_OscInitStruct.PLL.PLLN            = 16; // Sets SYSCLK to run at 32MHz
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        error_handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        error_handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        error_handler();
    }

    /** Configure the main internal regulator output voltage
     */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        error_handler();
    }

    /** Enable MSI Auto calibration
     */
    HAL_RCCEx_EnableMSIPLLMode();
}

/**
 * @brief Handles initialisation errors
 *
 */
void error_handler(void) {

    // Initialisation error shown by blinking LED (LD3) in pattern
    while (1) {

        for (int i = 0; i < 5; i++) {
            HAL_GPIO_TogglePin(LD3_PORT, LD3_PIN);
            HAL_Delay(100);
        }

        HAL_Delay(500);
    }
}