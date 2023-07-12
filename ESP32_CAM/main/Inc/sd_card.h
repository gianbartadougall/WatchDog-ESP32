/**
 * @file sd_card.h
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SD_CARD_H
#define SD_CARD_H

/* C Library Includes */
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

/* ESP Includes */
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

/* Personal Includes */
#include "uart_comms.h"
#include "bpacket.h"
#include "datetime.h"

/* Public Macros */

#define LOG_ERR_OFFSET                   23
#define LOG_ERR_MSG_TOO_LONG             (LOG_ERR_OFFSET + 0)
#define SD_CARD_ERROR_UNREADABLE_CARD    (LOG_ERR_OFFSET + 1)
#define SD_CARD_ERROR_CONNECTION_FAILURE (LOG_ERR_OFFSET + 2)
#define SD_CARD_ERROR_IO_ERROR           (LOG_ERR_OFFSET + 3)
#define SD_CARD_NOT_MOUNTED              (LOG_ERR_OFFSET + 4)

/**
 * @brief Creates the given folderpath on the SD card. The folder
 * path can end in a folder or a file. If the path already exits
 * nothing will happen. If any folders do not exist, they will
 * be created along the way.
 *
 * @param folderPath The path to the folder/file to be created.
 * Examples of valid paths are
 *      - folder1/data
 *      - project/data/temperatureData/temps.txt
 * @param bpacket If the folders/file could be created the
 * request of the response packet will be SUCCESS else it will
 * be ERROR and the instruction/data parts of the response packet
 * may contain information about why it failed
 * @return uint8_t WD_SUCCESS if there were no problems else WD_ERROR
 */
uint8_t sd_card_create_path(char* folderPath, bpk_t* Bpacket);

/**
 * @brief Lists all folders and files in a given directory
 *
 * @param folderPath The path the directory to be listed. E.g
 * folder/data
 * @param response If the folders/file could be created the
 * request of the response packet will be SUCCESS else it will
 * be ERROR and the instruction/data parts of the response packet
 * may contain information about why it failed
 * @return uint8_t WD_SUCCESS if there were no problems else WD_ERROR
 */
uint8_t sd_card_list_directory(bpk_t* Bpacket, bpacket_char_array_t* bpacketCharArray);

uint8_t sd_card_list_dir(char* msg);

/**
 * @brief Appends a string to the given file
 *
 * @param filePath The path to the file to append the string to i.e
 * folder/data/data.txt
 * @param string The string to be appended to the file
 * @param response If the folders/file could be created the
 * request of the response packet will be SUCCESS else it will
 * be ERROR and the instruction/data parts of the response packet
 * may contain information about why it failed
 * @return uint8_t WD_SUCCESS if there were no problems else WD_ERROR
 */
uint8_t sd_card_write_to_file(char* filePath, char* string, bpk_t* Bpacket);

/**
 * @brief Saves an image to the SD card
 *
 * @param imageData The image data
 * @param imageLength The size of the image in bytes
 * @param bpacket If the folders/file could be created the
 * request of the response packet will be SUCCESS else it will
 * be ERROR and the instruction/data parts of the response packet
 * may contain information about why it failed
 * @return uint8_t uint8_t WD_SUCCESS if there were no problems else WD_ERROR
 */
uint8_t sd_card_save_image1(uint8_t* imageData, int imageLength, bpk_t* Bpacket);

uint8_t sd_card_save_image(uint8_t* imageData, int imageLength, dt_datetime_t* Datetime, float temperature,
                           char* returnMsg);

/**
 * @brief Finds all the images saved on the SD card in the image data folder and
 * returns the number of images that were found
 *
 * @param numImages A pointer to an int where the number of images found can be
 * stored
 * @param response If the folders/file could be created the
 * request of the response packet will be SUCCESS else it will
 * be ERROR and the instruction/data parts of the response packet
 * may contain information about why it failed
 * @return uint8_t WD_SUCCESS if there were no problems else WD_ERROR
 */
uint8_t sd_card_search_num_images(uint16_t* numImages, bpk_t* Bpacket);

/**
 * @brief Searches the SD card for all the images on the card and updates the
 * internal image number
 *
 * @param response If the folders/file could be created the
 * request of the response packet will be SUCCESS else it will
 * be ERROR and the instruction/data parts of the response packet
 * may contain information about why it failed
 * @return uint8_t WD_SUCCESS if there were no problems else WD_ERROR
 */
uint8_t sd_card_init(bpk_t* Bpacket);

// uint8_t sd_card_get_camera_settings(wd_camera_settings_t* cameraSettings);

uint8_t sd_card_format_sd_card(bpk_t* Bpacket);

void sd_card_copy_file(bpk_t* Bpacket, bpacket_char_array_t* bpacketCharArray);

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

// uint8_t sd_card_write_settings(bpk_t* Bpacket);

// uint8_t sd_card_read_settings(bpk_t* Bpacket);

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

/**
 * @brief Mounts the SD card and writes a message to a given file. If the
 * given file doesn't exit it will attempty to create that file
 *
 * @param filePath The directory to where the given file should be located
 * @param fileName The file name where the message is to be logged to. The directory
 * to this file should be given by the file path
 * @param message The message to be written to the given file. Note the number of
 * characters in this message must be within the set limit
 */
uint8_t sd_card_write(char* filePath, char* fileName, char* message);

/* Functions that don't work yet */
uint8_t sd_card_get_maximum_storage_capacity(uint16_t* maxStorageCapacityMb);
/* Functions that don't work yet */

#endif // SD_CARD_H