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

/* Personal Includes */
#include "gui.h"
#include "utilities.h"
#include "watchdog_defines.h"

// #define STB_IMAGE_IMPLEMENTATION // Required for stb_image
#include "stb_image.h"
#include "stb_image_resize.h"

/* Private Macros */
#define IDC_COMBOBOX  32511
#define LEFT_MARGIN   10
#define MIDDLE_MARGIN 270
#define TOP_MARGIN    10
#define RIGHT_MARGIN  500

#define BUTTON_WIDTH  200
#define BUTTON_HEIGHT 30

#define LABEL_WIDTH         200
#define HEADING_LABEL_WIDTH LABEL_WIDTH * 2
#define LABEL_HEIGHT        30

#define DROP_BOX_WIDTH       200
#define DROP_BOX_THIRD_WIDTH 60
#define DROP_BOX_HEIGHT      300

#define TEXT_BOX_WIDTH  95
#define TEXT_BOX_HEIGHT 30

// #define WINDOW_WIDTH  (RIGHT_MARGIN + LABEL_WIDTH + (GAP_WIDTH * 3))
// #define WINDOW_HEIGHT ((LABEL_HEIGHT * 2) + TOP_MARGIN + 300)

#define WINDOW_WIDTH  1300
#define WINDOW_HEIGHT 768

#define GAP_WIDTH  10
#define GAP_HEIGHT 10

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

#define COL_1           10
#define COL_1_ONE_THIRD (COL_1 + 70)
#define COL_1_ONE_HALF  (COL_1 + 105)
#define COL_1_TWO_THIRD (COL_1 + 140)
#define COL_2           (COL_1 + LABEL_WIDTH + GAP_WIDTH)
#define COL_2_ONE_THIRD (COL_2 + 70)
#define COL_2_TWO_THIRD (COL_2 + 140)
#define COL_3           (COL_2 + LABEL_WIDTH + GAP_WIDTH)
#define COL_4           (COL_3 + LABEL_WIDTH + GAP_WIDTH)
#define COL_5           (COL_4 + LABEL_WIDTH + GAP_WIDTH)
#define COL_6           (COL_5 + LABEL_WIDTH + GAP_WIDTH)
#define COL_7           (COL_6 + LABEL_WIDTH + GAP_WIDTH)
#define COL_8           (COL_7 + LABEL_WIDTH + GAP_WIDTH)

#define BUTTON_CAMERA_VIEW_HANDLE         1
#define BUTTON_OPEN_SD_CARD_HANDLE        2
#define BUTTON_EXPORT_DATA_HANDLE         3
#define BUTTON_RUN_TEST_HANDLE            4
#define BUTTON_NORMAL_VIEW_HANDLE         5
#define DROP_BOX_CAMERA_RESOLUTION_HANDLE 6
#define TEXT_BOX_START_TIME_HANDLE        7
#define TEXT_BOX_END_TIME_HANDLE          8
#define TEXT_BOX_TIME_INTERVAL_HANDLE     9
#define TEXT_BOX_RTC_DATE_HANDLE          10
#define TEXT_BOX_RTC_TIME_HANDLE          11

#define NUMBER_OF_CAM_RESOLUTIONS 7

const char* cameraResolutionStrings[50] = {"320x240",  "352x288",   "640x480",  "800x600",
                                           "1024x768", "1280x1024", "1600x1200"};

HWND textBoxStartTime, textBoxEndTime, textBoxTimeInterval, textBoxRtcDate, textBoxRtcDate, textBoxRtcTime;
HWND dropDownCameraResolution;
HWND labelCameraResolution, labelPhotoFrequency, labelStatus, labelID, labelNumImages, labelDateRtc, labelTimeRtc,
    labelSetUp, labelData, labelSettings, labelStartTime, labelEndTime, labelTimeInterval, labelDateHeading,
    labelTimeInfo;
HWND buttonCameraView, buttonOpenSDCard, buttonExportData, buttonRunTest, buttonNormalView;
HFONT hFont;
typedef struct rectangle_t {
    int startX;
    int startY;
    int width;
    int height;
} rectangle_t;

rectangle_t cameraViewImagePosition;

uint8_t draw_image(HWND hwnd, char* filePath, rectangle_t* position);
void draw_rectangle(HWND hwnd, rectangle_t* rectangle, uint8_t r, uint8_t g, uint8_t b);
void gui_update_camera_view(char* fileName);
framesize_t cameraResolutions[NUMBER_OF_CAM_RESOLUTIONS] = {
    FRAMESIZE_QVGA, FRAMESIZE_CIF, FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA, FRAMESIZE_SXGA, FRAMESIZE_UXGA};

watchdog_info_t* watchdog;
uint32_t* flags;
bpacket_circular_buffer_t* guiToMainCircularBuffer;
bpacket_circular_buffer_t* mainToGuiCircularBuffer;

HWND create_button(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindow("BUTTON", title, WS_VISIBLE | WS_CHILD, startX, startY, width, height, hwnd, handle, NULL,
                        NULL);
}

HWND create_label(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindow("STATIC", title, WS_VISIBLE | WS_CHILD, startX, startY, width, height, hwnd, handle, NULL,
                        NULL);
}

HWND create_dropbox(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle,
                    int numberOfOptions, const char* nameOfOptions[40], int indexOfDisplayedOption) {
    HWND dropBox =
        // CreateWindow("COMBOBOX", title, CBS_DROPDOWN | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED |
        // WS_VISIBLE, startX,
        CreateWindow("COMBOBOX", title, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, startX,
                     startY, width, height, hwnd, handle, NULL, NULL);
    for (int i = 0; i < numberOfOptions; i++) {
        SendMessage(dropBox, CB_ADDSTRING, 0, (LPARAM)nameOfOptions[i]);
    }
    SendMessage(dropBox, CB_SETCURSEL, (WPARAM)indexOfDisplayedOption, (LPARAM)0);
    return dropBox;
}

HWND create_textbox(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, startX, startY, width,
                          height, hwnd, handle, GetModuleHandle(NULL), NULL);
}

int cameraViewOn = FALSE;

void gui_set_camera_view(HWND hwnd) {

    // Show all the text boxes
    ShowWindow(textBoxStartTime, SW_HIDE);
    ShowWindow(textBoxEndTime, SW_HIDE);
    ShowWindow(textBoxTimeInterval, SW_HIDE);
    ShowWindow(textBoxRtcDate, SW_HIDE);
    ShowWindow(textBoxRtcTime, SW_HIDE);

    // Hide all the drop downs
    ShowWindow(dropDownCameraResolution, SW_HIDE);

    // Hide all the labels
    ShowWindow(labelCameraResolution, SW_HIDE);
    ShowWindow(labelPhotoFrequency, SW_HIDE);
    ShowWindow(labelStatus, SW_HIDE);
    ShowWindow(labelID, SW_HIDE);
    ShowWindow(labelNumImages, SW_HIDE);
    ShowWindow(labelDateRtc, SW_HIDE);
    ShowWindow(labelTimeRtc, SW_HIDE);
    ShowWindow(labelSetUp, SW_HIDE);
    ShowWindow(labelData, SW_HIDE);
    ShowWindow(labelSettings, SW_HIDE);
    ShowWindow(labelStartTime, SW_HIDE);
    ShowWindow(labelEndTime, SW_HIDE);
    ShowWindow(labelTimeInterval, SW_HIDE);
    ShowWindow(labelDateHeading, SW_HIDE);
    ShowWindow(labelTimeInfo, SW_HIDE);

    // Hide all the buttons
    ShowWindow(buttonCameraView, SW_HIDE);
    ShowWindow(buttonOpenSDCard, SW_HIDE);
    ShowWindow(buttonExportData, SW_HIDE);
    ShowWindow(buttonRunTest, SW_HIDE);

    // Show the buttons used in camera view
    ShowWindow(buttonNormalView, SW_SHOW);
    UpdateWindow(hwnd);
}

void gui_set_normal_view(HWND hwnd) {

    // Show all the text boxes
    ShowWindow(textBoxStartTime, SW_SHOW);
    ShowWindow(textBoxEndTime, SW_SHOW);
    ShowWindow(textBoxTimeInterval, SW_SHOW);
    ShowWindow(textBoxRtcDate, SW_SHOW);
    ShowWindow(textBoxRtcTime, SW_SHOW);

    // Show all the drop downs
    ShowWindow(dropDownCameraResolution, SW_SHOW);

    // Show all the labels
    ShowWindow(labelCameraResolution, SW_SHOW);
    ShowWindow(labelPhotoFrequency, SW_SHOW);
    ShowWindow(labelStatus, SW_SHOW);
    ShowWindow(labelID, SW_SHOW);
    ShowWindow(labelNumImages, SW_SHOW);
    ShowWindow(labelDateRtc, SW_SHOW);
    ShowWindow(labelTimeRtc, SW_SHOW);
    ShowWindow(labelSetUp, SW_SHOW);
    ShowWindow(labelData, SW_SHOW);
    ShowWindow(labelSettings, SW_SHOW);
    ShowWindow(labelStartTime, SW_SHOW);
    ShowWindow(labelEndTime, SW_SHOW);
    ShowWindow(labelTimeInterval, SW_SHOW);
    ShowWindow(labelDateHeading, SW_SHOW);
    ShowWindow(labelTimeInfo, SW_SHOW);

    // Show all the buttons
    ShowWindow(buttonCameraView, SW_SHOW);
    ShowWindow(buttonOpenSDCard, SW_SHOW);
    ShowWindow(buttonExportData, SW_SHOW);
    ShowWindow(buttonRunTest, SW_SHOW);

    // Hide the normal view button
    ShowWindow(buttonNormalView, SW_HIDE);

    // make the changes
    UpdateWindow(hwnd);
}
#include <stdio.h>
#include <string.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

        case WM_CREATE:;

            char text[150];

            // TEXT BOXES

            textBoxStartTime = create_textbox("Title", COL_2, ROW_9, TEXT_BOX_WIDTH, TEXT_BOX_HEIGHT, hwnd,
                                              (HMENU)TEXT_BOX_START_TIME_HANDLE);

            textBoxEndTime = create_textbox("Title", COL_2, ROW_10, TEXT_BOX_WIDTH, TEXT_BOX_HEIGHT, hwnd,
                                            (HMENU)TEXT_BOX_START_TIME_HANDLE);

            textBoxTimeInterval = create_textbox("Title", COL_2, ROW_11, TEXT_BOX_WIDTH, TEXT_BOX_HEIGHT, hwnd,
                                                 (HMENU)TEXT_BOX_START_TIME_HANDLE);

            textBoxRtcDate = create_textbox("Title", COL_2, ROW_15, TEXT_BOX_WIDTH, TEXT_BOX_HEIGHT, hwnd,
                                            (HMENU)TEXT_BOX_START_TIME_HANDLE);

            textBoxRtcTime = create_textbox("Title", COL_2, ROW_16, TEXT_BOX_WIDTH, TEXT_BOX_HEIGHT, hwnd,
                                            (HMENU)TEXT_BOX_START_TIME_HANDLE);

            // DROP BOXES

            // Drop down box to change the camera resolution
            dropDownCameraResolution =
                create_dropbox("Title", COL_2, ROW_8, DROP_BOX_WIDTH, DROP_BOX_HEIGHT, hwnd, (HMENU)IDC_COMBOBOX,
                               NUMBER_OF_CAM_RESOLUTIONS, cameraResolutionStrings, 0);

            // BUTTONS

            // Button use to open and view the data on the SD card
            sprintf(text, "View Data");
            buttonOpenSDCard =
                create_button(text, COL_2, ROW_5, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)BUTTON_OPEN_SD_CARD_HANDLE);

            // Button to get live stream feed of camera
            sprintf(text, "Camera View");
            buttonCameraView =
                create_button(text, COL_1, ROW_2, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)BUTTON_CAMERA_VIEW_HANDLE);

            // Button to run a test on the system
            sprintf(text, "Test System");
            buttonRunTest =
                create_button(text, COL_2, ROW_2, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)BUTTON_RUN_TEST_HANDLE);

            // button to copy data from the watchdog to the computer
            sprintf(text, "Send Data to Computer");
            buttonExportData =
                create_button(text, COL_1, ROW_5, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)BUTTON_EXPORT_DATA_HANDLE);

            // button to go back to camera View
            sprintf(text, "Exit Camera View");
            buttonNormalView =
                create_button(text, COL_1, ROW_2, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)BUTTON_NORMAL_VIEW_HANDLE);
            ShowWindow(buttonNormalView, SW_HIDE);

            // LABELS

            // Status label to show the health of the system
            sprintf(text, " Status: %s", watchdog->status == SYSTEM_STATUS_OK ? "Ok" : "Error");
            labelStatus = create_label(text, COL_6, ROW_1, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // ID label to show the ID of the system
            sprintf(text, " ID: %x", watchdog->id);
            labelID = create_label(text, COL_6, ROW_2, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label to show how many images are on the SD card
            sprintf(text, " %i Image%s Taken", watchdog->numImages, watchdog->numImages == 1 ? "" : "s");
            labelNumImages = create_label(text, COL_6, ROW_3, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label for the current date of the system
            sprintf(text, " Current Date     (dd/mm/yyyy)");
            labelDateRtc = create_label(text, COL_1, ROW_15, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label for the current time of the system
            sprintf(text, " Current Time    (hh:mm am) ");
            labelTimeRtc = create_label(text, COL_1, ROW_16, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label "Set Up" above the camera view and test system button
            sprintf(text, " SET UP");
            labelSetUp = create_label(text, COL_1, ROW_1, HEADING_LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label "Data" above the send data and view data button
            sprintf(text, " DATA");
            labelData = create_label(text, COL_1, ROW_4, HEADING_LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label "Camera settings" above the resolution and photo frequency settings
            sprintf(text, " CAMERA SETTINGS");
            labelSettings = create_label(text, COL_1, ROW_7, HEADING_LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label "DATE" above settings to calibrate the real time clock
            sprintf(text, " DATE");
            labelDateHeading = create_label(text, COL_1, ROW_14, HEADING_LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label "Camera Resolution"
            sprintf(text, " Camera Resolution");
            labelCameraResolution = create_label(text, COL_1, ROW_8, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label "Start Time"
            sprintf(text, " Start Time            (hh:mm am)");
            labelStartTime = create_label(text, COL_1, ROW_9, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label "End Time"
            sprintf(text, " End Time             (hh:mm am)");
            labelEndTime = create_label(text, COL_1, ROW_10, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label "Time Interval"
            sprintf(text, " Time Interval       (hh:mm)");
            labelTimeInterval = create_label(text, COL_1, ROW_11, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label "Time Interval"
            sprintf(text, " THIS IS TO TELL YOU WHEN THE PHOTOS WILL BE TAKEN");
            labelTimeInfo = create_label(text, COL_1, ROW_12, LABEL_WIDTH * 4, LABEL_HEIGHT, hwnd, NULL);

            break;

        case WM_COMMAND:
            // handle button clicks
            if (LOWORD(wParam) == BUTTON_OPEN_SD_CARD_HANDLE) {
                *flags |= GUI_TURN_RED_LED_ON;
                // printf("Displaying SD card data\n");
                // TRY TO PUT SOMETHING IN A BPACKET

                bpacket_create_p(guiToMainCircularBuffer->circularBuffer[*guiToMainCircularBuffer->writeIndex],
                                 WATCHDOG_BPK_R_LED_RED_ON, 0, NULL);
                bpacket_increment_circular_buffer_index(guiToMainCircularBuffer->writeIndex);
            }

            if (LOWORD(wParam) == BUTTON_EXPORT_DATA_HANDLE) {
                *flags |= GUI_TURN_RED_LED_OFF;

                bpacket_create_p(guiToMainCircularBuffer->circularBuffer[*guiToMainCircularBuffer->writeIndex],
                                 WATCHDOG_BPK_R_LED_RED_OFF, 0, NULL);
                bpacket_increment_circular_buffer_index(guiToMainCircularBuffer->writeIndex);
                // printf("Exporting SD card data\n");
            }

            if (LOWORD(wParam) == BUTTON_CAMERA_VIEW_HANDLE) {
                cameraViewOn = TRUE;
                gui_set_camera_view(hwnd);

                printf("Starting livestream\n");
                InvalidateRect(hwnd, NULL, TRUE);
                rectangle_t rectangle;
                rectangle.startX = COL_1;
                rectangle.startY = ROW_3;
                rectangle.width  = 600;
                rectangle.height = 480;
                draw_image(hwnd, "img6.jpg", &rectangle);
            }

            if (LOWORD(wParam) == BUTTON_NORMAL_VIEW_HANDLE) {
                cameraViewOn = FALSE;

                // Clear the screen
                rectangle_t rectangle;
                rectangle.startX = COL_1;
                rectangle.startY = ROW_2;
                rectangle.width  = 600;
                rectangle.height = 480;
                draw_rectangle(hwnd, &rectangle, 255, 255, 255);
                gui_set_normal_view(hwnd);

                // Hide all buttons and show camera view
                // printf("Stopping livestream\n");
            }

            if (LOWORD(wParam) == BUTTON_RUN_TEST_HANDLE) {
                printf("Testing system\n");
            }

            if ((HWND)lParam == dropDownCameraResolution) {
                // Sends multiple messages but we only want to check for when the message is the change box
                // message
                if (HIWORD(wParam) == CBN_SELCHANGE) {
                    // handle drop-down menu selection
                    int itemIndex = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                    TCHAR buffer[256];
                    SendMessage((HWND)lParam, CB_GETLBTEXT, itemIndex, (LPARAM)buffer);
                    printf("Selected item %i: %s\n", itemIndex, buffer);
                    // NEED TO UPDATE THE FLAG HERE
                }
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

// Just leaving this here incase we need to draw something pixel by pixel
void draw_pixels() {
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
}

void gui_update() {
    MSG msg;

    if (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

DWORD WINAPI gui(void* arg) {

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

    // Create the window
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "myWindowClass", "Watchdog", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                          CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, NULL, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    // message loop
    while (GetMessage(&msg, NULL, 0, 0) > 0) {

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Check flag from maple
        if ((*flags & GUI_UPDATE_CAMERA_VIEW) != 0) {
            // draw_image(hwnd, cameraViewFileName, &cameraViewImagePosition);
        }
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