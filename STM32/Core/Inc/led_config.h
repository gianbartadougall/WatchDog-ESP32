#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include "hardware_config.h"
#include "led.h"

typedef struct LED {
    const uint8_t id;
    GPIO_TypeDef* port;
    uint32_t pin;
} LED;

/* Initialise LEDs */

#define NUM_LEDS 1

const LED greenLED = {
    .id   = LED_GREEN_ID,
    .port = LED_GREEN_PORT,
    .pin  = LED_GREEN_PIN,
};

LED leds[NUM_LEDS] = {greenLED};

#endif // LED_CONFIG_H