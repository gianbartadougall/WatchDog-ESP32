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
#include "sd_card.h"
#include "chars.h"
#include "hardware_config.h"
#include "esp32_uart.h"
#include "datetime.h"
#include "watchdog_utils.h"

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

uint8_t sd_card_open_file(FILE** file, char* filePath, uint8_t sdCardCode, char* errMsg) {

    char* settings = (sdCardCode == SD_CARD_FILE_READ) ? "rb\0" : "r+b\0";

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

uint8_t sd_card_create_path(char* folderPath, bpk_t* Bpacket) {

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "SD card could not open\n");
        return FALSE;
    }

    // Validate the path length
    if (chars_get_num_bytes(folderPath) > MAX_PATH_LENGTH) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Folder path > 50 characters\n");
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
                    bpk_create_string_response(Bpacket, BPK_Code_Error, "File could not be made\n");
                    return FALSE;
                }

                break;
            }

            // Error if the directory does not exist and a new one could not be made
            if (stat(directory, &st) != 0 && mkdir(directory, 0700) != 0) {
                bpk_create_string_response(Bpacket, BPK_Code_Error, "Dir could not be made\n");
                return FALSE;
            }

            path[i] = '/';
        }
    }

    return TRUE;
}

uint8_t sd_card_list_dir(char* msg) {

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        sprintf(msg, "SD card could not open");
        return FALSE;
    }

    DIR* directory;
    directory = opendir(DATA_FOLDER_PATH_START_AT_ROOT);
    if (directory == NULL) {
        sprintf(msg, "Dir %s could not be opened", DATA_FOLDER_PATH_START_AT_ROOT);
        return FALSE;
    }

    // Loop through directory
    bpk_t Bpacket;
    Bpacket.Receiver = BPK_Addr_Receive_Stm32;
    Bpacket.Sender   = BPK_Addr_Send_Esp32;
    Bpacket.Request  = BPK_Req_List_Dir;
    Bpacket.Code     = BPK_Code_In_Progress;

    uint8_t index = 0;
    struct dirent* dirPtr;
    while ((dirPtr = readdir(directory)) != NULL) {

        // Copy the name of the folder into the folder structure string
        int k = 0;
        while (dirPtr->d_name[k] != '\0') {
            Bpacket.Data.bytes[index++] = dirPtr->d_name[k++];

            if (index == BPACKET_MAX_NUM_DATA_BYTES) {
                Bpacket.Data.numBytes = index;
                esp32_uart_send_bpacket(&Bpacket);
                index = 0;
            }
        }

        // Add a seperator
        Bpacket.Data.bytes[index++] = ':';

        if (index == BPACKET_MAX_NUM_DATA_BYTES) {
            Bpacket.Data.numBytes = index;
            esp32_uart_send_bpacket(&Bpacket);
            index = 0;
        }
    }

    // Send success bpacket with any remaining data
    Bpacket.Data.numBytes = index;
    Bpacket.Code          = BPK_Code_Success;
    esp32_uart_send_bpacket(&Bpacket);

    closedir(directory);
    sd_card_close();

    return TRUE;
}

uint8_t sd_card_list_directory(bpk_t* Bpacket, bpacket_char_array_t* bpacketCharArray) {

    char* folderPath = bpacketCharArray->string;

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "SD card could not open\0");
        esp32_uart_send_bpacket(Bpacket);
        return FALSE;
    }

    // Validate the path length
    if (chars_get_num_bytes(folderPath) > MAX_PATH_LENGTH) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Folder path > 50 chars\0");
        esp32_uart_send_bpacket(Bpacket);
        return FALSE;
    }

    // Append the mounting point of the SD card to the folder path
    char path[MAX_PATH_LENGTH + 9];
    sprintf(path, "%s%s%s", MOUNT_POINT_PATH, folderPath[0] == '\0' ? "" : "/", folderPath);

    DIR* directory;
    directory = opendir(path);
    if (directory == NULL) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Filepath could not open\0");
        esp32_uart_send_bpacket(Bpacket);
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

    int i = 0;
    bpk_swap_address(Bpacket);
    Bpacket->Code          = BPK_Code_In_Progress;
    Bpacket->Data.numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    int in                 = FALSE; // Variable just to know whether there was anything or not
    while ((dirPtr = readdir(directory)) != NULL) {
        in = 1;
        // Copy the name of the folder into the folder structure string
        int k = 0;
        while (dirPtr->d_name[k] != '\0') {
            Bpacket->Data.bytes[i++] = dirPtr->d_name[k];

            if (i == BPACKET_MAX_NUM_DATA_BYTES) {
                esp32_uart_send_bpacket(Bpacket);
                i = 0;
            }

            k++;
        }

        Bpacket->Data.bytes[i++] = '\r';

        if (i == BPACKET_MAX_NUM_DATA_BYTES) {
            esp32_uart_send_bpacket(Bpacket);
            i = 0;
        }

        Bpacket->Data.bytes[i++] = '\n';

        if (i == BPACKET_MAX_NUM_DATA_BYTES) {
            esp32_uart_send_bpacket(Bpacket);
            i = 0;
        }
    }

    if (i != 0) {
        Bpacket->Code          = BPK_Code_Success;
        Bpacket->Data.numBytes = i;
        esp32_uart_send_bpacket(Bpacket);
    }

    if (in == FALSE) {
        bpk_convert_to_response(Bpacket, BPK_Code_Success, 0, NULL);
        esp32_uart_send_bpacket(Bpacket);
    }

    closedir(directory);
    sd_card_close();

    return TRUE;
}

uint8_t sd_card_write_to_file(char* filePath, char* string, bpk_t* Bpacket) {

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "SD Card could not open\0");
        return FALSE;
    }

    // Validate the path length
    if (chars_get_num_bytes(filePath) > MAX_PATH_LENGTH) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Filepath > 50 characters\0");
        return FALSE;
    }

    // Add SD card mount point to start of path
    char directory[MAX_PATH_LENGTH + 9];
    sprintf(directory, "%s/%s", MOUNT_POINT_PATH, filePath);

    if (sd_card_create_path(directory, Bpacket) != TRUE) {
        return FALSE;
    }

    // Try open the file
    FILE* file = fopen(directory, "a+");

    if (file == NULL) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Filepath could not open\0");
        return FALSE;
    }

    fprintf(file, string);
    fclose(file);

    bpk_convert_to_response(Bpacket, BPK_Code_Success, 0, NULL);
    return TRUE;
}

uint8_t sd_card_search_num_images(uint16_t* numImages, bpk_t* Bpacket) {

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        return FALSE;
    }

    // Confirm the directory for the images exists
    if (sd_card_create_path(DATA_FOLDER_PATH, Bpacket) != TRUE) {
        return FALSE;
    }

    // Open the directory where the images are stored
    DIR* directory;
    directory = opendir(DATA_FOLDER_PATH_START_AT_ROOT);
    if (directory == NULL) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Img dir could not open\0");
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

uint8_t sd_card_add_temp_to_csv(dt_datetime_t Datetime, float temp1, float temp2) {
    // TODO: redo
    // FILE* excelFile;
    // if ((excelFile = fopen(DATA_FILE_PATH_START_AT_ROOT, "r")) == NULL) {
    //     if ((excelFile = fopen(DATA_FILE_PATH_START_AT_ROOT, "w")) == NULL) {
    //         printf("Probelm creating the csv file");
    //         return FALSE;
    //     }
    //     fprintf(excelFile, "Date, Time, Temp 1, Temp 2");
    // } else {
    //     fclose(excelFile);
    //     if ((excelFile = fopen(DATA_FILE_PATH_START_AT_ROOT, "a")) == NULL) {
    //         printf("Probelm opeing the csv file");
    //         return FALSE;
    //     }
    // }
    // char timeString[10];
    // dt_time_to_string(timeString, Datetime.Time, TRUE);
    // fprintf(excelFile, "\r%i/%i/%i, %s, %s%i.%i, %s%i.%i", Datetime.Date.day, Datetime.Date.month,
    // Datetime.Date.year,
    //         timeString, Temp1.info == 0 ? "" : "-", Temp1.integer, Temp1.decimal, Temp2.info == 0 ? "" : "-",
    //         Temp2.integer, Temp2.decimal);

    // fclose(excelFile);

    return TRUE;
}

uint8_t sd_card_save_image(uint8_t* imageData, int imageLength, dt_datetime_t* Datetime, float temperature,
                           char* returnMsg) {

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        sprintf(returnMsg, "Could not open the SD card");
        return FALSE;
    }

    // Create formatted image number in the format of xxx
    char imgNumString[8];
    sprintf(imgNumString, "%s%s%i", (imageNumber < 100) ? "0" : "", (imageNumber < 10) ? "0" : "", imageNumber);

    // Create formatted date time in the format yymmdd_hhmm
    char datetimeString[40];
    sprintf(datetimeString, "%i%s%i%s%i_%s%i%s%i", Datetime->Date.year - 2000, Datetime->Date.month < 10 ? "0" : "",
            Datetime->Date.month, Datetime->Date.day < 10 ? "0" : "", Datetime->Date.day,
            Datetime->Time.hour < 10 ? "0" : "", Datetime->Time.hour, Datetime->Time.minute < 10 ? "0" : "",
            Datetime->Time.minute);

    // Create path for image
    char filePath[80];
    sprintf(filePath, "%s/%s/img%s_%s.jpg", MOUNT_POINT_PATH, DATA_FOLDER_PATH, imgNumString, datetimeString);

    FILE* imageFile = fopen(filePath, "wb");
    if (imageFile == NULL) {
        sprintf(returnMsg, "Image file failed to open");
        return FALSE;
    }

    for (int i = 0; i < imageLength; i++) {
        fputc(imageData[i], imageFile);
    }

    fclose(imageFile);

    imageNumber++;

    // Save the temperature data
    // if (sd_card_add_temp_to_csv(DateTime, Temp1, Temp2) != TRUE) {
    //     bpk_create_string_response(Bpacket, BPK_Code_Error, "Failed to save temperature data\0");
    //     esp32_uart_send_bpacket(Bpacket);
    //     return FALSE;
    // }

    return TRUE;
}

uint8_t sd_card_save_image1(uint8_t* imageData, int imageLength, bpk_t* Bpacket) {

    // Extract the RTC and temperatre data from the Bpacket
    // cdt_dbl_16_t Temp1, Temp2;
    dt_datetime_t DateTime = {
        .Date.day    = 1,
        .Date.month  = 2,
        .Date.year   = 3,
        .Time.second = 1,
        .Time.minute = 1,
        .Time.hour   = 1,
    };
    // uint8_t result = wd_bpacket_to_photo_data(Bpacket, &DateTime, &Temp1, &Temp2);

    // if (result != TRUE) {
    //     char err[50];
    //     wd_get_error(result, err);
    //     bpk_create_string_response(Bpacket, BPK_Code_Error, err);
    //     esp32_uart_send_bpacket(Bpacket);
    //     return FALSE;
    // }

    // Try open the SD card
    if (sd_card_open() != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Could not open the SD card\0");
        esp32_uart_send_bpacket(Bpacket);
        return FALSE;
    }

    // // Create the data folder path if required
    // if (sd_card_create_path(DATA_FOLDER_PATH, Bpacket) != TRUE) {
    //     bpk_create_sp(Bpacket, sender, Receiver, Request, BPK_Code_Error,
    //                       "Could not create path to data folder\0");
    //     esp32_uart_send_bpacket(Bpacket);
    //     return FALSE;
    // }

    // Create formatted image number in the format of xxx
    char imgNumString[8];
    sprintf(imgNumString, "%s%s%i", (imageNumber < 100) ? "0" : "", (imageNumber < 10) ? "0" : "", imageNumber);

    // Create formatted date time in the format yymmdd_hhmm
    char datetimeString[40];
    sprintf(datetimeString, "%i%s%i%s%i_%s%i%s%i", DateTime.Date.year - 2000, DateTime.Date.month < 10 ? "0" : "",
            DateTime.Date.month, DateTime.Date.day < 10 ? "0" : "", DateTime.Date.day,
            DateTime.Time.hour < 10 ? "0" : "", DateTime.Time.hour, DateTime.Time.minute < 10 ? "0" : "",
            DateTime.Time.minute);

    // Create path for image
    char filePath[80];
    sprintf(filePath, "%s/%s/img%s_%s.jpg", MOUNT_POINT_PATH, DATA_FOLDER_PATH, imgNumString, datetimeString);

    FILE* imageFile = fopen(filePath, "wb");
    if (imageFile == NULL) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "Image file failed to open\0");
        esp32_uart_send_bpacket(Bpacket);
        return FALSE;
    }

    for (int i = 0; i < imageLength; i++) {
        fputc(imageData[i], imageFile);
    }

    fclose(imageFile);

    imageNumber++;

    // Save the temperature data
    // if (sd_card_add_temp_to_csv(DateTime, Temp1, Temp2) != TRUE) {
    //     bpk_create_string_response(Bpacket, BPK_Code_Error, "Failed to save temperature data\0");
    //     esp32_uart_send_bpacket(Bpacket);
    //     return FALSE;
    // }

    return TRUE;
}

uint8_t sd_card_init(bpk_t* Bpacket) {

    // Update the image number
    if (sd_card_search_num_images(&imageNumber, Bpacket) != TRUE) {
        return FALSE;
    }

    return TRUE;
}

void sd_card_copy_file(bpk_t* Bpacket, bpacket_char_array_t* bpacketCharArray) {

    // Return error message if the SD card cannot be opened
    if (sd_card_open() != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "SD card failed to open\0");
        esp32_uart_send_bpacket(Bpacket);
        return;
    }

    // Return an error if there was no specified file
    if (bpacketCharArray->string[0] == '\0') {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "No file was specified\0");
        esp32_uart_send_bpacket(Bpacket);
        return;
    }

    // Max file length is 50
    uint16_t filePathNameSize = chars_get_num_bytes(bpacketCharArray->string);

    if (filePathNameSize > (57)) { // including mount point which is current 7 bytes
        bpk_create_string_response(Bpacket, BPK_Code_Error, "File path > 50\0");
        esp32_uart_send_bpacket(Bpacket);
        return;
    }

    char filePath[filePathNameSize + 1]; // add one for null pointer!
    sprintf(filePath, "%s%s", MOUNT_POINT_PATH, bpacketCharArray->string);
    int i = 0;
    for (i = 0; i < filePathNameSize; i++) {
        filePath[i + 8] = bpacketCharArray->string[i];
    }
    filePath[i + 8] = '\0'; // Add null terminator

    // Get the length of the file
    uint32_t fileNumBytes;
    char errMsg[50];
    if (sd_card_get_file_size(filePath, &fileNumBytes, errMsg) != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, errMsg);
        esp32_uart_send_bpacket(Bpacket);
        sd_card_close();
        return;
    }

    FILE* file;
    if (sd_card_open_file(&file, filePath, SD_CARD_FILE_READ, errMsg) != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, errMsg);
        esp32_uart_send_bpacket(Bpacket);
        sd_card_close();
        return;
    }

    // Change the values of the Bpacket so they get sent to Maple
    bpk_swap_address(Bpacket);
    Bpacket->Code          = BPK_Code_In_Progress;
    Bpacket->Data.numBytes = BPACKET_MAX_NUM_DATA_BYTES;
    int pi                 = 0;

    for (uint32_t i = 0; i < fileNumBytes; i++) {

        Bpacket->Data.bytes[pi++] = fgetc(file);

        if (pi < BPACKET_MAX_NUM_DATA_BYTES && (i + 1) != fileNumBytes) {
            continue;
        }

        if ((i + 1) == fileNumBytes) {
            Bpacket->Code          = BPK_Code_Success;
            Bpacket->Data.numBytes = pi--;
        }

        esp32_uart_send_bpacket(Bpacket);

        pi = 0;
    }

    fclose(file);

    // Close the SD card
    sd_card_close();
}

// uint8_t sd_card_write_settings(bpk_t* Bpacket) {

//     // Return error message if the SD card cannot be opened
//     if (sd_card_open() != TRUE) {
//         bpk_create_string_response(Bpacket, BPK_Code_Error, "SD card failed to open\r\n\0");
//         esp32_uart_send_bpacket(Bpacket);
//         return FALSE;
//     }

//     FILE* file;
//     char errMsg[50];
//     if (sd_card_open_file(&file, SETTINGS_FILE_PATH_START_AT_ROOT, SD_CARD_FILE_WRITE, errMsg) != TRUE) {
//         bpk_create_string_response(Bpacket, BPK_Code_Error, errMsg);
//         esp32_uart_send_bpacket(Bpacket);
//         sd_card_close();
//         return FALSE;
//     }

//     switch (Bpacket->Request.val) {

//         case BPK_REQ_SET_CAMERA_SETTINGS:;

//             wd_camera_settings_t cameraSettings;
//             if (wd_bpacket_to_camera_settings(Bpacket, &cameraSettings) != TRUE) {
//                 bpk_create_string_response(Bpacket, BPK_Code_Error, "Bpacket to camera settings failed\0");
//                 esp32_uart_send_bpacket(Bpacket);
//                 // Clean up
//                 fclose(file);
//                 sd_card_close();
//                 return FALSE;
//             }

//             fseek(file, 0L, SEEK_SET); // Ensure the file is pointing to first byte
//             fputc(cameraSettings.frameSize, file);

//             break;

//         case BPK_REQ_SET_CAMERA_CAPTURE_TIMES:;

//             wd_camera_capture_time_settings_t captureTime;
//             if (wd_bpacket_to_capture_time_settings(Bpacket, &captureTime) != TRUE) {
//                 bpk_create_string_response(Bpacket, BPK_Code_Error,
//                                            "Bpacket to capture time settings failed. SD Card write attempt\r\n\0");
//                 // Clean up
//                 fclose(file);
//                 sd_card_close();
//                 return FALSE;
//             }

//             // Skip the first byte as this is where the camera resolution is stored
//             fseek(file, 1, SEEK_CUR);
//             fputc(captureTime.startTime.minute, file);
//             fputc(captureTime.startTime.hour, file);
//             fputc(captureTime.endTime.minute, file);
//             fputc(captureTime.endTime.hour, file);
//             fputc(captureTime.intervalTime.minute, file);
//             fputc(captureTime.intervalTime.hour, file);

//             break;

//         default:;

//             bpk_create_string_response(Bpacket, BPK_Code_Error, "WF: Invalid Request!\r\n\0");
//             esp32_uart_send_bpacket(Bpacket);
//             return FALSE;
//     }

//     // Clean up
//     fclose(file);
//     sd_card_close();

//     return TRUE;
// }

uint8_t sd_card_format_sd_card(bpk_t* Bpacket) {

    char errMsg[50];

    // Return error message if the SD card cannot be opened
    if (sd_card_open() != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, "SD card failed to open\r\n\0");
        return FALSE;
    }

    /****** START CODE BLOCK ******/
    // Description: Confirm all the correct folders on the SD card exist

    // Create watchdog folder if necessary
    if (sd_card_create_path(WATCHDOG_FOLDER_PATH, Bpacket) != TRUE) {
        sd_card_close();
        return FALSE;
    }

    // Create data folder if necessary
    if (sd_card_create_path(DATA_FOLDER_PATH, Bpacket) != TRUE) {
        sd_card_close();
        return FALSE;
    }

    // Create logs folder if necessary
    if (sd_card_create_path(LOG_FOLDER_PATH, Bpacket) != TRUE) {
        sd_card_close();
        return FALSE;
    }

    // Create settings folder if necessary
    // if (sd_card_create_path(SETTINGS_FOLDER_PATH, Bpacket) != TRUE) {
    //     sd_card_close();
    //     return FALSE;
    // }

    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Confirm the settings file exists and that it has valid data

    // Create the settings file if required
    // if (sd_card_create_file(SETTINGS_FILE_PATH_START_AT_ROOT, errMsg) != TRUE) {
    //     bpk_create_string_response(Bpacket, BPK_Code_Error, errMsg);
    //     esp32_uart_send_bpacket(Bpacket);
    //     return FALSE;
    // }

    uint32_t numBytes = 0;

    // Write default settings to settings file the file is empty
    if (sd_card_get_file_size(SETTINGS_FILE_PATH_START_AT_ROOT, &numBytes, errMsg) != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, errMsg);
        esp32_uart_send_bpacket(Bpacket);
        sd_card_close();
        return FALSE;
    }

    // if (numBytes == 0) {

    //     // Write the default camera settings to the SD card
    //     bpk_t cSettings;
    //     if (wd_camera_settings_to_bpacket(&cSettings, Bpacket->Receiver, Bpacket->Sender,
    //     BPK_Req_Set_Camera_Settings,
    //                                       BPK_Code_Execute, &deafultCameraSettings) != TRUE) {
    //         bpk_create_string_response(Bpacket, BPK_Code_Error, "Failed setting default camera settings\r\n\0");
    //         sd_card_close();
    //         return FALSE;
    //     }

    //     if (sd_card_write_settings(&cSettings) != TRUE) {
    //         bpk_create_string_response(Bpacket, BPK_Code_Error, "Failed setting default camera settings\r\n\0");
    //         sd_card_close();
    //         return FALSE;
    //     }

    //     // Write the default capture time settings to the SD card
    //     bpk_t ctSettings;
    //     if (wd_capture_time_settings_to_bpacket(&ctSettings, Bpacket->Receiver, Bpacket->Sender,
    //                                             BPK_Req_Set_Camera_Capture_Times, BPK_Code_Execute,
    //                                             &defaultCaptureTimeSettings) != TRUE) {
    //         bpk_create_string_response(Bpacket, BPK_Code_Error, "Failed setting default capture time
    //         settings\r\n\0"); sd_card_close(); return FALSE;
    //     }

    //     if (sd_card_write_settings(&ctSettings) != TRUE) {
    //         bpk_create_string_response(Bpacket, BPK_Code_Error, "Settings default capture time settings
    //         failed\r\n\0"); sd_card_close(); return FALSE;
    //     }
    // }

    /****** END CODE BLOCK ******/

    /****** START CODE BLOCK ******/
    // Description: Confirm the log and data files exist

    // Create the data file if required
    if (sd_card_create_file(DATA_FILE_PATH_START_AT_ROOT, errMsg) != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, errMsg);
        return FALSE;
    }

    // Create the logs file if required
    if (sd_card_create_file(LOG_FILE_PATH_START_AT_ROOT, errMsg) != TRUE) {
        bpk_create_string_response(Bpacket, BPK_Code_Error, errMsg);
        return FALSE;
    }

    /****** END CODE BLOCK ******/

    sd_card_close();

    return TRUE;
}

// uint8_t sd_card_read_settings(bpk_t* Bpacket) {

//     // Return error message if the SD card cannot be opened
//     if (sd_card_open() != TRUE) {
//         bpk_create_string_response(Bpacket, BPK_Code_Error, "SD card failed to open\r\n\0");
//         return FALSE;
//     }

//     // Confirm the file path to the settings exists
//     if (sd_card_create_path(SETTINGS_FOLDER_PATH, Bpacket) != TRUE) {
//         sd_card_close();
//         return FALSE;
//     }

//     // Get the file size of the settings file. This needs to be done first because the file
//     // needs to be opened in read mode
//     uint32_t fileNumBytes = 0;
//     char errMsg[50];
//     if (sd_card_get_file_size(SETTINGS_FILE_PATH_START_AT_ROOT, &fileNumBytes, errMsg) != TRUE) {
//         bpk_create_string_response(Bpacket, BPK_Code_Error, errMsg);
//         esp32_uart_send_bpacket(Bpacket);
//         return FALSE;
//     }

//     if (fileNumBytes == 0) {

//         switch (Bpacket->Request.val) {

//             case BPK_REQ_GET_CAMERA_SETTINGS:;

//                 bpk_t camSettingsBpacket;
//                 if (wd_camera_settings_to_bpacket(&camSettingsBpacket, Bpacket->Receiver, Bpacket->Sender,
//                                                   BPK_Req_Set_Camera_Settings, BPK_Code_Execute,
//                                                   &deafultCameraSettings) != TRUE) {
//                     bpk_create_string_response(Bpacket, BPK_Code_Error, "Setting deafult camera settings
//                     failed\r\n\0"); esp32_uart_send_bpacket(Bpacket); return FALSE;
//                 }

//                 if (sd_card_write_settings(&camSettingsBpacket) != TRUE) {
//                     bpk_create_string_response(Bpacket, BPK_Code_Error, "Failed to write settings\r\n\0");
//                     esp32_uart_send_bpacket(Bpacket);
//                 }
//                 break;

//             case BPK_REQ_GET_CAMERA_CAPTURE_TIMES:;

//                 bpk_t deafultCaptureTimeSettingsBpacket;

//                 if (wd_capture_time_settings_to_bpacket(&deafultCaptureTimeSettingsBpacket, Bpacket->Receiver,
//                                                         Bpacket->Sender, Bpacket->Request, BPK_Code_Execute,
//                                                         &defaultCaptureTimeSettings) != TRUE) {
//                     bpk_create_string_response(Bpacket, BPK_Code_Error, "Setting deafult camera settings
//                     failed\r\n\0"); esp32_uart_send_bpacket(Bpacket); return FALSE;
//                 }

//                 sd_card_write_settings(&deafultCaptureTimeSettingsBpacket);

//                 break;

//             default:;

//                 bpk_create_string_response(Bpacket, BPK_Code_Error, "RF: Invalid Request!\r\n\0");
//                 esp32_uart_send_bpacket(Bpacket);
//                 return FALSE;
//         }

//         // Open the SD card again
//         if (sd_card_open() != TRUE) {
//             bpk_create_string_response(Bpacket, BPK_Code_Error, "SD card failed to open\r\n\0");
//             return FALSE;
//         }
//     }

//     FILE* file;
//     if (sd_card_open_file(&file, SETTINGS_FILE_PATH_START_AT_ROOT, SD_CARD_FILE_READ, errMsg) != TRUE) {
//         bpk_create_string_response(Bpacket, BPK_Code_Error, errMsg);
//         esp32_uart_send_bpacket(Bpacket);
//         return FALSE;
//     }

//     if (Bpacket->Request.val == BPK_Req_Get_Camera_Settings.val) {

//         wd_camera_settings_t cameraSettings;
//         cameraSettings.frameSize = (uint8_t)fgetc(file);

//         if (wd_camera_settings_to_bpacket(Bpacket, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request,
//                                           BPK_Code_Success, &cameraSettings) != TRUE) {
//             esp32_uart_send_bpacket(Bpacket); // Send error back
//             // Clean up
//             fclose(file);
//             sd_card_close();
//             return FALSE;
//         }
//     }

//     if (Bpacket->Request.val == BPK_Req_Get_Camera_Capture_Times.val) {

//         // Set the cursor to the 1st index to skip the camera settings
//         fseek(file, 1, SEEK_CUR);
//         wd_camera_capture_time_settings_t captureTime;
//         // Seconds are required to be set to default otherwise they will assume
//         // the values of whatever is stored in that memory and will fail when
//         // being converted from a capture time to Bpacket
//         captureTime.startTime.second    = 0;
//         captureTime.startTime.minute    = fgetc(file);
//         captureTime.startTime.hour      = fgetc(file);
//         captureTime.endTime.second      = 0;
//         captureTime.endTime.minute      = fgetc(file);
//         captureTime.endTime.hour        = fgetc(file);
//         captureTime.intervalTime.second = 0;
//         captureTime.intervalTime.minute = fgetc(file);
//         captureTime.intervalTime.hour   = fgetc(file);

//         if (wd_capture_time_settings_to_bpacket(Bpacket, Bpacket->Receiver, Bpacket->Sender, Bpacket->Request,
//                                                 BPK_Code_Success, &captureTime) != TRUE) {
//             bpk_create_string_response(Bpacket, BPK_Code_Error,
//                                        "Capture time settings to Bpacket failed. SD Card read\r\n\0");

//             // Clean up
//             fclose(file);
//             sd_card_close();
//             return FALSE;
//         }
//     }

//     // Clean up
//     fclose(file);
//     sd_card_close();

//     return TRUE;
// }

// uint8_t sd_card_get_camera_settings(wd_camera_settings_t* cameraSettings) {

//     // Create a Bpacket with the correct information to read camera settings
//     bpk_t Bpacket;
//     bpk_create(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Req_Get_Camera_Settings, BPK_Code_Execute,
//     0,
//                NULL);

//     if (sd_card_read_settings(&Bpacket) != TRUE) {
//         bpk_create_sp(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message, BPK_Code_Error,
//                       "CS: Failed to read camera settings\r\n\0");
//         esp32_uart_send_bpacket(&Bpacket);
//         return FALSE;
//     }

//     // Convert the Bpacket to camera settings
//     if (wd_bpacket_to_camera_settings(&Bpacket, cameraSettings) != TRUE) {
//         bpk_create_sp(&Bpacket, BPK_Addr_Receive_Maple, BPK_Addr_Send_Esp32, BPK_Request_Message, BPK_Code_Error,
//                       "CS: Failed to parse camera settings\r\n\0");
//         esp32_uart_send_bpacket(&Bpacket);
//         return FALSE;
//     }

//     return TRUE;
// }

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

    int returnCode = sd_card_log(LOG_FILE_NAME, sdCardLog);
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
    sd_card_log(LOG_FILE_NAME, msg);

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
    return sd_card_write(LOG_FOLDER_PATH_START_AT_ROOT, name, message);
}