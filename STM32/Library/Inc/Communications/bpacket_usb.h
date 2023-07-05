/**
 * @file bpacket_usb.h
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-07-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef BPACKET_USB_H
#define BPACKET_USB_H

void usb_rx_handler(uint8_t* buffer, uint32_t length);

#endif // BPACKET_USB_H