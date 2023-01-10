#ifndef LED_H
#define LED_H

/**
 * @brief Toggles the state of the onboard LED on the ESP32
 *
 * @param ledPin The GPIO pin totoggle
 */
void led_toggle(int ledPin);

/**
 * @brief Turns the onboard LED on the ESP32 on
 *
 * @param ledPin The GPIO pin to set high
 */
void led_on(int ledPin);

/**
 * @brief Turns the onboard LED on the ESP32 off
 *
 * @param ledPin The GPIO pin to set low
 */
void led_off(int ledPin);

#endif // LED_H