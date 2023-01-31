/**
 * @file sd_card.c
 * @author Gian Barta-Dougall
 * @brief This file handles all data being read from or written to the SD card.
 * The current features that this library supports are:
 * -
 * @version 0.1
 * @date 2023-01-12
 * @copyright Copyright (c) 2023
 *
 */

/* Library Includes */
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "driver/uart.h"

/* Personal Includes */
#include "rtc.h"
#include "sd_card.h"
#include "chars.h"
#include "hardware_config.h"
#include "esp32_uart.h"

#include "uart_comms.h"

/* Private Macros */

#define LOG_MSG_MAX_CHARACTERS 200
#define NULL_CHAR_LENGTH       1
#define DATE_TIME_DELIMITER    '_'
#define IMG_NUM_START_INDEX    3
#define IMG_NUM_END_CHARACTER  '-'

#define MAX_PATH_LENGTH 200

static const char* SD_CARD_TAG = "SD CARD:";

/* Private Variables */
int mounted          = FALSE;
uint16_t imageNumber = 0;

// Options for mounting the SD Card are given in the following
// configuration
static esp_vfs_fat_sdmmc_mount_config_t sdCardConfiguration = {
    .format_if_mount_failed = false,
    .max_files              = 5,
    .allocation_unit_size   = 16 * 1024,
};

sdmmc_card_t* card;

/* Private Function Declarations */
uint8_t sd_card_check_file_path_exists(char* filePath);
uint8_t sd_card_check_directory_exists(char* directory);

/* GOOD FUNCTIONS */

uint8_t sd_card_create_path(char* folderPath, bpacket_t* bpacket) {

    // Save the address
    uint8_t address = bpacket->address;

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "SD card could not open\n");
        return FALSE;
    }

    // Validate the path length
    if (chars_get_num_bytes(folderPath) > MAX_PATH_LENGTH) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "Folder path > 50 characters\n");
        return FALSE;
    }

    // Try create folder
    char path[MAX_PATH_LENGTH + 1];
    struct stat st = {0};
    int fileFound  = 0;
    for (int i = 0; folderPath[i] != '\0'; i++) {
        path[i] = folderPath[i];

        // Finding a . signifies there are no more folders to be made, only one file
        if (path[i] == '.') {
            fileFound = 1;
        }

        if (path[i] == '/' || folderPath[i + 1] == '\0') {

            path[i + (folderPath[i + 1] == '\0')] = '\0';

            // Add SD card mount point to start of path
            char directory[MAX_PATH_LENGTH + 9];
            sprintf(directory, "%s/%s", MOUNT_POINT_PATH, path);

            // Try create the file if the end of the path is a file
            if (fileFound) {
                FILE* file = fopen(directory, "w");

                if (file == NULL) {
                    bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "File could not be made\n");
                    return FALSE;
                }

                break;
            }

            // Error if the directory does not exist and a new one could not be made
            if (stat(directory, &st) != 0 && mkdir(directory, 0700) != 0) {
                bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "Dir could not be made\n");
                return FALSE;
            }

            path[i] = '/';
        }
    }

    bpacket_create_p(bpacket, address, BPACKET_R_SUCCESS, 0, NULL);
    return TRUE;
}

uint8_t sd_card_list_directory(bpacket_t* bpacket, bpacket_char_array_t* bpacketCharArray) {

    // Save address
    uint8_t address = bpacket->address;

    char* folderPath = bpacketCharArray->string;

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "SD card could not open\0");
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    // Validate the path length
    if (chars_get_num_bytes(folderPath) > MAX_PATH_LENGTH) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "Folder path > 50 chars\0");
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    // Append the mounting point of the SD card to the folder path
    char path[MAX_PATH_LENGTH + 9];
    sprintf(path, "%s%s%s", MOUNT_POINT_PATH, folderPath[0] == '\0' ? "" : "/", folderPath);

    DIR* directory;
    directory = opendir(path);
    if (directory == NULL) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "Filepath could not open\0");
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    // Calculate number of characters in all folder names combined
    uint16_t numChars = 0;
    struct dirent* dirPtr;
    while ((dirPtr = readdir(directory)) != NULL) {
        numChars += strlen(dirPtr->d_name) + 2; // Adding 2 chars for \r\n
    }

    // Go back to the start of the directory
    rewinddir(directory);

    int i             = 0;
    bpacket->address  = address;
    bpacket->request  = BPACKET_R_IN_PROGRESS;
    bpacket->numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    int in            = FALSE; // Variable just to know whether there was anything or not
    while ((dirPtr = readdir(directory)) != NULL) {
        in = 1;
        // Copy the name of the folder into the folder structure string
        int j = 0;
        while (dirPtr->d_name[j] != '\0') {
            bpacket->bytes[i++] = dirPtr->d_name[j];

            if (i == BPACKET_MAX_NUM_DATA_BYTES) {
                esp32_uart_send_bpacket(bpacket);
                i = 0;
            }

            j++;
        }

        bpacket->bytes[i++] = '\r';

        if (i == BPACKET_MAX_NUM_DATA_BYTES) {
            esp32_uart_send_bpacket(bpacket);
            i = 0;
        }

        bpacket->bytes[i++] = '\n';

        if (i == BPACKET_MAX_NUM_DATA_BYTES) {
            esp32_uart_send_bpacket(bpacket);
            i = 0;
        }
    }

    if (i != 0) {
        bpacket->request  = BPACKET_R_SUCCESS;
        bpacket->numBytes = i;
        esp32_uart_send_bpacket(bpacket);
    }

    if (in == FALSE) {
        bpacket_create_p(bpacket, address, BPACKET_R_SUCCESS, 0, NULL);
        esp32_uart_send_bpacket(bpacket);
    }

    closedir(directory);
    sd_card_close();

    return TRUE;
}

uint8_t sd_card_write_to_file(char* filePath, char* string, bpacket_t* bpacket) {

    // Save the address
    uint8_t address = bpacket->address;

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "SD Card could not open\0");
        return FALSE;
    }

    // Validate the path length
    if (chars_get_num_bytes(filePath) > MAX_PATH_LENGTH) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "Filepath > 50 characters\0");
        return FALSE;
    }

    // Add SD card mount point to start of path
    char directory[MAX_PATH_LENGTH + 9];
    sprintf(directory, "%s/%s", MOUNT_POINT_PATH, filePath);

    if (sd_card_create_path(directory, bpacket) != TRUE) {
        return FALSE;
    }

    // Try open the file
    FILE* file = fopen(directory, "a+");

    if (file == NULL) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "Filepath could not open\0");
        return FALSE;
    }

    fprintf(file, string);
    fclose(file);

    bpacket_create_p(bpacket, address, BPACKET_R_SUCCESS, 0, NULL);
    return TRUE;
}

uint8_t sd_card_search_num_images(uint16_t* numImages, bpacket_t* bpacket) {

    // Save the address
    uint8_t address = bpacket->address;

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        return FALSE;
    }

    // Confirm the directory for the images exists
    if (sd_card_create_path(IMAGE_DATA_FOLDER, bpacket) != TRUE) {
        return FALSE;
    }

    // Open the directory where the images are stored
    DIR* directory;
    directory = opendir(ROOT_IMAGE_DATA_FOLDER);
    if (directory == NULL) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "Img dir could not open\0");
        return FALSE;
    }

    // Loop through each file and calculate the number of images
    struct dirent* dirPtr;
    *numImages = 0;
    while ((dirPtr = readdir(directory)) != NULL) {
        // Checking both img and IMG just incase as SD card is weird when it comes allowing/not allowing
        // lower case letters
        if (chars_contains(dirPtr->d_name, "img") == TRUE || chars_contains(dirPtr->d_name, "IMG") == TRUE) {
            *numImages += 1;
        }
    }

    return TRUE;
}

uint8_t sd_card_get_maximum_storage_capacity(uint16_t* maxStorageCapacityMb) {

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        return FALSE;
    }

    *maxStorageCapacityMb = (card->csd.capacity * card->csd.sector_size) / (1024 * 1024);

    sd_card_close();

    return TRUE;
}

uint8_t sd_card_save_image(uint8_t* imageData, int imageLength, bpacket_t* bpacket) {

    // Save the address
    uint8_t address = bpacket->address;

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        return FALSE;
    }

    // Create the data folder path if required
    if (sd_card_create_path(IMAGE_DATA_FOLDER, bpacket) != TRUE) {
        return FALSE;
    }

    // Create path for image
    char filePath[60];
    sprintf(filePath, "%s/%s/img%i.jpg", MOUNT_POINT_PATH, IMAGE_DATA_FOLDER, imageNumber);

    FILE* imageFile = fopen(filePath, "wb");
    if (imageFile == NULL) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "Image file failed to open\0");
        return FALSE;
    }

    for (int i = 0; i < imageLength; i++) {
        fputc(imageData[i], imageFile);
    }

    fclose(imageFile);

    imageNumber++;
    return TRUE;
}

uint8_t sd_card_init(bpacket_t* bpacket) {

    // Update the image number
    if (sd_card_search_num_images(&imageNumber, bpacket) != TRUE) {
        return FALSE;
    }

    return TRUE;
}

void sd_card_copy_file(bpacket_t* bpacket, bpacket_char_array_t* bpacketCharArray) {

    // Save the address
    uint8_t address = bpacket->address;

    // Return error message if the SD card cannot be opened
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "SD card failed to open\0");
        esp32_uart_send_bpacket(bpacket);
        // uart_comms_create_packet(responsePacket, UART_ERROR_REQUEST_FAILED, , "\0");
        return;
    }

    char* filePath = bpacketCharArray->string;

    // Reconstruct file path from binary data
    // char filePath[bpacket->numBytes + 1];
    // for (int i = 0; i < bpacket->numBytes; i++) {
    //     filePath[i] = (char)bpacket->bytes[i];
    // }
    // filePath[bpacket->numBytes] = '\0';

    // Return an error if there was no specified file
    if (filePath[0] == '\0') {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, "No file specified\0");
        esp32_uart_send_bpacket(bpacket);
        // uart_comms_create_packet(responsePacket, UART_ERROR_REQUEST_FAILED, "No file was specified", "\0");
        return;
    }

    char fullPath[MAX_PATH_LENGTH];
    sprintf(fullPath, "%s/%s", MOUNT_POINT_PATH, filePath);
    FILE* file = fopen(fullPath, "rb"); // read binary file

    if (file == NULL) {
        bpacket_create_sp(bpacket, address, BPACKET_R_FAILED, strerror(errno));
        esp32_uart_send_bpacket(bpacket);
        return;
    }

    // Get the size of the file in bytes
    fseek(file, 0L, SEEK_END);
    uint32_t fileNumBytes = ftell(file);
    fseek(file, 0L, SEEK_SET);

    bpacket->request  = BPACKET_R_IN_PROGRESS;
    bpacket->numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    int pi            = 0;
    for (uint32_t i = 0; i < fileNumBytes; i++) {

        bpacket->bytes[pi++] = fgetc(file);

        if (pi < BPACKET_MAX_NUM_DATA_BYTES && (i + 1) != fileNumBytes) {
            continue;
        }

        if ((i + 1) == fileNumBytes) {
            bpacket->request  = BPACKET_R_SUCCESS;
            bpacket->numBytes = pi--;
        }

        esp32_uart_send_bpacket(bpacket);
        pi = 0;
    }

    fclose(file);

    // Close the SD card
    sd_card_close();
}

/* GOOD FUNCTIONS */

uint8_t sd_card_open(void) {

    // Return if the sd card has already been mounted
    if (mounted != FALSE) {
        return TRUE;
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width               = 1;

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // Mount the SD card
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT_PATH, &host, &slot_config, &sdCardConfiguration, &card);

    if (ret != ESP_OK) {

        if (ret == ESP_FAIL) {
            ESP_LOGE(SD_CARD_TAG, "Please reformat SD card");
            return SD_CARD_ERROR_UNREADABLE_CARD;
        }

        ESP_LOGE(SD_CARD_TAG, "SD card could not be connected");
        return SD_CARD_ERROR_CONNECTION_FAILURE;
    }

    // Log SD Card information
    char sdCardLog[LOG_MSG_MAX_CHARACTERS];
    if (card->max_freq_khz < 1000) {
        sprintf(sdCardLog, "SD CARD mounted. Speed: %dkhz\t", card->max_freq_khz);
    } else {
        sprintf(sdCardLog, "SD CARD mounted. Speed: %dMhz\t", card->max_freq_khz / 1000);
    }

    int returnCode = sd_card_log(SYSTEM_LOG_FILE, sdCardLog);
    if (returnCode != TRUE) {
        if (returnCode == SD_CARD_ERROR_IO_ERROR) {
            ESP_LOGE(SD_CARD_TAG, "Could not open file to write");
        }

        if (returnCode == LOG_ERR_MSG_TOO_LONG) {
            ESP_LOGE(SD_CARD_TAG, "Message was too long!");
        }
    }

    mounted = TRUE;
    return TRUE;
}

void sd_card_close(void) {

    if (mounted != TRUE) {
        return;
    }

    // Log the SD card being disabled
    char msg[40];
    sprintf(msg, "Unmounting SD card");
    sd_card_log(SYSTEM_LOG_FILE, msg);

    // All done, unmount partition and disable SDMMC peripheral
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT_PATH, card);
    // ESP_LOGI(SD_CARD_TAG, "Card unmounted");
    mounted = FALSE;
}

uint8_t sd_card_check_file_path_exists(char* filePath) {

    // Get the length of the string
    int length = 0;
    char c;
    while ((c = filePath[length]) && (c != '\0')) {
        length++;
    }

    // Loop through file path string. Confirming each directory exists
    // as the file path string is being looped through. If a directory
    // doesn't exist, it is created immediatley
    char copyFilePath[length + 1];
    for (int i = 0; i < length; i++) {

        // Copy character from file path
        copyFilePath[i] = filePath[i];

        if (filePath[i + 1] == '/' || i == (length - 1)) {
            copyFilePath[i + 1] = '\0'; // Add null terminator to string
            if (sd_card_check_directory_exists(copyFilePath) != TRUE) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

uint8_t sd_card_check_directory_exists(char* directory) {

    // Structure to store infromation about directory. We do not need the information
    // but this struct is required for the function call
    struct stat st = {0};

    // if stat returns 0 then the directory was able to be found
    if (stat(directory, &st) == 0) {
        return TRUE;
    }

    // Try to create the directory. mkdir() returning 0 => directory was created
    if (mkdir(directory, 0700) == 0) {
        return TRUE;
    }

    ESP_LOGE(SD_CARD_TAG, "Error trying to create directory %s. Error: '%s'", directory, strerror(errno));
    return FALSE;
}

uint8_t sd_card_write(char* filePath, char* fileName, char* message) {

    // Log to console that data is being written to this file name
    // ESP_LOGI(SD_CARD_TAG, "Writing data to %s", filePath);

    if (sd_card_check_file_path_exists(filePath) != TRUE) {
        return SD_CARD_ERROR_IO_ERROR;
    }

    // Open the given file path for appending (a+). If the file does not exist
    // fopen will create a new file with the given file path. Note fopen() can
    // not make new directories!
    char filePathName[100];
    sprintf(filePathName, "%s/%s", filePath, fileName);
    FILE* file = fopen(filePathName, "a+");

    // Confirm the file was opened/created correctly
    if (file == NULL) {
        ESP_LOGE(SD_CARD_TAG, "Could not open/create %s", filePathName);
        return SD_CARD_ERROR_IO_ERROR;
    }

    // Get the time from the real time clock
    char formattedDateTime[20];
    sprintf(formattedDateTime, "1/1/2023 9:00am");

    // Confirm the message is within character limit. The character
    // limit can be increased if necessary. To do so, change the
    // maximum character limit macro
    int length = 0;
    while (message[length++] != '\0') {
        if (length > LOG_MSG_MAX_CHARACTERS) {
            ESP_LOGE(SD_CARD_TAG, "Message '%s' is too long", message);
            return LOG_ERR_MSG_TOO_LONG;
        }
    }

    // Append the time to the message to create log that will be written to file
    int msgLength = 20 + LOG_MSG_MAX_CHARACTERS + NULL_CHAR_LENGTH + 5;
    char log[msgLength];
    sprintf(log, "%s\t%s\n", formattedDateTime, message);

    // Write log to file
    fprintf(file, log);

    fclose(file);

    return TRUE;
}

uint8_t sd_card_log(char* fileName, char* message) {
    char name[30];
    sprintf(name, "%s", fileName); // Doing this so \0 is added to the end
    return sd_card_write(ROOT_LOG_FOLDER_PATH, name, message);

    // // Print log to console
    // ESP_LOGI("LOG", "%s", message);

    // if (sd_card_check_file_path_exists(ROOT_LOG_FOLDER_PATH) != TRUE) {
    //     return SD_CARD_ERROR_IO_ERROR;
    // }

    // // Open the given file path for appending (a+). If the file does not exist
    // // fopen will create a new file with the given file path. Note fopen() can
    // // not make new directories!
    // char filePath[100];
    // sprintf(filePath, "%s/%s", ROOT_LOG_FOLDER_PATH, fileName);
    // FILE* file = fopen(filePath, "a+");

    // // Confirm the file was opened/created correctly
    // if (file == NULL) {
    //     ESP_LOGE(SD_CARD_TAG, "Could not open/create %s", filePath);
    //     return SD_CARD_ERROR_IO_ERROR;
    // }

    // // Get the time from the real time clock
    // char formattedDateTime[20];
    // rtc_get_formatted_date_time(formattedDateTime);

    // // Confirm the message is within character limit. The character
    // // limit can be increased if necessary. To do so, change the
    // // maximum character limit macro
    // int length = 0;
    // while (message[length++] != '\0') {
    //     if (length > LOG_MSG_MAX_CHARACTERS) {
    //         ESP_LOGE(SD_CARD_TAG, "Message '%s' is too long", message);
    //         return LOG_ERR_MSG_TOO_LONG;
    //     }
    // }

    // // Append the time to the message to create log that will be written to file
    // int msgLength = RTC_DATE_TIME_CHAR_LENGTH + LOG_MSG_MAX_CHARACTERS + NULL_CHAR_LENGTH + 5;
    // char log[msgLength];
    // sprintf(log, "%s\t%s\n", formattedDateTime, message);

    // // Write log to file
    // fprintf(file, log);

    // fclose(file);

    // return TRUE;
}