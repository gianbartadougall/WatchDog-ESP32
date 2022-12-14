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
// #include "led_strip.h"
#include "sdkconfig.h"

static const char* TAG = "example";

#include "camera.h"
#include "sd_card.h"
#include "wd_utils.h"

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 33

#define BLINK_PERIOD 100
#define GPIO_4       4 // This is the COB LED

#define GPIO_0  0
#define GPIO_16 16
#define GPIO_2  2
#define GPIO_14 14
#define GPIO_15 15
#define GPIO_13 13
#define GPIO_12 12

static void configure_led(void) {
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);

    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    vTaskDelay(1000 / portTICK_RATE_MS);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
}

void gpio_test(void) {

    gpio_reset_pin(GPIO_0);
    gpio_reset_pin(GPIO_16);
    gpio_reset_pin(GPIO_2);
    gpio_reset_pin(GPIO_14);
    gpio_reset_pin(GPIO_15);
    gpio_reset_pin(GPIO_13);
    gpio_reset_pin(GPIO_12);

    gpio_set_direction(GPIO_0, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_16, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_14, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_15, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_13, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_12, GPIO_MODE_OUTPUT);

    vTaskDelay(5000 / portTICK_RATE_MS);

    gpio_set_direction(GPIO_0, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_2, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_16, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_14, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_15, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_13, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_12, GPIO_MODE_INPUT);

    gpio_reset_pin(GPIO_0);
    gpio_reset_pin(GPIO_16);
    gpio_reset_pin(GPIO_2);
    gpio_reset_pin(GPIO_14);
    gpio_reset_pin(GPIO_15);
    gpio_reset_pin(GPIO_13);
    gpio_reset_pin(GPIO_12);
}

void watchdog_test() {

    char msg[100];
    if (sd_card_open() != WD_SUCCESS) {
        return;
    }

    while (1) {

        if (sd_card_open() != WD_SUCCESS) {
            return;
        }

        sprintf(msg, "Taking image");
        sd_card_log(SYSTEM_LOG_FILE, msg);

        camera_fb_t* pic = esp_camera_fb_get();

        if (pic != NULL) {

            sprintf(msg, "Image captured. Size was %zu bytes", pic->len);
            sd_card_log(SYSTEM_LOG_FILE, msg);

            // Save the image
            sd_card_save_image(pic->buf, pic->len);
        } else {
            sprintf(msg, "Camera failed to take image");
            sd_card_log(SYSTEM_LOG_FILE, msg);
        }

        esp_camera_fb_return(pic);

        sprintf(msg, "Unmounting SD card");
        sd_card_log(SYSTEM_LOG_FILE, msg);
        sd_card_close();

        // Test the use of other GPIO pins
        // ESP_LOGI(TAG, "GPIO TEST COMMENCING");
        // gpio_test();

        ESP_LOGI(TAG, "TAKING PHOTO");
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        vTaskDelay(5000 / portTICK_RATE_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
    }
}

uint8_t hardware_config(void) {

    /* Configure all the required hardware */

    // Configure the SD card and ensure it can be used. This is done before
    // any other hardware so that everything else can be logged

    char msg[100];
    if (sd_card_open() != WD_SUCCESS) {
        return WD_ERROR;
    }

    // Configure Camera
    sprintf(msg, "Configuring Camera");
    sd_card_log(SYSTEM_LOG_FILE, msg);
    if (init_camera() != ESP_OK) {
        return WD_ERROR;
    }

    sprintf(msg, "Configuring RTC");
    sd_card_log(SYSTEM_LOG_FILE, msg);
    // TODO: Configure RTC

    sprintf(msg, "Configuring Temperature Sensor");
    sd_card_log(SYSTEM_LOG_FILE, msg);
    // TODO: Configure Temperature sensor

    // Unmount the SD card
    sd_card_close();

    return WD_SUCCESS;
}

uint8_t software_config(void) {

    if (sd_card_init() != WD_SUCCESS) {
        return WD_ERROR;
    }

    return WD_SUCCESS;
}

void app_main(void) {

    /* Configure the peripheral according to the LED type */
    configure_led();

    /* Initialise all the hardware used */
    if ((hardware_config() == WD_SUCCESS) && (software_config() == WD_SUCCESS)) {
        watchdog_test();
    }

    char msg[100];
    if (sd_card_open() == WD_SUCCESS) {
        sprintf(msg, "Exited Watchdog System. Waiting for shutdown");
        sd_card_log(SYSTEM_LOG_FILE, msg);
        sd_card_close();
    }

    while (1) {
        ESP_LOGE(TAG, "Blink");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);
    }
}
