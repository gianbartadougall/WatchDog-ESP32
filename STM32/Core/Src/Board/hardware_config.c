/**
 * @file hardware_configuration.c
 * @author Gian Barta-Dougall
 * @brief System file for hardware_configuration
 * @version 0.1
 * @date --
 *
 * @copyright Copyright (c)
 *
 */
/* C Library Includes Includes */

/* Personal Includes */
#include "hardware_config.h"
#include "log.h"
#include "gpio.h"

/* Private STM Includes */
#include "stm32l4xx_hal.h"

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

/* Private Macros */

#define TIMER_SETTINGS_NOT_VALID(frequency, maxCount) ((SystemCoreClock / frequency) > maxCount)

/* Private Structures and Enumerations */

/* Private Variable Declarations */
I2C_HandleTypeDef hi2c1;

/* Private Function Prototypes */
void hardware_config_gpio_init(void);
void hardware_config_timer_init(void);
void hardware_error_handler(void);
void hardware_config_gpio_reset(void);
void hardware_config_uart_init(void);
void hardware_config_stm32_peripherals(void);
void hardware_config_usb(void);

void hardware_config_i2c_init(void);
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c);

/* Public Functions */

void hardware_config_init(void) {

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // Initialise uart communication and debugging
    hardware_config_uart_init();

    // Initialise all GPIO ports
    hardware_config_gpio_init();

    // Initialise all timers
    hardware_config_timer_init();

    // Initialise stm32 peripherals such as
    // internal rtc and comparators
    // hardware_config_stm32_peripherals();

    // It is important that this is done after __HAL_RCC_SYSCFG_CLK_ENABLE and
    // __HAL_RCC_PWR_CLK_ENABLE otherwise the USB will not work
    MX_USB_DEVICE_Init();

    hardware_config_i2c_init();
}

/* Private Functions */

void hardware_config_stm32_peripherals(void) {

    /* Configure the Real time clock*/

    // Enable The appropriate clocks for the RTC

    // Enable DPB bit so the RCC->BDCR register can be modified
    PWR->CR1 |= PWR_CR1_DBP;

    RCC->BDCR |= RCC_BDCR_RTCEN;
    RCC->BDCR |= RCC_BDCR_RTCSEL_0;
    RCC->BDCR |= RCC_BDCR_LSEON;
    while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {}
    // The APB clock frequency > 7 * RTC CLK frequency

    // Unlock the RTC registers by writing 0xCA followed by 0x53 into the WPR register
    STM32_RTC->WPR = 0xCA;
    STM32_RTC->WPR = 0x53;

    // Enter initialisation mode so the datetime of the RTC can be updated
    STM32_RTC->ISR |= RTC_ISR_INIT;

    // Wait for the INTIF bit to be set. This is done automatically by the
    // mcu and confirms the RTC is ready to be configured
    while ((STM32_RTC->ISR & RTC_ISR_INITF) == 0) {}

    // Generate 1Hz clock for the calendar counter. Assume freq = 32.768Khz
    RTC->PRER = 32768 - 1;

    // Set the time format to 24 hour time
    STM32_RTC->CR &= ~(RTC_CR_FMT);

    // Set the RTC time to 0
    STM32_RTC->TR &= ~(RTC_TR_HT | RTC_TR_HU | RTC_TR_MNT | RTC_TR_MNU | RTC_TR_ST | RTC_TR_SU);

    // Set the RTC date to 0
    STM32_RTC->DR &= ~(RTC_DR_YT | RTC_DR_YU | RTC_DR_MT | RTC_DR_MU | RTC_DR_DT | RTC_DR_DU);

    // Clear the initialisation to enable the RTC
    STM32_RTC->ISR &= ~(RTC_ISR_INIT);

    while ((STM32_RTC->ISR & RTC_ISR_INITF) != 0) {}

    // Confirm RSF flag is set
    while ((STM32_RTC->ISR & RTC_ISR_RSF) == 0) {}
}

/**
 * @brief Initialise the GPIO pins used by the system.
 */
void hardware_config_gpio_init(void) {

    // Enable clocks for GPIO port A and B
    RCC->AHB2ENR |= 0x03;

    /* Setup GPIO for LEDs */
    GPIO_SET_MODE_OUTPUT(LED_GREEN_PORT, LED_GREEN_PIN);
    GPIO_SET_LOW(LED_GREEN_PORT, LED_GREEN_PIN);

    GPIO_SET_MODE_OUTPUT(LED_RED_PORT, LED_RED_PIN);
    GPIO_SET_LOW(LED_RED_PORT, LED_RED_PIN);

    /* Set up ESP control pin */
    GPIO_SET_MODE_OUTPUT(ESP32_POWER_PORT, ESP32_POWER_PIN);
    GPIO_SET_HIGH(ESP32_POWER_PORT, ESP32_POWER_PIN);

    /* GPIO pin setup to trigger interrupt on rising and falling edge when USB C is connected */
    GPIO_SET_MODE_INPUT(USBC_CONN_PORT, USBC_CONN_PIN);
    SYSCFG->EXTICR[2] &= ~(0x07 << (4 * (USBC_CONN_PIN % 4)));
    EXTI->RTSR1 |= (0x01 << USBC_CONN_PIN); // Trigger on rising edge
    EXTI->FTSR1 |= (0x01 << USBC_CONN_PIN); // Trigger on falling edge
    EXTI->IMR1 |= (0x01 << USBC_CONN_PIN);  // Enable interrupts
    HAL_NVIC_SetPriority(USBC_CONN_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(USBC_CONN_IRQn);

    /* Setup GPIO for the RTC alarm */
    GPIO_SET_MODE_INPUT(RTC_ALARM_PORT, RTC_ALARM_PIN);
    GPIO_SET_PULL_AS_NONE(RTC_ALARM_PORT, RTC_ALARM_PIN);
    SYSCFG->EXTICR[1] &= ~(0x07 << (4 * (RTC_ALARM_PIN % 4))); // Clear bits
    SYSCFG->EXTICR[1] |= (0x01 << (4 * (RTC_ALARM_PIN % 4)));  // Set bits for
    EXTI->RTSR1 &= ~(0x01 << RTC_ALARM_PIN);                   // No trigger on rising edge
    EXTI->FTSR1 |= (0x01 << RTC_ALARM_PIN);                    // Trigger on falling edge
    EXTI->IMR1 |= (0x01 << RTC_ALARM_PIN);                     // Enable interrupts
    HAL_NVIC_SetPriority(RTC_ALARM_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(RTC_ALARM_IRQn);

    /****** START CODE BLOCK ******/
    // Description: GPIO configuration for the DS18B0 Sensor
    GPIO_SET_MODE_OUTPUT(DS18B20_PORT, DS18B20_PIN);
    GPIO_SET_TYPE_PUSH_PULL(DS18B20_PORT, DS18B20_PIN);
    GPIO_SET_SPEED_HIGH(DS18B20_PORT, DS18B20_PIN);
    GPIO_SET_PULL_AS_NONE(DS18B20_PORT, DS18B20_PIN);
    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Setup GPIO pins for communicating with ESP32 Cam
    // GPIO_SET_MODE_ALTERNATE_FUNCTION(UART_ESP32_RX_PORT, UART_ESP32_RX_PIN, AF15);
    // GPIO_SET_TYPE_PUSH_PULL(UART_ESP32_RX_PORT, UART_ESP32_RX_PIN);
    // GPIO_SET_SPEED_HIGH(UART_ESP32_RX_PORT, UART_ESP32_RX_PIN);
    // GPIO_SET_PULL_AS_NONE(UART_ESP32_RX_PORT, UART_ESP32_RX_PIN);

    // GPIO_SET_MODE_ALTERNATE_FUNCTION(UART_ESP32_TX_PORT, UART_ESP32_TX_PIN, AF7);
    // GPIO_SET_TYPE_PUSH_PULL(UART_ESP32_TX_PORT, UART_ESP32_TX_PIN);
    // GPIO_SET_SPEED_HIGH(UART_ESP32_TX_PORT, UART_ESP32_TX_PIN);
    // GPIO_SET_PULL_AS_NONE(UART_ESP32_TX_PORT, UART_ESP32_TX_PIN);

    UART_ESP32_RX_PORT->MODER &= ~(0x03 << (UART_ESP32_RX_PIN * 2));
    UART_ESP32_TX_PORT->MODER &= ~(0x03 << (UART_ESP32_TX_PIN * 2));
    UART_ESP32_RX_PORT->MODER |= (0x02 << (UART_ESP32_RX_PIN * 2));
    UART_ESP32_TX_PORT->MODER |= (0x02 << (UART_ESP32_TX_PIN * 2));

    // Set the RX TX pins to push pull
    UART_ESP32_RX_PORT->OTYPER &= ~(0x03 << UART_ESP32_RX_PIN);
    UART_ESP32_TX_PORT->OTYPER &= ~(0x03 << UART_ESP32_TX_PIN);

    UART_ESP32_RX_PORT->PUPDR &= ~(0x03 << (UART_ESP32_RX_PIN * 2));
    UART_ESP32_TX_PORT->PUPDR &= ~(0x03 << (UART_ESP32_TX_PIN * 2));

    // Set the RX TX pin speeds to high
    UART_ESP32_RX_PORT->OSPEEDR &= ~(0x03 << (UART_ESP32_RX_PIN * 2));
    UART_ESP32_TX_PORT->OSPEEDR &= ~(0x03 << (UART_ESP32_TX_PIN * 2));
    UART_ESP32_RX_PORT->OSPEEDR |= (0x02 << (UART_ESP32_RX_PIN * 2));
    UART_ESP32_TX_PORT->OSPEEDR |= (0x02 << (UART_ESP32_TX_PIN * 2));

    // Connect pin to alternate function
    UART_ESP32_RX_PORT->AFR[1] &= ~(0x0F << ((UART_ESP32_RX_PIN % 8) * 4));
    UART_ESP32_TX_PORT->AFR[1] &= ~(0x0F << ((UART_ESP32_TX_PIN % 8) * 4));
    UART_ESP32_RX_PORT->AFR[1] |= (0x07 << ((UART_ESP32_RX_PIN % 8) * 4));
    UART_ESP32_TX_PORT->AFR[1] |= (0x07 << ((UART_ESP32_TX_PIN % 8) * 4));
    HAL_NVIC_SetPriority(UART_ESP32_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(UART_ESP32_IRQn);
    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Setup GPIO pins for logging to console

    // Set the RX TX pins to alternate function mode
    UART_LOG_RX_PORT->MODER &= ~(0x03 << (UART_LOG_RX_PIN * 2));
    UART_LOG_TX_PORT->MODER &= ~(0x03 << (UART_LOG_TX_PIN * 2));
    UART_LOG_RX_PORT->MODER |= (0x02 << (UART_LOG_RX_PIN * 2));
    UART_LOG_TX_PORT->MODER |= (0x02 << (UART_LOG_TX_PIN * 2));

    // Set the RX TX pins to push pull
    UART_LOG_RX_PORT->OTYPER &= ~(0x01 << UART_LOG_RX_PIN);
    UART_LOG_TX_PORT->OTYPER &= ~(0x01 << UART_LOG_TX_PIN);

    // Set the RX TX pin speeds to high
    UART_LOG_RX_PORT->OSPEEDR &= ~(0x03 << (UART_LOG_RX_PIN * 2));
    UART_LOG_TX_PORT->OSPEEDR &= ~(0x03 << (UART_LOG_TX_PIN * 2));
    UART_LOG_RX_PORT->OSPEEDR |= (0x02 << (UART_LOG_RX_PIN * 2));
    UART_LOG_TX_PORT->OSPEEDR |= (0x02 << (UART_LOG_TX_PIN * 2));

    // Set the RX TX to have no pull up or pull down resistors
    UART_LOG_RX_PORT->PUPDR &= ~(0x03 << (UART_LOG_RX_PIN * 2));
    UART_LOG_TX_PORT->PUPDR &= ~(0x03 << (UART_LOG_TX_PIN * 2));

    // // Connect pin to alternate function
    UART_LOG_TX_PORT->AFR[0] &= ~(0x0F << ((UART_LOG_TX_PIN % 8) * 4));
    UART_LOG_RX_PORT->AFR[1] &= ~(0x0F << ((UART_LOG_RX_PIN % 8) * 4));
    UART_LOG_TX_PORT->AFR[0] |= (0x07 << (4 * (UART_LOG_TX_PIN % 8)));

    // Not sure why this isn't AF7 with PA2 but testing it, the STM32 only works with PA15 on AF3
    // which is a valid connection to UASAT_2_RX but so is PA3 on AF7 so not sure what the go is
    UART_LOG_RX_PORT->AFR[1] |= (0x03 << (4 * (UART_LOG_RX_PIN % 8)));
    HAL_NVIC_SetPriority(UART_LOG_IRQn, 11, 0);
    HAL_NVIC_EnableIRQ(UART_LOG_IRQn);

    /****** END CODE BLOCK ******/

    /* Setup GPIO pins for logging to console */
}

void hardware_config_timer_init(void) {

    /* Configure timer for DS18B20 Temperature Sensor*/
#if ((SYSTEM_CLOCK_CORE / DS18B20_TIMER_FREQUENCY) > DS18B20_TIMER_MAX_COUNT)
#    error System clock frequency is too high to generate the required timer frequnecy
#endif

    DS18B20_TIMER_CLK_ENABLE();                                           // Enable the clock
    DS18B20_TIMER->CR1 &= ~(TIM_CR1_CEN);                                 // Disable counter
    DS18B20_TIMER->PSC = (SystemCoreClock / DS18B20_TIMER_FREQUENCY) - 1; // Set timer frequency
    DS18B20_TIMER->ARR = DS18B20_TIMER_MAX_COUNT;                         // Set maximum count for timer
    DS18B20_TIMER->CNT = 0;                                               // Reset count to 0
    DS18B20_TIMER->DIER &= 0x00;                                          // Disable all interrupts by default
    DS18B20_TIMER->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);           // Set CH1 capture compare to mode to frozen

    /* Enable interrupt handler */
    // HAL_NVIC_SetPriority(HC_TS_TIMER_IRQn, HC_TS_TIMER_ISR_PRIORITY, 0);
    // HAL_NVIC_EnableIRQ(HC_TS_TIMER_IRQn);

    /* Configure timer for DS18B20 Temperature Sensor*/
    // #if ((SYSTEM_CLOCK_CORE / SERVO_TIMER_FREQUENCY) > SERVO_TIMER_MAX_COUNT)
    // #    error System clock frequency is too high to generate the required timer frequnecy
    // #endif

    //     SERVO_TIMER_CLK_ENABLE();                                         // Enable the clock
    //     SERVO_TIMER->CR1 &= ~(TIM_CR1_CEN);                               // Disable counter
    //     SERVO_TIMER->PSC = (SystemCoreClock / SERVO_TIMER_FREQUENCY) - 1; // Set timer frequency
    //     SERVO_TIMER->ARR = SERVO_TIMER_MAX_COUNT;                         // Set maximum count for timer
    //     SERVO_TIMER->CNT = 0;                                             // Reset count to 0
    //     SERVO_TIMER->DIER &= 0x00;                                        // Disable all interrupts by default
    //     SERVO_TIMER->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);         // Set CH1 capture compare to mode to
    //     frozen

    /* Enable interrupt handler */
    // HAL_NVIC_SetPriority(HC_TS_TIMER_IRQn, HC_TS_TIMER_ISR_PRIORITY, 0);
    // HAL_NVIC_EnableIRQ(HC_TS_TIMER_IRQn);

    /* If the system clock is too high, this timer will count too quickly and the timer
    will reach its maximum count and reset before the timer count reaches the number
    of ticks required to operate at the specified frequency. Lower system clock or
    increase the timer frequency if you get this error. E.g: System clock = 1Mhz and
    timer frequnecy = 1Hz => timer should reset after 1 million ticks to get a frequnecy
    of 1Hz but the max count < 1 million thus 1Hz can never be reached */
    // #if ((SYSTEM_CLOCK_CORE / TS_TIMER_FREQUENCY) > TS_TIMER_MAX_COUNT)
    // #    error System clock frequency is too high to generate the required timer frequnecy
    // #endif

    //     /* Configure timer for task scheduler*/
    //     TS_TIMER_CLK_ENABLE();                                      // Enable the clock
    //     TS_TIMER->CR1 &= ~(TIM_CR1_CEN);                            // Disable counter
    //     TS_TIMER->PSC = (SystemCoreClock / TS_TIMER_FREQUENCY) - 1; // Set timer frequency
    //     TS_TIMER->ARR = TS_TIMER_MAX_COUNT;                         // Set maximum count for timer
    //     TS_TIMER->CNT = 0;                                          // Reset count to 0
    //     TS_TIMER->DIER &= 0x00;                                     // Disable all interrupts by default
    //     TS_TIMER->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);      // Set CH1 capture compare to mode to frozen

    //     /* Enable interrupt handler */
    //     HAL_NVIC_SetPriority(TS_TIMER_IRQn, 10, 0);
    //     HAL_NVIC_EnableIRQ(TS_TIMER_IRQn);
}

void hardware_config_uart_init(void) {

    /**
     * Note that in the STM32 uart setup, the PWREN bit and the SYSCGEN bit are set
     * in the APB1ENR register. I'm pretty sure the SYSCGEN is only setup for exti
     * interrupts so I have that bit set in the exti interrupt function however if
     * you are setting up uart and there are no interrupts and it doesn't work, it
     * is most likely because either PWREN is not set or SYSCGEN is not set
     */

    /* Configure UART for Commuincating with ESP32 Cam */

    // Set baud rate
    UART_ESP32_CLK_ENABLE();
    UART_ESP32->BRR = SystemCoreClock / UART_ESP32_BUAD_RATE;
    UART_ESP32->CR1 = 0x00; // reset UART
    UART_ESP32->CR1 |= (USART_CR1_RE | USART_CR1_TE | USART_CR1_UE | USART_CR1_RXNEIE);

    // Set baud rate
    UART_LOG_CLK_ENABLE();
    UART_LOG->BRR = SystemCoreClock / UART_LOG_BUAD_RATE;
    UART_LOG->CR1 &= ~(USART_CR1_EOBIE); // Reset USART
    UART_LOG->CR1 |= (USART_CR1_RE | USART_CR1_TE | USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_PEIE);
}

void hardware_config_uart_sleep(void) {

    // Disable UART
    UART_LOG->CR1 &= ~(USART_CR1_UE);

    // Disable the UART CLK
    UART_LOG_CLK_DISABLE();
}

void hardware_config_uart_wakeup(void) {

    UART_LOG_CLK_ENABLE();

    // Disable the UART CLK
    UART_LOG->CR1 |= USART_CR1_UE;
}

void hardware_error_handler(void) {

    // Initialisation error shown by blinking LED (LD3) in pattern
    while (1) {}
}

void hardware_power_tests(void) {

    // Power cycle

    while (1) {}
}

void hardware_config_low_power_mode(void) {

    // Disable UART clocks
    // UART_ESP32_CLK_DISABLE();
    // UART_LOG_CLK_DISABLE();

    // Set the device to low power mode
    PWR->CR1 |= PWR_CR1_LPR;

    // Set the devcice to standby mode
    PWR->CR1 |= PWR_CR1_LPMS_STOP0 | PWR_CR1_LPMS_STOP1;
}

void hardware_config_normal_power_mode(void) {

    // Reenable the UART clocks
    // UART_ESP32_CLK_ENABLE();
    // UART_LOG_CLK_ENABLE();

    // Set the device to run mode
}

void hardware_config_i2c_init(void) {

    /* USER CODE BEGIN I2C1_Init 0 */

    /* USER CODE END I2C1_Init 0 */

    /* USER CODE BEGIN I2C1_Init 1 */

    /* USER CODE END I2C1_Init 1 */
    hi2c1.Instance              = I2C1;
    hi2c1.Init.Timing           = 0x00707CBB;
    hi2c1.Init.OwnAddress1      = 0;
    hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2      = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        error_handler();
    }

    /** Configure Analogue filter
     */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        error_handler();
    }

    /** Configure Digital filter
     */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        error_handler();
    }

    HAL_I2C_MspInit(&hi2c1);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct       = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if (hi2c->Instance == I2C1) {
        /* USER CODE BEGIN I2C1_MspInit 0 */

        /* USER CODE END I2C1_MspInit 0 */

        /** Initializes the peripherals clock
         */
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
        PeriphClkInit.I2c1ClockSelection   = RCC_I2C1CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            error_handler();
        }

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**I2C1 GPIO Configuration
        PB6     ------> I2C1_SCL
        PB7     ------> I2C1_SDA
        */
        GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C1_CLK_ENABLE();
        /* I2C1 interrupt Init */
        HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
        HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
        /* USER CODE BEGIN I2C1_MspInit 1 */

        /* USER CODE END I2C1_MspInit 1 */
    }
}