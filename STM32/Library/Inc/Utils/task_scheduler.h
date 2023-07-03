/**
 * @file task_scheduler.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-03
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

/* C Library Includes */
#include <stdint.h>

/* STM32 Includes */

/* Personal Includes */
#include "event_group.h"

/* Public Macros */
#define TS_MAX_NUM_RECIPES     3
#define TS_RECIPE_PRINT_LENGTH 70

typedef struct ts_task_t {
    EventGroup_t* EventGroup;
    enum eg_bit_e egBit;
    enum eg_trait_e egTrait;
    uint32_t delay;
    struct ts_task_t* NextTask;
} ts_task_t;

typedef struct ts_recipe_t {
    ts_task_t* Task;
    uint32_t executeOnCount;
} ts_recipe_t;

void ts_init(void);

uint8_t ts_add_recipe(ts_recipe_t* Recipe);

uint8_t ts_recipe_is_running(ts_recipe_t* Recipe);

void ts_isr(void);

void ts_cancel_recipe(ts_recipe_t* Recipe);

void ts_capture_state(void);

#endif // TASK_SCHEDULER_H