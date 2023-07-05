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
#define MAPLE_BUFFER_NUM_BYTES    (sizeof(bpk_t) * MAPLE_BUFFER_NUM_ELEMENTS)
cbuffer_t RxCbuffer;
cbuffer_t TxCbuffer;
uint8_t rxCbufferBytes[MAPLE_BUFFER_NUM_BYTES];
uint8_t txCbufferBytes[MAPLE_BUFFER_NUM_BYTES];

struct sp_port* gbl_activePort = NULL;

/* Private Variables */
bpk_t v_Bpk; // v_ stands for volatile
bpk_data_t v_Data;
cdt_u8_t v_u8;

/* Function Prototpes */
DWORD WINAPI ut_listen_rx(void* arg);
uint8_t ut_connect_to_device(bpk_addr_receive_t receiver, uint8_t pingCode);
void ut_print_data(uint8_t* data, uint8_t length);
void ut_run(void);
uint8_t ut_send_bpacket(bpk_t* Bpacket);
void ut_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                uint8_t numDataBytes, uint8_t* data);

int main(void) {

    // Initialise logging
    log_init(printf, ut_print_data);

    // Initialise rx and tx buffers
    cbuffer_init(&RxCbuffer, rxCbufferBytes, sizeof(bpk_t), MAPLE_BUFFER_NUM_ELEMENTS);
    cbuffer_init(&TxCbuffer, txCbufferBytes, sizeof(bpk_t), MAPLE_BUFFER_NUM_ELEMENTS);

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

    // log_message("Finished tests\r\n");
    while (1) {}
}

uint8_t ut_run_test(bpk_t* Bpacket, bpk_code_t ExpectedCode, bpk_data_t* Data, bpk_data_t* DataCopy,
                    uint32_t timeout) {

    // Send bpacket
    if (ut_send_bpacket(Bpacket) != TRUE) {
        LOG_ERROR();
        return FALSE;
    }

    // Check the response
    bpk_t BpkResponse;
    clock_t startTime = clock();
    while ((clock() - startTime) < timeout) {

        // Wait for next response
        while (cbuffer_read_next_element(&RxCbuffer, (void*)&BpkResponse) != TRUE) {
            // Check for timeout
            if ((clock() - startTime) > timeout) {
                log_error("Timeout\r\n");
                return FALSE;
            }
        }

        // Skip if the Bpacket is just a message
        if ((BpkResponse.Request.val == BPK_Request_Message.val) ||
            (BpkResponse.Code.val == BPK_Code_Debug.val)) {
            continue;
        }

        // Message received. Exit while loop
        break;
    }

    // Confirm the request received is the same request that was sent
    if (BpkResponse.Request.val != Bpacket->Request.val) {
        log_error("Request error. Expected %i but got %i", Bpacket->Request.val,
                  BpkResponse.Request.val);
        return FALSE;
    }

    // Confirm the code is the expected code
    if (BpkResponse.Code.val != ExpectedCode.val) {
        log_error("Code error. Expected %i but got %i", ExpectedCode.val, BpkResponse.Code.val);
        return FALSE;
    }

    if (Data != NULL) {

        if (BpkResponse.Data.numBytes != Data->numBytes) {
            log_error("Data error. Expected %i bytes but got %i\r\n", Data->numBytes,
                      BpkResponse.Data.numBytes);
            return FALSE;
        }

        // Confirm the data contains the expected data
        for (uint8_t i = 0; i < Data->numBytes; i++) {
            if (BpkResponse.Data.bytes[i] != Data->bytes[i]) {
                log_error("Data error index %i: Expected %i but got %i\r\n", i, Data->bytes[i],
                          BpkResponse.Data.bytes[i]);
                return FALSE;
            }
        }
    }

    // Copy data if requested
    if (DataCopy != NULL) {
        DataCopy->numBytes = BpkResponse.Data.numBytes;
        for (uint8_t i = 0; i < BpkResponse.Data.numBytes; i++) {
            DataCopy->bytes[i] = BpkResponse.Data.bytes[i];
        }
    }

    return TRUE;
}

uint8_t ut_response_is_valid(bpk_t* Bpacket, bpk_request_t ExpectedRequest, uint16_t timeout) {

    // Print response
    clock_t startTime = clock();
    while ((clock() - startTime) < timeout) {

        // Wait for next response
        while (cbuffer_read_next_element(&RxCbuffer, (void*)Bpacket) != TRUE) {
            // Check for timeout
            if ((clock() - startTime) > timeout) {
                return FALSE;
            }
        }

        // Skip if the Bpacket is just a message
        if ((Bpacket->Request.val == BPK_Request_Message.val) ||
            (Bpacket->Code.val == BPK_Code_Debug.val)) {
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

    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Led_Red_On,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 1000) != TRUE) {
        log_error("Failed to turn red LED on via STM32");
        return;
    }

    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Led_Red_Off,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 1000) != TRUE) {
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

    /* Turn the red LED on directly */
    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple, BPK_Req_Led_Red_On,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 1000) != TRUE) {
        log_error("Failed to turn red LED on via ESP32");
        return;
    }

    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple, BPK_Req_Led_Red_Off,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 1000) != TRUE) {
        log_error("Failed to turn red LED off via ESP32");
        return;
    }

    log_success("ESP32: Red LED tests passed\n");
}

/**
 * @brief This function tests updating the camera settings on the ESP32. The
 * tests are
 *      - Update camera settings with a valid setting
 *      - Read camera settings on the ESP32
 *      - Try update camera settings with an invalid setting
 */
void ut_test_esp32_camera_settings(void) {

    /****** START CODE BLOCK ******/
    // Description: Test updating the camera settings on the ESP32
    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple, BPK_Req_Set_Camera_Settings,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Add data to the bpacket
    cdt_u8_t NewCameraSettings = {.value = WD_CAM_RES_320x240};
    bpk_utils_write_cdt_u8(&v_Bpk, &NewCameraSettings, 1);

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 1000) != TRUE) {
        LOG_ERROR();
        return;
    }
    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Test reading the camera settings from the ESP32

    // Restart the ESP32 to ensure that the camera settings being read are the ones
    // saved on the ESP32
    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Esp32_Off,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 1000) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Delay for 1000ms
    Sleep(1000);

    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Esp32_On,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 2000) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Wait for 1 second to ensure ESP32 has started up
    Sleep(1000);

    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple, BPK_Req_Get_Camera_Settings,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    v_Data.numBytes = 1;
    v_Data.bytes[0] = NewCameraSettings.value;
    if (ut_run_test(&v_Bpk, BPK_Code_Success, &v_Data, NULL, 1000) != TRUE) {
        LOG_ERROR();
        return;
    }
    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Test setting invalid camera settings on the ESP32 returns
    // an error
    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple, BPK_Req_Set_Camera_Settings,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Write invalid camera settings to bpacket
    v_u8.value = 23;
    bpk_utils_write_cdt_u8(&v_Bpk, &v_u8, 1);

    // Expecting an error from the ESP32
    if (ut_run_test(&v_Bpk, BPK_Code_Error, NULL, NULL, 1000) != TRUE) {
        LOG_ERROR();
        return;
    }
    /****** END CODE BLOCK ******/

    log_success("ESP32: Updating camera settings tests passed\r\n");
}

/**
 * @brief This function tests the datetime on the STM32. This is done by
 * creating a datetime and sending this to the STM32 to set. The datetime
 * is then read from the STM32. The read datetime is then compared to the
 * sent datetime to ensure they match
 */
void ut_test_stm32_datetime(void) {

    /****** START CODE BLOCK ******/
    // Description: Test setting the datetime on the ESP32
    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Set_Datetime,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Add datetime to bpacket
    dt_datetime_t NewDatetime;
    if (dt_datetime_init(&NewDatetime, 30, 15, 8, 1, 2, 2023) != TRUE) {
        LOG_ERROR();
        return;
    }

    dt_datetime_t* datetimes[1];
    datetimes[0] = &NewDatetime;
    if (bpk_utils_write_datetimes(&v_Bpk, datetimes, 1) != TRUE) {
        LOG_ERROR();
        return;
    }

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 1000) != TRUE) {
        LOG_ERROR();
        return;
    }

    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Test reading the datetime from the STM32
    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Get_Datetime,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Set the expected datetime. Note this may lead to a false negative every now and then
    // if the time changes between setting the datetime above and reading the datetime here
    v_Data.bytes[0] = NewDatetime.Time.second;
    v_Data.bytes[1] = NewDatetime.Time.minute;
    v_Data.bytes[2] = NewDatetime.Time.hour;
    v_Data.bytes[3] = NewDatetime.Date.day;
    v_Data.bytes[4] = NewDatetime.Date.month;
    v_Data.bytes[5] = NewDatetime.Date.year >> 8;
    v_Data.bytes[6] = NewDatetime.Date.year & 0xFF;
    v_Data.numBytes = 7;

    if (ut_run_test(&v_Bpk, BPK_Code_Success, &v_Data, NULL, 1000) != TRUE) {
        LOG_ERROR();
        return;
    }

    /****** END CODE BLOCK ******/

    log_success("STM32 Datetime tests passed\n");
}

void ut_test_stm32_capture_time_settings(void) {

    /****** START CODE BLOCK ******/
    // Description: Write new capture time settings to the STM32

    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple,
                   BPK_Req_Set_Camera_Capture_Times, BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Populate bpacket with new capture time data
    dt_time_t Start    = {.second = 15, .minute = 0, .hour = 9};
    dt_time_t End      = {.second = 35, .minute = 45, .hour = 15};
    dt_time_t Interval = {.second = 0, .minute = 0, .hour = 1};

    dt_time_t* Times[3];
    Times[0] = &Start;
    Times[1] = &End;
    Times[2] = &Interval;

    if (bpk_utils_write_times(&v_Bpk, Times, 3) != TRUE) {
        LOG_ERROR();
        return;
    };

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 3000) != TRUE) {
        LOG_ERROR();
        return;
    }
    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Reads the capture time settings from he STM32 to ensure
    // they were written correctly
    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple,
                   BPK_Req_Get_Camera_Capture_Times, BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Create the expected data
    v_Data.bytes[0] = Start.second;
    v_Data.bytes[1] = Start.minute;
    v_Data.bytes[2] = Start.hour;
    v_Data.bytes[3] = End.second;
    v_Data.bytes[4] = End.minute;
    v_Data.bytes[5] = End.hour;
    v_Data.bytes[6] = Interval.second;
    v_Data.bytes[7] = Interval.minute;
    v_Data.bytes[8] = Interval.hour;
    v_Data.numBytes = 9;

    if (ut_run_test(&v_Bpk, BPK_Code_Success, &v_Data, NULL, 1000) != TRUE) {
        LOG_ERROR();
        return;
    }

    /****** END CODE BLOCK ******/

    log_success("STM32: Update Capture time settings passed\r\n");
}

void ut_test_stm32_take_photo(void) {

    /****** START CODE BLOCK ******/
    // Description: Tests taking a photo on the ESP32

    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Take_Photo,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Run test
    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 3000) != TRUE) {
        LOG_ERROR();
        return;
    }

    /****** END CODE BLOCK ******/

    log_success("STM32: Taking photo tests passed\r\n");
}

void ut_test_esp32_list_directory(void) {

    /****** START CODE BLOCK ******/
    // Description: Print out the current directory on the ESP32

    if (bp_create_string_packet(&v_Bpk, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple,
                                BPK_Req_List_Dir, BPK_Code_Execute, "/watchdog/data") != TRUE) {
        LOG_ERROR();
        return;
    }

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, &v_Data, 3000) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Print out the data
    ut_print_data(v_Data.bytes, v_Data.numBytes);

    /****** END CODE BLOCK ******/

    log_success("ESP32: Listing directory tests passed\r\n");
}

void ut_test_esp32_download_photo(void) {

    /****** START CODE BLOCK ******/
    // Description: Take a photo to ensure that at least one photo
    // has been taken on the ESP32. List the directory. Search for
    // the name of a valid image. Execute the request to copy that
    // image from the ESP32 to Maple
    if (bpk_create(&v_Bpk, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Take_Photo,
                   BPK_Code_Execute, 0, NULL) != TRUE) {
        LOG_ERROR();
        return;
    }

    // Run test
    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, NULL, 5000) != TRUE) {
        LOG_ERROR();
        return;
    }

    if (bp_create_string_packet(&v_Bpk, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple,
                                BPK_Req_List_Dir, BPK_Code_Execute, "/watchdog/data") != TRUE) {
        LOG_ERROR();
        return;
    }

    if (ut_run_test(&v_Bpk, BPK_Code_Success, NULL, &v_Data, 3000) != TRUE) {
        LOG_ERROR();
        return;
    }

    ut_print_data(v_Data.bytes, v_Data.numBytes);

    // Loop through directory and search for a valid image to download
    char imageName[100];
    uint8_t imageIndex = 0;
    for (uint8_t i = 0; i < v_Data.numBytes; i++) {

        imageName[imageIndex++] = v_Data.bytes[i];

        // Search for start of an image name
        if ((imageName[imageIndex - 3] == '.') && (imageName[imageIndex - 2] == 'j') &&
            (v_Data.bytes[i - 1] == 'p') && (v_Data.bytes[i] == 'g')) {
            imageName[imageIndex] = '\0';
            break;
        }

        if ((v_Data.bytes[i] == '\r') || (v_Data.bytes[i] == '\n')) {
            imageIndex = 0;
        }
    }

    log_success("Found image name: [%s]", imageName);

    /****** END CODE BLOCK ******/
}

void ut_run(void) {

    /* Test communication over UART */
    // ut_test_uart_communications();

    /* Test Communication with STM32 */
    ut_test_stm32_red_led();
    ut_test_stm32_datetime();
    ut_test_stm32_capture_time_settings();
    ut_test_stm32_take_photo();

    /* Test Communication with ESP32 */
    ut_test_esp32_red_led();
    ut_test_esp32_camera_settings();
    ut_test_esp32_list_directory();
    // ut_test_esp32_download_photo();
}

uint8_t ut_send_bpacket(bpk_t* Bpacket) {

    bpk_buffer_t packetBuffer;
    bpk_to_buffer(Bpacket, &packetBuffer);

    if (sp_blocking_write(gbl_activePort, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
        return FALSE;
    }

    return TRUE;
}

void ut_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                uint8_t numDataBytes, uint8_t* data) {
    bpk_t Bpacket;
    if (bpk_create(&Bpacket, Receiver, BPK_Addr_Send_Maple, Request, BPK_Code_Execute, numDataBytes,
                   data) != TRUE) {
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
        bpk_t Bpacket;
        bpk_buffer_t bpacketBuffer;
        bpk_create(&Bpacket, receiver, BPK_Addr_Send_Maple, BPK_Request_Ping, BPK_Code_Execute, 0,
                   NULL);
        bpk_to_buffer(&Bpacket, &bpacketBuffer);

        if (sp_blocking_write(gbl_activePort, bpacketBuffer.buffer, bpacketBuffer.numBytes, 100) <
            0) {
            printf("Unable to write\n");
            continue;
        }

        // Response may include other incoming messages as well, not just a response to a ping.
        // Create a timeout of 2 seconds and look for response from ping

        clock_t startTime = clock();
        bpk_t PingBpacket;
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
    bpk_t Bpacket = {0};
    uint8_t bi    = 0;

    while (gbl_activePort == NULL) {}

    while ((numBytes = sp_blocking_read(gbl_activePort, &byte, 1, 0)) >= 0) {

        // Read the current index of the byte buffer and store the result into
        // expected byte
        cbuffer_read_current_element(&ByteBuffer, (void*)&expectedByte);

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
                    cbuffer_append_element(&RxCbuffer, (void*)&Bpacket);

                    // log_success("Bpacket data received: %i %i %i %i %i\n", Bpacket.Receiver.val,
                    //             Bpacket.Sender.val, Bpacket.Request.val, Bpacket.Code.val,
                    //             Bpacket.Data.numBytes);
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
                cbuffer_append_element(&RxCbuffer, (void*)&Bpacket);

                // log_success("Bpacket no data received: %i %i %i %i %i\n", Bpacket.Receiver.val,
                //             Bpacket.Sender.val, Bpacket.Request.val, Bpacket.Code.val,
                //             Bpacket.Data.numBytes);

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