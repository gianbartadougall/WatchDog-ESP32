
/* Private Includes */
#include "watchdog.h"
#include "comms.h"
#include "ds18b20.h"
#include "hardware_config.h"
#include "log.h"
#include "uart_comms.h"
#include "wd_utils.h"
#include <stdio.h>

void print_64_bit1(uint64_t number);

void watchdog_init(void) {
    log_clear();
    ds18b20_init();
}

void watchdog_update(void) {

    comms_usart1_print_buffer();
}