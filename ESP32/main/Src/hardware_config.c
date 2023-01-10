/**
 * @file hardware_config.c
 * @author Gian Barta-Dougall
 * @brief Hardware configuration for ESP32
 * @version 0.1
 * @date 2022-12-19
 *
 * @copyright Copyright (c) 2022
 *
 */
/* Public Includes */
#include <driver/uart.h>
#include <esp_log.h>
#include <driver/gpio.h>

/* Private Includes */
#include "wd_utils.h"
#include "hardware_config.h"

/* Private Variable Declarations */
const static char* TAG = "Hardware config";

/* Private Function Definitions */
void hardware_config_leds(void);
void hardware_config_uart_comms(void);
void hardware_config_timers(void);
void hardware_config_gpio(void);

uint8_t hardware_config(void) {

    /* Configure all the required hardware */
    ESP_LOGI(TAG, "Configuring UART");
    hardware_config_uart_comms();

    hardware_config_gpio();

    ESP_LOGI(TAG, "Configuring RTC");
    // TODO: Configure RTC

    ESP_LOGI(TAG, "Configuring Temperature Sensor");
    hardware_config_timers();

    return WD_SUCCESS;
}

void hardware_config_gpio(void) {

    /* Configure GPIO for LEDs */

    // Input/output is required for gpio_get_level() function to work
    gpio_reset_pin(HC_RED_LED);
    gpio_set_direction(HC_RED_LED, GPIO_MODE_INPUT_OUTPUT);

    gpio_reset_pin(HC_LED_2);
    gpio_set_direction(HC_LED_2, GPIO_MODE_INPUT_OUTPUT);
}

void hardware_config_uart_comms(void) {

    const uart_config_t uart_config = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // We won't use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(HC_UART_COMMS_UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(HC_UART_COMMS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(HC_UART_COMMS_UART_NUM, HC_UART_COMMS_TX_PIN, HC_UART_COMMS_RX_PIN, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));

    // TODO: Come up with way to check if UART was configured correctly!
}

void hardware_config_timers(void) {

    /****** START CODE BLOCK ******/
    // Description: Configure a microsecond timer for the temperature sensor
    // ds18b20TimerConfig.divider     = 40;
    // ds18b20TimerConfig.counter_dir = TIMER_COUNT_UP;
    // ds18b20TimerConfig.counter_en  = TIMER_START;
    // ds18b20TimerConfig.alarm_en    = TIMER_ALARM_DIS;
    // ds18b20TimerConfig.intr_type   = TIMER_INTR_LEVEL;
    // ds18b20TimerConfig.auto_reload = TIMER_AUTORELOAD_DIS;

    // timer_init(DS18B20_TIMER_GROUP, DS18B20_TIMER, &ds18b20TimerConfig);

    // // Set the timer period
    // timer_set_counter_value(DS18B20_TIMER_GROUP, DS18B20_TIMER, 0x00000000ULL);

    /****** END CODE BLOCK ******/
}