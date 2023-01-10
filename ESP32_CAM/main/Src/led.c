
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
    gpio_set_level(ledPin, ON);
}

void led_off(int ledPin) {
    gpio_set_level(ledPin, OFF);
}