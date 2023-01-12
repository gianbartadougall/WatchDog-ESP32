
/* Public Includes */
#include <driver/gpio.h>
#include <esp_log.h>

/* Private Includes */
#include "led.h"
#include "hardware_config.h"

/* Private Variable Declarations */
const static char* TAG = "LED";

#define OFF 0
#define ON  1

void led_toggle(int ledPin) {
    if (gpio_get_level(ledPin) == ON) {
        led_off(ledPin);
    } else {
        led_on(ledPin);
    }
}

void led_on(int ledPin) {

    if (ledPin == HC_RED_LED) {
        gpio_set_level(ledPin, OFF); // The RED LED is inverted
        return;
    }

    if (ledPin == HC_COB_LED) {
        gpio_set_level(HC_COB_LED, ON); // The COB LED is not inverted
    }
}

void led_off(int ledPin) {
    if (ledPin == HC_RED_LED) {
        gpio_set_level(ledPin, ON); // The RED LED is inverted
        return;
    }

    if (ledPin == HC_COB_LED) {
        gpio_set_level(HC_COB_LED, OFF); // The COB LED is not inverted
    }
}