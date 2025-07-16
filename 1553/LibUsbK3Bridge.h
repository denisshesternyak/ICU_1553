#ifndef __LIBUSBK3_BRIDGE_H
#define __LIBUSBK3_BRIDGE_H

#include "excdef.h"
#include "libusbk.h"

// Let's use and return FTDI errors:
#include "FTD2xx.h"

// Let's also gain access to the Host2Dpr collection of errors:
#include "Host2Dpr_Error.h"

#define BUF_SIZE_BRIDGE 64
#define NUM_USBBRIDGE_ENDS 2
#define PIPE_DEFAULT_TIMEOUT_MS		100
#define TIMEOUT_ERRORCODE	121	//BARF - this was commented out of LibFt2232UsbPx.h ?

// Device endpoint(s)
#define EP_IN 0x81
#define EP_OUT 0x02

typedef struct
{
	int allocated;
	KLST_HANDLE deviceList;
	KLST_DEVINFO_HANDLE deviceInfo;
	KUSB_HANDLE usbHandle;
	KUSB_DRIVER_API usbApi;
	int interfaceClaimed;
} t_DEVINFO_USB;

typedef int EXC_USB_HANDLE;

/*
FT_STATUSes:
	FT_OK = 0
	FT_INVALID_HANDLE = 1
	FT_DEVICE_NOT_FOUND = 2
	FT_DEVICE_NOT_OPENED = 3
	FT_IO_ERROR = 4
	FT_INSUFFICIENT_RESOURCES = 5
	FT_INVALID_PARAMETER = 6
	FT_INVALID_BAUD_RATE = 7
	FT_DEVICE_NOT_OPENED_FOR_ERASE = 8
	FT_DEVICE_NOT_OPENED_FOR_WRITE = 9
	FT_FAILED_TO_WRITE_DEVICE = 10
	FT_EEPROM_READ_FAILED = 11
	FT_EEPROM_WRITE_FAILED = 12
	FT_EEPROM_ERASE_FAILED = 13
	FT_EEPROM_NOT_PRESENT = 14
	FT_EEPROM_NOT_PROGRAMMED = 15
	FT_INVALID_ARGS = 16
	FT_NOT_SUPPORTED = 17
	FT_OTHER_ERROR = 18
	FT_DEVICE_LIST_NOT_READY = 19
*/


/* function prototypes */
#ifdef __cplusplus
extern "C" {
#endif

int Init_UsbBridge(unsigned short vendorId, unsigned short productId, unsigned int maxDevIdLen, char * pDeviceIdString, EXC_USB_HANDLE * pEUHandle);
int Init_UsbBridge_Instance(unsigned short vendorId, unsigned short productId, unsigned char * pInstanceId, unsigned int maxDevIdLen, char * pDeviceIdString, EXC_USB_HANDLE * pEUHandle);
int Release_UsbBridge(EXC_USB_HANDLE EUHandle);
int SetRWTimeouts_UsbBridge(EXC_USB_HANDLE EUHandle, ULONG pipeTimeoutMillisec);
int Write_BUF_SIZE_UsbBridge(EXC_USB_HANDLE EUHandle, const unsigned char * pBuffer, unsigned int numCharsToWrite, unsigned int * pNumCharsWritten);
int Read_BUF_SIZE_UsbBridge(EXC_USB_HANDLE EUHandle, unsigned char * pBuffer, unsigned int maxCharsToRead, unsigned int * pNumCharsRead);

#ifdef __cplusplus
}
#endif



#endif