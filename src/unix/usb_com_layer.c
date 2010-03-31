#include "../usb_com_layer.h"

#if !_WIN32

TUSBHandle* USBCom_Create() {
	TUSBHandle* handle;

	handle = malloc(sizeof(TUSBHandle));
	handle->isOpened = FALSE;
	handle->lastError = USBCOM_NOEXCEPTION_ERR;

	handle->dev = NULL;

	libusb_init(&handle->ctx);
	libusb_set_debug(handle->ctx, 3);

	return handle;
}

void USBCom_Destroy(TUSBHandle* handle) {
	libusb_exit(handle->ctx);
	free(handle);
}

BOOL USBCom_Open(TUSBHandle* handle, WORD const vid, WORD const pid) {
	struct libusb_device_descriptor desc;
	libusb_device **list;
	libusb_device *found = NULL;
	UINT cnt;
	UINT i = 0;
	int ret = 0;

	handle->vid = vid;
	handle->pid = pid;

	/* Enumération des périphériques connectés */
	cnt = libusb_get_device_list(handle->ctx, &list);
	if (cnt < 0) {
		/* Pas besoin d'appeler libusb_free_device_list() ici puisqu'elle n'a pas pu être créée. */
		handle->lastError = USBCOM_MEMORY_ALLOC_ERR;
		return FALSE;
	}

	for (i = 0; i < cnt; i++) {
		ret = libusb_get_device_descriptor(list[i], &desc);

		if (ret < 0) {
			libusb_free_device_list(list, 1);

			fprintf(stderr, "libusb_get_device_descriptor failed with return code %d\n", ret);
			handle->lastError = USBCOM_DEVICE_OPEN_ERR;
			return FALSE;
		}
		else if (desc.idVendor == vid && desc.idProduct == pid) {
			found = list[i];
			break;
		}
	}

	if (found) {
		/* Créé le handle pour le périphérique */
		ret = libusb_open(found, &handle->dev);

		libusb_free_device_list(list, 1); /* libusb_open() doit être appelée avant ! */

		if (ret < 0) {
			fprintf(stderr, "libusb_open failed with return code %d\n", ret);
			handle->lastError = USBCOM_DEVICE_OPEN_ERR;
			return FALSE;
		}

		/* On vérifie si le kernel est actif sur ce périphérique */
		ret = libusb_kernel_driver_active(handle->dev, 0);
		if (ret == 1) {
			/* Si c'est le cas, il faut détacher le périphérique du kernel */
			ret = libusb_detach_kernel_driver(handle->dev, 0);
			if (ret < 0) {
				fprintf(stderr, "libusb_detach_kernel_driver failed with return code %d\n", ret);
				handle->lastError = USBCOM_DEVICE_OPEN_ERR;
				return FALSE;
			}
		}
		else if (ret < 0) {
			fprintf(stderr, "libusb_kernel_driver_active failed with return code %d\n", ret);
			handle->lastError = USBCOM_DEVICE_OPEN_ERR;
			return FALSE;
		}

		ret = libusb_set_configuration(handle->dev, 1); /* Nécessaire pour les version de kernel <= 2.6.27 */
		if (ret < 0) {
			fprintf(stderr, "libusb_set_configuration failed with return code %d\n", ret);
			handle->lastError = USBCOM_DEVICE_OPEN_ERR;
			return FALSE;
		}

		/* Ensuite seulement on peut s'approprier l'interface du périphérique */
		ret = libusb_claim_interface(handle->dev, 0);
		if (ret < 0) {
			fprintf(stderr, "libusb_claim_interface failed with return code %d\n", ret);
			handle->lastError = USBCOM_DEVICE_OPEN_ERR;
			return FALSE;
		}
	}
	else {
		libusb_free_device_list(list, 1);
		handle->lastError = USBCOM_DEVICE_NOT_FOUND_ERR;
		return FALSE;
	}

	handle->isOpened = TRUE;
	return TRUE;
}

void USBCom_Close(TUSBHandle* handle) {
	if (handle->dev) {
		libusb_release_interface(handle->dev, 0);
		libusb_reset_device(handle->dev); /* Nécessaire pour les version de kernel <= 2.6.27 */
		libusb_close(handle->dev);
		handle->dev = NULL;
	}
}

BOOL USBCom_IsOpened(TUSBHandle* handle) {
	return handle->isOpened;
}

UINT USBCom_InterruptWrite(TUSBHandle* handle, BYTE* const buffer, UINT const size, UINT const timeout) {
	int ret;
	int len;

	if (!handle->isOpened) {
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}

	ret = libusb_interrupt_transfer(handle->dev, 0x01, buffer, size, &len, timeout);

	if (ret == LIBUSB_ERROR_NO_DEVICE) {
		USBCom_Close(handle);
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}
	else if (ret == LIBUSB_ERROR_PIPE) {
		handle->lastError = USBCOM_DEVICE_WRITE_ERR;
		return 0;
	}
	else if (ret != 0) {
		fprintf(stderr, "libusb_interrupt_transfer failed with return code %d\n", ret);
		handle->lastError = USBCOM_DEVICE_WRITE_ERR;
		return 0;
	}

	return len;
}

UINT USBCom_InterruptRead(TUSBHandle* handle, BYTE* buffer, UINT const size, UINT const mask, UINT const timeout) {
	int ret;
	int len;

	if (!handle->isOpened) {
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}

	ret = libusb_interrupt_transfer(handle->dev, 0x81, buffer, size, &len, timeout);

	if (ret == LIBUSB_ERROR_NO_DEVICE) {
		USBCom_Close(handle);
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}
	else if (ret == LIBUSB_ERROR_PIPE) {
		handle->lastError = USBCOM_DEVICE_READ_ERR;
		return 0;
	}
	else if (ret != 0) {
		fprintf(stderr, "libusb_interrupt_transfer failed with return code %d\n", ret);
		handle->lastError = USBCOM_DEVICE_READ_ERR;
		return 0;
	}

	return len;
}

TUSBException USBCom_GetLastError(TUSBHandle* handle) {
	TUSBException error = handle->lastError;
	handle->lastError = USBCOM_NOEXCEPTION_ERR;

	return error;
}

#endif
