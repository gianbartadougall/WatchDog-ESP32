/**
 * @file help.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-01-23
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef HELP_H
#define HELP_H

/* Personal Includes */
#include "log.h"

// Generic Commands
#define HEADER    "------------------------------------\n"
#define COMMAND_0 "COMMAND                             "
#define COMMAND_1 "help                                "
#define COMMAND_2 "ping                                "

// Project Specific Commands
#define COMMAND_3 "cpy <file to copy> <new file name>  "
#define COMMAND_4 "photo                               "
#define COMMAND_5 "record                              "
#define COMMAND_6 "list <directory>                    "
#define COMMAND_7 "write <filePath>                    "

// Command definitions
#define COMMAND_0_DEF "ACTION\n"
#define COMMAND_1_DEF "Prints available commands\n"
#define COMMAND_2_DEF "Pings device\n"
#define COMMAND_3_DEF "Copies <file to copy> to <new file name>\n"
#define COMMAND_4_DEF "Takes a photo\n"
#define COMMAND_5_DEF "Records temperature data + takes a photo\n"
#define COMMAND_6_DEF "Lists the files/folders in <directory>\n"
#define COMMAND_7_DEF "Writes <filepath> to the SD card\n"

char uartHelp[] = {HEADER ASCII_COLOR_CYAN COMMAND_0 COMMAND_0_DEF ASCII_COLOR_WHITE COMMAND_1 COMMAND_1_DEF COMMAND_2
                       COMMAND_2_DEF COMMAND_3 COMMAND_3_DEF COMMAND_4 COMMAND_4_DEF COMMAND_5 COMMAND_5_DEF COMMAND_6
                           COMMAND_6_DEF COMMAND_7 COMMAND_7_DEF HEADER};

#endif // HELP_H