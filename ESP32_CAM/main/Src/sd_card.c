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
#include "ds18b20.h"
#include "datetime.h"

#include "uart_comms.h"
#include "watchdog_defines.h"

/* Private Macros */

#define LOG_MSG_MAX_CHARACTERS 200
#define NULL_CHAR_LENGTH       1
#define DATE_TIME_DELIMITER    '_'
#define IMG_NUM_START_INDEX    3
#define IMG_NUM_END_CHARACTER  '-'

#define SD_CARD_FILE_READ  0
#define SD_CARD_FILE_WRITE 1

#define MAX_PATH_LENGTH 280

static const char* SD_CARD_TAG = "SD CARD:";

wd_camera_settings_t deafultCameraSettings = {
    .resolution = WD_CAM_RES_800x600,
};

wd_camera_capture_time_settings_t defaultCaptureTimeSettings = {
    .startTime.second    = 0,
    .startTime.minute    = 0,
    .startTime.hour      = 9,
    .endTime.second      = 0,
    .endTime.minute      = 0,
    .endTime.hour        = 18,
    .intervalTime.second = 0,
    .intervalTime.minute = 0,
    .intervalTime.hour   = 2,
};

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

uint8_t sd_card_open_file(FILE** file, char* filePath, uint8_t code, char* errMsg) {

    char* settings = (code == SD_CARD_FILE_READ) ? "rb\0" : "ab\0";

    if ((*file = fopen(filePath, settings)) != NULL) {
        return TRUE;
    }

    sprintf(errMsg, "Failed to open file %s. Error: '%s'\r\n", filePath, strerror(errno));
    return FALSE;
}

uint8_t sd_card_create_file(char* filePath, char* errMsg) {

    FILE* file;
    if ((file = fopen(filePath, "ab+")) == NULL) {
        sprintf(errMsg, "Failed to open file %s. Error: '%s'\r\n", filePath, strerror(errno));
        return FALSE;
    }

    fclose(file);
    return TRUE;
}

uint8_t sd_card_get_file_size(char* filePath, uint32_t* numBytes, char* errMsg) {

    FILE* file;

    // Try open the filepath in read mode
    if (sd_card_open_file(&file, filePath, SD_CARD_FILE_READ, errMsg) != TRUE) {
        return FALSE;
    }

    // Set pointer to end of the file
    fseek(file, 0L, SEEK_END);

    // Get the file length in bytes
    *numBytes = ftell(file);

    // Set file pointer to start of file
    fseek(file, 0L, SEEK_SET);

    fclose(file);

    return TRUE;
}

uint8_t sd_card_create_path(char* folderPath, bpacket_t* bpacket) {

    // Save the address
    uint8_t request  = bpacket->request;
    uint8_t receiver = bpacket->receiver;
    uint8_t sender   = bpacket->sender;

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "SD card could not open\n");
        return FALSE;
    }

    // Validate the path length
    if (chars_get_num_bytes(folderPath) > MAX_PATH_LENGTH) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "Folder path > 50 characters\n");
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
                    bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR,
                                      "File could not be made\n");
                    return FALSE;
                }

                break;
            }

            // Error if the directory does not exist and a new one could not be made
            if (stat(directory, &st) != 0 && mkdir(directory, 0700) != 0) {
                bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "Dir could not be made\n");
                return FALSE;
            }

            path[i] = '/';
        }
    }

    // For some reason, creating a success packet here would stuff up. Testing showed that the code halted on the
    // line bpacket->sender = sender; I have no idea why this is the case. Simplest fix was to just comment out the
    // code. These result occured when testing the read settings function
    // if (bpacket_create_p(&response, sender, receiver, request, BPACKET_CODE_SUCCESS, 0, NULL) != TRUE) {
    //     bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS,
    //                       "CP: Setting bpacket failed!\r\n\0");
    //     esp32_uart_send_bpacket(&b1);
    // }

    return TRUE;
}

uint8_t sd_card_list_directory(bpacket_t* bpacket, bpacket_char_array_t* bpacketCharArray) {
    bpacket_t b1;
    // Save address
    uint8_t request  = bpacket->request;
    uint8_t receiver = bpacket->receiver;
    uint8_t sender   = bpacket->sender;

    char* folderPath = bpacketCharArray->string;

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "SD card could not open\0");
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    // Validate the path length
    if (chars_get_num_bytes(folderPath) > MAX_PATH_LENGTH) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "Folder path > 50 chars\0");
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    // Append the mounting point of the SD card to the folder path
    char path[MAX_PATH_LENGTH + 9];
    sprintf(path, "%s%s%s", MOUNT_POINT_PATH, folderPath[0] == '\0' ? "" : "/", folderPath);

    DIR* directory;
    directory = opendir(path);
    if (directory == NULL) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "Filepath could not open\0");
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
    bpacket->receiver = sender;
    bpacket->sender   = receiver;
    bpacket->request  = request;
    bpacket->code     = BPACKET_CODE_IN_PROGRESS;
    bpacket->numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    int in            = FALSE; // Variable just to know whether there was anything or not

    bpacket_create_sp(&b1, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS,
                      "DIR: About to list directories\r\n\0");
    esp32_uart_send_bpacket(&b1);
    while ((dirPtr = readdir(directory)) != NULL) {
        in = 1;
        // Copy the name of the folder into the folder structure string
        int k = 0;
        while (dirPtr->d_name[k] != '\0') {
            bpacket->bytes[i++] = dirPtr->d_name[k];

            if (i == BPACKET_MAX_NUM_DATA_BYTES) {
                bpacket_create_sp(&b1, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, BPACKET_GEN_R_MESSAGE,
                                  BPACKET_CODE_SUCCESS, "DIR: BPACKET SENT 1\r\n\0");
                esp32_uart_send_bpacket(&b1);
                esp32_uart_send_bpacket(bpacket);
                i = 0;
            }

            k++;
        }

        bpacket->bytes[i++] = '\r';

        if (i == BPACKET_MAX_NUM_DATA_BYTES) {
            bpacket_create_sp(&b1, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, BPACKET_GEN_R_MESSAGE,
                              BPACKET_CODE_SUCCESS, "DIR: BPACKET SENT 2\r\n\0");
            esp32_uart_send_bpacket(&b1);
            esp32_uart_send_bpacket(bpacket);
            i = 0;
        }

        bpacket->bytes[i++] = '\n';

        if (i == BPACKET_MAX_NUM_DATA_BYTES) {
            bpacket_create_sp(&b1, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, BPACKET_GEN_R_MESSAGE,
                              BPACKET_CODE_SUCCESS, "DIR: BPACKET SENT 3\r\n\0");
            esp32_uart_send_bpacket(&b1);
            esp32_uart_send_bpacket(bpacket);
            i = 0;
        }
    }

    if (i != 0) {
        bpacket->code     = BPACKET_CODE_SUCCESS;
        bpacket->numBytes = i;
        bpacket_create_sp(&b1, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, BPACKET_GEN_R_MESSAGE,
                          BPACKET_CODE_SUCCESS, "DIR: BPACKET SENT 4\r\n\0");
        esp32_uart_send_bpacket(&b1);
        esp32_uart_send_bpacket(bpacket);
    }

    if (in == FALSE) {
        bpacket_create_sp(&b1, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, BPACKET_GEN_R_MESSAGE,
                          BPACKET_CODE_SUCCESS, "DIR: BPACKET SENT 5\r\n\0");
        esp32_uart_send_bpacket(&b1);
        bpacket_create_p(bpacket, sender, receiver, request, BPACKET_CODE_SUCCESS, 0, NULL);
        esp32_uart_send_bpacket(bpacket);
    }

    bpacket_create_sp(&b1, BPACKET_ADDRESS_MAPLE, BPACKET_ADDRESS_ESP32, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS,
                      "DIR: Leaving\r\n\0");
    esp32_uart_send_bpacket(&b1);

    closedir(directory);
    sd_card_close();

    return TRUE;
}

uint8_t sd_card_write_to_file(char* filePath, char* string, bpacket_t* bpacket) {

    // Save the address
    uint8_t request  = request;
    uint8_t receiver = bpacket->receiver;
    uint8_t sender   = bpacket->sender;

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "SD Card could not open\0");
        return FALSE;
    }

    // Validate the path length
    if (chars_get_num_bytes(filePath) > MAX_PATH_LENGTH) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "Filepath > 50 characters\0");
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
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "Filepath could not open\0");
        return FALSE;
    }

    fprintf(file, string);
    fclose(file);

    bpacket_create_p(bpacket, sender, receiver, request, BPACKET_CODE_SUCCESS, 0, NULL);
    return TRUE;
}

uint8_t sd_card_search_num_images(uint16_t* numImages, bpacket_t* bpacket) {

    // Save the address
    uint8_t request  = bpacket->request;
    uint8_t receiver = bpacket->receiver;
    uint8_t sender   = bpacket->sender;

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
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "Img dir could not open\0");
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
    uint8_t request  = bpacket->request;
    uint8_t receiver = bpacket->receiver;
    uint8_t sender   = bpacket->sender;

    // Extract the RTC and temperatre data from the bpacket
    ds18b20_temp_t temp1, temp2;
    dt_datetime_t datetime;
    uint8_t result = wd_bpacket_to_photo_data(bpacket, &datetime, &temp1, &temp2);

    if (result != TRUE) {
        char err[50];
        wd_get_error(result, err);
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_IN_PROGRESS, err);
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "Could not open the SD card\0");
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    // Create the data folder path if required
    if (sd_card_create_path(IMAGE_DATA_FOLDER, bpacket) != TRUE) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR,
                          "Could not create path to data folder\0");
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    // Create formatted image number in the format of xxx
    char imgNumString[8];
    sprintf(imgNumString, "%s%s%i", (imageNumber < 100) ? "0" : "", (imageNumber < 10) ? "0" : "", imageNumber);

    // Create formatted date time in the format yymmdd_hhmm
    char datetimeString[40];
    sprintf(datetimeString, "%i%s%i%s%i_%s%i%s%i", datetime.date.year - 2000, datetime.date.month < 10 ? "0" : "",
            datetime.date.month, datetime.date.day < 10 ? "0" : "", datetime.date.day,
            datetime.time.hour < 10 ? "0" : "", datetime.time.hour, datetime.time.minute < 10 ? "0" : "",
            datetime.time.minute);

    // Create path for image
    char filePath[80];
    sprintf(filePath, "%s/%s/img%s_%s.jpg", MOUNT_POINT_PATH, IMAGE_DATA_FOLDER, imgNumString, datetimeString);

    FILE* imageFile = fopen(filePath, "wb");
    if (imageFile == NULL) {
        bpacket_create_sp(bpacket, sender, receiver, request, BPACKET_CODE_ERROR, "Image file failed to open\0");
        esp32_uart_send_bpacket(bpacket);
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
    uint8_t receiver = bpacket->receiver;
    uint8_t sender   = bpacket->sender;

    bpacket_t b1;
    char j[305];

    // Return error message if the SD card cannot be opened
    if (sd_card_open() != TRUE) {

        sprintf(j, "SD Card did not open\r\n");
        bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS, j);
        esp32_uart_send_bpacket(&b1);

        bpacket_create_sp(bpacket, sender, receiver, WATCHDOG_BPK_R_COPY_FILE, BPACKET_CODE_ERROR,
                          "SD card failed to open\0");
        esp32_uart_send_bpacket(bpacket);
        return;
    }

    // Return an error if there was no specified file
    if (bpacketCharArray->string[0] == '\0') {

        sprintf(j, "bpacket array was empty: [%s]\r\n", bpacketCharArray->string);
        bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS, j);
        esp32_uart_send_bpacket(&b1);

        bpacket_create_sp(bpacket, sender, receiver, WATCHDOG_BPK_R_COPY_FILE, BPACKET_CODE_ERROR,
                          "No file was specified\0");
        esp32_uart_send_bpacket(bpacket);
        return;
    }

    // Max file length is 50
    uint16_t filePathNameSize = chars_get_num_bytes(bpacketCharArray->string);

    if (filePathNameSize > (57)) { // including mount point which is current 7 bytes

        sprintf(j, "file name size > 57 [%s] = %i\r\n", bpacketCharArray->string,
                chars_get_num_bytes(bpacketCharArray->string));
        bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS, j);
        esp32_uart_send_bpacket(&b1);

        bpacket_create_sp(bpacket, sender, receiver, WATCHDOG_BPK_R_COPY_FILE, BPACKET_CODE_ERROR, "File path > 50\0");
        esp32_uart_send_bpacket(bpacket);
        return;
    }

    sprintf(j, "Creating file path\r\n");
    bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS, j);
    esp32_uart_send_bpacket(&b1);

    char filePath[filePathNameSize + 1]; // add one for null pointer!
    sprintf(filePath, "%s/", MOUNT_POINT_PATH);
    int i = 0;
    for (i = 0; i < filePathNameSize; i++) {
        filePath[i + 8] = bpacketCharArray->string[i];
    }
    filePath[i + 8] = '\0'; // Add null terminator

    sprintf(j, "File path made: [%s]\r\n", filePath);
    bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS, j);
    esp32_uart_send_bpacket(&b1);
    // char* filePath = bpacketCharArray->string;

    // char fullPath[MAX_PATH_LENGTH];
    // sprintf(fullPath, "%s/%s", MOUNT_POINT_PATH, filePath);
    FILE* file = fopen(filePath, "rb"); // read binary file

    if (file == NULL) {

        sprintf(j, "Couldn't open file [%s]\r\n", filePath);
        bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS, j);
        esp32_uart_send_bpacket(&b1);

        char msg[100];
        sprintf(msg, "File was NULL: [%i] [%s] => [%s]\r\n", filePathNameSize, filePath, strerror(errno));
        bpacket_create_sp(bpacket, sender, receiver, WATCHDOG_BPK_R_COPY_FILE, BPACKET_CODE_ERROR, msg);
        esp32_uart_send_bpacket(bpacket);
        return;
    }

    // Get the size of the file in bytes
    fseek(file, 0L, SEEK_END);
    uint32_t fileNumBytes = ftell(file);
    fseek(file, 0L, SEEK_SET);

    sprintf(j, "Image size: %i bytes\r\n", fileNumBytes);
    bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS, j);
    esp32_uart_send_bpacket(&b1);

    // Change the values of the bpacket so they get sent to Maple
    bpacket->sender   = receiver;
    bpacket->receiver = sender;
    bpacket->code     = BPACKET_CODE_IN_PROGRESS;
    bpacket->numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    int pi            = 0;

    char info[50];
    bpacket_get_info(bpacket, info);
    sprintf(j, "Image size: %i bytes\r\n", fileNumBytes);
    bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS, j);
    esp32_uart_send_bpacket(&b1);

    for (uint32_t i = 0; i < fileNumBytes; i++) {

        bpacket->bytes[pi++] = fgetc(file);

        if (pi < BPACKET_MAX_NUM_DATA_BYTES && (i + 1) != fileNumBytes) {
            continue;
        }

        if ((i + 1) == fileNumBytes) {
            bpacket->code     = BPACKET_CODE_SUCCESS;
            bpacket->numBytes = pi--;
        }

        esp32_uart_send_bpacket(bpacket);

        pi = 0;
    }

    sprintf(j, "Image fully sent. Closeing file\r\n");
    bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS, j);
    esp32_uart_send_bpacket(&b1);

    fclose(file);

    // Close the SD card
    sd_card_close();
}

uint8_t sd_card_write_settings(bpacket_t* bpacket) {

    // Return error message if the SD card cannot be opened
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                          "SD card failed to open\r\n\0");
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    FILE* file;
    char errMsg[50];
    if (sd_card_open_file(&file, SETTINGS_FILE_PATH_START_AT_ROOT, SD_CARD_FILE_WRITE, errMsg) != TRUE) {
        bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR, errMsg);
        esp32_uart_send_bpacket(bpacket);
        sd_card_close();
        return FALSE;
    }

    switch (bpacket->request) {

        case WATCHDOG_BPK_R_SET_CAMERA_SETTINGS:;

            wd_camera_settings_t cameraSettings;
            if (wd_bpacket_to_camera_settings(bpacket, &cameraSettings) != TRUE) {
                bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                                  "Bpacket to camera settings failed. SD Card write attempt\r\n\0");
                esp32_uart_send_bpacket(bpacket);
                // Clean up
                fclose(file);
                sd_card_close();
                return FALSE;
            }

            fseek(file, 0, SEEK_SET); // Ensure the file is pointing to first byte
            fputc(cameraSettings.resolution, file);

            break;

        case WATCHDOG_BPK_R_SET_CAPTURE_TIME_SETTINGS:;

            wd_camera_capture_time_settings_t captureTime;
            if (wd_bpacket_to_capture_time_settings(bpacket, &captureTime) != TRUE) {
                bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                                  "Bpacket to capture time settings failed. SD Card write attempt\r\n\0");
                // Clean up
                fclose(file);
                sd_card_close();
                return FALSE;
            }

            // Skip the first byte as this is where the camera resolution is stored
            fseek(file, 1, SEEK_CUR);
            fputc(captureTime.startTime.minute, file);
            fputc(captureTime.startTime.hour, file);
            fputc(captureTime.endTime.minute, file);
            fputc(captureTime.endTime.hour, file);
            fputc(captureTime.intervalTime.minute, file);
            fputc(captureTime.intervalTime.hour, file);

            break;

        default:;

            bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                              "WF: Invalid request!\r\n\0");
            esp32_uart_send_bpacket(bpacket);
            return FALSE;
    }

    // Clean up
    fclose(file);
    sd_card_close();

    uint8_t receiver = bpacket->receiver;
    uint8_t sender   = bpacket->sender;

    // Update bpacket to send as success response back
    bpacket->numBytes = 0;
    bpacket->receiver = sender;
    bpacket->sender   = receiver;
    bpacket->code     = BPACKET_CODE_SUCCESS;

    return TRUE;
}

uint8_t sd_card_format_sd_card(bpacket_t* bpacket) {

    char errMsg[50];
    bpacket_t b1;

    // Return error message if the SD card cannot be opened
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                          "SD card failed to open\r\n\0");
        return FALSE;
    }

    /****** START CODE BLOCK ******/
    // Description: Confirm all the correct folders on the SD card exist

    // Create watchdog folder if necessary
    if (sd_card_create_path(WATCHDOG_FOLDER_PATH, bpacket) != TRUE) {
        sd_card_close();
        return FALSE;
    }

    // Create data folder if necessary
    if (sd_card_create_path(DATA_FOLDER_PATH, bpacket) != TRUE) {
        sd_card_close();
        return FALSE;
    }

    // Create logs folder if necessary
    if (sd_card_create_path(LOG_FOLDER_PATH, bpacket) != TRUE) {
        sd_card_close();
        return FALSE;
    }

    // Create settings folder if necessary
    if (sd_card_create_path(SETTINGS_FOLDER_PATH, bpacket) != TRUE) {
        sd_card_close();
        return FALSE;
    }

    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Confirm the settings file exists and that it has valid data

    // Create the settings file if required
    if (sd_card_create_file(SETTINGS_FILE_PATH_START_AT_ROOT, errMsg) != TRUE) {
        bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_ERROR, errMsg);
        return FALSE;
    }

    uint32_t numBytes = 0;

    // Write default settings to settings file the file is empty
    if (sd_card_get_file_size(SETTINGS_FILE_PATH_START_AT_ROOT, &numBytes, errMsg) != TRUE) {
        bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR, errMsg);
        sd_card_close();
        return FALSE;
    }

    if (numBytes == 0) {

        // Write the default camera settings to the SD card
        bpacket_t cSettings;
        if (wd_camera_settings_to_bpacket(&cSettings, bpacket->receiver, bpacket->sender,
                                          WATCHDOG_BPK_R_SET_CAMERA_SETTINGS, BPACKET_CODE_EXECUTE,
                                          &deafultCameraSettings) != TRUE) {
            bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                              "Failed setting default camera settings\r\n\0");
            sd_card_close();
            return FALSE;
        }

        if (sd_card_write_settings(&cSettings) != TRUE) {
            bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                              "Failed setting default camera settings\r\n\0");
            sd_card_close();
            return FALSE;
        }

        // Write the default capture time settings to the SD card
        bpacket_t ctSettings;
        if (wd_capture_time_settings_to_bpacket(&ctSettings, bpacket->receiver, bpacket->sender,
                                                WATCHDOG_BPK_R_SET_CAPTURE_TIME_SETTINGS, BPACKET_CODE_EXECUTE,
                                                &defaultCaptureTimeSettings) != TRUE) {
            bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                              "Failed setting default capture time settings\r\n\0");
            sd_card_close();
            return FALSE;
        }

        if (sd_card_write_settings(&ctSettings) != TRUE) {
            bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                              "Settings default capture time settings failed\r\n\0");
            sd_card_close();
            return FALSE;
        }
    }

    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Confirm the log and data files exist

    // Create the data file if required
    if (sd_card_create_file(DATA_FILE_PATH_START_AT_ROOT, errMsg) != TRUE) {
        bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_ERROR, errMsg);
        return FALSE;
    }

    // Create the logs file if required
    if (sd_card_create_file(LOG_FILE_PATH_START_AT_ROOT, errMsg) != TRUE) {
        bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_ERROR, errMsg);
        return FALSE;
    }

    /****** END CODE BLOCK ******/

    sd_card_close();

    return TRUE;
}

uint8_t sd_card_read_settings(bpacket_t* bpacket) {

    bpacket_t b1;

    // Return error message if the SD card cannot be opened
    if (sd_card_open() != TRUE) {
        bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                          "SD card failed to open\r\n\0");
        return FALSE;
    }

    // Confirm the file path to the settings exists
    if (sd_card_create_path(SETTINGS_FOLDER_PATH, bpacket) != TRUE) {
        sd_card_close();
        return FALSE;
    }

    // Get the file size of the settings file. This needs to be done first because the file
    // needs to be opened in read mode
    uint32_t fileNumBytes = 0;
    char errMsg[50];
    if (sd_card_get_file_size(SETTINGS_FILE_PATH_START_AT_ROOT, &fileNumBytes, errMsg) != TRUE) {
        bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR, errMsg);
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    if (fileNumBytes == 0) {

        switch (bpacket->request) {

            case WATCHDOG_BPK_R_GET_CAMERA_SETTINGS:;

                bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS,
                                  "RF: 0 bytes req was camera settings\r\n\0");
                esp32_uart_send_bpacket(&b1);

                bpacket_t camSettingsBpacket;
                if (wd_camera_settings_to_bpacket(&camSettingsBpacket, bpacket->receiver, bpacket->sender,
                                                  WATCHDOG_BPK_R_SET_CAMERA_SETTINGS, BPACKET_CODE_EXECUTE,
                                                  &deafultCameraSettings) != TRUE) {
                    bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                                      "Setting deafult camera settings failed\r\n\0");
                    esp32_uart_send_bpacket(bpacket);
                    return FALSE;
                }

                if (sd_card_write_settings(&camSettingsBpacket) != TRUE) {
                    bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE,
                                      BPACKET_CODE_SUCCESS, "FAILED TO WRITE SETTINGS CORRECTLY\r\n\0");
                    esp32_uart_send_bpacket(&b1);
                } else {
                    bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE,
                                      BPACKET_CODE_SUCCESS, "WROTE SETTINGS CORECTLY\r\n\0");
                    esp32_uart_send_bpacket(&b1);
                }
                break;

            case WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS:;

                bpacket_t deafultCaptureTimeSettingsBpacket;

                if (wd_capture_time_settings_to_bpacket(&deafultCaptureTimeSettingsBpacket, bpacket->receiver,
                                                        bpacket->sender, bpacket->request, BPACKET_CODE_EXECUTE,
                                                        &defaultCaptureTimeSettings) != TRUE) {
                    bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                                      "Setting deafult camera settings failed\r\n\0");
                    esp32_uart_send_bpacket(bpacket);
                    return FALSE;
                }

                sd_card_write_settings(&deafultCaptureTimeSettingsBpacket);

                break;

            default:;

                bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                                  "RF: Invalid request!\r\n\0");
                esp32_uart_send_bpacket(bpacket);
                return FALSE;
        }

        // Open the SD card again
        if (sd_card_open() != TRUE) {
            bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR,
                              "SD card failed to open\r\n\0");
            return FALSE;
        }
    }

    FILE* file;
    if (sd_card_open_file(&file, SETTINGS_FILE_PATH_START_AT_ROOT, SD_CARD_FILE_READ, errMsg) != TRUE) {
        bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR, errMsg);
        esp32_uart_send_bpacket(bpacket);
        return FALSE;
    }

    bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS,
                      "RF: File Created\r\n\0");
    esp32_uart_send_bpacket(&b1);

    // if (bpacket->request == WATCHDOG_BPK_R_GET_CAMERA_SETTINGS) {

    //     bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS,
    //                       "RF: Request get camera settings\r\n\0");
    //     esp32_uart_send_bpacket(&b1);

    //     wd_camera_settings_t cameraSettings;
    //     cameraSettings.resolution = fgetc(file);

    //     uint8_t result = wd_camera_settings_to_bpacket(bpacket, bpacket->sender, bpacket->receiver, bpacket->request,
    //                                                    BPACKET_CODE_SUCCESS, &cameraSettings);
    //     if (result != TRUE) {
    //         char errMsg[50];
    //         wd_get_error(result, errMsg);
    //         char j[80];
    //         sprintf(j, "FAIELD CRES: %i -> %i %s\r\n", cameraSettings.resolution, result, errMsg);
    //         bpacket_create_sp(bpacket, bpacket->sender, bpacket->receiver, bpacket->request, BPACKET_CODE_ERROR, j);
    //         esp32_uart_send_bpacket(bpacket);

    //         // Clean up
    //         fclose(file);
    //         sd_card_close();
    //         return FALSE;
    //     }

    //     bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS,
    //                       "RF: Converted settings to bpacket\r\n\0");
    //     esp32_uart_send_bpacket(&b1);
    // }

    // if (bpacket->request ==
    // WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS) {

    //     bpacket_create_sp(&b1, bpacket->sender,
    //     bpacket->receiver, BPACKET_GEN_R_MESSAGE,
    //     BPACKET_CODE_SUCCESS,
    //                       "RF: Request get capture
    //                       time\r\n\0");
    //     esp32_uart_send_bpacket(&b1);

    //     wd_camera_capture_time_settings_t captureTime;
    //     captureTime.startTime.minute    = fgetc(file);
    //     captureTime.startTime.hour      = fgetc(file);
    //     captureTime.endTime.minute      = fgetc(file);
    //     captureTime.endTime.hour        = fgetc(file);
    //     captureTime.intervalTime.minute = fgetc(file);
    //     captureTime.intervalTime.hour   = fgetc(file);

    //     if (wd_capture_time_settings_to_bpacket(bpacket,
    //     bpacket->sender, bpacket->receiver,
    //     bpacket->request,
    //                                             BPACKET_CODE_SUCCESS,
    //                                             &captureTime)
    //                                             != TRUE)
    //                                             {
    //         bpacket_create_sp(bpacket, bpacket->sender,
    //         bpacket->receiver, bpacket->request,
    //         BPACKET_CODE_ERROR,
    //                           "Capture time settings to
    //                           bpacket failed. SD Card
    //                           read\r\n\0");

    //         // Clean up
    //         fclose(file);
    //         sd_card_close();
    //         return FALSE;
    //     }

    //     bpacket_create_sp(&b1, bpacket->sender,
    //     bpacket->receiver, BPACKET_GEN_R_MESSAGE,
    //     BPACKET_CODE_SUCCESS,
    //                       "Capture time read\r\n\0");
    //     esp32_uart_send_bpacket(&b1);
    // }

    // Clean up
    fclose(file);
    sd_card_close();

    bpacket_create_sp(&b1, bpacket->sender, bpacket->receiver, BPACKET_GEN_R_MESSAGE, BPACKET_CODE_SUCCESS,
                      "RF: Cleaning up\r\n\0");
    esp32_uart_send_bpacket(&b1);

    return TRUE;
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