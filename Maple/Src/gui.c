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

/* Personal Includes */
#include "gui.h"
#include "utilities.h"

// #define STB_IMAGE_IMPLEMENTATION // Required for stb_image
#include "stb_image.h"
#include "stb_image_resize.h"

/* Private Macros */
#define LEFT_MARGIN   10
#define MIDDLE_MARGIN 270
#define TOP_MARGIN    10
#define RIGHT_MARGIN  500

#define BUTTON_WIDTH  200
#define BUTTON_HEIGHT 30

#define LABEL_WIDTH  200
#define LABEL_HEIGHT 30

#define DROP_BOX_WIDTH  200
#define DROP_BOX_HEIGHT 30

// #define WINDOW_WIDTH  (RIGHT_MARGIN + LABEL_WIDTH + (GAP_WIDTH * 3))
// #define WINDOW_HEIGHT ((LABEL_HEIGHT * 2) + TOP_MARGIN + 300)

#define WINDOW_WIDTH  1600
#define WINDOW_HEIGHT 1200

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
#define BUTTON_NORMAL_VIEW_HANDLE         5
#define DROP_BOX_CAMERA_RESOLUTION_HANDLE 6
#define DROP_BOX_PHOTO_FREQUENCY_HANDLE   7

HWND dropDownCameraResolution, dropDownPhotoFrequency;
HWND LabelCameraResolution, labelPhotoFrequency, labelStatus, labelID, labelNumImages, labelDate;
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

watchdog_info_t* watchdog;
uint32_t* flags;

void DrawPixels(HWND hwnd);

HWND create_button(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindow("BUTTON", title, WS_VISIBLE | WS_CHILD, startX, startY, width, height, hwnd, handle, NULL,
                        NULL);
}

HWND create_label(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindow("STATIC", title, WS_VISIBLE | WS_CHILD, startX, startY, width, height, hwnd, handle, NULL,
                        NULL);
}

HWND create_dropbox(char* title, int startX, int startY, int width, int height, HWND hwnd, HMENU handle) {
    return CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", title,
                          CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, startX, startY, width,
                          height, hwnd, handle, GetModuleHandle(NULL), NULL);
}

int cameraViewOn = FALSE;

void gui_set_camera_view(HWND hwnd) {

    // Hide all the drop downs
    // ShowWindow(dropDownCameraResolution, SW_HIDE);
    // ShowWindow(dropDownPhotoFrequency, SW_HIDE);

    // Hide all the labels
    // ShowWindow(LabelCameraResolution, SW_HIDE);
    // ShowWindow(labelPhotoFrequency, SW_HIDE);
    // ShowWindow(labelStatus, SW_HIDE);
    // ShowWindow(labelID, SW_HIDE);
    // ShowWindow(labelNumImages, SW_HIDE);
    // ShowWindow(labelDate, SW_HIDE);

    // Hide all the buttons
    ShowWindow(buttonCameraView, SW_HIDE);
    // ShowWindow(buttonOpenSDCard, SW_HIDE);
    // ShowWindow(buttonExportData, SW_HIDE);
    // ShowWindow(buttonRunTest, SW_HIDE);

    // Show the buttons used in camera view
    ShowWindow(buttonNormalView, SW_SHOW);
    UpdateWindow(hwnd);
}

void gui_set_normal_view(HWND hwnd) {

    // Hide all the drop downs
    // ShowWindow(dropDownCameraResolution, SW_SHOW);
    // ShowWindow(dropDownPhotoFrequency, SW_SHOW);

    // Hide all the labels
    // ShowWindow(LabelCameraResolution, SW_SHOW);
    // ShowWindow(labelPhotoFrequency, SW_SHOW);
    // ShowWindow(labelStatus, SW_SHOW);
    // ShowWindow(labelID, SW_SHOW);
    // ShowWindow(labelNumImages, SW_SHOW);
    // ShowWindow(labelDate, SW_SHOW);

    // Hide all the buttons
    ShowWindow(buttonCameraView, SW_SHOW);
    // ShowWindow(buttonOpenSDCard, SW_SHOW);
    // ShowWindow(buttonExportData, SW_SHOW);
    // ShowWindow(buttonRunTest, SW_SHOW);

    ShowWindow(buttonNormalView, SW_HIDE);
    UpdateWindow(hwnd);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:;

            char text[150];

            // /* BEGINNING OF SECTION 1 */

            // Button use to open and view the data on the SD card
            // sprintf(text, "View Data");
            // buttonOpenSDCard = create_button(text, LEFT_MARGIN, ROW_1, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd,
            //                                  (HMENU)BUTTON_OPEN_SD_CARD_HANDLE);

            // // Status label to show the health of the system
            // sprintf(text, "Status: %s", watchdog->status == SYSTEM_STATUS_OK ? "Ok" : "Error");
            // labelStatus = create_label(text, COL_2, ROW_1, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // // ID label to show the ID of the system
            // sprintf(text, "ID: %x", watchdog->id);
            // labelID = create_label(text, COL_3, ROW_1, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // button to copy data from the watchdog to the computer
            // sprintf(text, "Send Data to Computer");
            // buttonExportData = create_button(text, LEFT_MARGIN, ROW_2, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd,
            //                                  (HMENU)BUTTON_EXPORT_DATA_HANDLE);

            // button to go back to camera View
            sprintf(text, "Exit Camera View");
            buttonNormalView = create_button(text, LEFT_MARGIN, ROW_1, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd,
                                             (HMENU)BUTTON_NORMAL_VIEW_HANDLE);
            ShowWindow(buttonNormalView, SW_HIDE);

            // // Label to show how many images are on the SD card
            // sprintf(text, "%i Image%s Taken", watchdog->numImages, watchdog->numImages == 1 ? "" : "s");
            // labelNumImages = create_label(text, COL_3, ROW_2, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // // Label to show the current date time on the system
            // sprintf(text, "%s", watchdog->datetime);
            // labelDate = create_label(text, COL_3, ROW_3, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // /* BEGINNING OF SECTION 2 */

            // // Label for setting to change the camera resolution
            // sprintf(text, "Camera Resolution");
            // LabelCameraResolution = create_label(text, COL_1, ROW_4, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // // Button to change the camera resolution
            // dropDownCameraResolution = create_dropbox("", COL_2, ROW_4, DROP_BOX_WIDTH, DROP_BOX_HEIGHT, hwnd,
            //                                           (HMENU)DROP_BOX_CAMERA_RESOLUTION_HANDLE);

            // SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "Resolution 640x480");
            // SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "Resolution 240x240");
            // SendMessage(dropDownCameraResolution, CB_SETCURSEL, 1, 0);

            // // SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "Resolution 320x240");
            // // SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "Resolution 640x480");
            // // SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "Resolution 1024x768");
            // // SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "Resolution 1280x1024");
            // // SendMessage(dropDownCameraResolution, CB_ADDSTRING, 0, (LPARAM) "Resolution 1600x1200");

            // // Label for setting to change the frequency at which photos are taken
            // sprintf(text, "Photo Frequency");
            // labelPhotoFrequency = create_label(text, COL_1, ROW_5, LABEL_WIDTH, LABEL_HEIGHT, hwnd, NULL);

            // // Button to change the frequency at which photos are taken
            // dropDownPhotoFrequency = create_dropbox("", COL_2, ROW_5, DROP_BOX_WIDTH, DROP_BOX_HEIGHT, hwnd,
            //                                         (HMENU)DROP_BOX_PHOTO_FREQUENCY_HANDLE);
            // SendMessage(dropDownPhotoFrequency, CB_ADDSTRING, 0, (LPARAM) "1 time per day");
            // SendMessage(dropDownPhotoFrequency, CB_ADDSTRING, 0, (LPARAM) "2 times per day");
            // SendMessage(dropDownPhotoFrequency, CB_ADDSTRING, 0, (LPARAM) "3 times per day");

            // /* START OF SECTION 3 */

            // Button to get live stream feed of camera
            sprintf(text, "Camera View");
            buttonCameraView =
                create_button(text, COL_1, ROW_6, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)BUTTON_CAMERA_VIEW_HANDLE);

            // // Button to run a test on the system
            // sprintf(text, "Test System");
            // buttonRunTest =
            //     create_button(text, COL_2, ROW_6, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)BUTTON_RUN_TEST_HANDLE);

            break;

        case WM_COMMAND:
            // handle button clicks
            // if (LOWORD(wParam) == BUTTON_OPEN_SD_CARD_HANDLE) {
            //     *flags |= GUI_TURN_RED_LED_ON;
            //     // printf("Displaying SD card data\n");
            // }

            // if (LOWORD(wParam) == BUTTON_EXPORT_DATA_HANDLE) {
            //     *flags |= GUI_TURN_RED_LED_OFF;
            //     // printf("Exporting SD card data\n");
            // }

            if (LOWORD(wParam) == BUTTON_CAMERA_VIEW_HANDLE) {
                cameraViewOn = TRUE;
                gui_set_camera_view(hwnd);

                printf("Starting livestream\n");
                InvalidateRect(hwnd, NULL, TRUE);
                rectangle_t rectangle;
                rectangle.startX = COL_1;
                rectangle.startY = ROW_2;
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

            // if (LOWORD(wParam) == BUTTON_RUN_TEST_HANDLE) {
            //     printf("Testing system\n");
            // }

            // if ((HWND)lParam == dropDownCameraResolution) {
            //     // handle drop-down menu selection
            //     int itemIndex = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
            //     TCHAR buffer[256];
            //     SendMessage((HWND)lParam, CB_GETLBTEXT, itemIndex, (LPARAM)buffer);
            //     printf("Selected item %i: %s\n", itemIndex, buffer);
            // }

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

// void gui_init(watchdog_info_t* watchdogInfo, uint32_t* guiFlags) {

//     watchdog = watchdogInfo;
//     flags    = guiFlags;

//     WNDCLASSEX wc;
//     MSG msg;

//     // register the window class
//     wc.cbSize        = sizeof(WNDCLASSEX);
//     wc.style         = 0;
//     wc.lpfnWndProc   = WndProc;
//     wc.cbClsExtra    = 0;
//     wc.cbWndExtra    = 0;
//     wc.hInstance     = NULL;
//     wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
//     wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
//     wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//     wc.lpszMenuName  = NULL;
//     wc.lpszClassName = "myWindowClass";
//     wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

//     if (!RegisterClassEx(&wc)) {
//         MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
//         return;
//     }

//     // create the window
//     hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "myWindowClass", "My Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
//                           CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, NULL, NULL);

//     if (hwnd == NULL) {
//         MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
//         return;
//     }

//     ShowWindow(hwnd, SW_SHOWNORMAL);
//     UpdateWindow(hwnd);

//     // message loop
//     while (GetMessage(&msg, NULL, 0, 0) > 0) {
//         TranslateMessage(&msg);
//         DispatchMessage(&msg);
//     }
// }

void gui_update() {
    MSG msg;

    if (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

DWORD WINAPI gui(void* arg) {

    // watchdog = watchdogInfo;
    flags = (uint32_t*)arg;

    cameraViewImagePosition.startX = COL_1;
    cameraViewImagePosition.starty = ROW_2;
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

    // create the window
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "myWindowClass", "My Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
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
        if ((flag & GUI_UPDATE_CAMERA_VIEW) != 0) {
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