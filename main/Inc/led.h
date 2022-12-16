#ifndef LED_H
#define LED_H

/**
 * @brief Toggles the state of the onboard LED on the ESP32
 */
void led_board_toggle(void);

/**
 * @brief Turns the onboard LED on the ESP32 on
 */
void led_board_on(void);

/**
 * @brief Turns the onboard LED on the ESP32 off
 */
void led_board_off(void);

/**
 * @brief Toggles the state of the COB LED on the ESP32
 */
void led_cob_toggle(void);

/**
 * @brief Turns the COB LED on the ESP32 on
 */
void led_cob_on(void);

/**
 * @brief Turns the COB LED on the ESP32 off
 */
void led_cob_off(void);

#endif // LED_H