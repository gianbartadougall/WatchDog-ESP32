/**
 * @file utilities.c
 * @author Gian Barta-Dougall
 * @brief System file for utilities
 * @version 0.1
 * @date --
 *
 * @copyright Copyright (c)
 *
 */
/* Public Includes */

/* Private Includes */
#include "utilities.h"

/* Private STM Includes */

/* Private #defines */

/* Private Structures and Enumerations */

/* Private Variable Declarations */

/* Private Function Prototypes */
void utilities_hardware_init(void);

/* Public Functions */

void utilities_init(void) {

  // Initialise hardware
  utilities_hardware_init();
}

/* Private Functions */

/**
 * @brief Initialise the hardware for the library.
 */
void utilities_hardware_init(void) {}
