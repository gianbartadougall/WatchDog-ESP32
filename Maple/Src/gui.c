/**
 * @file gui.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-01-27
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <stdio.h>
#include <sensor.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Personal Includes */
#include "gui.h"
#include "utilities.h"
#include "chars.h"
#include "watchdog_defines.h"
#include "bpacket.h"

// #define STB_IMAGE_IMPLEMENTATION // Required for stb_image
#include "stb_image.h"
#include "stb_image_resize.h"

/* Private Macros */
#define LEFT_MARGIN   10
#define MIDDLE_MARGIN 270
#define TOP_MARGIN    10
#define RIGHT_MARGIN  500

#define GAP_WIDTH  10
#define GAP_HEIGHT 10

#define BUTTON_WIDTH  200
#define BUTTON_HEIGHT 30

#define LABEL_WIDTH         200
#define LABEL_WIDTH_HALF    ((LABEL_WIDTH - GAP_WIDTH) / 2)
#define LABEL_WIDTH_HEADING (LABEL_WIDTH * 2 + GAP_WIDTH)
#define LABEL_HEIGHT        30

#define DROP_BOX_WIDTH       200
#define DROP_BOX_THIRD_WIDTH 60
#define DROP_BOX_HEIGHT      300

#define TEXT_BOX_WIDTH  95
#define TEXT_BOX_HEIGHT 30

#define WINDOW_WIDTH  1300
#define WINDOW_HEIGHT 768

#define ROW_1  10
#define ROW_2  (ROW_1 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_3  (ROW_2 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_4  (ROW_3 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_5  (ROW_4 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_6  (ROW_5 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_7  (ROW_6 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_8  (ROW_7 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_9  (ROW_8 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_10 (ROW_9 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_11 (ROW_10 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_12 (ROW_11 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_13 (ROW_12 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_14 (ROW_13 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_15 (ROW_14 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_16 (ROW_15 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_17 (ROW_16 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_18 (ROW_17 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_19 (ROW_18 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_20 (ROW_19 + LABEL_HEIGHT + GAP_HEIGHT)

#define COL_1                       10
#define COL_1_ONE_THIRD             (COL_1 + (LABEL_WIDTH + GAP_WIDTH) / 3)
#define COL_1_ONE_HALF              (COL_1 + (LABEL_WIDTH + GAP_WIDTH) / 2)
#define COL_1_TWO_THIRD             (COL_1 + (LABEL_WIDTH + GAP_WIDTH) * 2 / 3)
#define COL_2                       (COL_1 + LABEL_WIDTH + GAP_WIDTH)
#define COL_2_ONE_THIRD             (COL_2 + (LABEL_WIDTH + GAP_WIDTH) / 3)
#define COL_2_ONE_HALF              (COL_2 + (LABEL_WIDTH + GAP_WIDTH) / 2)
#define COL_2_TWO_THIRD             (COL_2 + (LABEL_WIDTH + GAP_WIDTH) * 2 / 3)
#define COL_3                       (COL_2 + LABEL_WIDTH + GAP_WIDTH)
#define COL_4                       (COL_3 + LABEL_WIDTH + GAP_WIDTH)
#define COL_5                       (COL_4 + LABEL_WIDTH + GAP_WIDTH)
#define COL_6                       (COL_5 + LABEL_WIDTH + GAP_WIDTH)
#define COL_7                       (COL_6 + LABEL_WIDTH + GAP_WIDTH)
#define COL_8                       (COL_7 + LABEL_WIDTH + GAP_WIDTH)
#define BUTTON_HELP_HANDLE          12
#define TEXT_BOX_START_ERROR_HANDLE 13

#define TEXT_BOX_START_TIME_FLAG    (0x01 << 0)
#define TEXT_BOX_END_TIME_FLAG      (0x01 << 1)
#define TEXT_BOX_TIME_INTERVAL_FLAG (0x01 << 2)

#define CAMERA_VIEW SW_HIDE
#define NORMAL_VIEW SW_SHOW

// Struct that contains all of the settings
// wd_camera_settings_t settings;
uint8_t cameraSettingsFlag;
uint8_t captureTimeFlag;

typedef struct rectangle_t {
    int startX;
    int startY;
    int width;
    int height;
} rectangle_t;

rectangle_t cameraViewImagePosition;

// Function prototypes
uint8_t draw_image(HWND hwnd, char* filePath, rectangle_t* position);
void draw_rectangle(HWND hwnd, rectangle_t* rectangle, uint8_t r, uint8_t g, uint8_t b);
void gui_update_camera_view(char* fileName);
void send_current_settings(void);

// Create instances of structs that are used for the GUI
watchdog_info_t* watchdog;
uint32_t* flags;
bpacket_circular_buffer_t* guiToMainCircularBuffer;
bpacket_circular_buffer_t* mainToGuiCircularBuffer;

// Macro that takes the bpacket out of the circular buffer at the read index
#define MTG_CB_CURRENT_BPACKET (mainToGuiCircularBuffer->circularBuffer[*mainToGuiCircularBuffer->readIndex])

/*
 * Here all of the structs are made and populated with all of the current information of from the ESP32, start time,
 * end time, time interval, etc
 */

dt_time_t startTime;
dt_time_t endTime;
uint8_t intervalMinute;
uint8_t intervalHour;
// uint8_t resolution;
wd_camera_settings_t cameraSettings;
wd_camera_capture_time_settings_t captureTime;

/*
ALL THE LABEL INFORMATION IS HERE
*/

#define NUMBER_OF_LABELS        11
#define LONGEST_LABEL_TITLE     50
#define LABEL_STATUS            0
#define LABEL_ID                1
#define LABEL_NUM_IMAGES        2
#define LABEL_SET_UP            3
#define LABEL_DATE              4
#define LABEL_SETTINGS          5
#define LABEL_CAMERA_RESOLUTION 6
#define LABEL_START_TIME        7
#define LABEL_END_TIME          8
#define LABEL_TIME_INTERVAL     9
#define LABEL_TIME_INFO         10
#define LABEL_START_INVALID     11
#define LABEL_END_INVALID       12
#define LABEL_INTERVAL_INVAILD  13
HWND labelStatus, labelID, labelNumImages, labelSetUp, labelData, labelSettings, labelCameraResolution, labelStartTime,
    labelEndTime, labelTimeInterval, labelTimeInfo;
char* labelTitleList[LONGEST_LABEL_TITLE] = {" Status",
                                             " ID",
                                             " Images Taken",
                                             " SET UP",
                                             " DATA",
                                             " CAMERA SETTINGS",
                                             " Camera Resolution",
                                             " Start Time             (hh:mm am)",
                                             " End Time              (hh:mm am)",
                                             " Time Interval        (hh:mm)",
                                             " THIS WILL TELL YOU WHEN THE PHOTOS ARE TAKEN"};
int labelStartXList[NUMBER_OF_LABELS] = {COL_6, COL_6, COL_6, COL_1, COL_1, COL_1, COL_1, COL_1, COL_1, COL_1, COL_1};
int labelStartYList[NUMBER_OF_LABELS] = {ROW_1, ROW_2, ROW_3,  ROW_1,  ROW_4, ROW_7,
                                         ROW_8, ROW_9, ROW_10, ROW_11, ROW_12};
int labelWidthList[NUMBER_OF_LABELS]  = {
    LABEL_WIDTH, LABEL_WIDTH, LABEL_WIDTH, LABEL_WIDTH_HEADING, LABEL_WIDTH_HEADING,    LABEL_WIDTH_HEADING,
    LABEL_WIDTH, LABEL_WIDTH, LABEL_WIDTH, LABEL_WIDTH,         LABEL_WIDTH_HEADING * 2};
int labelHeightList[NUMBER_OF_LABELS] = {LABEL_HEIGHT, LABEL_HEIGHT, LABEL_HEIGHT, LABEL_HEIGHT,
                                         LABEL_HEIGHT, LABEL_HEIGHT, LABEL_HEIGHT, LABEL_HEIGHT,
                                         LABEL_HEIGHT, LABEL_HEIGHT, LABEL_HEIGHT};

typedef struct label_info_t {
    HWND handle;
    char* text;
    int startX;
    int startY;
    int width;
    int height;
} label_info_t;

label_info_t labelList[NUMBER_OF_LABELS];

HWND* labelHandleList[NUMBER_OF_LABELS] = {&labelStatus,  &labelID,           &labelNumImages,        &labelSetUp,
                                           &labelData,    &labelSettings,     &labelCameraResolution, &labelStartTime,
                                           &labelEndTime, &labelTimeInterval, &labelTimeInfo};

void initalise_label_structs(label_info_t* labelList) {
    for (int i = 0; i < NUMBER_OF_LABELS; i++) {
        labelList[i].handle = *labelHandleList[i];
        labelList[i].text   = labelTitleList[i];
        labelList[i].startX = labelStartXList[i];
        labelList[i].startY = labelStartYList[i];
        labelList[i].width  = labelWidthList[i];
        labelList[i].height = labelHeightList[i];
    }
}

/*
ALL THE BUTTON INFORMATION IS HERE
*/

#define NUMBER_OF_BUTTONS    6
#define LONGEST_BUTTON_TITLE 50
#define BUTTON_OPEN_SD_CARD  0
#define BUTTON_CAMERA_VIEW   1
#define BUTTON_RUN_TEST      2
#define BUTTON_EXPORT_DATA   3
#define BUTTON_NORMAL_VIEW   4
#define BUTTON_HELP          5
HWND buttonOpenSDCard, buttonCameraView, buttonRunTest, buttonExportData, buttonNormalView, buttonHelp;
char* buttonTitleList[LONGEST_BUTTON_TITLE] = {
    "View Data", "Live Camera View", "Test System", "Download data from SD card", "Exit Camera View", "Help"};
int buttonStartXList[NUMBER_OF_BUTTONS] = {COL_2, COL_1, COL_2, COL_1, COL_1, COL_6};
int buttonStartYList[NUMBER_OF_BUTTONS] = {
    ROW_5, ROW_2, ROW_2, ROW_5, ROW_2, ROW_4,
};
int buttonWidthList[NUMBER_OF_BUTTONS]  = {BUTTON_WIDTH, BUTTON_WIDTH, BUTTON_WIDTH,
                                          BUTTON_WIDTH, BUTTON_WIDTH, BUTTON_WIDTH};
int buttonHeightList[NUMBER_OF_BUTTONS] = {BUTTON_HEIGHT, BUTTON_HEIGHT, BUTTON_HEIGHT,
                                           BUTTON_HEIGHT, BUTTON_HEIGHT, BUTTON_HEIGHT};

typedef struct button_info_t {
    HWND handle;
    char* text;
    int startX;
    int startY;
    int width;
    int height;
} button_info_t;

button_info_t buttonList[NUMBER_OF_BUTTONS];

HWND* buttonHandleList[NUMBER_OF_BUTTONS] = {&buttonOpenSDCard, &buttonCameraView, &buttonRunTest,
                                             &buttonExportData, &buttonNormalView, &buttonHelp};

void initalise_button_structs(button_info_t* buttonList) {
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        buttonList[i].handle = *buttonHandleList[i];
        buttonList[i].text   = buttonTitleList[i];
        buttonList[i].startX = buttonStartXList[i];
        buttonList[i].startY = buttonStartYList[i];
        buttonList[i].width  = buttonWidthList[i];
        buttonList[i].height = buttonHeightList[i];
    }
}

/*
ALL THE TEXT BOX INFORMATION IS HERE
*/

#define NUMBER_OF_TEXT_BOXES   3
#define TEXT_BOX_START_TIME    0
#define TEXT_BOX_END_TIME      1
#define TEXT_BOX_TIME_INTERVAL 2
uint32_t textBoxFlags = 0;
int startTimeHr, startTimeMin, endTimeHr, endTimeMin, timeIntervalHr, timeIntervalMin;
HWND textBoxStartTime, textBoxEndTime, textBoxTimeInterval;
HWND textBoxLabelStart, textBoxLabelEnd, textBoxLabelInterval;
int textBoxStartXList[NUMBER_OF_TEXT_BOXES]       = {COL_2, COL_2, COL_2};
int textBoxStartYList[NUMBER_OF_TEXT_BOXES]       = {ROW_9, ROW_10, ROW_11};
int textBoxWidthList[NUMBER_OF_TEXT_BOXES]        = {TEXT_BOX_WIDTH, TEXT_BOX_WIDTH, TEXT_BOX_WIDTH};
int textBoxHeightList[NUMBER_OF_TEXT_BOXES]       = {TEXT_BOX_HEIGHT, TEXT_BOX_HEIGHT, TEXT_BOX_HEIGHT};
char* textBoxLabelTitleList[NUMBER_OF_TEXT_BOXES] = {"Invalid Input", "Invalid Input", "Invalid Input"};
char* textBoxDefaultText[NUMBER_OF_TEXT_BOXES]; // Unfortunately this has to made in main by a function call

typedef struct text_box_info_t {
    HWND handle;
    int startX;
    int startY;
    int width;
    int height;
    label_info_t label;
} text_box_info_t;

text_box_info_t textBoxList[NUMBER_OF_TEXT_BOXES];

HWND* textBoxHandleList[NUMBER_OF_TEXT_BOXES]      = {&textBoxStartTime, &textBoxEndTime, &textBoxTimeInterval};
HWND* textBoxLabelHandleList[NUMBER_OF_TEXT_BOXES] = {&textBoxLabelStart, &textBoxLabelEnd, &textBoxLabelInterval};

void initalise_text_box_structs(text_box_info_t* textBoxList) {
    for (int i = 0; i < NUMBER_OF_TEXT_BOXES; i++) {
        textBoxList[i].handle       = *textBoxHandleList[i];
        textBoxList[i].startX       = textBoxStartXList[i];
        textBoxList[i].startY       = textBoxStartYList[i];
        textBoxList[i].width        = textBoxWidthList[i];
        textBoxList[i].height       = textBoxHeightList[i];
        textBoxList[i].label.handle = *textBoxLabelHandleList[i];
        textBoxList[i].label.text   = textBoxLabelTitleList[i];
        textBoxList[i].label.startX = (textBoxStartXList[i] + textBoxWidthList[i] + GAP_WIDTH);
        textBoxList[i].label.startY = textBoxStartYList[i];
        textBoxList[i].label.width  = textBoxWidthList[i];
        textBoxList[i].label.height = textBoxHeightList[i];
    }
}

void text_box_default_text(wd_camera_capture_time_settings_t captureTime) {
    textBoxDefaultText[0] = malloc(9 * sizeof(char));
    textBoxDefaultText[1] = malloc(9 * sizeof(char));
    textBoxDefaultText[2] = malloc(9 * sizeof(char));
    dt_time_to_string(textBoxDefaultText[0], captureTime.startTime, TRUE);
    dt_time_to_string(textBoxDefaultText[1], captureTime.endTime, TRUE);
    dt_time_to_string(textBoxDefaultText[2], captureTime.intervalTime, FALSE);
}

void free_text_box_default_text(void) {
    free(textBoxDefaultText[0]);
    free(textBoxDefaultText[1]);
    free(textBoxDefaultText[2]);
}

/*
ALL THE DROPBOX INFORMATION IS HERE
*/

HWND dropDownCameraResolution;

#define NUMBER_OF_CAM_RESOLUTIONS 7
const char* cameraResolutionStrings[50]                  = {"320x240",  "352x288",   "640x480",  "800x600",
                                           "1024x768", "1280x1024", "1600x1200"};
framesize_t cameraResolutions[NUMBER_OF_CAM_RESOLUTIONS] = {
    FRAMESIZE_QVGA, FRAMESIZE_CIF, FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA, FRAMESIZE_SXGA, FRAMESIZE_UXGA};

int cam_res_to_list_index(uint8_t camRes) {
    switch (camRes) {
        case (WD_CAM_RES_320x240):
            return 0;
        case (WD_CAM_RES_352x288):
            return 1;
        case (WD_CAM_RES_640x480):
            return 2;
        case (WD_CAM_RES_800x600):
            return 3;
        case (WD_CAM_RES_1024x768):
            return 4;
        case (WD_CAM_RES_1280x1024):
            return 5;
        case (WD_CAM_RES_1600x1200):
            return 6;
        default:
            printf("error when converting the camera resolution into a list index, cam_res_to_list_index "
                   "function\n");
            return 0;
    }
}

/*
FUNCTIONS TO CREATE BUTTONS, LABELS, DROPBOXES and TEXTBOXES
*/

HWND create_button(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindow("BUTTON", title, WS_VISIBLE | WS_CHILD, startX, startY, width, height, hwnd, handle, NULL,
                        NULL);
}

HWND create_label(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindow("STATIC", title, WS_VISIBLE | WS_CHILD | SS_NOTIFY, startX, startY, width, height, hwnd, handle,
                        NULL, NULL);
}

HWND create_dropbox(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle,
                    int numberOfOptions, const char* nameOfOptions[40], int indexOfDisplayedOption) {
    HWND dropBox =
        CreateWindow("COMBOBOX", title, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, startX,
                     startY, width, height, hwnd, handle, NULL, NULL);
    for (int i = 0; i < numberOfOptions; i++) {
        SendMessage(dropBox, CB_ADDSTRING, 0, (LPARAM)nameOfOptions[i]);
    }
    SendMessage(dropBox, CB_SETCURSEL, (WPARAM)indexOfDisplayedOption, (LPARAM)0);
    return dropBox;
}

HWND create_text_box(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle,
                     char* defaultText) {
    return CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", defaultText, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, startX, startY,
                          width, height, hwnd, handle, GetModuleHandle(NULL), NULL);
}

/*
THESE FUNCTIONS ARE FOR WHAT HAPPENS WHEN THE USER CLICKS OFF A TEXT BOX
*/
uint8_t click_off_start_tb(HWND hwnd) {
    uint8_t success;

    // pull the text from the text box
    int textBoxCharLen = GetWindowTextLength(textBoxList[TEXT_BOX_START_TIME].handle) + 1;
    char* textBoxText  = (char*)malloc(textBoxCharLen * sizeof(char));
    GetWindowText(textBoxList[TEXT_BOX_START_TIME].handle, textBoxText, textBoxCharLen);

    // Check if the text in the text box is valid
    if (dt_is_valid_hour_min_period(textBoxText)) {
        ShowWindow(textBoxList[TEXT_BOX_START_TIME].label.handle, SW_HIDE);
        success = TRUE;
        UpdateWindow(hwnd);
        char period[3];
        sscanf(textBoxText, "%d:%d %2s", &startTimeHr, &startTimeMin, period);
        // The info is stored in 24 hour time so if it is pm add 12 hrs, if it is 12, take 12
        if (startTimeHr == 12) {
            startTimeHr -= 12;
        }
        if (chars_same(period, "pm\0") == TRUE) {
            startTimeHr += 12;
        }
        // TODO: Send the bpacket to change the start time
    } else {
        // Write, in red, invalid input over the textbox
        ShowWindow(textBoxList[TEXT_BOX_START_TIME].label.handle, SW_SHOW);
        success = FALSE;
        UpdateWindow(hwnd);
    }
    free(textBoxText);
    return success;
}

uint8_t click_off_end_tb(HWND hwnd) {
    uint8_t success;

    // pull the text from the text box
    int textBoxCharLen = GetWindowTextLength(textBoxList[TEXT_BOX_END_TIME].handle) + 1;
    char* textBoxText  = (char*)malloc(textBoxCharLen * sizeof(char));
    GetWindowText(textBoxList[TEXT_BOX_END_TIME].handle, textBoxText, textBoxCharLen);

    // Check if the text in the text box is valid and that the end time is after the start time
    if (dt_is_valid_hour_min_period(textBoxText)) {
        char period[3];
        sscanf(textBoxText, "%d:%d %2s", &endTimeHr, &endTimeMin, period);
        // The info is stored in 24 hour time so if it is pm add 12 hrs and if its a 12, minus 12
        // hours
        if (endTimeHr == 12) {
            endTimeHr -= 12;
        }
        if (chars_same(period, "pm\0") == TRUE) {
            endTimeHr += 12;
        }
        // Make sure that the end time is after the start time
        if ((startTimeHr * 60 + startTimeMin) < (endTimeHr * 60 + endTimeMin)) {
            ShowWindow(textBoxList[TEXT_BOX_END_TIME].label.handle, SW_HIDE);
            success = TRUE;
            UpdateWindow(hwnd);
        } else {
            ShowWindow(textBoxList[TEXT_BOX_END_TIME].label.handle, SW_SHOW);
            success = FALSE;
            UpdateWindow(hwnd);
        }
    } else {
        ShowWindow(textBoxList[TEXT_BOX_END_TIME].label.handle, SW_SHOW);
        success = FALSE;
        UpdateWindow(hwnd);
    }
    free(textBoxText);
    return success;
}

uint8_t click_off_interval_tb(HWND hwnd) {
    uint8_t success;

    // pull the text from the text box
    int textBoxCharLen = GetWindowTextLength(textBoxList[TEXT_BOX_TIME_INTERVAL].handle) + 1;
    char* textBoxText  = (char*)malloc(textBoxCharLen * sizeof(char));
    GetWindowText(textBoxList[TEXT_BOX_TIME_INTERVAL].handle, textBoxText, textBoxCharLen);

    // Check if the text in the text box is valid
    if (dt_time_format_is_valid(textBoxText)) {
        sscanf(textBoxText, "%d:%d", &timeIntervalHr, &timeIntervalMin);
        // Make sure that the time interval plus the start time isnt into the next day
        if (((startTimeHr * 60 + startTimeMin + timeIntervalHr * 60 + timeIntervalMin) < (24 * 60)) &&
            ((timeIntervalHr * 60 + timeIntervalMin) > 0)) {
            ShowWindow(textBoxList[TEXT_BOX_TIME_INTERVAL].label.handle, SW_HIDE);
            success = TRUE;
            UpdateWindow(hwnd);
        } else {
            ShowWindow(textBoxList[TEXT_BOX_TIME_INTERVAL].label.handle, SW_SHOW);
            success = FALSE;
            UpdateWindow(hwnd);
        }
    } else {
        ShowWindow(textBoxList[TEXT_BOX_TIME_INTERVAL].label.handle, SW_SHOW);
        success = FALSE;
        UpdateWindow(hwnd);
    }
    free(textBoxText);
    return success;
}

int cameraViewOn = FALSE;

// This function is for changing from normal -> camera view and back, takes CAMERA_VIEW and NORMAL_VIEW view macros,
// these are just SW_HIDE and SW_SHOW respectively
void gui_change_view(int cameraViewMode, HWND hwnd) {

    // Text boxes
    for (int i = 0; i < NUMBER_OF_TEXT_BOXES; i++) {
        ShowWindow(textBoxList[i].handle, cameraViewMode);
        ShowWindow(textBoxList[i].label.handle, cameraViewMode);
    }

    // Drop downs
    ShowWindow(dropDownCameraResolution, cameraViewMode);

    // The labels
    for (int i = 0; i < NUMBER_OF_LABELS; i++) {
        ShowWindow(labelList[i].handle, cameraViewMode);
    }

    // The buttons
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
        ShowWindow(buttonList[i].handle, cameraViewMode);
    }

    if (cameraViewMode == CAMERA_VIEW) {
        ShowWindow(buttonList[BUTTON_NORMAL_VIEW].handle, SW_SHOW);

    } else if (cameraViewMode == NORMAL_VIEW) {
        ShowWindow(buttonList[BUTTON_NORMAL_VIEW].handle, SW_HIDE);
        textBoxFlags |= (TEXT_BOX_START_TIME_FLAG | TEXT_BOX_END_TIME_FLAG | TEXT_BOX_TIME_INTERVAL_FLAG);
    }

    UpdateWindow(hwnd);
}

void send_current_camera_settings(void) {

    // uint8_t result = wd_settings_to_bpacket(
    //     guiToMainCircularBuffer->circularBuffer[*guiToMainCircularBuffer->writeIndex], BPACKET_ADDRESS_STM32,
    //     BPACKET_ADDRESS_MAPLE, WATCHDOG_BPK_R_SET_SETTINGS, BPACKET_CODE_EXECUTE, &settings);
    // if (result != TRUE) {
    //     char msg[50];
    //     wd_get_error(result, msg);
    //     printf(msg);
    // }
    // bpacket_increment_circular_buffer_index(guiToMainCircularBuffer->writeIndex);
    // return;
}

void send_current_capture_time_settings(void) {

    // uint8_t result = wd_settings_to_bpacket(
    //     guiToMainCircularBuffer->circularBuffer[*guiToMainCircularBuffer->writeIndex], BPACKET_ADDRESS_STM32,
    //     BPACKET_ADDRESS_MAPLE, WATCHDOG_BPK_R_SET_SETTINGS, BPACKET_CODE_EXECUTE, &settings);
    // if (result != TRUE) {
    //     char msg[50];
    //     wd_get_error(result, msg);
    //     printf(msg);
    // }
    // bpacket_increment_circular_buffer_index(guiToMainCircularBuffer->writeIndex);
    // return;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

        case WM_CREATE:;

            // CREATE TEXT BOXES
            initalise_text_box_structs(&textBoxList[0]);
            for (int i = 0; i < NUMBER_OF_TEXT_BOXES; i++) {
                textBoxList[i].handle =
                    create_text_box("Text_Box", textBoxList[i].startX, textBoxList[i].startY, textBoxList[i].width,
                                    textBoxList[i].height, hwnd, NULL, textBoxDefaultText[i]);
                textBoxList[i].label.handle =
                    create_label(textBoxList[i].label.text, textBoxList[i].label.startX, textBoxList[i].label.startY,
                                 textBoxList[i].label.width, textBoxList[i].label.height, hwnd, NULL);
                ShowWindow(textBoxList[i].label.handle, SW_HIDE);
            }

            // CREATE DROP BOXES
            dropDownCameraResolution = create_dropbox("Title", COL_2, ROW_8, DROP_BOX_WIDTH, DROP_BOX_HEIGHT, hwnd,
                                                      NULL, NUMBER_OF_CAM_RESOLUTIONS, cameraResolutionStrings,
                                                      cam_res_to_list_index(cameraSettings.resolution));

            // CREATE BUTTONS
            initalise_button_structs(&buttonList[0]);
            for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
                buttonList[i].handle = create_button(buttonList[i].text, buttonList[i].startX, buttonList[i].startY,
                                                     buttonList[i].width, buttonList[i].height, hwnd, NULL);
            }
            ShowWindow(buttonList[BUTTON_NORMAL_VIEW].handle, SW_HIDE);

            // create LABELS
            initalise_label_structs(&labelList[0]);
            for (int i = 0; i < NUMBER_OF_LABELS; i++) {
                labelList[i].handle = create_label(labelList[i].text, labelList[i].startX, labelList[i].startY,
                                                   labelList[i].width, labelList[i].height, hwnd, NULL);
            }

            break;

        case WM_COMMAND:

            // Handle button clicks
            if ((HWND)lParam == buttonList[BUTTON_OPEN_SD_CARD].handle) {
                *flags |= GUI_TURN_RED_LED_ON;
                // TODO: decide what is going to happen when this button is clicked

                bpacket_create_p(guiToMainCircularBuffer->circularBuffer[*guiToMainCircularBuffer->writeIndex],
                                 BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_MAPLE, BPACKET_CODE_EXECUTE,
                                 WATCHDOG_BPK_R_LED_RED_ON, 0, NULL);
                bpacket_increment_circular_buffer_index(guiToMainCircularBuffer->writeIndex);
            }

            if ((HWND)lParam == buttonList[BUTTON_EXPORT_DATA].handle) {
                *flags |= GUI_TURN_RED_LED_OFF;
                // TODO: decide what is going to happen when this button is clicked

                bpacket_create_p(guiToMainCircularBuffer->circularBuffer[*guiToMainCircularBuffer->writeIndex],
                                 BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_MAPLE, BPACKET_CODE_EXECUTE,
                                 WATCHDOG_BPK_R_LED_RED_OFF, 0, NULL);
                bpacket_increment_circular_buffer_index(guiToMainCircularBuffer->writeIndex);
                // printf("Exporting SD card data\n");
            }

            if ((HWND)lParam == buttonList[BUTTON_CAMERA_VIEW].handle) {
                cameraViewOn = TRUE;
                gui_change_view(CAMERA_VIEW, hwnd);

                printf("Starting livestream\n");
                InvalidateRect(hwnd, NULL, TRUE);
                rectangle_t rectangle;
                rectangle.startX = COL_1;
                rectangle.startY = ROW_3;
                rectangle.width  = 600;
                rectangle.height = 480;
                bpacket_create_p(guiToMainCircularBuffer->circularBuffer[*guiToMainCircularBuffer->writeIndex],
                                 BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_MAPLE, BPACKET_CODE_EXECUTE,
                                 WATCHDOG_BPK_R_STREAM_IMAGE, 0, NULL);
                bpacket_increment_circular_buffer_index(guiToMainCircularBuffer->writeIndex);

                draw_image(hwnd, CAMERA_VIEW_FILENAME, &rectangle);
            }

            if (LOWORD(wParam) == BUTTON_HELP_HANDLE) {
                system("start help.pdf");
            }

            if ((HWND)lParam == buttonList[BUTTON_NORMAL_VIEW].handle) {
                cameraViewOn = FALSE;

                // Clear the screen
                rectangle_t rectangle;
                rectangle.startX = COL_1;
                rectangle.startY = ROW_2;
                rectangle.width  = 600;
                rectangle.height = 480;
                draw_rectangle(hwnd, &rectangle, 255, 255, 255);
                gui_change_view(NORMAL_VIEW, hwnd);
            }

            if ((HWND)lParam == buttonList[BUTTON_RUN_TEST].handle) {
                printf("Testing system\n");
                // TODO: decide what this button is exactly going to do
            }

            if ((HWND)lParam == buttonList[BUTTON_HELP].handle) {
                system("start help.pdf");
            }

            // Handle drop down box
            if ((HWND)lParam == dropDownCameraResolution) {
                // Sends multiple messages but we only want to check for when the message is the change box
                // message
                if (HIWORD(wParam) == CBN_SELCHANGE) {
                    // handle drop-down menu selection
                    int itemIndex = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                    TCHAR buffer[256];
                    SendMessage((HWND)lParam, CB_GETLBTEXT, itemIndex, (LPARAM)buffer);
                    printf("Selected item %i: %s\n", itemIndex, buffer);
                    cameraSettings.resolution = (uint8_t)cameraResolutions[itemIndex];
                    send_current_camera_settings();
                    // TODO: send bpacket of which camera resolution is wanted
                }
            }

            // Handle the text boxes here

            if ((HWND)lParam == textBoxList[TEXT_BOX_START_TIME].handle) {
                textBoxFlags |= TEXT_BOX_START_TIME_FLAG;
            }

            if ((HWND)lParam == textBoxList[TEXT_BOX_END_TIME].handle) {
                textBoxFlags |= TEXT_BOX_END_TIME_FLAG;
            }

            if ((HWND)lParam == textBoxList[TEXT_BOX_TIME_INTERVAL].handle) {
                textBoxFlags |= TEXT_BOX_TIME_INTERVAL_FLAG;
            }

            // This if statment will be evaluated as true when the user click off the start time textbox
            if (((HWND)lParam != textBoxList[TEXT_BOX_START_TIME].handle) &&
                (textBoxFlags & TEXT_BOX_START_TIME_FLAG) && (cameraViewOn == FALSE)) {
                // Unset the flag
                textBoxFlags &= ~TEXT_BOX_START_TIME_FLAG;

                uint8_t success = click_off_start_tb(hwnd);
                if (success == TRUE) {
                    // Add startTimeHr and startTimeMinute into the
                    captureTime.startTime.hour   = startTimeHr;
                    captureTime.startTime.minute = startTimeMin;
                    printf("Valid start time. Sending to STM32\n");
                    send_current_capture_time_settings();
                }

                // Set the flags so the other textboxes will be checked if this change made them invalid
                textBoxFlags |= (TEXT_BOX_END_TIME_FLAG | TEXT_BOX_TIME_INTERVAL_FLAG);
            }

            // This if statment will be evaluated as true when the user click off the end time textbox
            if (((HWND)lParam != textBoxList[TEXT_BOX_END_TIME].handle) && (textBoxFlags & TEXT_BOX_END_TIME_FLAG) &&
                (cameraViewOn == FALSE)) {
                // Unset the flag
                textBoxFlags &= ~TEXT_BOX_END_TIME_FLAG;

                uint8_t success = click_off_end_tb(hwnd);
                if (success == TRUE) {
                    captureTime.endTime.hour   = endTimeHr;
                    captureTime.endTime.minute = endTimeMin;
                    printf("Valid end time. Sending to STM32\n");
                    send_current_capture_time_settings();
                }

                // Set the flags so the other textboxes will be checked if this change made them invalid
                textBoxFlags |= (TEXT_BOX_START_TIME_FLAG | TEXT_BOX_TIME_INTERVAL_FLAG);
            }

            // This if statment will be evaluated as true when the user click off the time interval time textbox
            if (((HWND)lParam != textBoxList[TEXT_BOX_TIME_INTERVAL].handle) &&
                (textBoxFlags & TEXT_BOX_TIME_INTERVAL_FLAG) && (cameraViewOn == FALSE)) {
                // Unset the flag
                textBoxFlags &= ~TEXT_BOX_TIME_INTERVAL_FLAG;
                uint8_t success = click_off_interval_tb(hwnd);
                if (success == TRUE) {
                    captureTime.intervalTime.hour   = timeIntervalHr;
                    captureTime.intervalTime.minute = timeIntervalMin;
                    printf("Valid time interval. Sending to STM32\n");
                    send_current_capture_time_settings();
                }
                // Set the flags so the other textboxes will be checked if this change made them invalid
                textBoxFlags |= (TEXT_BOX_START_TIME_FLAG | TEXT_BOX_END_TIME_FLAG);
            }

            break;

        case WM_PAINT: // This is called anytime the window needs to be redrawn
            // if (cameraViewOn == TRUE) {
            //     DrawPixels(hwnd);
            // }
            break;
        case WM_DESTROY:
            // close the application
            PostQuitMessage(0);
            *flags |= GUI_CLOSE;
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int folder_exists(const char* path) {
    struct stat st;
    int result = stat(path, &st);
    if (result == 0 && S_ISDIR(st.st_mode)) {
        return TRUE;
    }
    return FALSE;
}

// Main function for the Maple thread
DWORD WINAPI gui(void* arg) {

    const char* path = "Watchdog";
    if (folder_exists(path) == FALSE) {
        printf("Watchdog file not found");
        // TODO: make the file
        return FALSE;
    }

    gui_initalisation_t* guiInit = (gui_initalisation_t*)arg;
    watchdog                     = guiInit->watchdog;
    flags                        = guiInit->flags;
    guiToMainCircularBuffer      = guiInit->guiToMain;
    mainToGuiCircularBuffer      = guiInit->mainToGui;

    cameraViewImagePosition.startX = COL_1;
    cameraViewImagePosition.startY = ROW_2;
    cameraViewImagePosition.width  = 600;
    cameraViewImagePosition.height = 480;

    WNDCLASSEX wc;
    HWND hwnd;
    MSG msg;

    // register the window class
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = NULL;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "myWindowClass";
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    /* GET THE CAPTURE TIME SETTINGS*/
    uint8_t result = bpacket_create_p(guiToMainCircularBuffer->circularBuffer[*guiToMainCircularBuffer->writeIndex],
                                      BPACKET_ADDRESS_STM32, BPACKET_ADDRESS_MAPLE,
                                      WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS, BPACKET_CODE_EXECUTE, 0, NULL);
    if (result != TRUE) {
        char msg[50];
        bpacket_get_error(result, msg);
        printf("%s\n", msg);
    }
    bpacket_increment_circular_buffer_index(guiToMainCircularBuffer->writeIndex);

    /* GET THE CAMERA SETTINGS*/
    result = bpacket_create_p(guiToMainCircularBuffer->circularBuffer[*guiToMainCircularBuffer->writeIndex],
                              BPACKET_ADDRESS_ESP32, BPACKET_ADDRESS_MAPLE, WATCHDOG_BPK_R_GET_CAMERA_SETTINGS,
                              BPACKET_CODE_EXECUTE, 0, NULL);
    if (result != TRUE) {
        char msg[50];
        bpacket_get_error(result, msg);
        printf("%s\n", msg);
    }
    bpacket_increment_circular_buffer_index(guiToMainCircularBuffer->writeIndex);

    bpacket_t* receivedBpacket;
    cameraSettingsFlag = FALSE;

    printf("HERE 1\n");
    // Before it goes into the main loop the settings need to be returned
    while (cameraSettingsFlag == FALSE || captureTimeFlag == FALSE) {
        // printf("Y\n");
        if (*mainToGuiCircularBuffer->readIndex != *mainToGuiCircularBuffer->writeIndex) {
            printf("HERE 1.5\n");

            receivedBpacket = MTG_CB_CURRENT_BPACKET;
            bpacket_increment_circular_buffer_index(mainToGuiCircularBuffer->readIndex);

            if (receivedBpacket->request == WATCHDOG_BPK_R_GET_CAPTURE_TIME_SETTINGS) {
                wd_bpacket_to_camera_settings(receivedBpacket, &cameraSettings);
                cameraSettingsFlag = TRUE;
                text_box_default_text(captureTime);
            }
            if (receivedBpacket->request == WATCHDOG_BPK_R_GET_CAMERA_SETTINGS) {
                wd_bpacket_to_capture_time_settings(receivedBpacket, &captureTime);
                captureTimeFlag = TRUE;
            }
        }
    }

    printf("HERE 2\n");

    // Create the window
    // The 0, 0 is coodinates of the top left of the window, orginally it was CW_USEDEFAULT, CW_USEDEFAULT
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "myWindowClass", "Watchdog", WS_OVERLAPPEDWINDOW, 0, 0, WINDOW_WIDTH,
                          WINDOW_HEIGHT, NULL, NULL, NULL, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    // Free the memory for the default text box text
    free_text_box_default_text();

    // message loop
    while (GetMessage(&msg, NULL, 0, 0) > 0) {

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Check flag from maple
        if ((*flags & GUI_UPDATE_CAMERA_VIEW) != 0) {
            // draw_image(hwnd, cameraViewFileName, &cameraViewImagePosition);
        }

        // If a Bpacket is received from main, deal with it in here
        if (*mainToGuiCircularBuffer->readIndex != *mainToGuiCircularBuffer->writeIndex) {

            receivedBpacket = MTG_CB_CURRENT_BPACKET;
            bpacket_increment_circular_buffer_index(mainToGuiCircularBuffer->readIndex);
            if (receivedBpacket->code != TRUE) {

                char msg[50];
                bpacket_get_info(receivedBpacket, msg);
                printf(msg);

                printf("The code of the Bpacket wasn't 'success'\n"); // TODO: make this a better print
            }

            if (receivedBpacket->request == GUI_BPK_R_UPDATE_STREAM_IMAGE) {
                // Clear the screen
                rectangle_t rectangle;
                rectangle.startX = COL_1;
                rectangle.startY = ROW_2;
                rectangle.width  = 600;
                rectangle.height = 480;
                draw_rectangle(hwnd, &rectangle, 255, 255, 255);
                draw_image(hwnd, CAMERA_VIEW_FILENAME, &rectangle);
            }
            // The bpacket is now received, now it can be one of a bunch of possible requets.
            // Now check which request it is and do what you need to do
        }
        // switch (receivedBpacket->request) {
        //     case WATCHDOG_BPK_R_GET_DATETIME:

        //         break;
        // }
    }

    return FALSE;
}

/* Private Functions */

uint8_t draw_image(HWND hwnd, char* filePath, rectangle_t* position) {

    /* Try to load images using stb libary. Only jpg images have been tested */

    // Variables to store image data. n represents number of colour channels.
    // For jpg RGB images n = 3 (R, G, B)
    int width, height, n;
    unsigned char* imageData = stbi_load(filePath, &width, &height, &n, 0);

    // Confirm file data was able to be opened
    if (imageData == NULL) {
        printf("Image failed to load\n");
        stbi_image_free(imageData);
        return FALSE;
    }

    // Confirm image was RGB
    if (n != 3) {
        printf("Image was not RGB image");
        stbi_image_free(imageData);
        return FALSE;
    }

    unsigned char* pixelData;

    // Resize the image if the desired size does not meet the image size
    int resizeImage = (position->width != width || position->height != height) ? TRUE : FALSE;
    if (resizeImage == TRUE) {

        // Allocate space for resized image data
        pixelData = malloc(sizeof(unsigned char) * position->width * position->height * n);

        // Confirm the space could be allocated
        if (pixelData == NULL) {
            printf("Unable to allocate space to draw image\n");
            return FALSE;
        }

        // Resize the image using stb library
        if (stbir_resize_uint8(imageData, width, height, 0, pixelData, position->width, position->height, 0, n) !=
            TRUE) {
            printf("Error resizing image\n");
            stbi_image_free(imageData);
            free(pixelData);
            return FALSE;
        }

        // Free the image data
        stbi_image_free(imageData);
    } else {
        pixelData = imageData;
        printf("%p %p\n", pixelData, imageData);
    }

    // Invalidate the window. This will tell windows that the window needs to be redrawn
    // so the painting below will render
    InvalidateRect(hwnd, NULL, TRUE);

    // Convert pixel data to a bit map
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = position->width;
    bmi.bmiHeader.biHeight      = -position->height; // Negative to ensure picture is drawn vertical correctly
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    // // Being painting
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // Render image onto window
    if (StretchDIBits(hdc, position->startX, position->startY, position->width, position->height, 0, 0, position->width,
                      position->height, pixelData, &bmi, DIB_RGB_COLORS, SRCCOPY) == FALSE) {
        printf("Failed to render image\n");
        return FALSE;
    }

    // End painting
    EndPaint(hwnd, &ps);

    // Free used resources
    if (resizeImage == TRUE) {
        free(pixelData);
    } else {
        stbi_image_free(pixelData);
    }

    return TRUE;
}

void draw_rectangle(HWND hwnd, rectangle_t* rectangle, uint8_t r, uint8_t g, uint8_t b) {

    // Invalidate the window. This will tell windows that the window needs to be redrawn
    // so the painting below will render
    InvalidateRect(hwnd, NULL, TRUE);

    // Create a paint struct so the window can be painted on
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // Create a rectangle
    RECT rect = {rectangle->startX, rectangle->startY, rectangle->width, rectangle->width};

    // Create a colour for the rectangle
    HBRUSH hBrush = CreateSolidBrush(RGB(r, g, b));

    // Apply colour to the rectangle
    FillRect(hdc, &rect, hBrush);

    // Clean up
    DeleteObject(hBrush);
    EndPaint(hwnd, &ps);
}

void gui_test(void) {

    /* Test that the GUI can turn the red LED on */
}

/* FUNCTIONS THAT WE ARENT USING ANYMORE*/

// Just leaving this here incase we need to draw something pixel by pixel
// void draw_pixels() {
// Loop through every pixel
// int y;
// int x;
// for (y = 0; y < newHeight; y++) {
//     for (x = 0; x < newWidth; x++) {
//         if (n == 3) {
//             int r = outputData[(y * newWidth * n) + x * n + 0];
//             int g = outputData[(y * newWidth * n) + x * n + 1];
//             int b = outputData[(y * newWidth * n) + x * n + 2];
//             SetPixel(hdc, x, y, RGB(r, g, b));
//         }
//         // Extract the R, G, B components
//         // int r = (y * width) + x + 0;
//         // int g = (y * width) + x + 1;
//         // int b = (y * width) + x + 2;
//         // printf("%i %i %i ", r, g, b);
//         // SetPixel(hdc, x, y, RGB(r, g, b));
//     }
//     // printf("\n");
// }

// for (int i = 0; i < 1000; i++) {

//     int x = rand() % r.right;
//     int y = rand() % r.bottom;
//     SetPixel(hdc, x, y, RGB(255, 0, 0));
// }

// free(outputData);
// stbi_image_free(data);
// EndPaint(hwnd, &ps);
// }