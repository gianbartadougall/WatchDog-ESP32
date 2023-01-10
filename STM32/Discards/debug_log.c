/**
 * @file debug_log.c
 * @author Gian Barta-Dougall
 * @brief Library for debugging
 * @version 0.1
 * @date 2022-05-31
 *
 * @copyright Copyright (c) 2022
 *
 */

/* Private Includes */
#include "debug_log.h"

// Handle for uart
UART_HandleTypeDef *debugLogHandle;

void debug_log_init(UART_HandleTypeDef *huart) { debugLogHandle = huart; }

HAL_StatusTypeDef debug_prints(char *str) {
  uint8_t length = 0;

  // Looping through a maximum of 255 characters to find the null terminator
  // string
  for (int i = 0; i < 256; i++) {
    if (str[i] == '\0') {
      // Storing index of null terminator and breaking
      length = (uint8_t)i;
      break;
    }
  }

  uint8_t array[256];
  // Convert char array to int array
  for (int i = 0; i < length; i++) {
    array[i] = (uint8_t)str[i];
  }

  // Transmitting over uart using the length found earlier. Timeout set 1000ms
  // arbitrarily
  return HAL_UART_Transmit(debugLogHandle, array, length, 1000);
}

HAL_StatusTypeDef debug_printa(uint8_t *ptr, uint8_t length) {
  return HAL_UART_Transmit(debugLogHandle, ptr, length, 1000);
}

char debug_getc(void) {
  uint8_t recievedChar = '\0';

  // Receiving uart transmission and storing value in the created uint8_t
  // variable. Returning value immiediatly if receiving data was succesful
  if (HAL_UART_Receive(debugLogHandle, &recievedChar, 1, 1) == HAL_OK) {
    return (char)recievedChar;
  }

  // Error when receiving. Returning null terminator to signify this
  return '\0';
}

HAL_StatusTypeDef debug_clear(void) {
  // Prints ANSI escape codes that will clear the terminal screen
  char str[] = "\033[2J\033[H";
  char *pstr = &str[0];

  return HAL_UART_Transmit(debugLogHandle, (uint8_t *)pstr, 11, 1000);
}

HAL_StatusTypeDef debug_print_uint64(uint64_t number) {

  char m[30];
  uint16_t sec1 = ((number << 0) >> (16 * 3));
  uint16_t sec2 = ((number << 16) >> (16 * 3));
  uint16_t sec3 = ((number << 32) >> (16 * 3));
  uint16_t sec4 = ((number << 48) >> (16 * 3));

  sprintf(m, "Number: %x %x %x %x\r\n", sec1, sec2, sec3, sec4);

  if (debug_prints(m) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}