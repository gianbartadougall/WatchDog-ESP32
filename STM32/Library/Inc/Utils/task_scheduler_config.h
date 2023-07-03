#ifndef TASK_SCHEDULER_CONFIG_H
#define TASK_SCHEDULER_CONFIG_H

/* C Library Includes */

/* STM32 Library Includes */

/* Personal Includes */
#include "watchdog.h"
#include "task_scheduler.h"

struct ts_task_t ledGreenOff = {
    .EventGroup = &gbl_EventsStm,
    .egBit      = EGB_GREEN_LED_OFF,
    .egTrait    = EGT_ACTIVE,
    .delay      = 1000,
    .NextTask   = NULL,
};

struct ts_task_t ledGreenOn = {
    .EventGroup = &gbl_EventsStm,
    .egBit      = EGB_GREEN_LED_ON,
    .egTrait    = EGT_ACTIVE,
    .delay      = 2000,
    .NextTask   = &ledGreenOff,
};

struct ts_task_t ledRedOff = {
    .EventGroup = &gbl_EventsStm,
    .egBit      = EGB_RED_LED_OFF,
    .egTrait    = EGT_ACTIVE,
    .delay      = 1000,
    .NextTask   = NULL,
};

struct ts_task_t ledRedOn = {
    .EventGroup = &gbl_EventsStm,
    .egBit      = EGB_RED_LED_ON,
    .egTrait    = EGT_ACTIVE,
    .delay      = 1500,
    .NextTask   = &ledRedOff,
};

ts_recipe_t BlinkGreenLED = {
    .Task           = &ledGreenOn,
    .executeOnCount = 0,
};

ts_recipe_t BlinkRedLED = {
    .Task           = &ledRedOn,
    .executeOnCount = 0,
};

#endif // TASK_SCHEDULER_CONFIG_H