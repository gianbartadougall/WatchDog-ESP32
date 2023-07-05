/**
 * @file task_scheduler.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-03
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <stdio.h>

/* STM32 Includes */
#include "stm32l4xx_hal.h"

/* Personal Includes */
#include "task_scheduler.h"
#include "utils.h"
#include "hardware_config.h"
#include "log_usb.h"

/* Private Macros */
#ifndef TS_TIMER
#    define TS_TIMER TIM1
#    error Task scheduler timer has not been declared
#endif

#ifndef TS_TIMER_MAX_COUNT
#    define TS_TIMER_MAX_COUNT 0 // Define macro so compiler will not raise same error later on
#    error Task scheduler timer max count has not been declared
#endif

/* Private Variables */
ts_recipe_t* Recipes[TS_MAX_NUM_RECIPES];

/* Function prototypes */
void task_scheduler_update_cc_time(void);
void task_scheduler_update_execution_time(ts_recipe_t* Recipe);
void task_scheduler_enable(void);
void task_scheduler_disable(void);

/**
 * @brief Sets all the pointers in the Recipes list to NULL. Disables the
 * timer as no tasks are currently running
 */
void task_scheduler_init(void) {

    // Set all the recipes to null
    for (uint8_t i = 0; i < TS_MAX_NUM_RECIPES; i++) {
        Recipes[i] = NULL;
    }

    // Disable ask scheduler as there are no recipes added yet
    task_scheduler_disable();
}

/**
 * @brief Adds a recipe to the Recipes list if there is an available spot.
 * The new recipe will be added the first available position in the list.
 * The Recipe will only be added if it does not already exist in the list
 *
 * @param Recipe A pointer to the new recipe to be added
 * @return uint8_t Returns TRUE if a recipe could be added and FALSe if the
 * recipe could not be added.
 */
uint8_t task_scheduler_add_recipe(ts_recipe_t* Recipe) {

    // Exit function if the recipe is already running
    if (task_scheduler_recipe_is_running(Recipe) == TRUE) {
        return TRUE;
    }

    // Recipe is not in the task scheduler. Add this recipe
    // to a free spot in the Recipes list
    for (uint8_t i = 0; i < TS_MAX_NUM_RECIPES; i++) {

        // Null implies a free spot in the Recipes list
        if (Recipes[i] == NULL) {

            // Add recipe to the list
            Recipes[i] = Recipe;

            // Calculate when the task of the recipe needs to be run by the isr
            task_scheduler_update_execution_time(Recipes[i]);

            // Update the capture compare of the timer incase this new task
            // needs to be run before any tasks that are currently in the list
            task_scheduler_update_cc_time();

            return TRUE;
        }
    }

    // There were no free spots in the Recipes list to add this recipe
    return FALSE;
}

/**
 * @brief Loops through all the recipes and compares the execution times of all
 * their current tasks that need to be run. The task that needs to be executed
 * the soonest will be chosen and its execution time will be set as the new
 * capture compare value for the timer
 *
 * This function will automatically disable/eneable the timer based on whether
 * there are any recipes in the recipe list to run. If there are none the timer
 * is disabled to prevent random interrupts from occuring
 */
void task_scheduler_update_cc_time(void) {

    // Store the current count on the timer
    uint32_t currentTimerCount = TS_TIMER->CNT;

    // Reset the next ISR count to be the maximum
    uint32_t smallestCountUntilISR = TS_TIMER_MAX_COUNT;

    // Flag to determine whether the ISRs for the timer need to be enabled or disabled
    uint8_t recipeFound = FALSE;

    for (uint8_t i = 0; i < TS_MAX_NUM_RECIPES; i++) {

        // Because the Recipe list is not reordered when Recipes complete or are
        // cancelled, there are likely to be Recipe pointers pointing to NULL
        // in between Recipe pointers pointing to recipes.
        if (Recipes[i] == NULL) {
            continue;
        }

        recipeFound = TRUE;

        // Compute how many timer counts until this task needs to run
        uint32_t countsUntilISR = 0;
        if (Recipes[i]->executeOnCount > currentTimerCount) {
            countsUntilISR = Recipes[i]->executeOnCount - currentTimerCount;
        } else {
            countsUntilISR = Recipes[i]->executeOnCount + (TS_TIMER_MAX_COUNT - currentTimerCount);
        }

        // Update the capture compare time if the number of counts for the task of this recipe
        // is smaller than all the tasks that have been checked so far
        if (countsUntilISR <= smallestCountUntilISR) {
            TS_TIMER->CCR1 = Recipes[i]->executeOnCount;

            // Update the smallest count to the new smallest count
            smallestCountUntilISR = countsUntilISR;
        }
    }

    // Disable interrupts if there are no more recipes. If this is not done then
    // the interrupts will continue to fire when they reach whatever count is currently
    // in the capture compare register
    if (recipeFound == TRUE) {
        task_scheduler_enable();
    } else {
        task_scheduler_disable();
    }
}

void task_scheduler_update_execution_time(ts_recipe_t* Recipe) {

    if (TS_TIMER->CNT > (TS_TIMER_MAX_COUNT + Recipe->Task->delay)) {
        Recipe->executeOnCount = Recipe->Task->delay - (TS_TIMER_MAX_COUNT - TS_TIMER->CNT);
        return;
    }

    Recipe->executeOnCount = TS_TIMER->CNT + Recipe->Task->delay;
}

uint8_t task_scheduler_recipe_is_running(ts_recipe_t* Recipe) {

    for (uint8_t i = 0; i < TS_MAX_NUM_RECIPES; i++) {
        if (Recipes[i] == Recipe) {
            return TRUE;
        }
    }

    return FALSE;
}

void task_scheduler_cancel_recipe(ts_recipe_t* Recipe) {

    for (uint8_t i = 0; i < TS_MAX_NUM_RECIPES; i++) {
        if (Recipes[i] == Recipe) {
            Recipes[i] = NULL;

            // Update the capture compare of the timer as the execution
            // of this recipe is no longer needed
            task_scheduler_update_cc_time();
        }
    }
}

/**
 * @brief This function is run anytime the timer reaches the capture compare value.
 * Loops through every recipe and checks whether the execution time matches the
 * capture compare value. The corresponding flag that each task points to will be
 * set if the recipes execution time for that task matches the capture compare value.
 *
 * Any recipes with no more tasks to run will be removed from the Recipes list and any
 * recipes with more tasks to run will have their tasks incremented to the next task
 * in their recipe
 *
 * The capture compare value for the timer is updated at the end of the ISR
 */
void task_scheduler_isr(void) {

    // Loop through each Recipe in the list and set the appropriate flag of the current task
    // if the recipues exeecution time matches the execution time of this ISR
    for (uint8_t i = 0; i < TS_MAX_NUM_RECIPES; i++) {

        // Because the Recipe list is not reordered when Recipes complete or are
        // cancelled, there are likely to be Recipe pointers pointing to NULL
        // in between Recipe pointers pointing to recipes.
        if (Recipes[i] == NULL) {
            continue;
        }

        // Skip any recipes whos tasks are not supposed to execute at this time
        if (Recipes[i]->executeOnCount != TS_TIMER->CCR1) {
            continue;
        }

        if (Recipes[i]->executeOnCount == TS_TIMER->CCR1) {
            ts_task_t* Task = Recipes[i]->Task;
            event_group_set_bit(Task->EventGroup, Task->egBit, Task->egTrait);
        }

        // Set the Recipe to NULL if this was the last task to execute in the recipe
        if (Recipes[i]->Task->NextTask == NULL) {
            Recipes[i] = NULL;
            continue;
        }

        // More tasks to run in recipe. Set the task in the recipe to the next task
        Recipes[i]->Task = Recipes[i]->Task->NextTask;

        // Update the execution time of the recipe so it matches that of the new task
        task_scheduler_update_execution_time(Recipes[i]);
    }

    // Update the capture compare of the timer so the capture compare can be set for
    // the next task that needs to run
    task_scheduler_update_cc_time();
}

/**
 * @brief Prints out the current state of the recipes
 */
void task_scheduler_capture_state(void) {

    for (uint8_t i = 0; i < TS_MAX_NUM_RECIPES; i++) {

        if (Recipes[i] == NULL) {
            log_usb_message("Recipe: NULL\tNum tasks: 0\tCurrent task: None\tNext Task: None\r\n");
        } else {
            uint8_t numTasks    = 0;
            ts_task_t* TempTask = Recipes[i]->Task;

            while (TempTask != NULL) {
                numTasks++;
                TempTask = TempTask->NextTask;
            }

            log_usb_message("Recipe: %p\tNum tasks: %i\tCurrent task: %p\tEx time: %li\tNext Task: %p\r\n", Recipes[i],
                            numTasks, Recipes[i]->Task, Recipes[i]->executeOnCount,
                            Recipes[i]->Task == NULL ? NULL : Recipes[i]->Task->NextTask);
        }
    }
}

/**
 * @brief Enables and resets the timer and enables and clears the capture compare 1 interrupt
 */
void task_scheduler_enable(void) {
    // Only enable timer if its currently disabled
    if ((TS_TIMER->CR1 & TIM_CR1_CEN) == 0) {
        TS_TIMER->EGR |= (TIM_EGR_UG);    // Reset counter to 0 and update all registers
        TS_TIMER->DIER |= TIM_DIER_CC1IE; // Enable interrupts
        TS_TIMER->CR1 |= TIM_CR1_CEN;     // Start the timer
    }
}

/**
 * @brief Disables the timer and all interrupts
 */
void task_scheduler_disable(void) {
    TS_TIMER->DIER &= 0x00;          // Disable all interrupts
    TS_TIMER->CR1 &= ~(TIM_CR1_CEN); // Disbable timer
}