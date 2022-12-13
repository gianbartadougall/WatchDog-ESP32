#ifndef SD_CARD_H
#define SD_CARD_H

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include <stdlib.h>
#include <errno.h>

/* Public Macros */

#define LOG_ERR_OFFSET                   23
#define LOG_ERR_MSG_TOO_LONG             (LOG_ERR_OFFSET + 0)
#define SD_CARD_ERROR_UNREADABLE_CARD    (LOG_ERR_OFFSET + 1)
#define SD_CARD_ERROR_CONNECTION_FAILURE (LOG_ERR_OFFSET + 2)
#define SD_CARD_ERROR_IO_ERROR           (LOG_ERR_OFFSET + 3)
#define SD_CARD_NOT_MOUNTED              (LOG_ERR_OFFSET + 4)

#define SYSTEM_LOG_FILE ("logs.txt")

uint8_t save_image(uint8_t* imageData, int imageLength, char* imageName, int number);

/**
 * @brief Mounts the SD Card for reading and writing
 *
 */
uint8_t sd_card_open(void);

/**
 * @brief Unmounts the SD card so it can no longer be accessed
 *
 */
void sd_card_close(void);

/**
 * @brief Mounts the SD card and writes a message to a given log file. If the
 * given log file doesn't exit it will create that file
 *
 * @param fileName The file name where the message is to be logged to. The path to reach the
 * given file in the Log folder of the SD card is handled by the function
 * @param message The message to be logged to the SD card. Note the number of characters in this
 * message must be within the set limit
 */
uint8_t sd_card_log(char* fileName, char* message);

#endif // SD_CARD_H