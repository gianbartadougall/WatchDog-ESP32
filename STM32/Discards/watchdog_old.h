/**
 * @file watchdog.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-02-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

void watchdog_update(void);

void watchdog_init(void);

void watchdog_enter_state_machine(void);

void watchdog_rtc_alarm_triggered(void);

#endif // WATCHDOG_H