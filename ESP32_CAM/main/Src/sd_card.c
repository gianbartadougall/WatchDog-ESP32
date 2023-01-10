/* Public Includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>

/* Private Includes */
#include "sd_card.h"
#include "rtc.h"
#include "wd_utils.h"

/* Private Macros */
#define MOUNT_POINT_PATH     ("/sdcard")
#define WATCHDOG_FOLDER_PATH ("/sdcard/WATCHDOG")
#define LOG_FOLDER_PATH      ("/sdcard/WATCHDOG/LOGS")
#define DATA_FOLDER_PATH     ("/sdcard/WATCHDOG/DATA")

#define LOG_MSG_MAX_CHARACTERS 200
#define NULL_CHAR_LENGTH       1
#define DATE_TIME_DELIMITER    '_'
#define IMG_NUM_START_INDEX    3
#define IMG_NUM_END_CHARACTER  '-'

static const char* SD_CARD_TAG = "SD CARD:";

/* Private Variables */
int mounted = FALSE;

int imageNumber = 0;

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

uint8_t sd_card_init(void) {

    // Update the image number
    if (sd_card_update_image_number() != WD_SUCCESS) {
        return WD_ERROR;
    }

    return WD_SUCCESS;
}

uint8_t sd_card_update_image_number(void) {

    // Ensure the SD card is mounted before attempting to save the image
    if (sd_card_open() != WD_SUCCESS) {
        return WD_ERROR;
    }

    // If there is no data on the SD card, return an image number of 0. The
    // function will log this error internally
    if (sd_card_check_file_path_exists(DATA_FOLDER_PATH) != WD_SUCCESS) {
        imageNumber = 0;
        return WD_ERROR;
    }

    // Loop through images in the data folder path and find the last image
    // number saved
    DIR* dataDir = opendir(DATA_FOLDER_PATH);

    char msg[50];
    if (dataDir == NULL) {
        sprintf(msg, "Could not open directory '%s'", DATA_FOLDER_PATH);
        sd_card_log(SYSTEM_LOG_FILE, msg);
        imageNumber = 0;
        return WD_ERROR;
    }

    // Loop through all the files in the folder and set the image number to the
    // highest number found
    struct dirent* file;
    while ((file = readdir(dataDir)) != NULL) {

        // File name is stored in file->d_name
        int number = wd_utils_extract_number(file->d_name, IMG_NUM_START_INDEX, IMG_NUM_END_CHARACTER);

        if (number > imageNumber) {
            imageNumber = number;
        }
    }

    closedir(dataDir);

    // Increment image number by 1
    sprintf(msg, "Highest image number found was %i", imageNumber);
    sd_card_log(SYSTEM_LOG_FILE, msg);

    imageNumber += 1;
    return WD_SUCCESS;
}

uint8_t sd_card_save_image(uint8_t* imageData, int imageLength) {

    // Ensure the SD card is mounted before attempting to save the image
    int returnCode = sd_card_open();
    if (returnCode != WD_SUCCESS) {
        return returnCode;
    }

    char msg[120];

    if (sd_card_check_file_path_exists(DATA_FOLDER_PATH) != WD_SUCCESS) {
        return WD_ERROR;
    }

    // Create name for image
    char imageName[30];
    sprintf(imageName, "img%d%c.jpg", imageNumber, IMG_NUM_END_CHARACTER);

    // Create path for image
    char filePath[70];
    sprintf(filePath, "%s/%s", DATA_FOLDER_PATH, imageName);

    sprintf(msg, "File path '%s' created to save image", filePath);
    sd_card_log(SYSTEM_LOG_FILE, msg);

    FILE* imageFile = fopen(filePath, "wb");

    if (imageFile == NULL) {
        sprintf(msg, "File path '%s' could not be opened", filePath);
        sd_card_log(SYSTEM_LOG_FILE, msg);
        return SD_CARD_ERROR_IO_ERROR;
    }

    sprintf(msg, "File path '%s' validated", filePath);
    sd_card_log(SYSTEM_LOG_FILE, msg);

    for (int i = 0; i < imageLength; i++) {
        fputc(imageData[i], imageFile);
    }

    sprintf(msg, "Image saved succesfully to '%s'", filePath);
    sd_card_log(SYSTEM_LOG_FILE, msg);

    fclose(imageFile);

    // Increment the image number
    imageNumber++;

    return WD_SUCCESS;
}

uint8_t sd_card_open(void) {

    // Return if the sd card has already been mounted
    if (mounted != FALSE) {
        return WD_SUCCESS;
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
    if (returnCode != WD_SUCCESS) {
        if (returnCode == SD_CARD_ERROR_IO_ERROR) {
            ESP_LOGE(SD_CARD_TAG, "Could not open file to write");
        }

        if (returnCode == LOG_ERR_MSG_TOO_LONG) {
            ESP_LOGE(SD_CARD_TAG, "Message was too long!");
        }
    }

    mounted = TRUE;
    return WD_SUCCESS;
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
    ESP_LOGI(SD_CARD_TAG, "Card unmounted");
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
            if (sd_card_check_directory_exists(copyFilePath) != WD_SUCCESS) {
                return WD_ERROR;
            }
        }
    }

    return WD_SUCCESS;
}

uint8_t sd_card_check_directory_exists(char* directory) {

    // Structure to store infromation about directory. We do not need the information
    // but this struct is required for the function call
    struct stat st = {0};

    // if stat returns 0 then the directory was able to be found
    if (stat(directory, &st) == 0) {
        return WD_SUCCESS;
    }

    // Try to create the directory. mkdir() returning 0 => direcotry was created
    if (mkdir(directory, 0700) == 0) {
        return WD_SUCCESS;
    }

    ESP_LOGE(SD_CARD_TAG, "Error trying to create directory %s. Error: '%s'", directory, strerror(errno));
    return WD_ERROR;
}

uint8_t sd_card_write(char* filePath, char* fileName, char* message) {

    // Log to console that data is being written to this file name
    ESP_LOGI(SD_CARD_TAG, "Writing data to %s", filePath);

    if (sd_card_check_file_path_exists(filePath) != WD_SUCCESS) {
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

    return WD_SUCCESS;
}

uint8_t sd_card_log(char* fileName, char* message) {
    char name[30];
    sprintf(name, "%s", fileName); // Doing this so \0 is added to the end
    return sd_card_write(LOG_FOLDER_PATH, name, message);

    // // Print log to console
    // ESP_LOGI("LOG", "%s", message);

    // if (sd_card_check_file_path_exists(LOG_FOLDER_PATH) != WD_SUCCESS) {
    //     return SD_CARD_ERROR_IO_ERROR;
    // }

    // // Open the given file path for appending (a+). If the file does not exist
    // // fopen will create a new file with the given file path. Note fopen() can
    // // not make new directories!
    // char filePath[100];
    // sprintf(filePath, "%s/%s", LOG_FOLDER_PATH, fileName);
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

    // return WD_SUCCESS;
}