/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char* TAG = "example";

/* CAMERA TEST CODE */
#define BOARD_ESP32CAM_AITHINKER

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// support IDF 5.x
#ifndef portTICK_RATE_MS
    #define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#include "esp_camera.h"

// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

    #define CAM_PIN_PWDN  32
    #define CAM_PIN_RESET -1 // software reset will be performed
    #define CAM_PIN_XCLK  0
    #define CAM_PIN_SIOD  26
    #define CAM_PIN_SIOC  27

    #define CAM_PIN_D7    35
    #define CAM_PIN_D6    34
    #define CAM_PIN_D5    39
    #define CAM_PIN_D4    36
    #define CAM_PIN_D3    21
    #define CAM_PIN_D2    19
    #define CAM_PIN_D1    18
    #define CAM_PIN_D0    5
    #define CAM_PIN_VSYNC 25
    #define CAM_PIN_HREF  23
    #define CAM_PIN_PCLK  22

#endif

// static const char* TAG = "example:take_picture";

static camera_config_t camera_config = {
    .pin_pwdn     = CAM_PIN_PWDN,
    .pin_reset    = CAM_PIN_RESET,
    .pin_xclk     = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7    = CAM_PIN_D7,
    .pin_d6    = CAM_PIN_D6,
    .pin_d5    = CAM_PIN_D5,
    .pin_d4    = CAM_PIN_D4,
    .pin_d3    = CAM_PIN_D3,
    .pin_d2    = CAM_PIN_D2,
    .pin_d1    = CAM_PIN_D1,
    .pin_d0    = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href  = CAM_PIN_HREF,
    .pin_pclk  = CAM_PIN_PCLK,

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG - This was the deafultPIXFORMAT_RGB565
    .frame_size =
        FRAMESIZE_QVGA, // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the
                        // ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, // 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count     = 1,  // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
};

static esp_err_t init_camera() {
    // initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}
/* CAMERA TEST CODE */

/* SD CARD TEST CODE */
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include <stdlib.h>
#include <errno.h>
#define MOUNT_POINT "/sdcard"

void save_image(uint8_t* imageData, int imageLength, int number) {
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files            = 5,
        .allocation_unit_size = 16 * 1024};
    sdmmc_card_t* card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // Set bus width to use:
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    slot_config.width = 4;
#else
    slot_config.width = 1;
#endif

    // On chips where the GPIOs used for SD card can be configured, set them in
    // the slot_config structure:
#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
    slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
    slot_config.d0  = CONFIG_EXAMPLE_PIN_D0;
    #ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    slot_config.d1 = CONFIG_EXAMPLE_PIN_D1;
    slot_config.d2 = CONFIG_EXAMPLE_PIN_D2;
    slot_config.d3 = CONFIG_EXAMPLE_PIN_D3;
    #endif // CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
#endif // CONFIG_SOC_SDMMC_USE_GPIO_MATRIX

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG,
                     "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG,
                     "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files:

    /****** START CODE BLOCK ******/
    // Description: Test code for saving an image to the SD card

    // Open the destination location

    FILE* dest;
    char imageName[20];
    sprintf(imageName, MOUNT_POINT "/img%d.jpg", number);
    dest = fopen(imageName, "wb");

    if (dest != NULL) {

        for (int i = 0; i < imageLength; i++) {
            fputc(imageData[i], dest);
        }

        if (fclose(dest) == 0) {
            ESP_LOGI(TAG, "File has been transfered");
        } else {
            ESP_LOGE(TAG, "Error closing file");
        }

    } else {
        ESP_LOGE(TAG, "Failed to open destination folder for writing error: %s", strerror(errno));
    }

    /****** END CODE BLOCK ******/

    /*
        // First create a file.
        const char *file_hello = MOUNT_POINT"/hello.txt";

        ESP_LOGI(TAG, "Opening file %s", file_hello);
        FILE *f = fopen(file_hello, "w");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            return;
        }
        fprintf(f, "Hello %s!\n", card->cid.name);
        fclose(f);
        ESP_LOGI(TAG, "File written");

        const char *file_foo = MOUNT_POINT"/foo.txt";

        // Check if destination file exists before renaming
        struct stat st;
        if (stat(file_foo, &st) == 0) {
            // Delete it if it exists
            unlink(file_foo);
        }

        // Rename original file
        ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
        if (rename(file_hello, file_foo) != 0) {
            ESP_LOGE(TAG, "Rename failed");
            return;
        }

        // Open renamed file for reading
        ESP_LOGI(TAG, "Reading file %s", file_foo);
        f = fopen(file_foo, "r");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for reading");
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
        ESP_LOGI(TAG, "Read from file: '%s'", line);

    */
    // All done, unmount partition and disable SDMMC peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");
}
/* SD CARD TEST CODE */

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 33

#define BLINK_PERIOD 100

static uint8_t s_led_state = 0;

static void blink_led(void) {
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void) {
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void watchdog_test() {

    /* Configure the camera */
    if (ESP_OK != init_camera()) {
        ESP_LOGE(TAG, "Camera failed to init. Error: %s", strerror(errno));
        return;
    }

    int count = 0;
    while (1) {
        ESP_LOGI(TAG, "Taking picture...");
        camera_fb_t* pic = esp_camera_fb_get();

        if (pic != NULL) {
            // use pic->buf to access the image
            ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);

            // Save the image
            save_image(pic->buf, pic->len, count);
            count++;
        } else {
            ESP_LOGE(TAG, "Camera returned NULL!");
        }

        esp_camera_fb_return(pic);

        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        vTaskDelay(5000 / portTICK_RATE_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
    }
}

void app_main(void) {

    /* Configure the peripheral according to the LED type */
    configure_led();

    watchdog_test();

    while (1) {
        vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS);
    }

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
