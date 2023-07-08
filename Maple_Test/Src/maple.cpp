/* C++ Library Includes */
#include <chrono> // Required for getting system time
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>

#include <stdio.h>
#include <time.h> // Use for ms timer

/** GLEW Includes. This must be included before GLFW includes. GLEW must
 * be included before GLFW includes. Including GLEW before private includes
 * is also recommended to avoid header file conflicts
 */
#define GLEW_STATIC
#include <GL/glew.h>

/* GLFW Includes */
#include <GLFW/glfw3.h>

/* C Library Includes for COM Port Interfacing */
#include <libserialport.h>

/* Personal Includes */
#include "maple.h"
#include "gameSettings.h"
#include "cbuffer.h"
#include "debugLog.h"

using namespace maple;
using namespace debugLog;

/* Private variable Declarations */
GLFWwindow* window;

struct sp_port* gbl_activePort = NULL;

#define WATCHDOG_PING_CODE_STM32  47
#define MAPLE_BUFFER_NUM_ELEMENTS 10
#define MAPLE_BUFFER_NUM_BYTES    (sizeof(bpk_t) * MAPLE_BUFFER_NUM_ELEMENTS)
cbuffer_t RxCbuffer;
uint8_t rxCbufferBytes[MAPLE_BUFFER_NUM_BYTES];
uint8_t watchdogConnected = FALSE;

/* Function Prototypes */

void drawRectangle() {
    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.0f, 0.0f); // Red color
    glVertex2f(-0.5f, -0.5f);    // Bottom-left vertex
    glVertex2f(0.5f, -0.5f);     // Bottom-right vertex
    glVertex2f(0.5f, 0.5f);      // Top-right vertex
    glVertex2f(-0.5f, 0.5f);     // Top-left vertex
    glEnd();
}

void Maple::print_data(uint8_t* data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        printf("%c", data[i]);
    }
    printf("\r\n");
}

void Maple::handle_watchdog_response(bpk_t* Bpacket) {
    // TODO: Implement
}

void Maple::run() {

    /* Create watchdog folder if it doesn't exist */
    // TODO: Implement

    /* Initialise the GUI */
    // TODO: Implement

    uint8_t terminateThread = 0;
    cbuffer_init(&RxCbuffer, (void*)rxCbufferBytes, sizeof(bpk_t), MAPLE_BUFFER_NUM_ELEMENTS);
    std::thread listenThread(&Maple::maple_listen_rx, this, (void*)&terminateThread);

    bpk_t BpkWatchdogResponse;

    // Main loop
    while (!glfwWindowShouldClose(window)) {

        /* Update the screen */
        detect_keys();

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the rectangle
        drawRectangle();

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();

        if (watchdogConnected != TRUE) {

            if (connect_to_device(BPK_Addr_Receive_Stm32, WATCHDOG_PING_CODE_STM32) != TRUE) {

                // TODO: Delay 500ms

                // TODO: Display error on GUI

                continue;
            }

            watchdogConnected = TRUE;

            // TODO: Display connection status on GUI
            char* portName = sp_get_port_name(gbl_activePort);
            DebugLog::log_success("Watchdog connected (%s)\r\n", portName);
            free(portName);
        }

        // Process any bpackets received from a Watchdog
        if (cbuffer_read_next_element(&RxCbuffer, &BpkWatchdogResponse) == TRUE) {
            handle_watchdog_response(&BpkWatchdogResponse);
        }
    }
}

Maple::Maple() {

    // Create the window for the game
    if (glfwInit() != GLFW_TRUE) {
        DebugLog::log_error("GLFW could not be initialised");
        exit(1);
    }

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Sequoia", NULL, NULL);
    if (window == NULL) {
        DebugLog::log_error("Window could not be created");
        exit(2);
    }

    // Make the window's context current. This must happen before glew init
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        DebugLog::log_error("GLEW could not be initialised");
    }

    // glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

Maple::~Maple() {
    // Free any resources that were used
    glfwTerminate();
}

void Maple::detect_mouse() {

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (xpos < 0 || ypos < 0 || xpos > WINDOW_WIDTH || ypos > WINDOW_HEIGHT) {
        return;
    }

    glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
}

void Maple::detect_keys() {

    detect_mouse();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        exit(1);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        printf("A key pressed\r\n");
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {}

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {}

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {}
}

int Maple::read_port(void* buf, size_t count, unsigned int timeout_ms) {

    if (gbl_activePort == NULL) {
        return 0;
    }

    return sp_blocking_read(gbl_activePort, buf, count, timeout_ms);
}

void Maple::maple_listen_rx(void* arg) {

    uint8_t byte, expectedByte;
    int numBytes;
    cbuffer_t ByteBuffer;
    uint8_t byteBuffer[8];
    bpk_init_expected_byte_buffer(byteBuffer);
    cbuffer_init(&ByteBuffer, (void*)byteBuffer, sizeof(uint8_t), 8);
    bpk_t Bpacket;
    uint8_t bi = 0;

    uint8_t* terminate = (uint8_t*)arg;

    while (1) {

        numBytes = read_port(&byte, 1, 0);

        if (*terminate == true) {
            break;
        }

        // If received bytes <0, this implies error has occured and connection
        // has broken
        if (numBytes < 0) {
            watchdogConnected = FALSE;
            continue;
        }

        if (numBytes == 0) {
            continue;
        }

        // Read the current index of the byte buffer and store the result into
        // expected byte
        cbuffer_read_current_element(&ByteBuffer, (void*)&expectedByte);

        switch (expectedByte) {

                //     /* Expecting bpacket 'data' bytes */
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
                if (bpk_decode_non_data_byte(&Bpacket, expectedByte, byte) == TRUE) {
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

    DebugLog::log_error("Connection terminated with code %i\n", numBytes);

    watchdogConnected = FALSE;
}

uint8_t Maple::connect_to_device(bpk_addr_receive_t receiver, uint8_t pingCode) {

    // Create a struct to hold all the COM ports currently in use
    struct sp_port** port_list;
    printf("Connecting to device\n");
    if (sp_list_ports(&port_list) != SP_OK) {
        DebugLog::log_error("Failed to list ports");
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
        // uint8_t terminateThread = false;
        // std::thread listenThread(&Maple::maple_listen_rx, this, (void*)&terminateThread);

        // Configure the port settings for communication
        sp_set_baudrate(gbl_activePort, 115200);
        sp_set_bits(gbl_activePort, 8);
        sp_set_parity(gbl_activePort, SP_PARITY_NONE);
        sp_set_stopbits(gbl_activePort, 1);
        sp_set_flowcontrol(gbl_activePort, SP_FLOWCONTROL_NONE);
        printf("Sending ping\n");
        // Send a ping
        bpk_t Bpacket;
        bpk_buffer_t bpacketBuffer;
        bpk_create(&Bpacket, receiver, BPK_Addr_Send_Maple, BPK_Request_Ping, BPK_Code_Execute, 0, NULL);
        bpk_to_buffer(&Bpacket, &bpacketBuffer);

        if (sp_blocking_write(gbl_activePort, bpacketBuffer.buffer, bpacketBuffer.numBytes, 100) < 0) {
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

            // DebugLog::log_error("Terminating thread\n");
            // Terminate the thread
            // terminateThread = true;

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