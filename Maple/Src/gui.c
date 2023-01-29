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
#include <windows.h>
#include <wingdi.h>
#include <CommCtrl.h>
#include <winuser.h>

#include <stdio.h>
#include <sensor.h>

/* Personal Includes */
#include "gui.h"

/* Private Macros */
#define IDC_COMBOBOX  32511
#define LEFT_MARGIN   10
#define MIDDLE_MARGIN 270
#define TOP_MARGIN    10
#define RIGHT_MARGIN  500

#define BUTTON_WIDTH  200
#define BUTTON_HEIGHT 30

#define LABEL_WIDTH  200
#define LABEL_HEIGHT 30

#define DROP_BOX_WIDTH  200
#define DROP_BOX_HEIGHT 300

#define WINDOW_WIDTH  (RIGHT_MARGIN + LABEL_WIDTH + (GAP_WIDTH * 3))
#define WINDOW_HEIGHT ((LABEL_HEIGHT * 2) + TOP_MARGIN + 300)

#define GAP_WIDTH  10
#define GAP_HEIGHT 10

#define ROW_1 10
#define ROW_2 (ROW_1 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_3 (ROW_2 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_4 (ROW_3 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_5 (ROW_4 + LABEL_HEIGHT + GAP_HEIGHT)
#define ROW_6 (ROW_5 + LABEL_HEIGHT + GAP_HEIGHT)

#define COL_1 10
#define COL_2 (COL_1 + LABEL_WIDTH + GAP_WIDTH)
#define COL_3 (COL_2 + LABEL_WIDTH + GAP_WIDTH)

#define BUTTON_CAMERA_VIEW_HANDLE         1
#define BUTTON_OPEN_SD_CARD_HANDLE        2
#define BUTTON_EXPORT_DATA_HANDLE         3
#define BUTTON_RUN_TEST_HANDLE            4
#define DROP_BOX_CAMERA_RESOLUTION_HANDLE 5
#define DROP_BOX_PHOTO_FREQUENCY_HANDLE   6

#define NUMBER_OF_CAM_RESOLUTIONS 7

HWND dropDownCameraResolution, dropDownPhotoFrequency;
HWND LabelCameraResolution, labelPhotoFrequency, labelStatus, labelID, labelNumImages, labelDate;
HWND buttonCameraView, buttonOpenSDCard, buttonExportData, buttonRunTest;
HFONT hFont;

framesize_t cameraResolutions[NUMBER_OF_CAM_RESOLUTIONS] = {
    FRAMESIZE_QVGA, FRAMESIZE_CIF, FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA, FRAMESIZE_SXGA, FRAMESIZE_UXGA};
const char* cameraResolutionStrings[40] = {"320x240",  "352x288",   "640x480",  "800x600",
                                           "1024x768", "1280x1024", "1600x1200"};

watchdog_info_t* watchdog;
uint32_t* flags;

HWND create_button(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindow("BUTTON", title, WS_VISIBLE | WS_CHILD, startX, startY, width, height, hwnd, handle, NULL,
                        NULL);
}

HWND create_label(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindow("STATIC", title, WS_VISIBLE | WS_CHILD, startX, startY, width, height, hwnd, handle, NULL,
                        NULL);
}

HWND create_dropbox(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle,
                    int numberOfOptions, const char* nameOfOptions[40], int indexOfDeafaultOption) {
    HWND dropBox =
        CreateWindow("COMBOBOX", title, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, startX,
                     startY, width, height, hwnd, handle, NULL, NULL);
    /*
    CreateWindow("COMBOBOX", "", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, COL_2, ROW_4,
                 DROP_BOX_WIDTH, DROP_BOX_HEIGHT, hwnd, (HMENU)IDC_COMBOBOX, NULL, NULL);
    */
    for (int i = 0; i < numberOfOptions; i++) {
        SendMessage(dropBox, CB_ADDSTRING, indexOfDeafaultOption, (LPARAM)nameOfOptions[i]);
    }
    return dropBox;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:;

            char text[150];

            /* BEGINNING OF SECTION 1 */

            // Button use to open and view the data on the SD card
            sprintf(text, "View Data");
            buttonOpenSDCard = create_button(text, LEFT_MARGIN, ROW_1, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd,
                                             (HMENU)BUTTON_OPEN_SD_CARD_HANDLE);

            // Status label to show the health of the system
            sprintf(text, "Status: %s", watchdog->status == SYSTEM_STATUS_OK ? "Ok" : "Error");
            labelStatus = create_label(text, COL_2, ROW_1, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // ID label to show the ID of the system
            sprintf(text, "ID: %x", watchdog->id);
            labelID = create_label(text, COL_3, ROW_1, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // button to copy data from the watchdog to the computer
            sprintf(text, "Send Data to Computer");
            buttonExportData = create_button(text, LEFT_MARGIN, ROW_2, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd,
                                             (HMENU)BUTTON_EXPORT_DATA_HANDLE);

            // Label to show how many images are on the SD card
            sprintf(text, "%i Image%s Taken", watchdog->numImages, watchdog->numImages == 1 ? "" : "s");
            labelNumImages = create_label(text, COL_3, ROW_2, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label to show the current date time on the system
            sprintf(text, "%s", watchdog->datetime);
            labelDate = create_label(text, COL_3, ROW_3, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            /* BEGINNING OF SECTION 2 */

            // Label for setting to change the camera resolution
            sprintf(text, "Camera Resolution");
            LabelCameraResolution = create_label(text, COL_1, ROW_4, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // Label for setting to change the frequency at which photos are taken
            sprintf(text, "Photo Frequency");
            labelPhotoFrequency = create_label(text, COL_1, ROW_5, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            /* START OF SECTION 3 */

            // Button to get live stream feed of camera
            sprintf(text, "Camera View");
            buttonCameraView =
                create_button(text, COL_1, ROW_6, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)BUTTON_CAMERA_VIEW_HANDLE);

            // Button to run a test on the system
            sprintf(text, "Test System");
            buttonRunTest =
                create_button(text, COL_2, ROW_6, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)BUTTON_RUN_TEST_HANDLE);

            break;

        case WM_COMMAND:
            // handle button clicks
            if (LOWORD(wParam) == BUTTON_OPEN_SD_CARD_HANDLE) {
                *flags |= GUI_TURN_RED_LED_ON;
                // printf("Displaying SD card data\n");
            }

            if (LOWORD(wParam) == BUTTON_EXPORT_DATA_HANDLE) {
                *flags |= GUI_TURN_RED_LED_OFF;
                // printf("Exporting SD card data\n");
            }

            if (LOWORD(wParam) == BUTTON_CAMERA_VIEW_HANDLE) {
                printf("Starting livestream\n");
            }

            if (LOWORD(wParam) == BUTTON_RUN_TEST_HANDLE) {
                printf("Testing system\n");
            }

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

void gui_init(watchdog_info_t* watchdogInfo, uint32_t* guiFlags) {

    watchdog = watchdogInfo;
    flags    = guiFlags;

    WNDCLASSEX wc;
    HWND hwnd;
    // MSG msg;

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
        return;
    }

    // create the window
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "myWindowClass", "My Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                          CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, NULL, NULL);

    dropDownCameraResolution =
        create_dropbox("Title", COL_2, ROW_4, DROP_BOX_WIDTH, DROP_BOX_HEIGHT, hwnd, (HMENU)IDC_COMBOBOX,
                       NUMBER_OF_CAM_RESOLUTIONS, cameraResolutionStrings, 0);
    /*
    CreateWindow("COMBOBOX", "", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, COL_2,
                 ROW_4, DROP_BOX_WIDTH, DROP_BOX_HEIGHT, hwnd, (HMENU)IDC_COMBOBOX, NULL, NULL);

SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "320x240");
SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "352x288");
SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "640x480");
SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "800x600");
SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "1024x768");
SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "1280x1024");
SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "1600x1200");
SendMessage(dropDownCameraResolution, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
*/
    // Button to change the frequency at which photos are taken
    dropDownPhotoFrequency =
        CreateWindow("COMBOBOX", "", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, COL_2,
                     ROW_5, DROP_BOX_WIDTH, DROP_BOX_HEIGHT, hwnd, (HMENU)IDC_COMBOBOX, NULL, NULL);
    SendMessage(dropDownPhotoFrequency, CB_ADDSTRING, 0, (LPARAM) "1 time per day");
    SendMessage(dropDownPhotoFrequency, CB_ADDSTRING, 0, (LPARAM) "2 times per day");
    SendMessage(dropDownPhotoFrequency, CB_ADDSTRING, 0, (LPARAM) "3 times per day");
    SendMessage(dropDownPhotoFrequency, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    // CODE TO FIX DROP DOWN BOXES (5 lines)

    // // message loop
    // while (GetMessage(&msg, NULL, 0, 0) > 0) {
    //     TranslateMessage(&msg);
    //     DispatchMessage(&msg);
    // }
}

void gui_update() {
    MSG msg;

    if (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}