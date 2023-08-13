/**
 * @file bpacket_usb.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-04
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <stdint.h>
#include <stddef.h>

/* STM32 Library Includes */
#include "stm32l432xx.h"
#include "usbd_cdc_if.h"

/* Personal Includes */
#include "bpacket_usb.h"
#include "utils.h"
#include "cbuffer.h"
#include "bpacket.h"
#include "bpacket_utils.h"

#include "gpio.h"
#include "hardware_config.h"
#include "log_usb.h"

/* Private Macros */
#define RX_MAX_NUM_BPACKETS 2
#define RX_BUFFER_SIZE      (BPACKET_BUFFER_LENGTH_BYTES * RX_MAX_NUM_BPACKETS)

/* Private Variables */
uint8_t bpkUsbBuffer[RX_BUFFER_SIZE]; // Array that the circular buffer will use
cbuffer_t UsbBuffer = {
    .elements       = (void*)bpkUsbBuffer,
    .elementSize    = sizeof(uint8_t),
    .maxNumElements = RX_BUFFER_SIZE,
    .rIndex         = 0,
    .wIndex         = 0,
};

#define BPK_NUM_DIFFERENT_BYTES 8
uint8_t bpkByteOrder[BPK_NUM_DIFFERENT_BYTES] = {BPK_BYTE_START_BYTE_UPPER, BPK_BYTE_START_BYTE_LOWER,
                                                 BPK_BYTE_ADDRESS_RECEIVER, BPK_BYTE_ADDRESS_SENDER,
                                                 BPK_BYTE_REQUEST,          BPK_BYTE_CODE,
                                                 BPK_BYTE_LENGTH,           BPK_BYTE_DATA};
cbuffer_t ByteOrder                           = {
    .elements       = (void*)bpkByteOrder,
    .elementSize    = sizeof(uint8_t),
    .maxNumElements = BPK_NUM_DIFFERENT_BYTES,
    .rIndex         = 0,
    .wIndex         = 0,
};

USART_TypeDef* UartAddress = NULL;
uint8_t bpkIndex           = 0;

void usb_rx_handler(uint8_t* buffer, uint32_t numBytes) {

    for (uint32_t i = 0; i < numBytes; i++) {
        cbuffer_append_element(&UsbBuffer, (void*)&buffer[i]);
    }

    /* ###### START DEBUGGING BLOCK ###### */
    // Description:
    // GPIO_TOGGLE(PA0_PORT, PA0_PIN);
    /* ####### END DEBUGGING BLOCK ####### */
}

uint8_t usb_read_packet(bpk_t* Bpacket) {

    uint8_t byte;         // Stores the current byte being processed
    uint8_t expectedByte; // Stores a what the expected byte is (i.e start byte, data etc)

    // Loop through the circular buffer storing each byte into the 'byte' variable
    // until there are no more bytes to read
    while (cbuffer_read_next_element(&UsbBuffer, (void*)(&byte)) == TRUE) {

        // Read the current index of the byte buffer and store the result into
        // expected byte
        cbuffer_read_current_element(&ByteOrder, (void*)(&expectedByte));
        GPIO_TOGGLE(PA0_PORT, PA0_PIN);
        switch (expectedByte) {

            /* Expecting bpacket 'data' bytes */
            case BPK_BYTE_DATA:

                // Store the byte into the bpacket and increment the bpacket index
                Bpacket->Data.bytes[bpkIndex++] = byte;

                // Skip resetting if there are more date bytes to read
                if (bpkIndex < Bpacket->Data.numBytes) {
                    continue;
                }

                // No more data bytes to read. Execute request and then exit switch statement
                // to reset
                cbuffer_reset_read_index(&ByteOrder);
                bpkIndex = 0;

                // Bpacket to deal with thus returning true
                return TRUE;

            /* Expecting bpacket 'length' byte */
            case BPK_BYTE_LENGTH:

                // Store the length and updated the expected byte
                Bpacket->Data.numBytes = byte;
                cbuffer_increment_read_index(&ByteOrder);

                // Skip reseting if there is data to be read
                if (Bpacket->Data.numBytes > 0) {
                    continue;
                }

                // No data in Bpacket. Execute request and exit switch statement to reset
                cbuffer_reset_read_index(&ByteOrder);
                bpkIndex = 0;

                return TRUE;

            /* Expecting another bpacket byte that is not 'data' or 'length' */
            default:

                // Updated the expected byte
                cbuffer_increment_read_index(&ByteOrder);

                // If decoding the byte succeeded, move to the next expected byte by
                // incrementing the circrular buffer holding the expected bytes
                if (bpk_utils_decode_non_data_byte(Bpacket, expectedByte, byte) == TRUE) {
                    continue;
                }
        }

        // Reset the expected byte back to the start and bpacket buffer index
        cbuffer_reset_read_index(&ByteOrder);
        bpkIndex = 0;
    }

    return FALSE;
}