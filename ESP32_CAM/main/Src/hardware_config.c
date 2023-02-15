
/* Public Includes */
#include <driver/uart.h>

/* Private Includes */
#include "hardware_config.h"
#include "sd_card.h"
#include "camera.h"
#include "uart_comms.h"
#include "utilities.h"

/* Private Function Definitions */
void hardware_config_leds(void);
void hardware_config_uart_comms(void);

uint8_t hardware_config(bpacket_t* bpacket) {

    /* Configure all the required hardware */
    uint8_t request = bpacket->request;

    // Configure the SD card and ensure it can be used. This is done before
    // any other hardware so that everything else can be logged
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, request, BPACKET_CODE_ERROR,
                          "SD Card failed to init\0");
        return FALSE;
    }

    sd_card_log(SYSTEM_LOG_FILE, "Configuring UART\0");
    hardware_config_uart_comms();

    sd_card_log(SYSTEM_LOG_FILE, "Configuring LEDs\0");
    hardware_config_leds();

    // Unmount the SD card
    sd_card_close();

    return TRUE;
}

void hardware_config_leds(void) {

    // Set the onboard red LED to be an output
    gpio_reset_pin(HC_RED_LED);
    gpio_set_direction(HC_RED_LED, GPIO_MODE_INPUT_OUTPUT);

    gpio_reset_pin(HC_COB_LED);
    gpio_set_direction(HC_COB_LED, GPIO_MODE_INPUT_OUTPUT);
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
    uart_driver_install(HC_UART_COMMS_UART_NUM, RX_RING_BUFFER_BYTE_SIZE, 0, 0, NULL, 0);
    uart_param_config(HC_UART_COMMS_UART_NUM, &uart_config);
    uart_set_pin(HC_UART_COMMS_UART_NUM, HC_UART_COMMS_TX_PIN, HC_UART_COMMS_RX_PIN, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);

    // TODO: Come up with way to check if UART was configured correctly!
}