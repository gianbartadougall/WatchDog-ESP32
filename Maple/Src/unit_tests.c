/**
 * @file unit_tests.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-02-14
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include "time.h"
#include <string.h>
#include <stdio.h>

/* Personal Includes */
#include "uart_lib.h"
#include "utilities.h"
#include "watchdog_defines.h"

uint32_t* packetBufferIndexPtr;
uint32_t* packetPendingIndexPtr;
bpacket_t* packetBufferPtr;
uint8_t packetBufferSize;

void unit_tests_init(bpacket_t* packetBuffer, uint32_t* packetBufferIndex, uint32_t* packetPendingIndex, uint8_t pbs) {

    packetBufferIndexPtr  = packetBufferIndex;
    packetPendingIndexPtr = packetPendingIndex;
    packetBufferPtr       = packetBuffer;
    packetBufferSize      = pbs;
}

bpacket_t* unit_tests_get_next_bpacket_response(void) {

    if (*packetPendingIndexPtr == *packetBufferIndexPtr) {
        return NULL;
    }

    bpacket_t* buffer = packetBufferPtr + (*packetPendingIndexPtr);
    bpacket_increment_circ_buff_index(packetPendingIndexPtr, packetBufferSize);
    return buffer;
}

uint8_t unit_tests_response_is_valid(uint8_t expectedRequest, uint16_t timeout) {

    // Print response
    clock_t startTime = clock();
    while ((clock() - startTime) < timeout) {

        bpacket_t* bpacket = unit_tests_get_next_bpacket_response();

        // If there is next response yet then skip
        if (bpacket == NULL) {
            continue;
        }

        // Skip if the bpacket is just a message
        if (bpacket->request == BPACKET_GEN_R_MESSAGE) {
            continue;
        }

        // Confirm the received bpacket matches the correct request and code
        if (bpacket->request != expectedRequest) {
            printf("Rec: %i Snd: %i Req: %i Code: %i num bytes: %i\r\n", bpacket->receiver, bpacket->sender,
                   bpacket->request, bpacket->code, bpacket->numBytes);
            // printf("Expected request %i but got %i\n", expectedRequest, bpacket->request);
            return FALSE;
        }

        if (bpacket->code != BPACKET_CODE_SUCCESS) {
            printf("Expected code %i but got %i\n", BPACKET_CODE_SUCCESS, bpacket->request);
            return FALSE;
        }

        return TRUE;
    }

    printf("Failed to get response\n");
    return FALSE;
}

uint8_t ut_test_red_led_on(void) {

    /* Turn the red LED on */
    maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_ON, BPACKET_ADDRESS_STM32, 0, NULL);
    if (unit_tests_response_is_valid(WATCHDOG_BPK_R_LED_RED_ON, 1000) == FALSE) {
        printf("%sTurning the red LED on via STM32 failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    /* Turn the red LED on directly */
    maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_ON, BPACKET_ADDRESS_ESP32, 0, NULL);
    if (unit_tests_response_is_valid(WATCHDOG_BPK_R_LED_RED_ON, 1000) == FALSE) {
        printf("%sTurning the red LED on directly failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    return TRUE;
}

uint8_t ut_test_red_led_off(void) {

    /* Turn the red LED on */
    maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_OFF, BPACKET_ADDRESS_STM32, 0, NULL);
    if (unit_tests_response_is_valid(WATCHDOG_BPK_R_LED_RED_OFF, 1000) == FALSE) {
        printf("%sTurning the red LED off via STM32 failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    /* Turn the red LED on directly */
    maple_create_and_send_bpacket(WATCHDOG_BPK_R_LED_RED_OFF, BPACKET_ADDRESS_ESP32, 0, NULL);
    if (unit_tests_response_is_valid(WATCHDOG_BPK_R_LED_RED_OFF, 1000) == FALSE) {
        printf("%sTurning the red LED off directly failed%s\n", ASCII_COLOR_RED, ASCII_COLOR_WHITE);
        return FALSE;
    }

    return TRUE;
}