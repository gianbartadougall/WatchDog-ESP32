/**
 * @file watchdog.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-2
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

/* C Library Includes */

/* STM32 Library Includes */

/* Personal Includes */
#include "event_group.h"

/* Public Variables */
extern EventGroup_t gbl_EventsStm, gbl_EventsEsp;

void wd_start(void);

#endif // WATCHDOG_H