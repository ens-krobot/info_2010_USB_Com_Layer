#ifndef _UNIX__USB_COM_LAYER_H
#define _UNIX__USB_COM_LAYER_H

typedef unsigned char		BYTE;				/* < 8-bit unsigned */
typedef unsigned short int	WORD;				/* < 16-bit unsigned */
typedef unsigned long		DWORD;				/* < 32-bit unsigned */
typedef unsigned int		UINT;

typedef enum {
	FALSE = 0,
	TRUE
} BOOL;

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <libusb.h>

#include "../usb_com_layer.h"

typedef struct {
	WORD vid;
	WORD pid;
	libusb_context* ctx;
	libusb_device_handle* dev;
	BOOL isOpened;
	TUSBException lastError;
} TUSBHandle;

#endif /* _UNIX__USB_COM_LAYER_H */
