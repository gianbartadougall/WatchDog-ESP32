/**
 * @file unit_tests.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-02-14
 *
 * @copyright Copyright (c) 2023
 *
 */

/* Personal Includes */
#include "bpacket.h"

void unit_tests_init(bpacket_t* packetBuffer, uint32_t* packetBufferIndex, uint32_t* packetPendingIndex);

uint8_t ut_test_red_led_on(void);

uint8_t ut_test_red_led_off(void);