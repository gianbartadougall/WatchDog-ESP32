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

/* Public includes */

/* Private includes */
#include "main.h"
#include "board.h"
#include "hardware_config.h"
#include "utilities.h"
#include "watchdog.h"
#include "comms.h"

/* STM32 Includes */
#include "stm32l4xx_hal.h"

/* Variable Declarations */

/* Private #defines */

/* Variable Declarations */
// UART handle
UART_HandleTypeDef huart2;

/* Function prototypes */
void error_handler(void);
void SystemClock_Config(void);

/****** START CODE BLOCK ******/
// Description:

// #include "stm32l4xx.h"
// #include "stm32l4xx_hal.h"

// // UART2 pin configuration:
// //   - PA2: UART2_TX
// //   - PA3: UART2_RX

// #define UART2_TX_PIN       GPIO_PIN_2
// #define UART2_RX_PIN       GPIO_PIN_3
// #define UART2_TX_GPIO_PORT GPIOA
// #define UART2_RX_GPIO_PORT GPIOA

// // UART2 handle and transmission buffer
// UART_HandleTypeDef huart2;
// uint8_t tx_buffer[] = "Hello, World!\r\n";

// // void SystemClock_Config(void);

// void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
//     if (huart->Instance == USART2) {
//         // Receive buffer and variable to store received data
//         uint8_t rx_buffer[1];
//         uint8_t rx_data;

//         // Receive 1 byte of data
//         HAL_UART_Receive_IT(&huart2, rx_buffer, 1);

//         // Store received data in rx_data variable
//         rx_data = rx_buffer[0];

//         // Do something with the received data (e.g. print it to the terminal)
//         printf("Received: %c\n", rx_data);
//     }
// }

// void UART2_Init(void) {
//     // Configure UART2_TX pin as alternate function
//     GPIO_InitTypeDef gpio_init;
//     gpio_init.Pin       = UART2_TX_PIN;
//     gpio_init.Mode      = GPIO_MODE_AF_PP;
//     gpio_init.Pull      = GPIO_NOPULL;
//     gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
//     gpio_init.Alternate = GPIO_AF7_USART2;
//     HAL_GPIO_Init(UART2_TX_GPIO_PORT, &gpio_init);

//     // Configure UART2_RX pin as input with pull-up
//     gpio_init.Pin       = UART2_RX_PIN;
//     gpio_init.Mode      = GPIO_MODE_AF_PP;
//     gpio_init.Pull      = GPIO_PULLUP;
//     gpio_init.Alternate = GPIO_AF7_USART2;
//     HAL_GPIO_Init(UART2_RX_GPIO_PORT, &gpio_init);

//     // Enable clock for UART2
//     __HAL_RCC_USART2_CLK_ENABLE();

//     // Configure UART2
//     huart2.Instance                    = USART2;
//     huart2.Init.BaudRate               = 115200;
//     huart2.Init.WordLength             = UART_WORDLENGTH_8B;
//     huart2.Init.StopBits               = UART_STOPBITS_1;
//     huart2.Init.Parity                 = UART_PARITY_NONE;
//     huart2.Init.Mode                   = UART_MODE_TX_RX;
//     huart2.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
//     huart2.Init.OverSampling           = UART_OVERSAMPLING_16;
//     huart2.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
//     huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
//     if (HAL_UART_Init(&huart2) != HAL_OK) {
//         // error handling
//     }
// }

// void SystemClock_Config(void) {
//     RCC_OscInitTypeDef RCC_OscInitStruct   = {0};
//     RCC_ClkInitTypeDef RCC_ClkInitStruct   = {0};
//     RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

//     // Initializes the CPU, AHB and APB busses clocks
//     RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
//     RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
//     RCC_OscInitStruct.MSICalibrationValue = 0;
//     RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_6;
//     RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
//     RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_MSI;
//     RCC_OscInitStruct.PLL.PLLM            = 1;
//     RCC_OscInitStruct.PLL.PLLN            = 40;
//     RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV7;
//     RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
//     RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
//     if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
//         // error handling
//     }

//     // Initializes the CPU, AHB and APB busses clocks
//     RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
//     RCC_CLOCKTYPE_PCLK2; RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK; RCC_ClkInitStruct.AHBCLKDivider
//     = RCC_SYSCLK_DIV1; RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; RCC_ClkInitStruct.APB2CLKDivider =
//     RCC_HCLK_DIV1; if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
//         // error handling
//     }

//     PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
//     PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
//     if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
//         // error handling
//     }
// }

// int main(void) {
//     HAL_Init();
//     SystemClock_Config();
//     UART2_Init();

//     uint8_t rx_buffer[100];
//     // uint8_t tx_buffer[] = "Requesting a message:\r\n";
//     // uint8_t m[]         = "Got a message:\r\n";
//     HAL_UART_Receive_IT(&huart2, rx_buffer, 1);

//     // while (1) {
//     //     // Transmit prompt
//     //     HAL_UART_Transmit(&huart2, tx_buffer, sizeof(tx_buffer), HAL_MAX_DELAY);

//     //     // Receive message
//     //     if (HAL_UART_Receive(&huart2, rx_buffer, sizeof(rx_buffer), 1000) == HAL_OK) {
//     //         // Echo message back
//     //         HAL_UART_Transmit(&huart2, m, sizeof(m), HAL_MAX_DELAY);
//     //     }
//     // }
//     while (1) {
//         // Wait indefinitely for data to be received
//         HAL_UART_RxCpltCallback(&huart2);
//     }
// }

/****** END CODE BLOCK ******/

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

    while (1) {
        watchdog_update();
        // debug_prints("Gian was here\r\n");
        // HAL_Delay(1000);
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