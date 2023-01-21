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
/* Public Includes */
#include "hardware_config.h"
#include "board.h"
#include "log.h"

// #include "stm32l4xx_hal_msp.h"

/* Private Includes */

/* Private STM Includes */
#include "stm32l4xx_hal.h"

/* Private Macros */

#define TIMER_SETTINGS_NOT_VALID(frequency, maxCount) ((SystemCoreClock / frequency) > maxCount)

/* Private Structures and Enumerations */

/* Private Variable Declarations */

/* Private Function Prototypes */
void hardware_config_gpio_init(void);
void hardware_config_timer_init(void);
void hardware_error_handler(void);
void hardware_config_gpio_reset(void);
void hardware_config_uart_init(void);
void hardware_config_stm32_peripherals(void);

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
    hardware_config_stm32_peripherals();
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

    /* Setup GPIO for LED */
    LED_GREEN_PORT->MODER &= ~(0x3 << (LED_GREEN_PIN * 2));
    LED_GREEN_PORT->MODER |= (0x01 << (LED_GREEN_PIN * 2));

    /****** START CODE BLOCK ******/
    // Description:

    // Set to be output
    DS18B20_PORT->MODER &= ~(0x3 << (DS18B20_PIN * 2));
    DS18B20_PORT->MODER |= (0x01 << (DS18B20_PIN * 2));

    DS18B20_PORT->OTYPER &= ~(0x01 << DS18B20_PIN); // Push pull

    DS18B20_PORT->OSPEEDR |= (0x03 << (DS18B20_PIN * 2)); // High speed

    DS18B20_PORT->PUPDR &= ~(0x03 << (DS18B20_PIN * 2)); // No pull up/pull down

    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Setup GPIO pins for communicating with ESP32 Cam

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
    UART_ESP32_RX_PORT->AFR[0] &= ~(0x0F << ((UART_ESP32_RX_PIN % 8) * 4));
    UART_ESP32_TX_PORT->AFR[1] &= ~(0x0F << ((UART_ESP32_TX_PIN % 8) * 4));
    UART_ESP32_RX_PORT->AFR[0] |= (0x07 << ((UART_ESP32_RX_PIN % 8) * 4));
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

    // Initialise onboad LED incase it hasn't been initialised
    board_init();

    // Initialisation error shown by blinking LED (LD3) in pattern
    while (1) {

        for (int i = 0; i < 5; i++) {
            brd_led_toggle();
            HAL_Delay(100);
        }

        HAL_Delay(500);
    }
}

void hardware_config_low_power_mode(void) {

    // Disable

    // Set the device to low power mode
    PWR->CR1 |= PWR_CR1_LPR;

    // Set the devcice to standby mode
    PWR->CR1 |= PWR_CR1_LPMS_STOP0 | PWR_CR1_LPMS_STOP1;
}