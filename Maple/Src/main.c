/**
 * @file main.c
 * @author Gian Barta-Dougall
 * @brief
 *
 * @note #define LIBSERIALPORT_MSBUILD must be specified in libserialport_internal.h
 * file for this to compile. There would be a way to compile everything doing this
 * differently but I have not found that out yet
 *
 * @version 0.1
 * @date 2023-01-16
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <string.h>
#include <stdio.h>
#include <windows.h> // Use for multi threading
#include <wingdi.h>
#include <CommCtrl.h>
#include <time.h> // Use for ms timer
#include <sys/stat.h>

/* C Library Includes for COM Port Interfacing */
#include <libserialport.h>

/* Personal Includes */
#include "chars.h"
#include "watchdog_defines.h"
#include "Bpacket.h"
#include "gui.h"
#include "gui_utils.h"
#include "datetime.h"
#include "log.h"
#include "bpacket_utils.h"
#include "cbuffer.h"
#include "event_timeout.h"
#include "event_group.h"
#include "watchdog_utils.h"
#include "event_scheduler.h"
#include "float.h"
#include "integer.h"

#define WATCHDOG_DIRECTORY   "Watchdog"
#define LOG_ERROR_CODE(code) (log_error("Error %s line %i. Code %i\r\n", __FILE__, __LINE__, code))
#define LOG_ERROR_MSG(msg)   (log_message("Error %s line %i: \'%s\'\r\n", __FILE__, __LINE__, msg))

char* minutes[50]      = {"00", "05", "10", "15", "20", "25", "30", "35", "40", "45", "50", "55"};
char* hours[50]        = {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11",
                   "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23"};
char* resolutions[50]  = {"Very Low", "Low", "Medium", "High", "Very High"};
char* compressions[50] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};

#define PACKET_BUFFER_SIZE 50

#define ET_TIMEOUT_SET_DATETIME    ET_TIMEOUT_ID_0
#define ET_TIMEOUT_GET_WD_SETTINGS ET_TIMEOUT_ID_1
#define ET_TIMEOUT_SET_WD_SETTINGS ET_TIMEOUT_ID_2
#define ET_TIMEOUT_GET_TEMPERATURE ET_TIMEOUT_ID_3
#define ET_TIMEOUT_TAKE_PHOTO      ET_TIMEOUT_ID_4
#define ET_TIMEOUT_LIST_DIR        ET_TIMEOUT_ID_5

/* Private Variables */
cbuffer_t RxCbuffer;
cbuffer_t TxCbuffer;
uint8_t lg_WatchdogConnected = FALSE;
uint8_t lg_GuiClosed         = FALSE;
HWND GuiHandle;

et_timeout_t lg_MapleTimeouts;
es_scheduler_t lg_MapleScheduler;

#define MAPLE_BUFFER_NUM_ELEMENTS 10
#define MAPLE_BUFFER_NUM_BYTES    (sizeof(bpk_t) * MAPLE_BUFFER_NUM_ELEMENTS)
uint8_t rxCbufferBytes[MAPLE_BUFFER_NUM_BYTES];
uint8_t txCbufferBytes[MAPLE_BUFFER_NUM_BYTES];

event_group_t lg_EventsMaple;

HWND lbl_WatchdogDateTime;
HWND lbl_TitleWatchdogDateTime;

HWND lbl_startTime;
HWND db_startTimeMinutes;
HWND lbl_startTimeMinutes;
HWND db_startTimeHours;
HWND lbl_startTimeHours;

HWND lbl_endTime;
HWND db_endTimeMinutes;
HWND lbl_endTimeMinutes;
HWND db_endTimeHours;
HWND lbl_endTimeHours;

HWND lbl_intervalTime;
HWND db_intervalTimeMinutes;
HWND lbl_intervalTimeMinutes;
HWND db_intervalTimeHours;
HWND lbl_intervalTimeHours;

HWND lbl_camResolution;
HWND db_camResolution;
HWND lbl_camQuality;
HWND db_camQuality;

HWND btn_SaveSettings;

HWND lbl_watchdogConnection;

HWND lbl_watchdogTemperature;

HWND btn_takePhoto;

HWND db_images;
HWND btn_listDir;
HWND btn_deleteFile;
HWND btn_copyFile;

camera_settings_t lg_CameraSettings;
capture_time_t lg_CaptureTime;

char lg_sdCardData[50][50];
char* lg_selectedFile = NULL;
uint8_t lg_imageNum   = 0;
uint16_t wdDirIndex   = 0;
FILE* file            = NULL;

uint8_t* lg_fileData      = NULL;
uint32_t lg_fileDataIndex = 0;

#define DD_START_MINUTE_ID    20
#define DD_START_HOUR_ID      21
#define DD_END_MINUTE_ID      22
#define DD_END_HOUR_ID        23
#define DD_INTERVAL_MINUTE_ID 24
#define DD_INTERVAL_HOUR_ID   25
#define DD_CAMERA_RESOLUTION  26
#define BTN_SAVE_SETTINGS_ID  27
#define BTN_TAKE_PHOTO_ID     28
#define DD_CAMERA_COMPRESSION 29
#define BTN_LIST_DIR_ID       30
#define DD_IMAGE_DATA_ID      31
#define BTN_COPY_FILE_ID      32
#define BTN_DELETE_FILE_ID    33
#define DD_FILE_SELECTOR      34

enum maple_event_e {
    EVENT_WRITE_WATCHDOG_SETTINGS = EGB_0,
    EVENT_READ_WATCHDOG_SETTINGS  = EGB_1,
    EVENT_GET_TEMPERATURE         = EGB_2,
    EVENT_TAKE_PHOTO              = EGB_3,
    EVENT_LIST_DIR                = EGB_4,
    EVENT_COPY_FILE               = EGB_5,
    EVENT_DELETE_FILE             = EGB_6,
};

/* Function Prototypes */
void maple_print_bpacket_data(bpk_t* Bpacket);
void maple_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                   uint8_t numDataBytes, uint8_t* data);
void maple_print_uart_response(void);
void maple_test(void);
uint8_t maple_gui_init(void);
void maple_handle_watchdog_response(bpk_t* Bpacket);
LRESULT CALLBACK eventHandler(HWND GuiHandle, UINT msg, WPARAM wParam, LPARAM lParam);
void maple_populate_gui(HWND hwnd);
void maple_handle_events(void);
void maple_handle_timeouts(void);
void maple_handle_scheduler(void);

uint32_t packetBufferIndex  = 0;
uint32_t packetPendingIndex = 0;
bpk_t packetBuffer[PACKET_BUFFER_SIZE];

struct sp_port* gbl_activePort = NULL;

void print_data(uint8_t* data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        printf("%c", data[i]);
    }
    printf("\r\n");
}

uint8_t maple_send_bpacket(bpk_t* Bpacket) {

    bpk_buffer_t packetBuffer;
    bpk_to_buffer(Bpacket, &packetBuffer);

    if (sp_blocking_write(gbl_activePort, packetBuffer.buffer, packetBuffer.numBytes, 100) < 0) {
        return FALSE;
    }

    return TRUE;
}

void maple_create_and_send_bpacket(const bpk_request_t Request, const bpk_addr_receive_t Receiver,
                                   uint8_t numDataBytes, uint8_t* data) {
    bpk_t Bpacket;
    if (bpk_create(&Bpacket, Receiver, BPK_Addr_Send_Maple, Request, BPK_Code_Execute, numDataBytes,
                   data) != TRUE) {
        log_error("Error %s %i. Code %i", __FILE__, __LINE__, Bpacket.ErrorCode.val);
        return;
    }

    if (maple_send_bpacket(&Bpacket) != TRUE) {
        log_error("Failed to send bpacket\n");
        return;
    }
}

void maple_print_bpacket_data(bpk_t* Bpacket) {

    if (Bpacket->Request.val != BPK_Request_Message.val) {
        log_message("Request: %i Len: %i Data: ", Bpacket->Request.val, Bpacket->Data.numBytes);
    }

    for (int i = 0; i < Bpacket->Data.numBytes; i++) {
        log_message("%c", Bpacket->Data.bytes[i]);
    }
}

int maple_read_port(void* buf, size_t count, unsigned int timeout_ms) {

    if (gbl_activePort == NULL) {
        return 0;
    }

    return sp_blocking_read(gbl_activePort, buf, count, timeout_ms);
}

DWORD WINAPI maple_listen_rx(void* arg) {

    uint8_t byte, expectedByte;
    int numBytes;
    cbuffer_t ByteBuffer;
    uint8_t byteBuffer[8];
    bpk_utils_init_expected_byte_buffer(byteBuffer);
    cbuffer_init(&ByteBuffer, (void*)byteBuffer, sizeof(uint8_t), 8);
    bpk_t Bpacket = {0};
    uint8_t bi    = 0;

    while ((numBytes = maple_read_port(&byte, 1, 0)) >= 0) {

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
                    print_data(Bpacket.Data.bytes, Bpacket.Data.numBytes);
                    printf(ASCII_COLOR_WHITE);
                } else {

                    // Write bpacket to circular buffer
                    cbuffer_append_element(&RxCbuffer, (void*)&Bpacket);
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
                // log_error("BPK Read failed. %i %i. Num bytes %i\n", expectedByte, byte,
                // numBytes);
        }

        // Reset the expected byte back to the start and bpacket buffer index
        cbuffer_reset_read_index(&ByteBuffer);
        bi = 0;
    }

    log_error("Connection terminated with code %i\n", numBytes);

    lg_WatchdogConnected = FALSE;

    return FALSE;
}

uint8_t maple_connect_to_device(bpk_addr_receive_t receiver, uint8_t pingCode) {

    // Create a struct to hold all the COM ports currently in use
    struct sp_port** port_list;

    if (sp_list_ports(&port_list) != SP_OK) {
        LOG_ERROR_MSG("Failed to list ports");
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

        // Creating thread inside this function because the thread needs to be active after a port
        // is open but before it closes. Anytime you try listen when a port isn't open the sp read
        // function will return error code and so the thread will exit
        HANDLE thread = CreateThread(NULL, 0, maple_listen_rx, NULL, 0, NULL);

        if (!thread) {
            log_error("Thread failed\n");
            return 0;
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

                if (PingBpacket.Data.bytes[0] != WATCHDOG_PING_CODE_STM32) {
                    continue;
                }

                portFound = TRUE;
                break;
            }
        }

        // Close port and check next port if this was not the correct port
        if (portFound != TRUE) {

            // Terminate the thread
            TerminateThread(thread, FALSE);

            // Close the port
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

DWORD WINAPI maple_start(void* args) {

    bpk_t BpkWatchdogResponse;
    cbuffer_init(&RxCbuffer, rxCbufferBytes, sizeof(bpk_t), MAPLE_BUFFER_NUM_ELEMENTS);

    et_clear_all_timeouts(&lg_MapleTimeouts);

    event_group_clear(&lg_EventsMaple);
    char statusText[50];

    es_add_task(&lg_MapleScheduler, EVENT_GET_TEMPERATURE, 1000);

    // Main loop
    while (lg_GuiClosed == FALSE) {

        /* Maple functionality */
        if (lg_WatchdogConnected != TRUE) {

            sprintf(statusText, "Not connected");
            SetWindowText(lbl_watchdogConnection, statusText);

            if (maple_connect_to_device(BPK_Addr_Receive_Stm32, WATCHDOG_PING_CODE_STM32) != TRUE) {

                // TODO: Delay 500ms

                // TODO: Display error on GUI

                continue;
            }

            lg_WatchdogConnected = TRUE;

            // Once connected, we want to read the current settings of the Watchdog
            event_group_set_bit(&lg_EventsMaple, EVENT_READ_WATCHDOG_SETTINGS, EGT_ACTIVE);

            char* portName = sp_get_port_name(gbl_activePort);
            sprintf(statusText, "Connected (%s)", portName);
            free(portName);
            SetWindowText(lbl_watchdogConnection, statusText);
        }

        // Process any bpackets received from a Watchdog
        if (cbuffer_read_next_element(&RxCbuffer, &BpkWatchdogResponse) == TRUE) {
            maple_handle_watchdog_response(&BpkWatchdogResponse);
        }

        /* Handle Events */
        maple_handle_events();

        maple_handle_scheduler();

        /* Handle timeouts */
        for (uint8_t i = 0; i < ET_NUM_TIMEOUTS; i++) {
            if (et_timeout_has_occured(&lg_MapleTimeouts, i) == TRUE) {
                et_clear_timeout(&lg_MapleTimeouts, i);
                log_error("Timeout occured [id = %i]\r\n", i);
            }
        }
    }

    return FALSE;
}

void maple_handle_scheduler(void) {

    if (es_task_needs_to_run(&lg_MapleScheduler, EVENT_GET_TEMPERATURE) == TRUE) {
        es_clear_task(&lg_MapleScheduler, EVENT_GET_TEMPERATURE);

        event_group_set_bit(&lg_EventsMaple, EVENT_GET_TEMPERATURE, EGT_ACTIVE);
    }
}

void maple_handle_events(void) {

    bpk_t BpkMapleRequest;

    if (event_group_poll_bit(&lg_EventsMaple, EVENT_TAKE_PHOTO, EGT_ACTIVE) == TRUE) {
        event_group_clear_bit(&lg_EventsMaple, EVENT_TAKE_PHOTO, EGT_ACTIVE);
        log_message("Maple trying to take a photo\r\n");
        /* Request watchdog to take a photo */
        bpk_t BpkTakePhoto;
        bpk_create(&BpkTakePhoto, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_Take_Photo,
                   BPK_Code_Execute, 0, NULL);
        maple_send_bpacket(&BpkTakePhoto);

        // Set timeout
        et_set_timeout(&lg_MapleTimeouts, ET_TIMEOUT_TAKE_PHOTO, 10000);
    }

    /* If the Save settings button is pressed, this event group bit will be set */
    if (event_group_poll_bit(&lg_EventsMaple, EVENT_WRITE_WATCHDOG_SETTINGS, EGT_ACTIVE) == TRUE) {
        event_group_clear_bit(&lg_EventsMaple, EVENT_WRITE_WATCHDOG_SETTINGS, EGT_ACTIVE);

        /* Store watchdog settings into a bpacket and send to the watchdog */

        // The STM currently supports start dates and end dates as well but for the moment we
        // will just make those dates always valid
        uint8_t settingsData[20];
        wd_utils_settings_to_array(settingsData, &lg_CaptureTime, &lg_CameraSettings);

        /* Update settings on watchdog */
        bpk_t BpkWdSettings;
        bpk_create(&BpkWdSettings, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple,
                   BPK_Req_Set_Watchdog_Settings, BPK_Code_Execute, 20, settingsData);
        maple_send_bpacket(&BpkWdSettings);

        // Start timeout for response
        et_set_timeout(&lg_MapleTimeouts, ET_TIMEOUT_SET_WD_SETTINGS, 1000);
    }

    if (event_group_poll_bit(&lg_EventsMaple, EVENT_LIST_DIR, EGT_ACTIVE) == TRUE) {
        event_group_clear_bit(&lg_EventsMaple, EVENT_LIST_DIR, EGT_ACTIVE);

        bpk_create(&BpkMapleRequest, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple, BPK_Req_List_Dir,
                   BPK_Code_Execute, 0, NULL);
        maple_send_bpacket(&BpkMapleRequest);

        // Set timeout
        et_set_timeout(&lg_MapleTimeouts, ET_TIMEOUT_LIST_DIR, 4000);
        return;
    }

    if (event_group_poll_bit(&lg_EventsMaple, EVENT_COPY_FILE, EGT_ACTIVE) == TRUE) {
        event_group_clear_bit(&lg_EventsMaple, EVENT_COPY_FILE, EGT_ACTIVE);

        // Confirm that a file has been selected
        if (lg_selectedFile == NULL) {
            log_warning("No file has been selected\r\n");
        } else {
            log_message("Copying file %s\r\n", lg_selectedFile);

            bpk_create_sp(&BpkMapleRequest, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple,
                          BPK_Req_Copy_File, BPK_Code_Execute, lg_selectedFile);
            maple_send_bpacket(&BpkMapleRequest);
        }
    }

    if (event_group_poll_bit(&lg_EventsMaple, EVENT_DELETE_FILE, EGT_ACTIVE) == TRUE) {
        event_group_clear_bit(&lg_EventsMaple, EVENT_DELETE_FILE, EGT_ACTIVE);

        // Confirm that a file has been selected
        if (lg_selectedFile == NULL) {
            log_warning("No file has been selected\r\n");
        } else {
            log_message("Deleting file %s\r\n", lg_selectedFile);
        }
    }

    if (event_group_poll_bit(&lg_EventsMaple, EVENT_READ_WATCHDOG_SETTINGS, EGT_ACTIVE) == TRUE) {
        event_group_clear_bit(&lg_EventsMaple, EVENT_READ_WATCHDOG_SETTINGS, EGT_ACTIVE);

        /* Create bpacket to request watchdog settings */
        bpk_t BpkReadSettings;
        bpk_create(&BpkReadSettings, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple,
                   BPK_Req_Get_Watchdog_Settings, BPK_Code_Execute, 0, NULL);
        maple_send_bpacket(&BpkReadSettings);

        // Create timeout
        et_set_timeout(&lg_MapleTimeouts, ET_TIMEOUT_GET_WD_SETTINGS, 2000);
    }

    if (event_group_poll_bit(&lg_EventsMaple, EVENT_GET_TEMPERATURE, EGT_ACTIVE) == TRUE) {
        event_group_clear_bit(&lg_EventsMaple, EVENT_GET_TEMPERATURE, EGT_ACTIVE);

        /* Request temperature from watchdog */
        bpk_t BpkGetTemp;
        bpk_create(&BpkGetTemp, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple,
                   BPK_Req_Get_Temperature, BPK_Code_Execute, 0, NULL);
        maple_send_bpacket(&BpkGetTemp);

        // Set timeout.
        et_set_timeout(&lg_MapleTimeouts, ET_TIMEOUT_GET_TEMPERATURE, 3000);
    }
}

void maple_write_file(char* fileName, uint8_t* data, uint32_t numBytes) {
    FILE* file = fopen(fileName, "w");
    if (file == NULL) {
        printf("Error opening file %s!\n", fileName);
        return;
    }

    for (uint32_t i = 0; i < numBytes; i++) {
        fputc(data[i], file);
    }

    fclose(file);
}

void maple_handle_watchdog_response(bpk_t* Bpacket) {

    if (Bpacket->Request.val == BPK_REQ_TAKE_PHOTO) {

        // Clear the timeout and confirm success
        et_clear_timeout(&lg_MapleTimeouts, ET_TIMEOUT_TAKE_PHOTO);

        if (Bpacket->Code.val != BPK_CODE_SUCCESS) {
            log_error("Failed to take photo on watchdog\r\n");
            return;
        }

        log_success("Watchdog took a photo!\r\n");
    }

    if (Bpacket->Request.val == BPK_REQ_COPY_FILE) {

        if (file == NULL) {
            file = fopen(lg_selectedFile, "wb"); // Write binary
        }

        for (int i = 0; i < Bpacket->Data.numBytes; i++) {
            fputc(Bpacket->Data.bytes[i], file);
        }

        /****** START CODE BLOCK ******/
        // Description: Debugging
        printf("255 bytes written\r\n");
        /****** END CODE BLOCK ******/

        // Check if the end of the file has been sent
        if (Bpacket->Code.val == BPK_CODE_SUCCESS) {
            log_success("Finished copying file\r\n");
            fclose(file);
            file = NULL;
        }

        return;
    }

    if (Bpacket->Request.val == BPK_REQ_LIST_DIR) {

        if (Bpacket->Code.val == BPK_CODE_ERROR) {
            log_error("Failed to list the SD card directory");
            wdDirIndex = 0;
            return;
        }

        /* Bpacket contains direcotry data. Add it to the list */
        for (int i = 0; i < Bpacket->Data.numBytes; i++) {
            if (Bpacket->Data.bytes[i] == ':') {
                lg_imageNum++;
                wdDirIndex = 0;
                continue;
            }
            lg_sdCardData[lg_imageNum][wdDirIndex++] = (char)Bpacket->Data.bytes[i];
        }

        /* Bpacket was either part of the directory or contains the last section of the
            directory to be sent across. If its the last section than the code will be
            success. If this is the case then print out the directory */
        if (Bpacket->Code.val == BPK_CODE_SUCCESS) {
            // Clear the timeout
            et_clear_timeout(&lg_MapleTimeouts, ET_TIMEOUT_LIST_DIR);

            /* Update the drop down with the new image data */
            SendMessage(db_images, CB_RESETCONTENT, 0, 0);
            for (int i = 0; i < lg_imageNum; i++) {
                SendMessage(db_images, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)lg_sdCardData[i]);
            }

            // Set the drop down to select the first image in the list
            SendMessage(db_images, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
            lg_selectedFile = lg_sdCardData[0];

            // Reset the index
            lg_imageNum = 0;
            wdDirIndex  = 0;
        }

        return;
    }

    if (Bpacket->Request.val == BPK_REQ_GET_DATETIME) {

        if (Bpacket->Code.val != BPK_CODE_SUCCESS) {
            log_error("Failed to get datetime from the watchdog\r\n");
            return;
        }

        // Extract datetime and update the label
        dt_datetime_t WatchdogDt;
        if (dt_datetime_init(&WatchdogDt, Bpacket->Data.bytes[0], Bpacket->Data.bytes[1],
                             Bpacket->Data.bytes[2], Bpacket->Data.bytes[3], Bpacket->Data.bytes[4],
                             (((uint16_t)Bpacket->Data.bytes[5]) << 8) | Bpacket->Data.bytes[6]) !=
            TRUE) {
            log_error("Watchdog has invalid datetime\r\n");
        }

        /* Update the label on the GUI with the datetime of the watchdog */
        char datetimeText[30];
        sprintf(datetimeText, "%i:%i:%i %i/%i/%i", WatchdogDt.Time.second, WatchdogDt.Time.minute,
                WatchdogDt.Time.hour, WatchdogDt.Date.day, WatchdogDt.Date.month,
                WatchdogDt.Date.year);
        SetWindowText(lbl_WatchdogDateTime, datetimeText);

        /* Get the current datetime on the computer */
        time_t t            = time(NULL);
        struct tm localTime = *localtime(&t);
        dt_datetime_t ComputerDt;
        if (dt_datetime_init(&ComputerDt, localTime.tm_sec, localTime.tm_min, localTime.tm_hour,
                             localTime.tm_mday, localTime.tm_mon,
                             localTime.tm_year + 1900) != TRUE) {
            log_error("Computer has invalid datetime\r\n");
            return;
        }

        /* Update the datetime on the watchdog if the time is incorrect */
        if (dt_datetimes_are_equal(&ComputerDt, &WatchdogDt) != TRUE) {

            /* Just commented this stuff out for the moment to reduce print statements in debug
             * window. */

            // log_warning("Watchdog datetime incorrect. Watchdog Datetime: %i:%i:%i %i/%i/%i. "
            //             "Computer datetime: %i:%i:%i %i/%i/%i\r\n",
            //             WatchdogDt.Time.second, WatchdogDt.Time.minute, WatchdogDt.Time.hour,
            //             WatchdogDt.Date.day, WatchdogDt.Date.month, WatchdogDt.Date.year,
            //             ComputerDt.Time.second, ComputerDt.Time.minute, ComputerDt.Time.hour,
            //             ComputerDt.Date.day, ComputerDt.Date.month, ComputerDt.Date.year);

            // uint8_t datetimeData[7] = {
            //     ComputerDt.Time.second,      ComputerDt.Time.minute, ComputerDt.Time.hour,
            //     ComputerDt.Date.day,         ComputerDt.Date.month,  ComputerDt.Date.year >> 8,
            //     ComputerDt.Date.year & 0xFF,
            // };

            // bpk_t BpkDatetime;
            // bpk_create(&BpkDatetime, BPK_Addr_Receive_Stm32, BPK_Addr_Send_Maple,
            //            BPK_Req_Set_Datetime, BPK_Code_Execute, 7, datetimeData);
            // maple_send_bpacket(&BpkDatetime);

            // // Set timeout for this request
            // if (et_poll_timeout(&lg_MapleTimeouts, ET_TIMEOUT_SET_DATETIME) == 0) {
            //     et_set_timeout(&lg_MapleTimeouts, ET_TIMEOUT_SET_DATETIME, 600);
            // }
        }
    }

    if (Bpacket->Request.val == BPK_REQ_SET_DATETIME) {

        // Clear the timeout
        et_clear_timeout(&lg_MapleTimeouts, ET_TIMEOUT_SET_DATETIME);

        if (Bpacket->Code.val != BPK_CODE_SUCCESS) {
            log_error("Failed to set datetime on watchdog\r\n");
            return;
        }

        return;
    }

    if (Bpacket->Request.val == BPK_REQ_GET_WATCHDOG_SETTINGS) {

        // Clear the timeout
        et_clear_timeout(&lg_MapleTimeouts, ET_TIMEOUT_GET_WD_SETTINGS);

        if (Bpacket->Code.val != BPK_CODE_SUCCESS) {
            log_error("Failed to get settings on watchdog\r\n");
            return;
        }

        /* Update the time structs with the watchdog settings as this
            not done automatically because the user has not physically
            clicked on and changed the values of the drop boxees */
        lg_CaptureTime.Start.Time.second = Bpacket->Data.bytes[0];
        lg_CaptureTime.Start.Time.minute = Bpacket->Data.bytes[1];
        lg_CaptureTime.Start.Time.hour   = Bpacket->Data.bytes[2];

        // Skipping start date as we are not currently using it

        lg_CaptureTime.End.Time.second = Bpacket->Data.bytes[7];
        lg_CaptureTime.End.Time.minute = Bpacket->Data.bytes[8];
        lg_CaptureTime.End.Time.hour   = Bpacket->Data.bytes[9];

        // Skipping end date as we are not currently using it

        // Skipping interval seconds as we are not currently using it
        lg_CaptureTime.intervalMinute = Bpacket->Data.bytes[15];
        lg_CaptureTime.intervalHour   = Bpacket->Data.bytes[16];
        // Skipping interval days as we are not currently using it

        lg_CameraSettings.frameSize       = Bpacket->Data.bytes[18];
        lg_CameraSettings.jpegCompression = Bpacket->Data.bytes[19];

        /* Update the dropboxes */
        SendMessage(db_startTimeMinutes, CB_SETCURSEL, lg_CaptureTime.Start.Time.minute / 5, 0);
        SendMessage(db_startTimeHours, CB_SETCURSEL, lg_CaptureTime.Start.Time.hour, 0);

        SendMessage(db_endTimeMinutes, CB_SETCURSEL, lg_CaptureTime.End.Time.minute / 5, 0);
        SendMessage(db_endTimeHours, CB_SETCURSEL, lg_CaptureTime.End.Time.hour, 0);

        SendMessage(db_intervalTimeMinutes, CB_SETCURSEL, lg_CaptureTime.intervalMinute / 5, 0);
        SendMessage(db_intervalTimeHours, CB_SETCURSEL, lg_CaptureTime.intervalHour, 0);

        SendMessage(db_camResolution, CB_SETCURSEL, lg_CameraSettings.frameSize, 0);
        SendMessage(db_camQuality, CB_SETCURSEL, lg_CameraSettings.jpegCompression, 0);
    }

    if (Bpacket->Request.val == BPK_REQ_SET_WATCHDOG_SETTINGS) {

        // Clear the timeout
        et_clear_timeout(&lg_MapleTimeouts, ET_TIMEOUT_SET_WD_SETTINGS);

        if (Bpacket->Code.val != BPK_CODE_SUCCESS) {
            log_error("Failed to save settings on watchdog\r\n");
        }

        log_success("Watchdog settings saved\r\n");
        return;
    }

    if (Bpacket->Request.val == BPK_REQ_GET_TEMPERATURE) {

        // Clear the timeout
        et_clear_timeout(&lg_MapleTimeouts, ET_TIMEOUT_GET_TEMPERATURE);

        if (Bpacket->Code.val != BPK_CODE_SUCCESS) {
            log_error("Failed get temperature on watchdog\r\n");
            SetWindowText(lbl_watchdogTemperature, "Temperature: -");
            return;
        }

        // Extract the temperature data from the bpacket and update the temperature label
        float temperature;
        u8_to_float(Bpacket->Data.bytes, &temperature);

        char tempText[30];
        sprintf(tempText, "Temperature: %.3f %cC", temperature, 176);
        SetWindowText(lbl_watchdogTemperature, tempText);

        // Request temperature in 5 seconds
        es_add_task(&lg_MapleScheduler, EVENT_GET_TEMPERATURE, 5000);
    }

    // TODO: Implement rest
}

LRESULT CALLBACK eventHandler(HWND GuiHandle, UINT msg, WPARAM wParam, LPARAM lParam) {

    int wmId    = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);

    if ((msg == WM_COMMAND) && (wmId == GetDlgCtrlID(btn_SaveSettings)) &&
        (wmEvent == BN_CLICKED)) {

        // Toggle flag to update settings to Watchdog
        event_group_set_bit(&lg_EventsMaple, EVENT_WRITE_WATCHDOG_SETTINGS, EGT_ACTIVE);
    }

    if ((msg == WM_COMMAND) && (wmId == GetDlgCtrlID(btn_takePhoto)) && (wmEvent == BN_CLICKED)) {
        event_group_set_bit(&lg_EventsMaple, EVENT_TAKE_PHOTO, EGT_ACTIVE);
    }

    if ((msg == WM_COMMAND) && (wmId == BTN_LIST_DIR_ID) && (wmEvent == BN_CLICKED)) {
        event_group_set_bit(&lg_EventsMaple, EVENT_LIST_DIR, EGT_ACTIVE);
    }

    if ((msg == WM_COMMAND) && (wmId == BTN_COPY_FILE_ID) && (wmEvent == BN_CLICKED)) {
        event_group_set_bit(&lg_EventsMaple, EVENT_COPY_FILE, EGT_ACTIVE);
    }

    if ((msg == WM_COMMAND) && (wmId == BTN_DELETE_FILE_ID) && (wmEvent == BN_CLICKED)) {
        event_group_set_bit(&lg_EventsMaple, EVENT_DELETE_FILE, EGT_ACTIVE);
    }

    /* Handle changes to drop down boxes */
    if (wmId == DD_START_HOUR_ID && wmEvent == CBN_SELCHANGE) {
        uint8_t index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

        // The index is the same as the hour
        lg_CaptureTime.Start.Time.hour = index;
    }

    if (wmId == DD_START_MINUTE_ID && wmEvent == CBN_SELCHANGE) {
        uint8_t index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

        // The minutes go up in 5's so multiply the index by 5
        lg_CaptureTime.Start.Time.minute = index * 5;
    }

    if (wmId == DD_END_HOUR_ID && wmEvent == CBN_SELCHANGE) {
        uint8_t index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

        // The index is the same as the hour
        lg_CaptureTime.End.Time.hour = index;
    }

    if (wmId == DD_END_MINUTE_ID && wmEvent == CBN_SELCHANGE) {
        uint8_t index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

        // The minutes go up in 5's so multiply the index by 5
        lg_CaptureTime.End.Time.minute = index * 5;
    }

    if (wmId == DD_INTERVAL_HOUR_ID && wmEvent == CBN_SELCHANGE) {
        uint8_t index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

        // The index is the same as the hour
        lg_CaptureTime.intervalHour = index;
    }

    if (wmId == DD_INTERVAL_MINUTE_ID && wmEvent == CBN_SELCHANGE) {
        uint8_t index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

        // The minutes go up in 5's so multiply the index by 5
        lg_CaptureTime.intervalMinute = index * 5;
    }

    if (wmId == DD_CAMERA_RESOLUTION && wmEvent == CBN_SELCHANGE) {
        uint8_t index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

        lg_CameraSettings.frameSize = index;
    }

    if (wmId == DD_CAMERA_COMPRESSION && wmEvent == CBN_SELCHANGE) {
        uint8_t index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

        lg_CameraSettings.jpegCompression = index;
    }

    if (wmId == DD_FILE_SELECTOR && wmEvent == CBN_SELCHANGE) {
        uint8_t index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

        lg_selectedFile = lg_sdCardData[index];
    }

    /* Handle 'Connection Status label' */
    if ((HWND)lParam == lbl_watchdogConnection) {

        if (lg_WatchdogConnected == TRUE) {
            SetTextColor((HDC)wParam, RGB(0, 255, 0)); // Green color
        } else {
            SetTextColor((HDC)wParam, RGB(255, 0, 0)); // Red color
        }
        // Set the desired text color and background color

        // Return a handle to the brush used for the background color
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    if (msg == WM_DESTROY) {
        lg_GuiClosed = TRUE;
    }

    return DefWindowProc(GuiHandle, msg, wParam, lParam);
}

int main(int argc, char** argv) {

    // Initialise logging
    log_init(printf, print_data);

    // Create watchdog folder if it does not already exist
    struct stat st;
    int result = stat(WATCHDOG_DIRECTORY, &st);
    if (result != 0 || S_ISDIR(st.st_mode)) {
        mkdir(WATCHDOG_DIRECTORY);
    }

    /* Initialise the GUI */
    // Register the window class
    WNDCLASSEX wc    = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = eventHandler; // Button click event handler
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "myWindowClass";
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    // Create the window
    GuiHandle =
        CreateWindowEx(WS_EX_CLIENTEDGE, "myWindowClass", "Watchdog", WS_OVERLAPPEDWINDOW, 0, 0,
                       WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (GuiHandle == NULL) {
        MessageBox(NULL, "Failed to create window!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    maple_populate_gui(GuiHandle);

    // Create a thread to display the GUI
    HANDLE MapleThread = CreateThread(NULL, 0, maple_start, NULL, 0, NULL);

    if (!MapleThread) {
        log_error("Thread failed\n");
        return 0;
    }

    // Display the window
    ShowWindow(GuiHandle, SW_SHOWNORMAL);
    UpdateWindow(GuiHandle);
    MSG msg;

    while ((GetMessage(&msg, NULL, 0, 0) > 0) && (lg_GuiClosed == FALSE)) {

        /* GUI Functions that must be called */
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void maple_populate_gui(HWND hwnd) {

    /* Create peripheral for connection status */
    lbl_watchdogConnection = gui_utils_create_label("Not connected", 500, 10, 200, 30, hwnd, NULL);

    /* Create peripheral for temperature value */
    lbl_watchdogTemperature =
        gui_utils_create_label("Temperature: -", 500, 40, 200, 30, hwnd, NULL);

    /* Create peripheral for the take photo button */
    btn_takePhoto =
        gui_utils_create_button("Take Photo", 500, 80, 200, 30, hwnd, (HMENU)BTN_TAKE_PHOTO_ID);

    /* Create peripherals for the current datetime on the watchdog */
    lbl_TitleWatchdogDateTime =
        gui_utils_create_label("Watchdog Current Time", 10, 10, 250, 30, hwnd, NULL);
    lbl_WatchdogDateTime = gui_utils_create_label("00:00:00", 260, 10, 150, 30, hwnd, NULL);

    /* Create peripherals for start, end and interval times */
    const int lblTw     = 30;
    const int lblTw2    = 100;
    const int lblTh     = 30;
    const int ddWidth   = 50;
    const int ddHeight  = 500;
    const int xVals[10] = {10, 110, 170, 210, 270, 370, 470, 570, 670, 770};

    /* Start time */
    const int stY = 50;
    lbl_startTime =
        gui_utils_create_label("Start Time: ", xVals[0], stY, lblTw2, lblTh, hwnd, NULL);
    db_startTimeHours    = gui_utils_create_dropbox("Title", xVals[1], stY, ddWidth, ddHeight, hwnd,
                                                 (HMENU)DD_START_HOUR_ID, 24, hours);
    lbl_startTimeHours   = gui_utils_create_label("hrs", xVals[2], stY, lblTw, lblTh, hwnd, NULL);
    db_startTimeMinutes  = gui_utils_create_dropbox("Title", xVals[3], stY, ddWidth, ddHeight, hwnd,
                                                   (HMENU)DD_START_MINUTE_ID, 12, minutes);
    lbl_startTimeMinutes = gui_utils_create_label("min", xVals[4], stY, lblTw, lblTh, hwnd, NULL);

    /* End time */
    const int eTy = 110;
    lbl_endTime   = gui_utils_create_label("End Time: ", xVals[0], eTy, lblTw2, lblTh, hwnd, NULL);
    db_endTimeHours    = gui_utils_create_dropbox("Title", xVals[1], eTy, ddWidth, ddHeight, hwnd,
                                               (HMENU)DD_END_HOUR_ID, 24, hours);
    lbl_endTimeHours   = gui_utils_create_label("hrs", xVals[2], eTy, lblTw, lblTh, hwnd, NULL);
    db_endTimeMinutes  = gui_utils_create_dropbox("Title", xVals[3], eTy, ddWidth, ddHeight, hwnd,
                                                 (HMENU)DD_END_MINUTE_ID, 12, minutes);
    lbl_endTimeMinutes = gui_utils_create_label("min", xVals[4], eTy, lblTw, lblTh, hwnd, NULL);

    /* Internval time */
    const int iTy = 170;
    lbl_intervalTime =
        gui_utils_create_label("Interval: ", xVals[0], iTy, lblTw2, lblTh, hwnd, NULL);
    db_intervalTimeHours = gui_utils_create_dropbox("Title", xVals[1], iTy, ddWidth, ddHeight, hwnd,
                                                    (HMENU)DD_INTERVAL_HOUR_ID, 24, hours);
    lbl_intervalTimeHours  = gui_utils_create_label("hrs", xVals[2], iTy, lblTw, lblTh, hwnd, NULL);
    db_intervalTimeMinutes = gui_utils_create_dropbox(
        "Title", xVals[3], iTy, ddWidth, ddHeight, hwnd, (HMENU)DD_INTERVAL_MINUTE_ID, 12, minutes);
    lbl_intervalTimeMinutes =
        gui_utils_create_label("min", xVals[4], iTy, lblTw, lblTh, hwnd, NULL);

    /* Create peripherals for image quality */
    const int cTy1 = 230;
    lbl_camResolution =
        gui_utils_create_label("Frame Size: ", xVals[0], cTy1, lblTw2, lblTh, hwnd, NULL);
    db_camResolution = gui_utils_create_dropbox("Title", xVals[1], cTy1, 120, ddHeight, hwnd,
                                                (HMENU)DD_CAMERA_RESOLUTION, 5, resolutions);
    const int cTy2   = 270;
    lbl_camQuality =
        gui_utils_create_label("Compression: ", xVals[0], cTy2, lblTw2, lblTh, hwnd, NULL);
    db_camQuality = gui_utils_create_dropbox("Title", xVals[1], cTy2, 120, ddHeight, hwnd,
                                             (HMENU)DD_CAMERA_COMPRESSION, 10, compressions);

    /* Button for saving settings */
    const int bTy    = 310;
    btn_SaveSettings = gui_utils_create_button("Save Settings", xVals[0], bTy, 180, 30, hwnd,
                                               (HMENU)BTN_SAVE_SETTINGS_ID);

    /* Button for interacing with the SD card */
    const int lTy  = 350;
    btn_listDir    = gui_utils_create_button("List Image Data", xVals[0], lTy, 180, 30, hwnd,
                                          (HMENU)BTN_LIST_DIR_ID);
    btn_deleteFile = gui_utils_create_button("Delete Selected Image", xVals[3], lTy, 180, 30, hwnd,
                                             (HMENU)BTN_DELETE_FILE_ID);
    btn_copyFile   = gui_utils_create_button("Copy Selected Image", xVals[6], lTy, 180, 30, hwnd,
                                           (HMENU)BTN_COPY_FILE_ID);

    db_images = gui_utils_create_dropbox("Title", xVals[8], lTy, 300, ddHeight, hwnd,
                                         (HMENU)DD_FILE_SELECTOR, 0, NULL);
}

uint8_t maple_stream(char* cpyFileName) {

    FILE* target;
    target = fopen(cpyFileName, "wb"); // Read binary

    if (target == NULL) {
        log_error("Could not open file\n");
        fclose(target);
        return FALSE;
    }

    // Send command to copy file. Keeping reading until no more data to send across
    maple_create_and_send_bpacket(BPK_Req_Stream_Images, BPK_Addr_Receive_Esp32, 0, NULL);

    int packetsFinished = FALSE;
    clock_t startTime;
    while (packetsFinished == FALSE) {

        // Wait until the packet is ready
        startTime = clock();
        while (packetPendingIndex == packetBufferIndex) {

            if ((clock() - startTime) > 6000) {
                log_error("Timeout 1000ms\n");
                return FALSE;
            }
        }

        if (packetBuffer[packetPendingIndex].Request.val == BPK_Request_Message.val) {
            bpk_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
            continue;
        }

        if (packetBuffer[packetPendingIndex].Request.val != BPK_Code_In_Progress.val &&
            packetBuffer[packetPendingIndex].Request.val != BPK_Code_Success.val) {
            LOG_ERROR_CODE(packetBuffer[packetPendingIndex].Request.val);
            fclose(target);
            return FALSE;
        }

        if (packetBuffer[packetPendingIndex].Request.val == BPK_Code_Success.val) {
            packetsFinished = TRUE;
        }

        for (int i = 0; i < packetBuffer[packetPendingIndex].Data.numBytes; i++) {
            fputc(packetBuffer[packetPendingIndex].Data.bytes[i], target);
        }

        bpk_increment_circ_buff_index(&packetPendingIndex, PACKET_BUFFER_SIZE);
    }

    fclose(target);

    return TRUE;
}

void maple_test(void) {

    // bpk_t Bpacket;
    // uint8_t failed = FALSE;
    // char msg[100];

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test downloading photo that was just taken */

    // Extract the first image name from the Bpacket data
    // char imageFileName[30];

    // int i            = 0;
    // imageFileName[0] = '\0';
    // while (chars_contains(imageFileName, ".jpg") != TRUE) {

    //     int j;
    //     for (j = 0; dirBpacket->bytes[i] != '\n'; j++) {

    //         if (dirBpacket->bytes[i] != '\r') {
    //             imageFileName[j] = dirBpacket->bytes[i];
    //         }

    //         i++;
    //     }

    //     imageFileName[j] = '\0';
    //     i++;

    //     if (i >= dirBpacket->Data.numBytes) {
    //         break;
    //     }
    // }

    // printf("Image to copy: %s\n", imageFileName);

    // if (chars_contains(imageFileName, ".jpg") != TRUE) {
    //     printf("Failed\r\n");
    //     failed = TRUE;
    // } else {

    //     /* Test Copying a File by downloading the image that was just taken */

    //     FILE* testImage;
    //     if ((testImage = fopen("testImage.jpg", "wb")) != NULL) {

    //         // Send request to ESP32 to copy file

    //         // Append the data folder to the image
    //         char fileName[50];
    //         sprintf(fileName, "%s/%s", DATA_FOLDER_PATH, imageFileName);
    // bpk_t ImageBpacket;
    // bpk_create_sp(&ImageBpacket, BPK_Addr_Receive_Esp32, BPK_Addr_Send_Maple,
    //                 BPK_Req_Take_Photo, BPK_Code_Execute,
    //                 fileName);=

    //         uint8_t fileTransfered = FALSE;
    //         clock_t time           = clock();
    //         while (fileTransfered != TRUE) {

    //             if ((clock() - time) > 2000) {
    //                 fclose(testImage);
    //                 printf("%sTime out when receiving image data%s\n", ASCII_COLOR_RED,
    //                 ASCII_COLOR_WHITE); break;
    //             }

    //             // Wait for data
    //             bpk_t* packet = maple_get_next_bpacket_response();

    //             if (packet == NULL) {
    //                 continue;
    //             }

    //             if (packet->Request != WATCHDOG_BPK_R_COPY_FILE) {
    //                 printf("Skipping packet with request %i\r\n", packet->Request);
    //                 continue;
    //             }

    //             time = clock();

    //             // Store data
    //             for (int i = 0; i < packet->Data.numBytes; i++) {
    //                 fputc(packet->bytes[i], testImage);
    //             }

    //             // Check whether the packet is the end of the data stream
    //             if (packet->Code.val == BPK_Code_Success.val) {

    //                 // Close the file
    //                 fclose(testImage);

    //                 fileTransfered = TRUE;
    //             }
    //         }

    //         if (fileTransfered != TRUE) {
    //             failed = TRUE;
    //         }

    //     } else {
    //         printf("%sSkipping file download. fopen() returned NULL%s\n", ASCII_COLOR_RED,
    //         ASCII_COLOR_WHITE); failed = TRUE;
    //     }
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Streaming */

    // FILE* streamImage;
    // if ((streamImage = fopen("streamImage.jpg", "wb")) != NULL) {

    //     maple_create_and_send_bpacket(BPK_Req_Stream_Images, BPK_Addr_Receive_Esp32, 0,
    //     NULL);

    //     uint8_t fileTransfered = FALSE;
    //     clock_t time           = clock();
    //     while (fileTransfered != TRUE) {

    //         if ((clock() - time) > 6000) {
    //             fclose(streamImage);
    //             printf("%sTime out when receiving image data%s\n", ASCII_COLOR_RED,
    //             ASCII_COLOR_WHITE); break;
    //         }

    //         // Wait for data
    //         bpk_t* packet = maple_get_next_bpacket_response();

    //         if ((packet == NULL) || (packet->Request != WATCHDOG_BPK_R_STREAM_IMAGE)) {
    //             continue;
    //         }

    //         time = clock();

    //         // Store data
    //         for (int i = 0; i < packet->Data.numBytes; i++) {
    //             fputc(packet->bytes[i], streamImage);
    //         }

    //         // Check whether the packet is the end of the data stream
    //         if (packet->Code.val == BPK_Code_Success.val) {

    //             // Close the file
    //             fclose(streamImage);

    //             fileTransfered = TRUE;
    //         }
    //     }

    //     if (fileTransfered != TRUE) {
    //         printf("FAiled\n");
    //         failed = TRUE;
    //     }

    // } else {
    //     printf("%sSkipping file download. fopen() returned NULL%s\n", ASCII_COLOR_RED,
    //     ASCII_COLOR_WHITE); failed = TRUE;
    // }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Test Recording Data */

    /* Test Listing all the files on the SD card */

    /* Test Getting the Status */

    /* Test that updating the resolution on the camera actually changes the resolution of the
     * photos taken */

    // if (failed == FALSE) {
    //     printf("%sAll tests passed%s\n", ASCII_COLOR_GREEN, ASCII_COLOR_WHITE);
    // }
}
