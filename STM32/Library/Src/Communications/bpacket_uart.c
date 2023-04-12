/* C Library Includes */
#include <stdlib.h>

/* Personal Includes */
#include "bpacket_uart.h"

/* Private Variables */

bpk_uart_error_code_t BPK_Uart_Err_None            = {.val = BPK_UART_ERR_NONE};
bpk_uart_error_code_t BPK_Uart_Err_Not_Initialised = {.val = BPK_UART_ERR_NOT_INITIALISED};
bpk_uart_error_code_t BPK_Uart_Err_Read_Error      = {.val = BPK_UART_ERR_READ_ERROR};

typedef struct bpk_uart_params_t {
    bpk_uart_error_code_t ErrorCode;
    bpk_addr_receive_t DivertAddress;
    uint8_t divertBytes;
    uint8_t numDivertedDataBytes;
    uint8_t dataRead;
    cbuffer_t* RxBuffer;
    cbuffer_t ExpectedByte;
    void (*uart_transmit_byte)(uint8_t byte);
} bpk_uart_params_t;

bpk_uart_params_t* Params;
uint8_t numParams = 0;

#define BPACKET_BYTE_TYPES 7
uint8_t bpkByteBuffer[BPACKET_BYTE_TYPES] = {
    BPK_BYTE_START_BYTE_UPPER,
    BPK_BYTE_START_BYTE_LOWER,
    BPK_BYTE_ADDRESS_RECEIVER,
    BPK_BYTE_ADDRESS_SENDER,
    BPK_BYTE_REQUEST,
    BPK_BYTE_CODE,
    BPK_BYTE_DATA,
};

bpk_addr_receive_t SelfAddress = {.val = 255};

void bpk_uart_init(bpk_addr_receive_t Address) {
    SelfAddress = Address;
}

uint8_t bpk_uart_register_uart(bpk_uart_id_t* Id, void (*transmit_byte_func)(uint8_t byte), cbuffer_t* Buffer,
                               bpk_addr_receive_t DivertAdress) {

    // Confirm the self address has been set and that the divert address is valid
    if ((SelfAddress.val == 255) || (SelfAddress.val == DivertAdress.val)) {
        return FALSE;
    }

    // Confirm the Rxbuffer has been initialised
    if (Buffer == NULL) {
        return FALSE;
    }

    // Confirm the uart transmission function has been initialised
    if (transmit_byte_func == NULL) {
        return FALSE;
    }

    Id->val = numParams;
    numParams++;

    // Try allocate memory for uart line
    if (numParams == 1) {
        Params = malloc(sizeof(Params) * numParams);
    } else {
        Params = realloc(Params, sizeof(Params) * numParams);
    }

    if (Params == NULL) {
        return FALSE;
    }

    Params[Id->val].ErrorCode          = BPK_Uart_Err_None;
    Params[Id->val].DivertAddress      = DivertAdress;
    Params[Id->val].divertBytes        = FALSE;
    Params[Id->val].dataRead           = 0;
    Params[Id->val].RxBuffer           = Buffer;
    Params[Id->val].uart_transmit_byte = transmit_byte_func;
    cbuffer_init(&(Params[Id->val].ExpectedByte), bpkByteBuffer, BPACKET_BYTE_TYPES);

    return TRUE;
}

// uint8_t bpk_uart_read_packet(bpk_packet_t* Bpacket, const bpk_uart_id_t* Id) {

//     if (Id.val > (numParams - 1)) {
//         Params[Id->val].ErrorCode = BPK_Uart_Err_Not_Initialised;
//         return FALSE;
//     }

//     bpk_uart_params_t* Uart = &Params[Id->val];
//     uint8_t byte;
//     uint8_t expectedByte = Uart->ExpectedByte.bytes[0];

//     while (cbuffer_read_next_byte(Uart->RxBuffer, &byte) == TRUE) {

//         switch (expectedByte) {

//             case BPK_BYTE_START_BYTE_UPPER:
//             case BPK_BYTE_START_BYTE_LOWER:

//                 if (byte == expectedByte) {
//                     cbuffer_increment_read_index(&Uart->ExpectedByte);
//                     continue;
//                 }

//                 break;

//             case BPK_BYTE_ADDRESS_RECEIVER:

//                 if (SelfAddress.val != byte) {
//                     divertBytes[Uart->id] = TRUE;
//                     Uart->uart_transmit_byte(BPK_BYTE_START_BYTE_UPPER);
//                     Uart->uart_transmit_byte(BPK_BYTE_START_BYTE_LOWER);
//                     Uart->uart_transmit_byte(byte);

//                     while (cbuffer_read_next_byte(Uart->RxBuffer, &byte) == TRUE) {
//                         Uart->uart_transmit_byte(byte);
//                     }

//                     // Reset the expected byte
//                     cbuffer_reset_read_index(Uart->ExpectedByte);
//                     return FALSE;
//                 }

//                 if (bpk_set_receiver(Bpacket, byte) == TRUE) {
//                     cbuffer_increment_read_index(&Uart->ExpectedByte);
//                     continue;
//                 }

//                 break;

//             case BPK_BYTE_ADDRESS_SENDER:

//                 if (bpk_set_sender(Bpacket, byte) == TRUE) {
//                     cbuffer_increment_read_index(&Uart->ExpectedByte);
//                     continue;
//                 }

//                 break;

//             case BPK_BYTE_REQUEST:

//                 if (bpk_set_request(Bpacket, byte) == TRUE) {
//                     cbuffer_increment_read_index(&Uart->ExpectedByte);
//                     continue;
//                 }

//                 break;

//             case BPK_BYTE_CODE:

//                 if (bpk_set_code(Bpacket, byte) == TRUE) {
//                     cbuffer_increment_read_index(&Uart->ExpectedByte);
//                     continue;
//                 }

//                 break;

//             case BPK_BYTE_LENGTH:

//                 Bpacket->Data.numBytes = byte;
//                 cbuffer_increment_read_index(&Uart->ExpectedByte);
//                 continue;

//             case BPK_BYTE_DATA:

//                 if (Uart->dataRead == Bpacket->Data.numBytes) {
//                     cbuffer_reset_read_index(&Uart->ExpectedByte);
//                     Uart->dataRead = 0;
//                     return TRUE;
//                 }

//                 Bpacket->Data.bytes[Uart->dataRead++] = byte;
//                 continue;

//             default:
//                 // Should never reach here
//         }

//         // Error occured
//         Uart->ErrorCode = BPK_Uart_Err_Read_Error;
//         Uart->dataRead  = 0;
//         cbuffer_reset_read_index(&Uart->ExpectedByte);
//     }

//     return FALSE;
// }

uint8_t bpk_uart_get_error_code(bpk_uart_id_t* Id) {
    return Params[Id->val].ErrorCode.val;
}

uint8_t bpk_uart_read_packet(bpk_packet_t* Bpacket, const bpk_uart_id_t* Id) {

    if (Id->val > (numParams - 1)) {
        Params[Id->val].ErrorCode = BPK_Uart_Err_Not_Initialised;
        return FALSE;
    }

    bpk_uart_params_t* Uart = &Params[Id->val];
    uint8_t byte;
    uint8_t expectedByte = Uart->ExpectedByte.bytes[0];

    while (cbuffer_read_next_byte(Uart->RxBuffer, &byte) == TRUE) {

        if ((expectedByte == BPK_BYTE_START_BYTE_UPPER) || (expectedByte == BPK_BYTE_START_BYTE_LOWER)) {

            if (byte == expectedByte) {
                cbuffer_increment_read_index(&Uart->ExpectedByte);
                continue;
            }
        }

        if (expectedByte == BPK_BYTE_ADDRESS_RECEIVER) {

            cbuffer_increment_read_index(&Uart->ExpectedByte);

            if (SelfAddress.val == byte) {
                Uart->divertBytes = FALSE;

                if (bpk_set_receiver(Bpacket, byte) == TRUE) {
                    continue;
                }
            }

            if (SelfAddress.val == Uart->DivertAddress.val) {
                Uart->divertBytes = TRUE;

                Uart->uart_transmit_byte(BPK_BYTE_START_BYTE_UPPER);
                Uart->uart_transmit_byte(BPK_BYTE_START_BYTE_LOWER);
                Uart->uart_transmit_byte(byte);
                continue;
            }
        }

        if (expectedByte == BPK_BYTE_ADDRESS_SENDER) {

            if (Uart->divertBytes == TRUE) {
                Uart->uart_transmit_byte(byte);
                continue;
            }

            if (bpk_set_sender(Bpacket, byte) == TRUE) {
                cbuffer_increment_read_index(&Uart->ExpectedByte);
                continue;
            }
        }

        if (expectedByte == BPK_BYTE_REQUEST) {

            if (Uart->divertBytes == TRUE) {
                Uart->uart_transmit_byte(byte);
                continue;
            }

            if (bpk_set_request(Bpacket, byte) == TRUE) {
                cbuffer_increment_read_index(&Uart->ExpectedByte);
                continue;
            }
        }

        if (expectedByte == BPK_BYTE_CODE) {

            if (Uart->divertBytes == TRUE) {
                Uart->uart_transmit_byte(byte);
                continue;
            }

            if (bpk_set_code(Bpacket, byte) == TRUE) {
                cbuffer_increment_read_index(&Uart->ExpectedByte);
                continue;
            }
        }

        if (expectedByte == BPK_BYTE_LENGTH) {

            if (Uart->divertBytes == TRUE) {
                Uart->numDivertedDataBytes = byte;
                Uart->uart_transmit_byte(byte);
                continue;
            }

            Bpacket->Data.numBytes = byte;
            cbuffer_increment_read_index(&Uart->ExpectedByte);
            continue;
        }

        if (expectedByte == BPK_BYTE_DATA) {

            if (Uart->divertBytes == TRUE) {

                if (Uart->dataRead == Uart->numDivertedDataBytes) {
                    cbuffer_reset_read_index(&Uart->ExpectedByte);
                    Uart->dataRead = 0;
                    return TRUE;
                }

                Uart->uart_transmit_byte(byte);
                continue;
            }

            if (Uart->dataRead == Bpacket->Data.numBytes) {
                cbuffer_reset_read_index(&Uart->ExpectedByte);
                Uart->dataRead = 0;
                return TRUE;
            }

            Bpacket->Data.bytes[Uart->dataRead++] = byte;
            continue;
        }

        // Error occured
        Uart->ErrorCode = BPK_Uart_Err_Read_Error;
        Uart->dataRead  = 0;
        cbuffer_reset_read_index(&Uart->ExpectedByte);
    }

    return FALSE;
}