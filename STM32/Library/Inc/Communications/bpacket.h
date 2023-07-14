/**
 * @file bpacket.h
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-18
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef BPACKET_H
#define BPACKET_H

/* C Library Includes */
#include <stdint.h>

// 16 bit start and end byte means when checking
// there is a 1 in 2^32 chance of getting a start
// and stop byte together randomly. The start and
// stop bytes were chosen so they wouldn't occur
// very frequently together
#define BPACKET_START_BYTE_UPPER 'B'
#define BPACKET_START_BYTE_LOWER 'z'

/**
 * @brief BPacket codes. These gives context to the
 * request in a bpacket.
 *
 * E.g if you receive a bpacket with the code
 * BPACKET_CODE_ERROR then this states the sender
 * of the bpacket failed to execute the request.
 *
 * If you receive a bpacket with the code
 * BPACKET_CODE_IN_PROGRESS then the sender of the
 * bpacket is stil processing the request
 */
enum bpk_code_e {
    BPK_CODE_ERROR,
    BPK_CODE_SUCCESS,
    BPK_CODE_IN_PROGRESS,
    BPK_CODE_UNKNOWN,
    BPK_CODE_EXECUTE,
    BPK_CODE_TODO,
    BPK_CODE_DEBUG
};

enum bpk_byte_type_e {
    BPK_BYTE_START_BYTE_UPPER = 66,
    BPK_BYTE_START_BYTE_LOWER = 122,
    BPK_BYTE_ADDRESS_RECEIVER = 0,
    BPK_BYTE_ADDRESS_SENDER,
    BPK_BYTE_REQUEST,
    BPK_BYTE_CODE,
    BPK_BYTE_LENGTH,
    BPK_BYTE_DATA,
};

#define BPACKET_MAX_REQUEST_VALUE 63 // Allow maximum of 63 different request values

enum bpk_request_e {
    /* User defined Generic Bpacket requests */
    BPK_REQUEST_HELP,
    BPK_REQUEST_PING,
    BPK_REQUEST_STATUS,
    BPK_REQUEST_MESSAGE,

    /* User defined Watchdog requests */
    BPK_REQ_LIST_DIR,
    BPK_REQ_COPY_FILE,
    BPK_REQ_TAKE_PHOTO,
    BPK_REQ_WRITE_TO_FILE,
    BPK_REQ_DELETE_FILE,
    BPK_REQ_RECORD_DATA,
    BPK_REQ_LED_RED_ON,
    BPK_REQ_LED_RED_OFF,
    BPK_REQ_CAMERA_VIEW,
    BPK_REQ_GET_DATETIME,
    BPK_REQ_SET_DATETIME,
    BPK_REQ_GET_WATCHDOG_SETTINGS,
    BPK_REQ_SET_WATCHDOG_SETTINGS,
    BPK_REQ_GET_TEMPERATURE,
    BPK_REQ_STREAM_IMAGE,
    BPK_REQ_ESP32_ON,
    BPK_REQ_ESP32_OFF,

    /* User defined Maple Requeusts */
};

#define BP_DEFAULT_DATA_VALUE 0

// These represent the addresses of each system in the project. Sending a bpacket
// requires you to supply an address so the reciever knows whether the bpacket
// is for them or if they are supposed to pass the bpacket on
enum bpk_address_e {
    BPK_ADDRESS_ESP32,
    BPK_ADDRESS_STM32,
    BPK_ADDRESS_MAPLE,
};

#define BPACKET_MAX_NUM_DATA_BYTES  255 // Chosen to be 255 as the max number that can fit into one byte is 255
#define BPACKET_NUM_NON_DATA_BYTES  7
#define BPACKET_BUFFER_LENGTH_BYTES (BPACKET_MAX_NUM_DATA_BYTES + BPACKET_NUM_NON_DATA_BYTES)

#define BPACKET_CIRCULAR_BUFFER_SIZE 10

// Bpacket data type ids
#define BPACKET_START_BYTE_UPPER_ID 0
#define BPACKET_START_BYTE_LOWER_ID 1
#define BPACKET_STOP_BYTE_UPPER_ID  2
#define BPACKET_STOP_BYTE_LOWER_ID  3
#define BPACKET_DATA_BYTE_ID        4
#define BPACKET_NUM_BYTES_BYTE_ID   5
#define BPACKET_CODE_BYTE_ID        6
#define BPACKET_REQUEST_BYTE_ID     7
#define BPACKET_SENDER_BYTE_ID      8
#define BPACKET_RECEIVER_BYTE_ID    9

enum bpk_error_code_e {
    /* Bpacket error codes */
    BPK_ERR_INVALID_SENDER = 2,
    BPK_ERR_INVALID_RECEIVER,
    BPK_ERR_INVLIAD_REQUEST,
    BPK_ERR_INVALID_CODE,
    BPK_ERR_INVALID_DATA,
    BPK_ERR_INVALD_START_BYTE,

    /* Project specific error codes */
    BPK_ERR_INVALID_START_TIME,
    BPK_ERR_INVALID_END_TIME,
    BPK_ERR_INVALID_INTERVAL_TIME,
    BPK_ERR_INVALID_DATE,
    BPK_ERR_INVALID_YEAR,
    BPK_ERR_INVALID_PACKET_SIZE,
    BPK_ERR_INVALID_CAMERA_RESOLUTION,
};

extern uint8_t bpkErrRecordTemp[1];
extern uint8_t bpkErrReadDatetime[1];

typedef struct bpk_error_code_t {
    uint8_t val; // Value to store the error code
} bpk_error_code_t;

typedef struct bpk_data_t {
    uint8_t numBytes;
    uint8_t bytes[BPACKET_MAX_NUM_DATA_BYTES];
} bpk_data_t;

typedef struct bpk_addr_send_t {
    uint8_t val;
} bpk_addr_send_t;

typedef struct bpk_addr_receive_t {
    uint8_t val;
} bpk_addr_receive_t;

typedef struct bpk_code_t {
    uint8_t val;
} bpk_code_t;

typedef struct bpk_request_t {
    uint8_t val;
} bpk_request_t;

typedef struct bpk_t {
    bpk_addr_receive_t Receiver;
    bpk_addr_send_t Sender;
    bpk_request_t Request;
    /**
     * @brief The code gives context to the request. If you receive a request
     * and the code is BPACKET_CODE_EXECUTE then the bpacket is asking for
     * the receiver to execute the request. If the code is BPACKET_CODE_ERROR
     * then the bpacket is a response stating the request it tried to exute
     * failed
     */
    bpk_code_t Code;
    bpk_data_t Data;
    bpk_error_code_t ErrorCode; // Code from 0 - 255 which holds the current status of the bpacket
} bpk_t;

typedef struct bpk_buffer_t {
    uint16_t numBytes; // Bpacket size needs to be a uint16_t because bpacket buffer > 255 bytes when put into a
                       // buffer
    uint8_t buffer[BPACKET_BUFFER_LENGTH_BYTES];
} bpk_buffer_t;

typedef struct bpacket_char_array_t {
    uint16_t numBytes;
    char string[BPACKET_MAX_NUM_DATA_BYTES + 1]; // One extra for null character
} bpacket_char_array_t;

typedef struct bpacket_circular_buffer_t {
    uint8_t* rIndex; // The index that the writing end of the buffer would use to know where to put the bpacket
    uint8_t* wIndex; // The index that the reading end of the buffer would use to see if they are up to date
                     // with the reading
    bpk_t* buffer[BPACKET_CIRCULAR_BUFFER_SIZE];
} bpacket_circular_buffer_t;

extern const bpk_addr_send_t BPK_Addr_Send_Stm32;
extern const bpk_addr_send_t BPK_Addr_Send_Esp32;
extern const bpk_addr_send_t BPK_Addr_Send_Maple;

extern const bpk_addr_receive_t BPK_Addr_Receive_Stm32;
extern const bpk_addr_receive_t BPK_Addr_Receive_Esp32;
extern const bpk_addr_receive_t BPK_Addr_Receive_Maple;

extern const bpk_code_t BPK_Code_Error;
extern const bpk_code_t BPK_Code_Success;
extern const bpk_code_t BPK_Code_In_Progress;
extern const bpk_code_t BPK_Code_Unknown;
extern const bpk_code_t BPK_Code_Execute;
extern const bpk_code_t BPK_Code_Todo;
extern const bpk_code_t BPK_Code_Debug;

extern const bpk_request_t BPK_Request_Help;
extern const bpk_request_t BPK_Request_Ping;
extern const bpk_request_t BPK_Request_Status;
extern const bpk_request_t BPK_Request_Message;

extern const bpk_request_t BPK_Req_List_Dir;
extern const bpk_request_t BPK_Req_Copy_File;
extern const bpk_request_t BPK_Req_Delete_File;
extern const bpk_request_t BPK_Req_Take_Photo;
extern const bpk_request_t BPK_Req_Write_To_File;
extern const bpk_request_t BPK_Req_Record_Data;
extern const bpk_request_t BPK_Req_Led_Red_On;
extern const bpk_request_t BPK_Req_Led_Red_Off;
extern const bpk_request_t BPK_Req_Camera_View;
extern const bpk_request_t BPK_Req_Get_Datetime;
extern const bpk_request_t BPK_Req_Set_Datetime;
extern const bpk_request_t BPK_Req_Get_Watchdog_Settings;
extern const bpk_request_t BPK_Req_Set_Watchdog_Settings;
extern const bpk_request_t BPK_Req_Get_Temperature;
extern const bpk_request_t BPK_Req_Stream_Images;
extern const bpk_request_t BPK_Req_Esp32_On;
extern const bpk_request_t BPK_Req_Esp32_Off;

extern const bpk_error_code_t BPK_Err_Invalid_Sender;
extern const bpk_error_code_t BPK_Err_Invalid_Receiver;
extern const bpk_error_code_t BPK_Err_Invalid_Request;
extern const bpk_error_code_t BPK_Err_Invalid_Code;
extern const bpk_error_code_t BPK_Err_Invalid_Data;
extern const bpk_error_code_t BPK_Err_Invalid_Start_Byte;

extern const bpk_error_code_t BPK_Err_Invalid_Start_Time;
extern const bpk_error_code_t BPK_Err_Invalid_End_Time;
extern const bpk_error_code_t BPK_Err_Invalid_Interval_Time;
extern const bpk_error_code_t BPK_Err_Invalid_Date;
extern const bpk_error_code_t BPK_Err_Invalid_Year;
extern const bpk_error_code_t BPK_Err_Invalid_Bpacket_Size;
extern const bpk_error_code_t BPK_Err_Invalid_Camera_Resolution;

void bpk_increment_circular_buffer_index(uint8_t* writeIndex);

void bpk_create_circular_buffer(bpacket_circular_buffer_t* bufferStruct, uint8_t* writeIndex, uint8_t* readIndex,
                                bpk_t* buffer);

uint8_t bpk_buffer_decode(bpk_t* Bpacket, uint8_t data[BPACKET_BUFFER_LENGTH_BYTES]);

uint8_t bpk_create(bpk_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender, bpk_request_t Request,
                   bpk_code_t Code, uint8_t numDataBytes, uint8_t* data);

uint8_t bp_create_string_packet(bpk_t* Bpacket, const bpk_addr_receive_t RAddress, const bpk_addr_send_t SAddress,
                                const bpk_request_t Request, const bpk_code_t Code, char* string);

uint8_t bpk_create_sp(bpk_t* Bpacket, bpk_addr_receive_t Receiver, bpk_addr_send_t Sender, bpk_request_t Request,
                      bpk_code_t Code, char* string);

void bpk_to_buffer(bpk_t* Bpacket, bpk_buffer_t* packetBuffer);

void bpk_data_to_string(bpk_t* Bpacket, bpacket_char_array_t* bpacketCharArray);

void bpk_get_info(bpk_t* Bpacket, char* string);

void bpk_increment_circ_buff_index(uint32_t* cbIndex, uint32_t bufferMaxIndex);

void bpk_convert_to_response(bpk_t* Bpacket, bpk_code_t Code, uint8_t numBytes, uint8_t* data);

uint8_t bpk_create_string_response(bpk_t* Bpacket, bpk_code_t Code, char* string);

/* Bpacket helper functions */

uint8_t bpk_send_data(void (*transmit_bpacket)(uint8_t* data, uint16_t bufferNumBytes), bpk_addr_receive_t Receiver,
                      bpk_addr_send_t Sender, bpk_request_t Request, uint8_t* data, uint32_t numBytesToSend);

void bpk_swap_address(bpk_t* Bpacket);

void bpk_set_sender_receiver(bpk_t* Bpacket, bpk_addr_send_t Sender, bpk_addr_receive_t Receiver);

void bpk_reset(bpk_t* Bpacket);

void bpk_create_response(bpk_t* Bpacket, bpk_code_t Code);

uint8_t bpk_set_receiver(bpk_t* Bpacket, uint8_t receiver);
uint8_t bpk_set_sender(bpk_t* Bpacket, uint8_t sender);
uint8_t bpk_set_request(bpk_t* Bpacket, uint8_t request);
uint8_t bpk_set_code(bpk_t* Bpacket, uint8_t code);

void bpk_utils_init_expected_byte_buffer(uint8_t byteBuffer[8]);

void bpk_packet_is_valid(bpk_t* Bpacket);

#endif // BPACKET_H