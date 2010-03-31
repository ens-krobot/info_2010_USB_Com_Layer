%module USB_Com_Layer
%{
#include "usb_com_layer.h"
#include "PcInterface.h"
%}

%include "PcInterface.h"

%apply unsigned char { BYTE }
%apply char* { BYTE* }
%apply unsigned short int { WORD }
%apply unsigned long { DWORD }
%apply unsigned int { UINT }

%define %binary_mutable(TYPEMAP,SIZE)
	%typemap(in) TYPEMAP(char temp[SIZE]) {
	   char *t = PyString_AsString($input);
	   if (SWIG_arg_fail($argnum)) SWIG_fail;
	   memcpy(temp,t,SIZE);
	   $1 = ($1_ltype) temp;
	}
	%typemap(argout,fragment="t_output_helper") TYPEMAP {
	   PyObject *o = PyString_FromStringAndSize($1,SIZE);
	   $result = t_output_helper($result,o);
	}
%enddef

%binary_mutable(BYTE* buffer, 64);

typedef enum {
	FALSE = 0,
	TRUE
} BOOL;

typedef enum {
	USBCOM_NOEXCEPTION_ERR = 0,
	USBCOM_DEVICE_NOT_OPENED_ERR,
	USBCOM_DEVICE_NOT_FOUND_ERR,
	USBCOM_DEVICE_READ_ERR,
	USBCOM_DEVICE_WRITE_ERR,
	USBCOM_DEVICE_OPEN_ERR,
	USBCOM_MEMORY_ALLOC_ERR,
	USBCOM_UNIMPLEMENTED_ERR
} TUSBException;

TUSBHandle* USBCom_Create();
void USBCom_Destroy(TUSBHandle* handle);
BOOL USBCom_Open(TUSBHandle* handle, WORD const vid, WORD const pid);
void USBCom_Close(TUSBHandle* handle);
BOOL USBCom_IsOpened(TUSBHandle* handle);
UINT USBCom_InterruptWrite(TUSBHandle* handle, BYTE* const buffer, UINT const size, UINT const timeout = 1000);
UINT USBCom_InterruptRead(TUSBHandle* handle, BYTE* buffer, UINT const size, UINT const mask, UINT const timeout = 1000);
TUSBException USBCom_GetLastError(TUSBHandle* handle);
