#ifndef BPACKET_UART_H
#define BPACKET_UART_H

/* C Library Includes */
#include <stdint.h>

/* Personal Includes */
#include "bpacket.h"
#include "cbuffer.h"
#include "utils.h"

/* Public Macros */

/* Public Structs and Enumerations */
enum bpk_uart_error_code_e { BPK_UART_ERR_NONE, BPK_UART_ERR_NOT_INITIALISED, BPK_UART_ERR_READ_ERROR };

typedef struct bpk_uart_id_t {
    uint8_t val;
} bpk_uart_id_t;

typedef struct bpk_uart_error_code_t {
    uint8_t val;
} bpk_uart_error_code_t;

extern bpk_uart_error_code_t BPK_Uart_Err_None;
extern bpk_uart_error_code_t BPK_Uart_Err_Not_Initialised;
extern bpk_uart_error_code_t BPK_Uart_Err_Read_Error;

void bpk_uart_init(bpk_addr_receive_t Address);

uint8_t bpk_uart_register_uart(bpk_uart_id_t* Id, void (*transmit_byte_func)(uint8_t byte), cbuffer_t* Buffer,
                               bpk_addr_receive_t DivertAdress);

uint8_t bpk_uart_read_packet(bpk_packet_t* Bpacket, const bpk_uart_id_t* Id);

uint8_t bpk_uart_get_error_code(bpk_uart_id_t* Id);

#endif // BPACKET_UART_H