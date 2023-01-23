/**
 * @file watchdog_defines.h
 * @author Gian Barta-Dougall
 * @brief
 * @version 0.1
 * @date 2023-01-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef WATCHDOG_DEFINES_H
#define WATCHDOG_DEFINES_H

#define SYSTEM_LOG_FILE        ("logs.txt")
#define IMAGE_DATA_FOLDER      ("WATCHDOG/DATA")
#define ROOT_IMAGE_DATA_FOLDER ("/sdcard/WATCHDOG/DATA")

#define MOUNT_POINT_PATH     ("/sdcard")
#define WATCHDOG_FOLDER_PATH ("/sdcard/WATCHDOG")
#define ROOT_LOG_FOLDER_PATH ("/sdcard/WATCHDOG/LOGS")

// UART Packet codes

#define WATCHDOG_BPK_R_LIST_DIR      (BPACKET_SPECIFIC_R_OFFSET + 0)
#define WATCHDOG_BPK_R_COPY_FILE     (BPACKET_SPECIFIC_R_OFFSET + 1)
#define WATCHDOG_BPK_R_TAKE_PHOTO    (BPACKET_SPECIFIC_R_OFFSET + 2)
#define WATCHDOG_BPK_R_WRITE_TO_FILE (BPACKET_SPECIFIC_R_OFFSET + 3)
#define WATCHDOG_BPK_R_RECORD_DATA   (BPACKET_SPECIFIC_R_OFFSET + 4)
#define WATCHDOG_BPK_R_LED_RED_ON    (BPACKET_SPECIFIC_R_OFFSET + 6)
#define WATCHDOG_BPK_R_LED_RED_OFF   (BPACKET_SPECIFIC_R_OFFSET + 7)

#endif // WATCHDOG_DEFINES_H