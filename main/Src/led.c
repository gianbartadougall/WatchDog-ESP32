
/* Public Includes */

/* Private Includes */
#include "led.h"
#include "hardware_config.h"

#define ON  1
#define OFF 0

int redLedState = 0;
int cobLedState = 0;

void led_board_toggle(void) {
    if (redLedState == ON) {
        led_board_off();
    } else {
        led_board_on();
    }
}

void led_board_on(void) {
    redLedState = ON;
    gpio_set_level(HC_RED_LED, ON);
}

void led_board_off(void) {
    redLedState = OFF;
    gpio_set_level(HC_RED_LED, OFF);
}

void led_cob_toggle(void) {
    if (cobLedState == ON) {
        led_cob_off();
    } else {
        led_cob_on();
    }
}

void led_cob_on(void) {
    cobLedState = ON;
    gpio_set_level(HC_COB_LED, ON);
}

void led_cob_off(void) {
    cobLedState = OFF;
    gpio_set_level(HC_COB_LED, OFF);
}