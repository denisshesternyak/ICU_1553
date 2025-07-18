// Host2Dpr Errors
#pragma once
//#include "ftd2xx.h"

// ASSERTS:

// #define NDEBUG // comment to turn on the asserts; uncomment to turn them off
#include <assert.h>
#define Function_Not_Implemented 0
#define LogError_Recorded_An_Error 0
// #define Device_NOT_Open_Operation_Failed 0
// #define ReadRequest_ReadReply_Header_Mismatch 0


/* function prototype */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32
#define __declspec(dllexport) 
#endif
void __declspec(dllexport) Get_Error_String_UNET(int errorNum, int errStringLen, char *pErrorString);

#ifdef __cplusplus
}
#endif

// ****************************************************************************
// There are TWO classes of errors here: Those which are used in the internal
// Host2Dpr family classes (and are encountered in the log files when enabled),
// and those that are returned to the Software Tools that use the Host2Dpr
// extensions.
// ****************************************************************************

#define H2D_MAX_ERROR_STRING_LEN 254

#ifndef eSuccessfulCompletion
#define eSuccessfulCompletion 0
#endif


// ****************************************************************************
//
//  ERRORS returned to the Software Tools (and are likely propagated to users)
//
// ****************************************************************************



// ****************************************************************************
//
//                INTERNAL (Reported-In-Log-Files) ERRORS
//
// ****************************************************************************


// FT_STATUS errors (These are FTDI's errors (error numbers), translated to range from -3200 to -3219)

#define FTSTATUS_BASE_ERROR -3100		// moved from -3200, defined for the original UNet
#define eKappluey FTSTATUS_BASE_ERROR
#define FTSTATUS_MAX_ERROR 50			// originally 20; we can bump this up more if we want.

																										// FT_OK = 0

#define eUnsupportedFeature -3199

// These are the FTDI errors, copied (and renamed) from FTD3XX.h
typedef enum _FTD_ERRORS
{
    eFT_OK,
    eFT_INVALID_HANDLE,
    eFT_DEVICE_NOT_FOUND,
    eFT_DEVICE_NOT_OPENED,
    eFT_IO_ERROR,
    eFT_INSUFFICIENT_RESOURCES,
    eFT_INVALID_PARAMETER,
    eFT_INVALID_BAUD_RATE,
    eFT_DEVICE_NOT_OPENED_FOR_ERASE,
    eFT_DEVICE_NOT_OPENED_FOR_WRITE,
    eFT_FAILED_TO_WRITE_DEVICE,
    eFT_EEPROM_READ_FAILED,
    eFT_EEPROM_WRITE_FAILED,
    eFT_EEPROM_ERASE_FAILED,
    eFT_EEPROM_NOT_PRESENT,
    eFT_EEPROM_NOT_PROGRAMMED,
    eFT_INVALID_ARGS,
    eFT_NOT_SUPPORTED,

    eFT_NO_MORE_ITEMS,
    eFT_TIMEOUT,
    eFT_OPERATION_ABORTED,
    eFT_RESERVED_PIPE,
    eFT_INVALID_CONTROL_REQUEST_DIRECTION,
    eFT_INVALID_CONTROL_REQUEST_TYPE,
    eFT_IO_PENDING,
    eFT_IO_INCOMPLETE,
    eFT_HANDLE_EOF,
    eFT_BUSY,
    eFT_NO_SYSTEM_RESOURCES,
    eFT_DEVICE_LIST_NOT_READY,
    eFT_DEVICE_NOT_CONNECTED,
	eFT_INCORRECT_DEVICE_PATH,

    eFT_OTHER_ERROR,
}FTD_ERRORS;

/*
// These errors are for the original UNet

#define eFtStat_INVALID_HANDLE					(FTSTATUS_BASE_ERROR-FT_INVALID_HANDLE)					// FT_INVALID_HANDLE = 1
#define eFtStat_DEVICE_NOT_FOUND				(FTSTATUS_BASE_ERROR-FT_DEVICE_NOT_FOUND)				// FT_DEVICE_NOT_FOUND = 2
#define eFtStat_DEVICE_NOT_OPENED				(FTSTATUS_BASE_ERROR-FT_DEVICE_NOT_OPENED)				// FT_DEVICE_NOT_OPENED= 3
#define eFtStat_IO_ERROR						(FTSTATUS_BASE_ERROR-FT_IO_ERROR)						// FT_IO_ERROR = 4
#define eFtStat_INSUFFICIENT_RESOURCES			(FTSTATUS_BASE_ERROR-FT_INSUFFICIENT_RESOURCES)			// FT_INSUFFICIENT_RESOURCES = 5
#define eFtStat_INVALID_PARAMETER				(FTSTATUS_BASE_ERROR-FT_INVALID_PARAMETER)				// FT_INVALID_PARAMETER = 6
#define eFtStat_INVALID_BAUD_RATE				(FTSTATUS_BASE_ERROR-FT_INVALID_BAUD_RATE)				// FT_INVALID_BAUD_RATE = 7
#define eFtStat_DEVICE_NOT_OPENED_FOR_ERASE		(FTSTATUS_BASE_ERROR-FT_DEVICE_NOT_OPENED_FOR_ERASE)	// FT_DEVICE_NOT_OPENED_FOR_ERASE = 8
#define eFtStat_DEVICE_NOT_OPENED_FOR_WRITE		(FTSTATUS_BASE_ERROR-FT_DEVICE_NOT_OPENED_FOR_WRITE)	// FT_DEVICE_NOT_OPENED_FOR_WRITE = 9
#define eFtStat_FAILED_TO_WRITE_DEVICE			(FTSTATUS_BASE_ERROR-FT_FAILED_TO_WRITE_DEVICE)			// FT_FAILED_TO_WRITE_DEVICE = 10
#define eFtStat_EEPROM_READ_FAILED				(FTSTATUS_BASE_ERROR-FT_EEPROM_READ_FAILED)				// FT_EEPROM_READ_FAILED = 11
#define eFtStat_EEPROM_WRITE_FAILED				(FTSTATUS_BASE_ERROR-FT_EEPROM_WRITE_FAILED)			// FT_EEPROM_WRITE_FAILED = 12
#define eFtStat_EEPROM_ERASE_FAILED				(FTSTATUS_BASE_ERROR-FT_EEPROM_ERASE_FAILED)			// FT_EEPROM_ERASE_FAILED = 13
#define eFtStat_EEPROM_NOT_PRESENT				(FTSTATUS_BASE_ERROR-FT_EEPROM_NOT_PRESENT)				// FT_EEPROM_NOT_PRESENT = 14
#define eFtStat_EEPROM_NOT_PROGRAMMED			(FTSTATUS_BASE_ERROR-FT_EEPROM_NOT_PROGRAMMED)			// FT_EEPROM_NOT_PROGRAMMED = 15
#define eFtStat_INVALID_ARGS					(FTSTATUS_BASE_ERROR-FT_INVALID_ARGS)					// FT_INVALID_ARGS = 16
#define eFtStat_NOT_SUPPORTED					(FTSTATUS_BASE_ERROR-FT_NOT_SUPPORTED)					// FT_NOT_SUPPORTED = 17
#define eFtStat_OTHER_ERROR						(FTSTATUS_BASE_ERROR-FT_OTHER_ERROR)					// FT_OTHER_ERROR = 18
#define eFtStat_DEVICE_LIST_NOT_READY			(FTSTATUS_BASE_ERROR-FT_DEVICE_LIST_NOT_READY)			// FT_DEVICE_LIST_NOT_READY = 19
#define eFtStat_UNKNOWN							(FTSTATUS_BASE_ERROR-FTSTATUS_MAX_ERROR)				// Unknown error; the number exceeds the documented error list
*/

// These errors are for the UNet 2

#define eFtStat_INVALID_HANDLE					(FTSTATUS_BASE_ERROR-eFT_INVALID_HANDLE)				// FT_INVALID_HANDLE = 1
#define eFtStat_DEVICE_NOT_FOUND				(FTSTATUS_BASE_ERROR-eFT_DEVICE_NOT_FOUND)				// FT_DEVICE_NOT_FOUND = 2
#define eFtStat_DEVICE_NOT_OPENED				(FTSTATUS_BASE_ERROR-eFT_DEVICE_NOT_OPENED)				// FT_DEVICE_NOT_OPENED= 3
#define eFtStat_IO_ERROR						(FTSTATUS_BASE_ERROR-eFT_IO_ERROR)						// FT_IO_ERROR = 4
#define eFtStat_INSUFFICIENT_RESOURCES			(FTSTATUS_BASE_ERROR-eFT_INSUFFICIENT_RESOURCES)		// FT_INSUFFICIENT_RESOURCES = 5
#define eFtStat_INVALID_PARAMETER				(FTSTATUS_BASE_ERROR-eFT_INVALID_PARAMETER)				// FT_INVALID_PARAMETER = 6
#define eFtStat_INVALID_BAUD_RATE				(FTSTATUS_BASE_ERROR-eFT_INVALID_BAUD_RATE)				// FT_INVALID_BAUD_RATE = 7
#define eFtStat_DEVICE_NOT_OPENED_FOR_ERASE		(FTSTATUS_BASE_ERROR-eFT_DEVICE_NOT_OPENED_FOR_ERASE)	// FT_DEVICE_NOT_OPENED_FOR_ERASE = 8
#define eFtStat_DEVICE_NOT_OPENED_FOR_WRITE		(FTSTATUS_BASE_ERROR-eFT_DEVICE_NOT_OPENED_FOR_WRITE)	// FT_DEVICE_NOT_OPENED_FOR_WRITE = 9
#define eFtStat_FAILED_TO_WRITE_DEVICE			(FTSTATUS_BASE_ERROR-eFT_FAILED_TO_WRITE_DEVICE)		// FT_FAILED_TO_WRITE_DEVICE = 10
#define eFtStat_EEPROM_READ_FAILED				(FTSTATUS_BASE_ERROR-eFT_EEPROM_READ_FAILED)			// FT_EEPROM_READ_FAILED = 11
#define eFtStat_EEPROM_WRITE_FAILED				(FTSTATUS_BASE_ERROR-eFT_EEPROM_WRITE_FAILED)			// FT_EEPROM_WRITE_FAILED = 12
#define eFtStat_EEPROM_ERASE_FAILED				(FTSTATUS_BASE_ERROR-eFT_EEPROM_ERASE_FAILED)			// FT_EEPROM_ERASE_FAILED = 13
#define eFtStat_EEPROM_NOT_PRESENT				(FTSTATUS_BASE_ERROR-eFT_EEPROM_NOT_PRESENT)			// FT_EEPROM_NOT_PRESENT = 14
#define eFtStat_EEPROM_NOT_PROGRAMMED			(FTSTATUS_BASE_ERROR-eFT_EEPROM_NOT_PROGRAMMED)			// FT_EEPROM_NOT_PROGRAMMED = 15
#define eFtStat_INVALID_ARGS					(FTSTATUS_BASE_ERROR-eFT_INVALID_ARGS)					// FT_INVALID_ARGS = 16
#define eFtStat_NOT_SUPPORTED					(FTSTATUS_BASE_ERROR-eFT_NOT_SUPPORTED)					// FT_NOT_SUPPORTED = 17

#define eFtStat_FT_NO_MORE_ITEMS				(FTSTATUS_BASE_ERROR-eFT_NO_MORE_ITEMS)					// FT_NO_MORE_ITEMS = 18
#define eFtStat_FT_TIMEOUT						(FTSTATUS_BASE_ERROR-eFT_TIMEOUT)						// FT_TIMEOUT = 19
#define eFtStat_FT_OPERATION_ABORTED			(FTSTATUS_BASE_ERROR-eFT_OPERATION_ABORTED)				// FT_OPERATION_ABORTED = 20
#define eFtStat_FT_RESERVED_PIPE				(FTSTATUS_BASE_ERROR-eFT_RESERVED_PIPE)					// FT_RESERVED_PIPE = 21
#define eFtStat_FT_INVALID_CONTROL_REQUEST_DIRECTION	(FTSTATUS_BASE_ERROR-eFT_INVALID_CONTROL_REQUEST_DIRECTION)	// FT_INVALID_CONTROL_REQUEST_DIRECTION = 22
#define eFtStat_FT_INVALID_CONTROL_REQUEST_TYPE			(FTSTATUS_BASE_ERROR-eFT_INVALID_CONTROL_REQUEST_TYPE)		// FT_INVALID_CONTROL_REQUEST_TYPE = 23
#define eFtStat_FT_IO_PENDING					(FTSTATUS_BASE_ERROR-eFT_IO_PENDING)						// FT_IO_PENDING = 24
#define eFtStat_FT_IO_INCOMPLETE				(FTSTATUS_BASE_ERROR-eFT_IO_INCOMPLETE)					// FT_IO_INCOMPLETE = 25
#define eFtStat_FT_HANDLE_EOF					(FTSTATUS_BASE_ERROR-eFT_HANDLE_EOF)						// FT_HANDLE_EOF = 26
#define eFtStat_FT_BUSY							(FTSTATUS_BASE_ERROR-eFT_BUSY)							// FT_BUSY = 27
#define eFtStat_FT_NO_SYSTEM_RESOURCES			(FTSTATUS_BASE_ERROR-eFT_NO_SYSTEM_RESOURCES)			// FT_NO_SYSTEM_RESOURCES = 28
#define eFtStat_DEVICE_LIST_NOT_READY			(FTSTATUS_BASE_ERROR-eFT_DEVICE_LIST_NOT_READY)			// FT_DEVICE_LIST_NOT_READY = 19
#define eFtStat_FT_DEVICE_NOT_CONNECTED			(FTSTATUS_BASE_ERROR-eFT_DEVICE_NOT_CONNECTED)			// FT_DEVICE_NOT_CONNECTED = 30
#define eFtStat_FT_INCORRECT_DEVICE_PATH		(FTSTATUS_BASE_ERROR-eFT_INCORRECT_DEVICE_PATH)			// FT_INCORRECT_DEVICE_PATH = 31
#define eFtStat_OTHER_ERROR						(FTSTATUS_BASE_ERROR-eFT_OTHER_ERROR)					// FT_OTHER_ERROR = 18
#define eFtStat_UNKNOWN							(FTSTATUS_BASE_ERROR-eFTSTATUS_MAX_ERROR)				// Unknown error; the number exceeds the documented error list

// #define eFtStatus			-3230

#define eDevNotReady				-3231
#define eDeviceTooMany				-3237			// too many devices initialized
#define eBadHandle					-3233			// bad (Excalibur) handle (as opposed to FTDI handle)
#define eReadHeaderMismatch			-3234			// ReadRequest header and ReadReply header do NOT match
#define eDeviceNotConnected			-3235			// UNet device is NOT connected
#define eNoUsbDevicesFound			-3236			// NO USB UNet devices are connected
#define eBadRDHandle				-3240			// bad (Excalibur b2drm remoteDevices) handle (as opposed to FTDI handle)

// Functionality discrepency between UNet and Local devices
#define eEomClearNotSupported		-3238			// 'Mesage-Read with EOM-Clear' not supported in local Exc-4000 Family devices
#define eFifoRWNotSupported			-3239			// 'UNet Multi FIFO reads/writes into buffer' are not supported

// These errors belong to the block of code that watches Windows message loop for (USB)
// connect & disconnect messages
#define eMsgLoopThread				-3241
#define eMsgMonWinReg				-3242
#define eMsgMonWinCreate			-3243
#define eDevChangeNotifyReg			-3244

// USB (methinks USB only) Read & Write timeouts 
#define eWriteTimeout				-3245			// Timeout occured attempting to write
#define eWriteIo					-3246			// Error (other than timeout) occurred attempting to write
#define eReadTimeout				-3247			// Timeout occurred attempting to read
#define eReadIo						-3248			// Error (other than timeout) occurred attempting to write

// Errors related to the Host-to-Bridge device, which uses the Lib3K driver
#define eLibK3_InitDevLst			-3251			// Error initializing UsbK3 device list
#define eLibK3_EmptyDevLst			-3252			// UsbK3 device list empty
#define eLibK3_InitUsbDev			-3253			// Error initializing the UsbK3 device
#define eLibK3_NoVidPidFound		-3254			// Device with matching VidPid NOT found
#define eLibK3_NoInstanceIdFound	-3255			// Device matched VidPid, but matching InstanceId not found
#define eLibK3_NoInterfaceClaimed	-3256			// Could not claim an interface for VidPid-matched device
#define eLibK3_SetReadTimeout		-3257			// Could not set the Read (EP_IN) timeout
#define eLibK3_SetWriteTimeout		-3258			// Could not set the Write (EP_OUT) timeout

// Errors related to the FTDI driver
#define eLibFt_SetTimeouts			-3261			// Error setting interface read/write timeouts

// Errors related to invalid Net parameters
#define eNet_InvalidIpAddr			-3262			// Invalid IP Address was specified
#define eNet_InvalidMacAddr			-3263			// Invalid MAC Address was specified

#define eInvalidUnetMod4Status		-3264			// Invalid UNet Bank4 Status (pUnetModuleFour->Status)
#define eUnetProgrammingFailed		-3265			// Failed the attempt to reprogram the UNet device

// Errors related to missing USB parameters
#define eUsb_NoVidSpecified			-3266			// No Vendor ID specified
#define eUsb_NoPidSpecified			-3267			// No Product ID specified
#define eUsb_NoSerNumSpecified		-3268			// No serial number specified

// And a build UsbSerialString from ExcSerialNumber error:
#define eInvalidUsbSerStrParam		-3269			// Invalid input to ExcSerNumToUsbSerString

// Errors related to Win32 NIC communication
#define eSetTimeoutError			-3270			// Error attempting to set send/recv timeouts
#define eOpenInvalidSocket			-3271			// Invalid socket when trying to create a socket
#define eGetHostInvalidSocket		-3272			// Invalid socket when trying to get the host name
#define eBindInvalidSocket			-3273			// Invalid socket when trying to call bind
#define eCloseInvalidSocket			-3274			// Invalid socket when trying to close the socket
#define eWriteSocketError			-3275			// Socket Error when trying to send a packet
#define ePartialWrite				-3276			// Send-packet call only sent part of the data
#define eNoPktReceived				-3277			// No new packet received trying to retrieve a new packet
#define eReadSocketError			-3278			// Socket Error when trying to retrieve a new packet
#define eZeroBytePayloadPkt			-3279			// Empty packet received when trying to receive network data
#define eReceivedFewerBytePkt		-3280			// Recv-packet call returned fewer bytes than expected/requested
#define eReceivedGreaterBytePkt		-3281			// Recv-packet call returned more bytes than expected/requested
#define eSocketReadError			-3282			// Select call for reading returned an error
#define eConnectFailed				-3283			// Error reading a UNet board register, hence declare the connection attempt FAILED
#define eReadReplyTimeout			-3284			// Read request sent, but timed out while waiting for the read-reply
#define eReadPacketTimeout			-3285			// NIC Select function timed out while waiting for a read

// Errors related to registry info/access
#define eDeviceNotListedAsUnet		-3286			// Device number is not listed in Registry or is not listed as UNet device
#define eMissingUsbRegData			-3287			// USB UNet device missing data in Registry
#define eUsbUnetDeviceNotFound		-3288			// Could not connect to the specified USB UNet device
#define eUsbUnetIntChanConnect		-3289			// Could not connect to the USB interrupt channel; entire connection aborted
#define eMissingNetRegData			-3290			// NET UNet device missing data in Registry
#define eNetUnetDeviceNotFound		-3291			// Could not connect to the specified NET UNet device
#define eNetUnetIntChanConnect		-3292			// Could not connect to the NET interrupt channel; entire connection aborted
#define eInvalidUnetConnectType		-3293			// Invalid UNet connection type (ie not 'NET' or 'USB')
#define eUsbUnetMultipleDevsAsStar	-3294			// Could not connect to USB UNet. Serial# is '*', but multiple devs found - run ExcConfig to config

#define eExcRegKeyNotFound			-3295			// Could not LOCATE the Excalibur Device's registry key
#define eExcRegKeyNotOpened			-3296			// Could not OPEN the Excalibur Device's registry key
#define eExcRegValNotRead			-3297			// Could not READ the Excalibur Device's registry value
#define eExcRegInvalidVal			-3298			// The Excalibur Device's registry value is INVALID
#define eExcRegKeyNotCreated		-3299			// Could not CREATE the Excalibur Device's registry key
#define eExcRegValNotWritten		-3300			// Could not WRITE the Excalibur Device's registry value
#define eExcRegValNotDeleted		-3315			// Could not DELETE the Excalibur Device's registry value


// Interrupt related errors
#define iIntRequestRcvd				-3301			// Info: WaitForInterrupt Request Received
#define iIntCancelRcvd				-3302			// Info: Cancel WaitForInterrupt Request Received
#define iGetIntCountRcvd			-3303			// Info: GetInterruptCount Request Received
#define eIntReplyMismatch			-3304			// Header or Data mismatch in interrupt reply
#define eCreateEventFailed			-3305			// Failed attempt to create a Windows Event

// Code implementation error
#define eFunctionNotImplemented		-3311			// Function has not been implemented (yet)

#define eNonUnetDeviceListedAsUnet		-3400			// Device number is listed as UNet device even though it is not //TODO:This needs a more appropriate number and a better name
// .. Used for interrupt timeouts; the error is the same number as if error_devio.h encountered ekerneltimeout
#ifndef ekerneltimeout
#define ekerneltimeout				-1038			// Timeout expired before interrupt occurred
#endif

