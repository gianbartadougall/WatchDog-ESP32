
/* Private Includes */
#include "watchdog.h"
#include "log.h"
#include <stdio.h>
#include "ds18b20.h"
#include "uart_comms.h"
#include "comms.h"
#include "wd_utils.h"
#include "hardware_config.h"

void print_64_bit1(uint64_t number);

void watchdog_init(void) {
    debug_clear();
}

void watchdog_update(void) {

    // Get message from console
    char msg[100];
    uint8_t i = 0;
    while (1) {

        char c = debug_getc();

        if (c == 0x0D) {
            msg[i++] = '\r';
            msg[i++] = '\n';
            msg[i++] = '\0';
            debug_prints("\r\n");
            break;
            i = 0;
        } 
        
        // Sometimes putty accidently sends FF on restart for some reason
        // this stops this from happening
        if (c != 0xFF) {
            msg[i] = c;
            char character[2];
            sprintf(character, "%c", c);
            debug_prints(character);
            i++;
        }
    }

    // Construct packet from string
    packet_t packet;
    int num = string_to_packet(&packet, msg);
    if (num != WD_SUCCESS) {
        char msg1[50];
        sprintf(msg1, "Failed to parse packet with error %d\r\n", num);
        log_error(msg1);
    }

    // Validate instruction

    // Turn packet back into string to send to esp32
    char message[RX_BUF_SIZE];
    if (packet_to_string(&packet, message) != WD_SUCCESS) {
        return;
    }

    // Send packet to esp32
    comms_send_data(UART_ESP32, message);

    // // Wait for response from ESP32
    // char esp32Response[RX_BUF_SIZE];
    // comms_read_data(UART_ESP32, esp32Response);

    // // Turn response into packet
    // packet_t esp32ResponsePacket;
    // string_to_packet(&esp32ResponsePacket);

    // if (esp32ResponsePacket.request != UC_REQUEST_ACKNOWLEDGED) {
    //     log_error("ESP32 did not acknowledge request\r\n");
    // }
}