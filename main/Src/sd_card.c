
/* Public Includes */
#include <sys/types.h>
#include <sys/stat.h>

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
static const char* SD_CARD_TAG = "SD CARD:";

/* Private Variables */
int mounted = FALSE;

// Options for mounting the SD Card are given in the following
// configuration
static esp_vfs_fat_sdmmc_mount_config_t sdCardConfiguration = {
    .format_if_mount_failed = false,
    .max_files              = 5,
    .allocation_unit_size   = 16 * 1024,
};

sdmmc_card_t* card;

uint8_t save_image(uint8_t* imageData, int imageLength, char* imageName, int number) {

    // Ensure the SD card is mounted before attempting to save the image
    int returnCode = sd_card_open();
    if (returnCode != WD_SUCCESS) {
        return returnCode;
    }

    char msg[120];

    // Create required directories if they do not exist. The stat() function checks
    // whether the directory exists already and the mkdir() function creates the
    // directory if the stat() could not find the directory. Note mkdir can only
    // create one directory at a time which is why each folder is checked and made
    // seperatley
    struct stat st = {0};
    if ((stat(WATCHDOG_FOLDER_PATH, &st) == -1) && (mkdir(WATCHDOG_FOLDER_PATH, 0700) == -1)) {
        sprintf(msg, "System could not create watchdog folder");
        sd_card_log(SYSTEM_LOG_FILE, msg);
        return SD_CARD_ERROR_IO_ERROR;
    }

    if ((stat(DATA_FOLDER_PATH, &st) == -1) && (mkdir(DATA_FOLDER_PATH, 0700) == -1)) {
        sprintf(msg, "System could not create data folder");
        sd_card_log(SYSTEM_LOG_FILE, msg);
        return SD_CARD_ERROR_IO_ERROR;
    }

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

    // All done, unmount partition and disable SDMMC peripheral
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT_PATH, card);
    ESP_LOGI(SD_CARD_TAG, "Card unmounted");
    mounted = FALSE;
}

uint8_t sd_card_log(char* fileName, char* message) {

    // Print log to console
    ESP_LOGE("LOG", "%s", message);

    // Create required directories if they do not exist. The stat() function checks
    // whether the directory exists already and the mkdir() function creates the
    // directory if the stat() could not find the directory. Note mkdir can only
    // create one directory at a time which is why each folder is checked and made
    // seperatley
    struct stat st = {0};
    if ((stat(WATCHDOG_FOLDER_PATH, &st) == -1) && (mkdir(WATCHDOG_FOLDER_PATH, 0700) == -1)) {
        ESP_LOGE(SD_CARD_TAG, "Could not create %s", WATCHDOG_FOLDER_PATH);
        return SD_CARD_ERROR_IO_ERROR;
    }

    if ((stat(LOG_FOLDER_PATH, &st) == -1) && (mkdir(LOG_FOLDER_PATH, 0700) == -1)) {
        ESP_LOGE(SD_CARD_TAG, "Could not create %s", LOG_FOLDER_PATH);
        return SD_CARD_ERROR_IO_ERROR;
    }

    // Open the given file path for appending (a+). If the file does not exist
    // fopen will create a new file with the given file path. Note fopen() can
    // not make new directories!
    char filePath[100];
    sprintf(filePath, "%s/%s", LOG_FOLDER_PATH, fileName);
    FILE* file = fopen(filePath, "a+");

    // Confirm the file was opened/created correctly
    if (file == NULL) {
        ESP_LOGE(SD_CARD_TAG, "Could not open/create %s", filePath);
        return SD_CARD_ERROR_IO_ERROR;
    }

    // Get the time from the real time clock
    char formattedDateTime[20];
    rtc_get_formatted_date_time(formattedDateTime);

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
    int msgLength = RTC_DATE_TIME_CHAR_LENGTH + LOG_MSG_MAX_CHARACTERS + NULL_CHAR_LENGTH + 5;
    char log[msgLength];
    sprintf(log, "%s\t%s\n", formattedDateTime, message);

    // Write log to file
    fprintf(file, log);

    fclose(file);

    return WD_SUCCESS;
}