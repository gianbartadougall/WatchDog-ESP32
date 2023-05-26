/* C Library Includes */
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h> // Use for multi threading
#include <time.h>    // Use for ms timer

#include <wingdi.h>
#include <CommCtrl.h>

/* 3rd Party Library Includes for COM Port Interfacing */
#include <libserialport.h>

/* Personal Includes */
#include "chars.h"
#include "Bpacket.h"
#include "watchdog_defines.h"
#include "datetime.h"
#include "log.h"
#include "bpacket_utils.h"
#include "cbuffer.h"
#include "utils.h"

/* Private Macros */
#define LOG_BPACKET(Bpkacket)                                                                   \
    (log_error("Receiver: %i\tSender: %i\tRequest: %i\tResponse %i\r\n", Bpkacket.Receiver.val, \
               Bpkacket.Sender.val, Bpkacket.Request.val, Bpkacket.Code.val))

// Setting up Rx and Tx buffers
#define MAPLE_BUFFER_NUM_ELEMENTS 10
#define MAPLE_BUFFER_NUM_BYTES    (sizeof(bpk_packet_t) * MAPLE_BUFFER_NUM_ELEMENTS)
cbuffer_t RxCbuffer;
cbuffer_t TxCbuffer;
uint8_t rxCbufferBytes[MAPLE_BUFFER_NUM_BYTES];
uint8_t txCbufferBytes[MAPLE_BUFFER_NUM_BYTES];

struct sp_port* gbl_activePort = NULL;

/* Function Prototpes */
DWORD WINAPI ut_listen_rx(void* arg);
uint8_t ut_connect_to_device(bpk_addr_receive_t receiver, uint8_t pingCode);
void ut_print_data(uint8_t* data, uint8_t length);
void ut_run(void);
uint8_t ut_send_bpacket(bpk_packet_t* Bpacket);
void ut_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                uint8_t numDataBytes, uint8_t* data);

int main(void) {

    // Initialise logging
    log_init(printf, ut_print_data);

    // Initialise rx and tx buffers
    cbuffer_init(&RxCbuffer, rxCbufferBytes, sizeof(bpk_packet_t), MAPLE_BUFFER_NUM_ELEMENTS);
    cbuffer_init(&TxCbuffer, txCbufferBytes, sizeof(bpk_packet_t), MAPLE_BUFFER_NUM_ELEMENTS);

    HANDLE thread = CreateThread(NULL, 0, ut_listen_rx, NULL, 0, NULL);

    if (!thread) {
        log_error("Thread failed\n");
        return FALSE;
    }

    // Try connect to the device
    if (ut_connect_to_device(BPK_Addr_Receive_Stm32, WATCHDOG_PING_CODE_STM32) != TRUE) {
        log_error("Unable to connect to device. Check if the COM port is already in use\n");
        TerminateThread(thread, 0);
        return FALSE;
    }

    log_success("Connected to port %s\n", sp_get_port_name(gbl_activePort));

    ut_run();

    while (1) {}
}

uint8_t ut_response_is_valid(bpk_packet_t* Bpacket, bpk_request_t ExpectedRequest,
                             uint16_t timeout) {

    // Print response
    clock_t startTime = clock();
    while ((clock() - startTime) < timeout) {

        // Wait for next response
        while (cbuffer_read_next_element(&RxCbuffer, (void*)Bpacket) != TRUE) {}

        // Skip if the Bpacket is just a message
        if (Bpacket->Request.val == BPK_Request_Message.val) {
            continue;
        }

        // Confirm the received Bpacket matches the correct request and code
        if (Bpacket->Request.val != ExpectedRequest.val) {
            log_error("Error %s %i. Request was %i but expected %i. Code %i\tData %i\n", __FILE__,
                      __LINE__, Bpacket->Request.val, ExpectedRequest.val, Bpacket->Code.val,
                      Bpacket->Data.numBytes);
            return FALSE;
        }

        if (Bpacket->Code.val != BPK_Code_Success.val) {
            printf("Error %s %i. Expected code %i but got %i\n", __FILE__, __LINE__,
                   BPK_Code_Success.val, Bpacket->Code.val);
            return FALSE;
        }

        return TRUE;
    }

    printf("Failed to get response\n");
    return FALSE;
}

/**
 * @brief This function tests the LEDs on the ESP32. This is done by sending
 * bpackets to the STM32 requesting the STM32 to send bpackets to the ESP32
 * to turn on and off the LED.
 */
void ut_test_stm32_red_led(void) {

    bpk_packet_t Bpacket;

    /* Turn the red LED on */
    ut_create_and_send_bpacket(BPK_Req_Led_Red_On, BPK_Addr_Receive_Stm32, 0, NULL);
    if (ut_response_is_valid(&Bpacket, BPK_Req_Led_Red_On, 2000) == FALSE) {
        log_error("Failed to turn red LED on via STM32");
        return;
    }

    /* Turn the red LED off */
    ut_create_and_send_bpacket(BPK_Req_Led_Red_Off, BPK_Addr_Receive_Stm32, 0, NULL);
    if (ut_response_is_valid(&Bpacket, BPK_Req_Led_Red_Off, 2000) == FALSE) {
        log_error("Failed to turn red LED off via STM32");
        return;
    }

    log_success("STM32: Red LED tests passed\n");
}

/**
 * @brief This function tests the LEDs on the ESP32. This is done by sending
 * bpackets to the ESP32 to turn on and off the LED.
 */
void ut_test_esp32_red_led(void) {

    bpk_packet_t Bpacket;

    /* Turn the red LED on directly */
    ut_create_and_send_bpacket(BPK_Req_Led_Red_On, BPK_Addr_Receive_Esp32, 0, NULL);
    if (ut_response_is_valid(&Bpacket, BPK_Req_Led_Red_On, 2000) == FALSE) {
        log_error("Failed to turn red LED on via Maple");
        return;
    }

    /* Turn the red LED off directly */
    ut_create_and_send_bpacket(BPK_Req_Led_Red_Off, BPK_Addr_Receive_Esp32, 0, NULL);
    if (ut_response_is_valid(&Bpacket, BPK_Req_Led_Red_Off, 2000) == FALSE) {
        log_error("Failed to turn red LED off via Maple");
        return;
    }

    log_success("ESP32: Red LED tests passed\n");
}

void ut_test_esp32_camera_settings(void) {

    bpk_packet_t Bpacket, BpkResponse;

    // Create the bpacket
    if (bpk_create_packet(&Bpacket, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple,
                          BPK_Req_Set_Camera_Settings, BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Add the camera settings to the bpacket
    cdt_u8_t NewCameraSettings = {.value = WD_CAM_RES_320x240};
    bpk_utils_write_cdt_u8(&Bpacket, &NewCameraSettings, 1);

    // Send bpacket to the ESP32
    if (ut_send_bpacket(&Bpacket) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Confirm the message was received with code success
    if (ut_response_is_valid(&BpkResponse, BPK_Req_Set_Camera_Settings, 1000)) {
        LOG_ERROR();
        return;
    }

    // Read the camera settings from the ESP32
    ut_create_and_send_bpacket(BPK_Req_Get_Camera_Settings, BPK_Addr_Receive_Esp32, 0, NULL);

    // Get response from the ESP32
    bpk_reset(&BpkResponse);
    if (ut_response_is_valid(&BpkResponse, BPK_Req_Get_Camera_Settings, 1000)) {
        LOG_ERROR();
        return;
    }

    // Confirm the response contains the correct camera settings
    if (BpkResponse.Data.bytes[0] != NewCameraSettings.value) {
        LOG_ERROR();
        return;
    }

    log_success("ESP32: Updating camera settings tests passed");
}

/**
 * @brief This function tests the datetime on the STM32. This is done by
 * creating a datetime and sending this to the STM32 to set. The datetime
 * is then read from the STM32. The read datetime is then compared to the
 * sent datetime to ensure they match
 */
void ut_test_stm32_datetime(void) {

    bpk_packet_t BpkResponse;

    // Create a datetime for the STM32
    dt_datetime_t NewDatetime;
    dt_time_init(&NewDatetime.Time, 30, 15, 8);
    dt_date_init(&NewDatetime.Date, 1, 2, 2023);

    // Create Bpacket
    bpk_packet_t Bpacket;
    if (bpk_create_packet(&Bpacket, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple,
                          BPK_Req_Set_Datetime, BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Convert datetime to bpacket
    dt_datetime_t* datetimes[1];
    datetimes[0] = &NewDatetime;
    if (bpk_utils_write_datetimes(&Bpacket, datetimes, 1) != TRUE) {
        LOG_ERROR();
        return;
    }

    ut_send_bpacket(&Bpacket);

    // Check for success code
    if (ut_response_is_valid(&BpkResponse, BPK_Req_Set_Datetime, 1000) != TRUE) {
        LOG_ERROR();
        LOG_BPACKET(BpkResponse);
        return;
    }

    // Get the datetime from the STM32
    ut_create_and_send_bpacket(BPK_Req_Get_Datetime, BPK_Addr_Receive_Stm32, 0, NULL);
    if (ut_response_is_valid(&BpkResponse, BPK_Req_Get_Datetime, 1000) != TRUE) {
        LOG_ERROR();
        LOG_BPACKET(BpkResponse);
        return;
    }

    // Response was received. Confirm it contains expected values
    dt_datetime_t DatetimeReceived;
    dt_datetime_t* DatetimesReceived[1];
    DatetimesReceived[0] = &DatetimeReceived;
    if (bpk_utils_read_datetimes(&BpkResponse, DatetimesReceived) != 1) {
        LOG_ERROR();
        return;
    }

    // Confirm the set datetime and the received datetime are the same. Note not checking whether
    // the seconds are the same as this will lead to many false positives
    if (NewDatetime.Time.minute != DatetimeReceived.Time.minute) {
        LOG_ERROR();
        return;
    }

    if (NewDatetime.Time.hour != DatetimeReceived.Time.hour) {
        LOG_ERROR();
        return;
    }

    if (NewDatetime.Date.day != DatetimeReceived.Date.day) {
        LOG_ERROR();
        return;
    }

    if (NewDatetime.Date.month != DatetimeReceived.Date.month) {
        LOG_ERROR();
        return;
    }

    if (NewDatetime.Date.year != DatetimeReceived.Date.year) {
        LOG_ERROR();
        return;
    }

    log_success("STM32 Datetime tests passed\n");
}

void ut_run(void) {

    /* Test Communication with STM32 */
    ut_test_stm32_red_led();
    ut_test_stm32_datetime();

    /* Test Communication with ESP32 */
    ut_test_esp32_red_led();
    ut_test_esp32_camera_settings();
}

uint8_t ut_send_bpacket(bpk_packet_t* Bpacket) {

    bpk_buffer_t packetBuffer;
    bpacket_to_buffer(Bpacket, &packetBuffer);

    if (sp_blocking_write(gbl_activePort, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
        return FALSE;
    }

    return TRUE;
}

void ut_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                uint8_t numDataBytes, uint8_t* data) {
    bpk_packet_t Bpacket;
    if (bpk_create_packet(&Bpacket, Receiver, BPK_Addr_Send_Maple, Request, BPK_Code_Execute,
                          numDataBytes, data) != TRUE) {
        log_error("Error %s %i. Code %i", __FILE__, __LINE__, Bpacket.ErrorCode.val);
        return;
    }

    if (ut_send_bpacket(&Bpacket) != TRUE) {
        log_error("Failed to send bpacket\n");
        return;
    }
}

uint8_t ut_connect_to_device(bpk_addr_receive_t receiver, uint8_t pingCode) {

    // Create a struct to hold all the COM ports currently in use
    struct sp_port** port_list;

    if (sp_list_ports(&port_list) != SP_OK) {
        log_error("Failed to list ports");
        return 0;
    }

    // Loop through all the ports
    uint8_t portFound = FALSE;

    for (int i = 0; port_list[i] != NULL; i++) {

        gbl_activePort = port_list[i];

        // Open the port
        enum sp_return result = sp_open(gbl_activePort, SP_MODE_READ_WRITE);
        if (result != SP_OK) {
            return result;
        }

        // Configure the port settings for communication
        sp_set_baudrate(gbl_activePort, 115200);
        sp_set_bits(gbl_activePort, 8);
        sp_set_parity(gbl_activePort, SP_PARITY_NONE);
        sp_set_stopbits(gbl_activePort, 1);
        sp_set_flowcontrol(gbl_activePort, SP_FLOWCONTROL_NONE);

        // Send a ping
        bpk_packet_t Bpacket;
        bpk_buffer_t bpacketBuffer;
        bpk_create_packet(&Bpacket, receiver, BPK_Addr_Send_Maple, BPK_Request_Ping,
                          BPK_Code_Execute, 0, NULL);
        bpacket_to_buffer(&Bpacket, &bpacketBuffer);

        if (sp_blocking_write(gbl_activePort, bpacketBuffer.buffer, bpacketBuffer.numBytes, 100) <
            0) {
            printf("Unable to write\n");
            continue;
        }

        // Response may include other incoming messages as well, not just a response to a ping.
        // Create a timeout of 2 seconds and look for response from ping

        clock_t startTime = clock();
        bpk_packet_t PingBpacket;
        while ((clock() - startTime) < 200) {

            // Wait for Bpacket to be received
            if (cbuffer_read_next_element(&RxCbuffer, (void*)&PingBpacket) == TRUE) {

                // Check if the bpacket matches the ping code
                if (PingBpacket.Request.val != BPK_REQUEST_PING) {
                    continue;
                }

                if (PingBpacket.Data.bytes[0] != pingCode) {
                    continue;
                }

                portFound = TRUE;
                break;
            }
        }

        // Close port and check next port if this was not the correct port
        if (portFound != TRUE) {
            sp_close(gbl_activePort);
            continue;
        }

        break;
    }

    if (portFound != TRUE) {
        return FALSE;
    }

    return TRUE;
}

DWORD WINAPI ut_listen_rx(void* arg) {

    uint8_t byte, expectedByte, numBytes;
    cbuffer_t ByteBuffer;
    uint8_t byteBuffer[8];
    bpk_utils_init_expected_byte_buffer(byteBuffer);
    cbuffer_init(&ByteBuffer, (void*)byteBuffer, sizeof(uint8_t), 8);
    bpk_packet_t Bpacket = {0};
    uint8_t bi           = 0;

    while (gbl_activePort == NULL) {}

    while ((numBytes = sp_blocking_read(gbl_activePort, &byte, 1, 0)) >= 0) {

        // Read the current index of the byte buffer and store the result into
        // expected byte
        cbuffer_read_current_byte(&ByteBuffer, (void*)&expectedByte);

        switch (expectedByte) {

            /* Expecting bpacket 'data' bytes */
            case BPK_BYTE_DATA:

                // Add byte to bpacket
                Bpacket.Data.bytes[bi++] = byte;

                // Skip resetting if there are more date bytes to read
                if (bi < Bpacket.Data.numBytes) {
                    continue;
                }

                // No more data bytes to read. Execute request and then exit switch statement
                // to reset
                if (Bpacket.Request.val == BPK_REQUEST_MESSAGE) {
                    switch (Bpacket.Code.val) {
                        case BPK_CODE_ERROR:
                            printf(ASCII_COLOR_RED);
                            break;

                        case BPK_CODE_SUCCESS:
                            printf(ASCII_COLOR_GREEN);
                            break;

                        case BPK_CODE_DEBUG:
                            printf(ASCII_COLOR_MAGENTA);
                            break;

                        default:
                            printf(ASCII_COLOR_BLUE);
                    }
                    ut_print_data(Bpacket.Data.bytes, Bpacket.Data.numBytes);
                    printf(ASCII_COLOR_WHITE);
                } else {

                    // Write bpacket to circular buffer
                    cbuffer_write_element(&RxCbuffer, (void*)&Bpacket);

                    log_success("Bpacket data received: %i %i %i %i %i\n", Bpacket.Receiver.val,
                                Bpacket.Sender.val, Bpacket.Request.val, Bpacket.Code.val,
                                Bpacket.Data.numBytes);
                }

                break;

            /* Expecting bpacket 'length' byte */
            case BPK_BYTE_LENGTH:

                // Store the length and updated the expected byte
                Bpacket.Data.numBytes = byte;
                cbuffer_increment_read_index(&ByteBuffer);

                // Skip reseting if there is data to be read
                if (Bpacket.Data.numBytes > 0) {
                    continue;
                }

                // Write bpacket to circular buffer
                cbuffer_write_element(&RxCbuffer, (void*)&Bpacket);

                log_success("Bpacket no data received: %i %i %i %i %i\n", Bpacket.Receiver.val,
                            Bpacket.Sender.val, Bpacket.Request.val, Bpacket.Code.val,
                            Bpacket.Data.numBytes);

                break;

            /* Expecting another bpacket byte that is not 'data' or 'length' */
            default:

                // If decoding the byte succeeded, move to the next expected byte by
                // incrementing the circrular buffer holding the expected bytes
                if (bpk_utils_decode_non_data_byte(&Bpacket, expectedByte, byte) == TRUE) {
                    cbuffer_increment_read_index(&ByteBuffer);
                    continue;
                }

                // Decoding failed, print error
                log_error("BPK Read failed. %i %i (%c)\n", expectedByte, byte, byte);
        }

        // Reset the expected byte back to the start and bpacket buffer index
        cbuffer_reset_read_index(&ByteBuffer);
        bi = 0;
    }

    log_error("Connection terminated\n");
}

void ut_print_data(uint8_t* data, uint8_t length) {
    for (int i = 0; i < length; i++) {
        printf("%c", data[i]);
    }
    printf("\r\n");
}