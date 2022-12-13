
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
    //     esp_err_t ret;

    //     // Options for mounting the filesystem.
    //     // If format_if_mount_failed is set to true, SD card will be partitioned and
    //     // formatted in case when mounting fails.
    //     esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    // #ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
    //         .format_if_mount_failed = true,
    // #else
    //         .format_if_mount_failed = false,
    // #endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
    //         .max_files            = 5,
    //         .allocation_unit_size = 16 * 1024};
    //     // sdmmc_card_t* card;
    //     const char mount_point[] = MOUNT_POINT_PATH;
    //     ESP_LOGI(SD_CARD_TAG, "Initializing SD card");

    //     // Use settings defined above to initialize SD card and mount FAT filesystem.
    //     // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    //     // Please check its source code and implement error recovery when developing
    //     // production applications.

    //     ESP_LOGI(SD_CARD_TAG, "Using SDMMC peripheral");
    //     sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    //     // This initializes the slot without card detect (CD) and write protect (WP) signals.
    //     // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    //     sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    //     // Set bus width to use:
    // #ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    //     slot_config.width = 4;
    // #else
    //     slot_config.width = 1;
    // #endif

    //     // On chips where the GPIOs used for SD card can be configured, set them in
    //     // the slot_config structure:
    // #ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    //     slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
    //     slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
    //     slot_config.d0  = CONFIG_EXAMPLE_PIN_D0;
    //     #ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    //     slot_config.d1 = CONFIG_EXAMPLE_PIN_D1;
    //     slot_config.d2 = CONFIG_EXAMPLE_PIN_D2;
    //     slot_config.d3 = CONFIG_EXAMPLE_PIN_D3;
    //     #endif // CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    // #endif // CONFIG_SOC_SDMMC_USE_GPIO_MATRIX

    //     // Enable internal pullups on enabled pins. The internal pullups
    //     // are insufficient however, please make sure 10k external pullups are
    //     // connected on the bus. This is for debug / example purpose only.
    //     slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    //     ESP_LOGI(SD_CARD_TAG, "Mounting filesystem");
    //     ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    //     if (ret != ESP_OK) {
    //         if (ret == ESP_FAIL) {
    //             ESP_LOGE(SD_CARD_TAG,
    //                      "Failed to mount filesystem. "
    //                      "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig
    //                      option.");
    //         } else {
    //             ESP_LOGE(SD_CARD_TAG,
    //                      "Failed to initialize the card (%s). "
    //                      "Make sure SD card lines have pull-up resistors in place.",
    //                      esp_err_to_name(ret));
    //         }
    //         return SD_CARD_ERROR_CONNECTION_FAILURE;
    //     }
    //     ESP_LOGI(SD_CARD_TAG, "Filesystem mounted");

    //     // Card has been initialized, print its properties
    //     sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files:

    /****** START CODE BLOCK ******/
    // Description: Test code for saving an image to the SD card

    // Open the destination location

    if (mounted != TRUE) {
        return SD_CARD_NOT_MOUNTED;
    }

    // char imageName[20];
    // sprintf(imageName, "%s/img%d.jpg", MOUNT_POINT_PATH, number);

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

    if ((stat(DATA_FOLDER_PATH, &st) == -1) && (mkdir(DATA_FOLDER_PATH, 0700) == -1)) {
        ESP_LOGE(SD_CARD_TAG, "Could not create %s", DATA_FOLDER_PATH);
        return SD_CARD_ERROR_IO_ERROR;
    }

    // Create path for image
    char filePath[100];
    sprintf(filePath, "%s/%s", DATA_FOLDER_PATH, imageName);
    FILE* imageFile = fopen(filePath, "wb");

    if (imageFile == NULL) {
        ESP_LOGE(SD_CARD_TAG, "Failed to open destination folder for writing error: %s", strerror(errno));
    }

    for (int i = 0; i < imageLength; i++) {
        fputc(imageData[i], imageFile);
    }

    fclose(imageFile);

    /****** END CODE BLOCK ******/

    /*
        // First create a file.
        const char *file_hello = MOUNT_POINT_PATH"/hello.txt";

        ESP_LOGI(SD_CARD_TAG, "Opening file %s", file_hello);
        FILE *f = fopen(file_hello, "w");
        if (f == NULL) {
            ESP_LOGE(SD_CARD_TAG, "Failed to open file for writing");
            return;
        }
        fprintf(f, "Hello %s!\n", card->cid.name);
        fclose(f);
        ESP_LOGI(SD_CARD_TAG, "File written");

        const char *file_foo = MOUNT_POINT_PATH"/foo.txt";

        // Check if destination file exists before renaming
        struct stat st;
        if (stat(file_foo, &st) == 0) {
            // Delete it if it exists
            unlink(file_foo);
        }

        // Rename original file
        ESP_LOGI(SD_CARD_TAG, "Renaming file %s to %s", file_hello, file_foo);
        if (rename(file_hello, file_foo) != 0) {
            ESP_LOGE(SD_CARD_TAG, "Rename failed");
            return;
        }

        // Open renamed file for reading
        ESP_LOGI(SD_CARD_TAG, "Reading file %s", file_foo);
        f = fopen(file_foo, "r");
        if (f == NULL) {
            ESP_LOGE(SD_CARD_TAG, "Failed to open file for reading");
            return;
        }

        // Read a line from file
        char line[64];
        fgets(line, sizeof(line), f);
        fclose(f);

        // Strip newline
        char *pos = strchr(line, '\n');
        if (pos) {
            *pos = '\0';
        }
        ESP_LOGI(SD_CARD_TAG, "Read from file: '%s'", line);

    */
    // All done, unmount partition and disable SDMMC peripheral
    // esp_vfs_fat_sdcard_unmount(mount_point, card);
    // ESP_LOGI(SD_CARD_TAG, "Card unmounted");

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