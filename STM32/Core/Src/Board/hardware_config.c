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

// Defines for the different configuration modes for the GPIO pins
#define MODER_INPUT                0x00
#define MODER_OUTPUT               0x01
#define MODER_ALTERNATE_FUNCTION   0x02
#define MODER_ANALOGUE             0x03
#define OTYPER_PUSH_PULL           0x00
#define OTYPER_OPEN_DRAIN          0x01
#define OSPEER_LOW_SPEED           0x00
#define OSPEER_MEDIUM_SPEED        0x01
#define OSPEER_HIGH_SPEED          0x02
#define OSPEER_VERY_HIGH_SPEED     0x03
#define PUPDR_NO_PULL_UP_PULL_DOWN 0x00
#define PUPDR_PULL_UP              0x01
#define PUPDR_PULL_DOWN            0x02

#define TIMER_SETTINGS_NOT_VALID(frequency, maxCount) ((SystemCoreClock / frequency) > maxCount)

/* Private Structures and Enumerations */

/* Private Variable Declarations */

/* Private Function Prototypes */
void hardware_config_gpio_init(void);
void hardware_config_timer_init(void);
void hardware_error_handler(void);
void hardware_config_gpio_reset(void);
void hardware_config_uart_init(void);

/* Public Functions */

void hardware_config_init(void) {

    // Initialise uart communication and debugging
    hardware_config_uart_init();

    // Initialise all GPIO ports
    hardware_config_gpio_init();

    // Initialise all timers
    hardware_config_timer_init();
}

/* Private Functions */

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

    // Set the RX TX pin speeds to high
    UART_ESP32_RX_PORT->OSPEEDR &= ~(0x03 << (UART_ESP32_RX_PIN * 2));
    UART_ESP32_TX_PORT->OSPEEDR &= ~(0x03 << (UART_ESP32_TX_PIN * 2));
    UART_ESP32_RX_PORT->OSPEEDR |= (0x02 << (UART_ESP32_RX_PIN * 2));
    UART_ESP32_TX_PORT->OSPEEDR |= (0x02 << (UART_ESP32_TX_PIN * 2));

    // Connect pin to alternate function
    UART_ESP32_RX_PORT->AFR[1] &= ~(0x0F << ((UART_ESP32_RX_PIN % 8) * 4));
    UART_ESP32_TX_PORT->AFR[1] &= ~(0x0F << ((UART_ESP32_TX_PIN % 8) * 4));
    UART_ESP32_RX_PORT->AFR[1] |= (7 << (4 * (UART_ESP32_RX_PIN % 8)));
    UART_ESP32_TX_PORT->AFR[1] |= (7 << (4 * (UART_ESP32_TX_PIN % 8)));

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

    // // Set the RX TX pin speeds to high
    UART_LOG_RX_PORT->OSPEEDR &= ~(0x03 << (UART_LOG_RX_PIN * 2));
    UART_LOG_TX_PORT->OSPEEDR &= ~(0x03 << (UART_LOG_TX_PIN * 2));
    UART_LOG_RX_PORT->OSPEEDR |= (0x02 << (UART_LOG_RX_PIN * 2));
    UART_LOG_TX_PORT->OSPEEDR |= (0x02 << (UART_LOG_TX_PIN * 2));

    // // Set the RX TX to have no pull up or pull down resistors
    UART_LOG_RX_PORT->PUPDR &= ~(0x03 << (UART_LOG_RX_PIN * 2));
    UART_LOG_TX_PORT->PUPDR &= ~(0x03 << (UART_LOG_TX_PIN * 2));

    // // Connect pin to alternate function
    UART_LOG_TX_PORT->AFR[0] &= ~(0x0F << ((UART_LOG_TX_PIN % 8) * 4));
    UART_LOG_RX_PORT->AFR[1] &= ~(0x0F << ((UART_LOG_RX_PIN % 8) * 4));
    UART_LOG_TX_PORT->AFR[0] |= (0x07 << (4 * (UART_LOG_TX_PIN % 8)));

    // Not sure why this isn't AF7 with PA2 but testing it, the STM32 only works with PA15 on AF3
    // which is a valid connection to UASAT_2_RX but so is PA3 on AF7 so not sure what the go is
    UART_LOG_RX_PORT->AFR[1] |= (0x03 << (4 * (UART_LOG_RX_PIN % 8)));
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
    // Enable UART clock
    // __HAL_RCC_SYSCFG_CLK_ENABLE();

    UART_ESP32_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // Set baud rate
    UART_ESP32->BRR = SystemCoreClock / UART_ESP32_BUAD_RATE;

    // Enable the USART to let comms occur
    UART_ESP32->CR1 |= (USART_CR1_RE | USART_CR1_TE | USART_CR1_UE);

    UART_LOG_CLK_ENABLE();

    // // Set baud rate
    UART_LOG->BRR = SystemCoreClock / UART_LOG_BUAD_RATE;

    // // Enable the USART to let comms occur
    UART_LOG->CR1 |= (USART_CR1_RE | USART_CR1_TE | USART_CR1_UE);

    // Enable peripheral clocks: GPIOA, USART2.
    // RCC->APB1ENR1 |= (RCC_APB1ENR1_USART2EN);
    // RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN);

    // Configure pins A2, A15 for USART2 (AF7, AF3).
    // GPIOA->MODER &= ~((0x3 << (2 * 2)) | (0x3 << (15 * 2)));
    // GPIOA->MODER |= ((0x2 << (2 * 2)) | (0x2 << (15 * 2)));
    // GPIOA->OTYPER &= ~((0x1 << 2) | (0x1 << 15));
    // GPIOA->OSPEEDR &= ~((0x3 << (2 * 2)) | (0x3 << (15 * 2)));
    // GPIOA->OSPEEDR |= ((0x2 << (2 * 2)) | (0x2 << (15 * 2)));
    // GPIOA->AFR[0] &= ~((0xF << (2 * 4)));
    // GPIOA->AFR[0] |= ((0x7 << (2 * 4)));
    // GPIOA->AFR[1] &= ~((0xF << ((15 - 8) * 4)));
    // GPIOA->AFR[1] |= ((0x3 << ((15 - 8) * 4)));

    // uint16_t uartdiv = SystemCoreClock / 115200;
    // USART2->BRR      = uartdiv;

    // Enable the USART to let comms occur
    // USART2->CR1 |= (USART_CR1_RE | USART_CR1_TE | USART_CR1_UE);
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