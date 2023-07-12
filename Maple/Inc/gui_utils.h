#ifndef GUI_UTILS_H
#define GUI_UTILS_H

/* C Library Includes */
#include <windows.h> // Use for multi threading
#include <wingdi.h>
#include <CommCtrl.h>
#include <stdint.h>

HWND gui_utils_create_label(char* title, int startX, int startY, int width, int height, HWND hwnd,
                            HMENU handle);

HWND gui_utils_create_dropbox(char* title, int startX, int startY, int width, int height, HWND hwnd,
                              HMENU handle, uint8_t numItems, char* items[50]);

HWND gui_utils_create_button(char* title, int startX, int startY, int width, int height, HWND hwnd,
                             HMENU handle);

HWND gui_utils_create_textbox(char* text, int startX, int startY, int width, int height, HWND hwnd,
                              HMENU handle);

#endif // GUI_UTILS