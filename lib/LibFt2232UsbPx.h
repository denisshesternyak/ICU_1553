#ifndef __LIBFT2232_USBPX_H
#define __LIBFT2232_USBPX_H

#include "excdef.h"
#include "flags_unet.h"
#include "ftd2xx.h"

// Let's also gain access to the Host2Dpr collection of errors:
#include "Host2Dpr_Error.h"


// MAX_DEVICE_ID_LEN is defined as 200 in the DDK, but is probably undefined without.
#ifndef MAX_DEVICE_ID_LEN
#define MAX_DEVICE_ID_LEN 200
#endif

#define DEVINFO_VID_MASK 0xFFFF0000
#define DEVINFO_PID_MASK 0x0000FFFF

#define DATA_CHAN_FLAG 'A'
#define INT_CHAN_FLAG 'B'

#define BUF_SIZE_FT2232 512
#define SERIALNUM_SIZE 256
#define DEVICETYPE_SIZE 256
#define NUM_USBPX_DEVS 8

#define INFINITE_RW_TIMEOUT			0xFFFFFFFF

// Device endpoint(s)
#define EP_IN 0x81
#define EP_OUT 0x02

typedef struct
{
	int allocated;
	FT_HANDLE ftHandle;
} t_DEVINFO_FTD2xx_USBPX;

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

void GetDevSerNums(unsigned int maxAltSerNumsListLen, char * pAltSerNumsList);
BOOL IsUsbPxDevicePresent(unsigned short vendorId, unsigned short productId, unsigned char * pSerialNum);
BOOL IsMoreThanOneUsbUnetDevicePresent();
BOOL IsExactlyOneUsbPxDevicePresent();
BOOL IsExactlyOneUsbRTxDevicePresent();
BOOL IsExactlyOneUsbMaccDevicePresent();
int Init_UsbPx(unsigned char * pDeviceTypeString, unsigned char * pSerialNum, unsigned char dataOrIntFlag, unsigned int maxDevIdLen, char * pDeviceIdString, EXC_USB_HANDLE * pEUHandle, unsigned int maxAltSerNumsStringLen, char * pAltSerNumsString);
int Release_UsbPx(EXC_USB_HANDLE EUHandle);
int SetRWTimeouts_UsbPx(EXC_USB_HANDLE EUHandle, unsigned int rwTimeoutsMillisec);
int Write_BUF_SIZE_UsbPx(EXC_USB_HANDLE EUHandle, const unsigned char * pBuffer, unsigned int numCharsToWrite, unsigned int * pNumCharsWritten);
int Read_BUF_SIZE_UsbPx(EXC_USB_HANDLE EUHandle, unsigned char * pBuffer, unsigned int maxCharsToRead, unsigned int * pNumCharsRead);

#ifdef __cplusplus
}
#endif



#endif
