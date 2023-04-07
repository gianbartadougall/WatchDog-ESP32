/**
 * @file utilities.h
 * @author Gian Barta-Dougall
 * @brief System file for utilities
 * @version 0.1
 * @date --
 *
 * @copyright Copyright (c)
 *
 */
#ifndef UTILS_H
#define UTILS_H

/* Public Macros */

// The definitions for high and low need to be 1 and 0
// because it they are used in if statement calculations
// when checking the states of pins which are defined by
// hardware to be 1 for high and 0 for low. Changing these
// will break any code that uses these defines
#define FALSE 0
#define TRUE  1

/* Max value for numbers */
#define UINT_8_BIT_MAX_VALUE  255
#define UINT_16_BIT_MAX_VALUE 65535
#define UINT_32_BIT_MAX_VALUE 4294967295

#endif // UTILS_H
