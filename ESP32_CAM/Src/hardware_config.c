
/* Public Includes */
#include <driver/uart.h>

/* Private Includes */
#include "wd_utils.h"
#include "hardware_config.h"
#include "sd_card.h"

/* Private Function Definitions */
void hardware_config_leds(void);
void hardware_config_uart_comms(void);

uint8_t hardware_config(void) {

    /* Configure all the required hardware */

    // Configure the SD card and ensure it can be used. This is done before
    // any other hardware so that everything else can be logged
    char msg[100];
    if (sd_card_open() != WD_SUCCESS) {
        return WD_ERROR;
    }

    // Configure Camera
    sprintf(msg, "Configuring Camera");
    sd_card_log(SYSTEM_LOG_FILE, msg);
    if (init_camera() != ESP_OK) {
        return WD_ERROR;
    }

    sprintf(msg, "Configuring UART");
    sd_card_log(SYSTEM_LOG_FILE, msg);
    hardware_config_uart_comms();

    sprintf(msg, "Configuring LEDs");
    sd_card_log(SYSTEM_LOG_FILE, msg);
    hardware_config_leds();

    // Unmount the SD card
    sd_card_close();

    return WD_SUCCESS;
}

void hardware_config_leds(void) {

    // Set the onboard red LED to be an output
    gpio_reset_pin(HC_RED_LED);
    gpio_set_direction(HC_RED_LED, GPIO_MODE_OUTPUT);
}

void hardware_config_uart_comms(void) {

    // Configure UART for communications
    const uart_config_t uart_config = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(HC_UART_COMMS_UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(HC_UART_COMMS_UART_NUM, &uart_config);
    uart_set_pin(HC_UART_COMMS_UART_NUM, HC_UART_COMMS_TX_PIN, HC_UART_COMMS_RX_PIN, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);

    // TODO: Come up with way to check if UART was configured correctly!
}