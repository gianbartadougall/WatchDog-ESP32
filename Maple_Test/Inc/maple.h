/**
 * @file game.h
 * @author Gian Barta-Dougall
 * @brief System file for game
 * @version 0.1
 * @date --
 *
 * @copyright Copyright (c)
 *
 */
#ifndef MAPLE_H
#define MAPLE_H

/* C Library Includes */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h> // Use for ms timer

/* GLEW Includes */
#define GLEW_STATIC
#include <GL/glew.h>

/* Personal Includes */
#include "bpacket.h"

namespace maple {

class Maple {

  public:
    Maple();
    ~Maple();

    void run();
    void detect_keys();
    void detect_mouse();
    void handle_watchdog_response(bpk_t* Bpacket);
    int read_port(void* buf, size_t count, unsigned int timeout_ms);
    void maple_listen_rx(void* arg);
    uint8_t connect_to_device(bpk_addr_receive_t receiver, uint8_t pingCode);
    void print_data(uint8_t* data, uint16_t length);
};

} // namespace maple

#endif // MAPLE_H