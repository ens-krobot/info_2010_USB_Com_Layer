#ifndef _MSW__USB_COM_LAYER_H
#define _MSW__USB_COM_LAYER_H

#include <Windows.h>	/* Definitions for various common and not so common types like DWORD, PCHAR, HANDLE, etc. */
#include <setupapi.h>	/* From Platform SDK. Definitions needed for the SetupDixxx() functions, which we use to
						   find our plug and play device. */
#include <stdio.h>
#include <ctype.h>

#include "../usb_com_layer.h"

typedef struct {
	WORD vid;
	WORD pid;
	HANDLE writeHandle;
	HANDLE readHandle;
	BOOL isOpened;
	TUSBException lastError;
} TUSBHandle;

#endif /* _MSW__USB_COM_LAYER_H */
