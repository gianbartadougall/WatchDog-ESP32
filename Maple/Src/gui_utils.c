/* C Library Includes */

/* Personal Includes */
#include "gui_utils.h"

HWND gui_utils_create_label(char* title, int startX, int startY, int width, int height, HWND hwnd,
                            HMENU handle) {
    return CreateWindow("STATIC", title, WS_VISIBLE | WS_CHILD | SS_NOTIFY, startX, startY, width,
                        height, hwnd, handle, NULL, NULL);
}

HWND gui_utils_create_dropbox(char* title, int startX, int startY, int width, int height, HWND hwnd,
                              HMENU handle, uint8_t numItems, char* items[50]) {

    HWND dropBox = CreateWindow(
        "COMBOBOX", title, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
        startX, startY, width, height, hwnd, handle, NULL, NULL);

    for (int i = 0; i < numItems; i++) {
        SendMessage(dropBox, CB_ADDSTRING, 0, (LPARAM)items[i]);
    }

    SendMessage(dropBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    return dropBox;
}

HWND gui_utils_create_button(char* title, int startX, int startY, int width, int height, HWND hwnd,
                             HMENU handle) {
    return CreateWindow("BUTTON", title, WS_VISIBLE | WS_CHILD, startX, startY, width, height, hwnd,
                        handle, NULL, NULL);
}

HWND gui_utils_create_textbox(char* text, int startX, int startY, int width, int height, HWND hwnd,
                              HMENU handle) {
    return CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", text, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                          startX, startY, width, height, hwnd, handle, GetModuleHandle(NULL), NULL);
}