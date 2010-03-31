#include "../usb_com_layer.h"

#if _WIN32

TUSBHandle* USBCom_Create() {
	TUSBHandle* handle;

	handle = malloc(sizeof(TUSBHandle));
	handle->writeHandle = INVALID_HANDLE_VALUE;
	handle->readHandle = INVALID_HANDLE_VALUE;
	handle->isOpened = FALSE;
	handle->lastError = USBCOM_NOEXCEPTION_ERR;

	return handle;
}

void USBCom_Destroy(TUSBHandle* handle) {
	free(handle);
}

BOOL USBCom_Open(TUSBHandle* handle, WORD const vid, WORD const pid) {
	/*
	Before we can "connect" our application to our USB embedded device, we must first find the device.
	A USB bus can have many devices simultaneously connected, so somehow we have to find our device, and only
	our device.  This is done with the Vendor ID (VID) and Product ID (PID).  Each USB product line should have
	a unique combination of VID and PID.

	Microsoft has created a number of functions which are useful for finding plug and play devices.  Documentation
	for each function used can be found in the MSDN library.  We will be using the following functions:

	SetupDiGetClassDevs()					//provided by setupapi.dll, which comes with Windows
	SetupDiEnumDeviceInterfaces()			//provided by setupapi.dll, which comes with Windows
	GetLastError()							//provided by kernel32.dll, which comes with Windows
	SetupDiDestroyDeviceInfoList()			//provided by setupapi.dll, which comes with Windows
	SetupDiGetDeviceInterfaceDetail()		//provided by setupapi.dll, which comes with Windows
	SetupDiGetDeviceRegistryProperty()		//provided by setupapi.dll, which comes with Windows
	CreateFile()							//provided by kernel32.dll, which comes with Windows

	We will also be using the following unusual data types and structures.  Documentation can also be found in
	the MSDN library:

	PSP_DEVICE_INTERFACE_DATA
	PSP_DEVICE_INTERFACE_DETAIL_DATA
	SP_DEVINFO_DATA
	HDEVINFO
	HANDLE
	GUID

	The ultimate objective of the following code is to call CreateFile(), which opens a communications
	pipe to a specific device (such as a HID class USB device endpoint).  CreateFile() returns a "handle"
	which is needed later when calling ReadFile() or WriteFile().  These functions are used to actually
	send and receive application related data to/from the USB peripheral device.

	However, in order to call CreateFile(), we first need to get the device path for the USB device
	with the correct VID and PID.  Getting the device path is a multi-step round about process, which
	requires calling several of the SetupDixxx() functions provided by setupapi.dll.
	*/

	/* Globally Unique Identifier (GUID) for HID class devices.  Windows uses GUIDs to identify things. */
	GUID InterfaceClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, {0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}};

	HDEVINFO DeviceInfoTable = INVALID_HANDLE_VALUE;
	PSP_DEVICE_INTERFACE_DATA InterfaceDataStructure = NULL;
	PSP_DEVICE_INTERFACE_DETAIL_DATA DetailedInterfaceDataStructure;
	SP_DEVINFO_DATA DevInfoData;

	DWORD InterfaceIndex = 0;
	DWORD dwRegType;
	DWORD dwRegSize;
	DWORD StructureSize;
	char* PropertyValueBuffer;
	DWORD ErrorStatus;
	BOOL MatchFound = FALSE;
	UINT c, i, len;

	char DeviceIDToFind[18];
	char* DeviceIDFromRegistry;

	handle->vid = vid;
	handle->pid = pid;
	handle->writeHandle = INVALID_HANDLE_VALUE;
	handle->readHandle = INVALID_HANDLE_VALUE;
	handle->isOpened = FALSE;

	/* First populate a list of plugged in devices (by specifying "DIGCF_PRESENT"), which are of the specified class
	   GUID. */
	DeviceInfoTable = SetupDiGetClassDevs(&InterfaceClassGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	InterfaceDataStructure = (PSP_DEVICE_INTERFACE_DATA) malloc(sizeof(SP_DEVICE_INTERFACE_DATA));

	if (InterfaceDataStructure == NULL) {
		handle->lastError = USBCOM_MEMORY_ALLOC_ERR;
		return FALSE;
	}

	sprintf(DeviceIDToFind, "vid_%04x&pid_%04x", vid, pid);

	/* Now look through the list we just populated.  We are trying to see if any of them match our device. */
	while (TRUE) {
		InterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		SetupDiEnumDeviceInterfaces(DeviceInfoTable, NULL, &InterfaceClassGuid, InterfaceIndex, InterfaceDataStructure);
		ErrorStatus = GetLastError();

		if (ErrorStatus == ERROR_NO_MORE_ITEMS) {	/* Did we reach the end of the list of matching devices in the
													   DeviceInfoTable? */
			/* Cound not find the device.  Must not have been attached. */
			SetupDiDestroyDeviceInfoList(DeviceInfoTable);	/* Clean up the old structure we no longer need. */
			break;
		}

		/* Now retrieve the hardware ID from the registry.  The hardware ID contains the VID and PID, which we will then
		   check to see if it is the correct device or not. */

		/* Initialize an appropriate SP_DEVINFO_DATA structure. We need this structure for
		   SetupDiGetDeviceRegistryProperty(). */
		DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		SetupDiEnumDeviceInfo(DeviceInfoTable, InterfaceIndex, &DevInfoData);

		/* First query for the size of the hardware ID, so we can know how big a buffer to allocate for the data. */
		SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID, &dwRegType, NULL, 0,
			&dwRegSize);

		/* Allocate a buffer for the hardware ID. */
		PropertyValueBuffer = (char*) malloc(dwRegSize);
		if(PropertyValueBuffer == NULL) {	/* if null, error, couldn't allocate enough memory */
			/* Can't really recover from this situation, just exit instead. */
			SetupDiDestroyDeviceInfoList(DeviceInfoTable);	/* Clean up the old structure we no longer need. */
			free(InterfaceDataStructure);

			handle->lastError = USBCOM_MEMORY_ALLOC_ERR;
			return FALSE;
		}

		/* Retrieve the hardware IDs for the current device we are looking at.  PropertyValueBuffer gets filled with a
		   REG_MULTI_SZ (array of null terminated strings).  To find a device, we only care about the very first string
		   in the buffer, which will be the "device ID".  The device ID is a string which contains the VID and PID, in
		   the example format "Vid_04d8&Pid_003f". */
		SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID, &dwRegType,
			(PBYTE) PropertyValueBuffer, dwRegSize, NULL);

		/* Now check if the first string in the hardware ID matches the device ID of my USB device. */
		#ifdef UNICODE
		DeviceIDFromRegistry = (char*) malloc(wcslen(PropertyValueBuffer) + 1);
		wcstombs(DeviceIDFromRegistry, PropertyValueBuffer, wcslen(PropertyValueBuffer));
		#else
		DeviceIDFromRegistry = (char*) malloc(strlen(PropertyValueBuffer) + 1);
		strcpy(DeviceIDFromRegistry, PropertyValueBuffer);
		#endif

		for (c = 0, i = 0, len = strlen(DeviceIDFromRegistry); c < len; c++) {
			if (tolower(DeviceIDFromRegistry[c]) == DeviceIDToFind[i]) {
				i++;

				if (DeviceIDToFind[i] == '\0') {
					MatchFound = TRUE;
					break;
				}
			}
			else
				i = 0;
		}

		free(DeviceIDFromRegistry);
		free(PropertyValueBuffer);

		/* Now check if the hardware ID we are looking at contains the correct VID/PID */
		if (MatchFound) {
			/* Device must have been found. Open read and write handles. In order to do this, we will need the actual
			   device path first. We can get the path by calling SetupDiGetDeviceInterfaceDetail(), however, we have to
			   call this function twice:  The first time to get the size of the required structure/buffer to hold the
			   detailed interface data, then a second time to actually get the structure (after we have allocated enough
			   memory for the structure.) */

			/* First call populates "StructureSize" with the correct value */
			SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure, NULL, 0, &StructureSize, NULL);
			DetailedInterfaceDataStructure = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(StructureSize);
			if (DetailedInterfaceDataStructure == NULL)	{ /* if null, error, couldn't allocate enough memory */
				/* Can't really recover from this situation, just exit instead. */
				SetupDiDestroyDeviceInfoList(DeviceInfoTable);	/* Clean up the old structure we no longer need. */
				free(InterfaceDataStructure);

				handle->lastError = USBCOM_MEMORY_ALLOC_ERR;
				return FALSE;
			}

			DetailedInterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			/* Now call SetupDiGetDeviceInterfaceDetail() a second time to receive the goods. */
			SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure, DetailedInterfaceDataStructure,
				StructureSize, NULL, NULL);
			free(InterfaceDataStructure);

			/* We now have the proper device path, and we can finally open read and write handles to the device.
			   We store the handles in the global variables "WriteHandle" and "ReadHandle", which we will use later to
			   actually communicate. */
			handle->writeHandle = CreateFile((DetailedInterfaceDataStructure->DevicePath), GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

			ErrorStatus = GetLastError();
			if (ErrorStatus != ERROR_SUCCESS) {
				free(DetailedInterfaceDataStructure);
				handle->writeHandle = INVALID_HANDLE_VALUE;

				fprintf(stderr, "CreateFile failed with return code %ld\n", ErrorStatus);
				handle->lastError = USBCOM_DEVICE_OPEN_ERR;
				return FALSE;
			}

			handle->readHandle = CreateFile((DetailedInterfaceDataStructure->DevicePath), GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

			ErrorStatus = GetLastError();
			if (ErrorStatus != ERROR_SUCCESS) {
				free(DetailedInterfaceDataStructure);
				handle->writeHandle = INVALID_HANDLE_VALUE;
				handle->readHandle = INVALID_HANDLE_VALUE;

				fprintf(stderr, "CreateFile failed with return code %ld\n", ErrorStatus);
				handle->lastError = USBCOM_DEVICE_OPEN_ERR;
				return FALSE;
			}

			free(DetailedInterfaceDataStructure);
			SetupDiDestroyDeviceInfoList(DeviceInfoTable);	/* Clean up the old structure we no longer need. */

			handle->isOpened = TRUE;
			return TRUE;
		}

		InterfaceIndex++;
		/* Keep looping until we either find a device with matching VID and PID, or until we run out of items. */
	}

	free(InterfaceDataStructure);

	handle->lastError = USBCOM_DEVICE_NOT_FOUND_ERR;
	return FALSE;
}

void USBCom_Close(TUSBHandle* handle) {
	handle->isOpened = FALSE;
	handle->writeHandle = INVALID_HANDLE_VALUE;
	handle->readHandle = INVALID_HANDLE_VALUE;
}

BOOL USBCom_IsOpened(TUSBHandle* handle) {
	return handle->isOpened;
}

UINT USBCom_InterruptWrite(TUSBHandle* handle, BYTE* const buffer, UINT const size, UINT const timeout) {
	DWORD BytesWritten = 0;
	DWORD ErrorStatus;
	BYTE* OutputPacketBuffer = NULL;
	UINT i;
	BOOL res;

	if (!handle->isOpened) {
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}

	OutputPacketBuffer = (BYTE*) malloc((size + 1)*sizeof(BYTE));

	if (OutputPacketBuffer == NULL) {
		handle->lastError = USBCOM_MEMORY_ALLOC_ERR;
		return 0;
	}

	OutputPacketBuffer[0] = 0;

	for (i = 0; i < size; i++)
		OutputPacketBuffer[i+1] = buffer[i];

	res = WriteFile(handle->writeHandle, OutputPacketBuffer, size + 1, &BytesWritten, 0);
	free(OutputPacketBuffer);

	if (!res) {
		ErrorStatus = GetLastError();

		if (ErrorStatus == ERROR_DEVICE_NOT_CONNECTED) {
			USBCom_Close(handle);
			handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
			return 0;
		}
		else {
			fprintf(stderr, "WriteFile failed with return code %ld\n", ErrorStatus);
			handle->lastError = USBCOM_DEVICE_WRITE_ERR;
			return 0;
		}
	}

	return BytesWritten;
}

UINT USBCom_InterruptRead(TUSBHandle* handle, BYTE* buffer, UINT const size, UINT const mask, UINT const timeout) {
	DWORD BytesRead = 0;
	DWORD ErrorStatus;
	BYTE* InputPacketBuffer = NULL;
	UINT i;
	BOOL Match;
	BOOL res;
	OVERLAPPED gOverlapped;

	if (!handle->isOpened) {
		handle->lastError = USBCOM_DEVICE_NOT_OPENED_ERR;
		return 0;
	}

	InputPacketBuffer = (BYTE*) malloc((size + 1)*sizeof(BYTE));

	if (InputPacketBuffer == NULL) {
		handle->lastError = USBCOM_MEMORY_ALLOC_ERR;
		return 0;
	}

	gOverlapped.Offset = 0;
	gOverlapped.OffsetHigh = 0;
	gOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	do {
		Match = TRUE;
		InputPacketBuffer[0] = 0;
		/* The following call to ReadFIle() retrieves 64 bytes of data from the USB device. */
		res = ReadFile(handle->readHandle, InputPacketBuffer, size + 1, &BytesRead, &gOverlapped);

		if (!res) {
			ErrorStatus = GetLastError();

			if (ErrorStatus != ERROR_IO_PENDING) {
				free(InputPacketBuffer);

				fprintf(stderr, "ReadFile failed with return code %ld\n", ErrorStatus);
				handle->lastError = USBCOM_DEVICE_READ_ERR;
				return 0;
			}
		}

		WaitForSingleObject(gOverlapped.hEvent, INFINITE);

		if (GetOverlappedResult(gOverlapped.hEvent, &gOverlapped, &BytesRead, FALSE)) {
			if (mask != 0) {
				for (i = 0; i < 8*sizeof(UINT); i++) {
					if (((mask >> i) & 1) == 1 && buffer[i] != InputPacketBuffer[i+1]) {
						Match = FALSE;
						break;
					}
				}
			}
		}
	}
	while (!Match);

	for (i = 0; i < size; i++)
		buffer[i] = InputPacketBuffer[i+1];

	free(InputPacketBuffer);
	return BytesRead;
}

TUSBException USBCom_GetLastError(TUSBHandle* handle) {
	TUSBException error = handle->lastError;
	handle->lastError = USBCOM_NOEXCEPTION_ERR;

	return error;
}

#endif
