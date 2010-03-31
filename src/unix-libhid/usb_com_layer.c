#include "../usb_com_layer.h"

#if !_WIN32

TUSBHandle* USBCom_Create() {
	TUSBHandle* handle;

	handle = malloc(sizeof(TUSBHandle));
	handle->isOpened = FALSE;
	handle->lastError = USBCOM_NOEXCEPTION_ERR;

	return handle;
}

void USBCom_Destroy(TUSBHandle* handle) {
	free(handle);
}

BOOL USBCom_Open(TUSBHandle* handle, WORD const vid, WORD const pid) {
	hid_return ret;
	int iface_num = 0;

	handle->vid = vid;
	handle->pid = pid;

	/* How to use a custom matcher function:
	 * 
	 * The third member of the HIDInterfaceMatcher is a function pointer, and
	 * the forth member will be passed to it on invocation, together with the
	 * USB device handle. The fifth member holds the length of the buffer
	 * passed. See above. This can be used to do custom selection e.g. if you
	 * have multiple identical devices which differ in the serial number.
	 *
	 *   char const* const serial = "01518";
	 *   HIDInterfaceMatcher matcher = {
	 *     0x06c2,                      // vendor ID
	 *     0x0038,                      // product ID
	 *     match_serial_number,         // custom matcher function pointer
	 *     (void*)serial,               // custom matching data
	 *     strlen(serial)+1             // length of custom data
	 *   };
	 *
	 * If you do not want to use this, set the third member to NULL.
	 * Then the match will only be on vendor and product ID.
	*/

	HIDInterfaceMatcher matcher = { vid, pid, NULL, NULL, 0 };

	/* see include/debug.h for possible values */
	hid_set_debug(HID_DEBUG_ALL & ~HID_DEBUG_TRACES);
	hid_set_debug_stream(stderr);
	/* passed directly to libusb */
	hid_set_usb_debug(0);

	ret = hid_init();
	if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_init failed with return code %d\n", ret);
		handle->lastError = USBCOM_DEVICE_OPEN_ERR;
		return FALSE;
	}

	handle->hid = hid_new_HIDInterface();
	if (handle->hid == 0) {
		fprintf(stderr, "hid_new_HIDInterface() failed, out of memory?\n");
		handle->lastError = USBCOM_DEVICE_OPEN_ERR;
		return FALSE;
	}

	/* How to detach a device from the kernel HID driver:
	 * 
	 * The hid.o or usbhid.ko kernel modules claim a HID device on insertion,
	 * usually. To be able to use it with libhid, you need to blacklist the
	 * device (which requires a kernel recompilation), or simply tell libhid to
	 * detach it for you. hid_open just opens the device, hid_force_open will
	 * try n times to detach the device before failing.
	 * In the following, n == 3.
	 *
	 * To open the HID, you need permission to the file in the /proc usbfs
	 * (which must be mounted -- most distros do that by default):
	 *   mount -t usbfs none /proc/bus/usb
	 * You can use hotplug to automatically give permissions to the device on
	 * connection. Please see
	 *   http://cvs.ailab.ch/cgi-bin/viewcvs.cgi/external/libphidgets/hotplug/
	 * for an example. Try NOT to work as root!
	*/

	ret = hid_force_open(handle->hid, iface_num, &matcher, 3);
	if (ret == HID_RET_DEVICE_NOT_FOUND) {
		handle->lastError = USBCOM_DEVICE_NOT_FOUND_ERR;
		return FALSE;
	}
	else if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_force_open failed with return code %d\n", ret);
		handle->lastError = USBCOM_DEVICE_OPEN_ERR;
		return FALSE;
	}
/*
	ret = hid_write_identification(stdout, handle->hid);
	if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_write_identification failed with return code %d\n", ret);
		return 1;
	}

	ret = hid_dump_tree(stdout, handle->hid);
	if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_dump_tree failed with return code %d\n", ret);
		return 1;
	}
*/
	handle->isOpened = TRUE;
	return TRUE;
}

void USBCom_Close(TUSBHandle* handle) {
	hid_return ret;

	handle->isOpened = FALSE;

	ret = hid_close(handle->hid);
	if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_close failed with return code %d\n", ret);
	}

	hid_delete_HIDInterface(&handle->hid);

	ret = hid_cleanup();
	if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_cleanup failed with return code %d\n", ret);
	}
}

BOOL USBCom_IsOpened(TUSBHandle* handle) {
	return handle->isOpened;
}

UINT USBCom_InterruptWrite(TUSBHandle* handle, BYTE* const buffer, UINT const size) {
	hid_return ret;
	UINT timeout = 1000; // milliseconds

	if (!handle->isOpened) {
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}

	// ret = usb_interrupt_write(handle->hid->dev_handle, 0x01, (char const*) buffer, size, timeout);
	ret = hid_interrupt_write(handle->hid, 0x01, (char const*) buffer, size, timeout);
	if (ret == HID_RET_DEVICE_NOT_OPENED) {
		USBCom_Close(handle);
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}
	// Non, ceci n'est pas un copier-coller malheureux, il s'agit bien de la constante
	// HID_RET_FAIL_INT_READ (voir sources de la fonction hid_interrupt_write)
	else if (ret == HID_RET_FAIL_INT_READ) {
		handle->lastError = USBCOM_DEVICE_WRITE_ERR;
		return 0;
	}
	else if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_interrupt_write failed with return code %d\n", ret);
		handle->lastError = USBCOM_DEVICE_WRITE_ERR;
		return 0;
	}

	return size;
}

UINT USBCom_InterruptRead(TUSBHandle* handle, BYTE* buffer, UINT const size, UINT const mask) {
	hid_return ret;
	UINT timeout = 1000; // milliseconds

	if (!handle->isOpened) {
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}

	//hid_set_idle(handle->hid, 0, 0x01);
	ret = hid_interrupt_read(handle->hid, 0x81, (char*) buffer, size, timeout);
	if (ret == HID_RET_DEVICE_NOT_OPENED) {
		USBCom_Close(handle);
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}
	else if (ret == HID_RET_FAIL_INT_READ) {
		handle->lastError = USBCOM_DEVICE_READ_ERR;
		return 0;
	}
	else if (ret != HID_RET_SUCCESS) {
		fprintf(stderr, "hid_interrupt_read failed with return code %d\n", ret);
		handle->lastError = USBCOM_DEVICE_READ_ERR;
		return 0;
	}

	return size;
}

TUSBException USBCom_GetLastError(TUSBHandle* handle) {
	TUSBException error = handle->lastError;
	handle->lastError = USBCOM_NOEXCEPTION_ERR;

	return error;
}

#endif
