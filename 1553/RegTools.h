#pragma once

/*

"Successful Completion [eSuccessfulCompletion]");

#define eExcRegKeyNotFound		// Could not LOCATE the Excalibur Device's registry key
#define eExcRegKeyNotOpened		// Could not OPEN the Excalibur Device's registry key
#define eExcRegValNotRead		// Could not READ the Excalibur Device's registry value
#define eExcRegInvalidVal		// The Excalibur Device's registry value is INVALID
#define eExcRegKeyNotCreated	// Could not CREATE the Excalibur Device's registry key
#define eExcRegValNotWritten	// Could not WRITE the Excalibur Device's registry value

*/



/*

Do i need this to do MFC and multibyte?

1) // This needs to go first in order to ensure the correct order of library linking
#include <Afx.h>

But problem! maybe another answer is to chose to ignore this library:

Project Properties | Linker | Ignore Specific Library |
LIBCMTD.lib

2) Project Properties | Linker | Additional Dependencies |
advapi32.lib

*/

// #include "Host2Dpr.h"
#ifdef _WIN32
#include <windows.h>
#else
#include "WinTypes.h"
#endif
#include <stdio.h>
#include "Host2Dpr_Error.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_LENGTH 255

#define EXC_CFG_KEY "SYSTEM\\CurrentControlSet\\Services\\Exc2000\\Devices\\ExcaliburDevice"
#define EXC_CFG64_KEY "SYSTEM\\CurrentControlSet\\Services\\Excx64\\Devices\\ExcaliburDevice"

typedef enum ConnectionType
{
	CONNECT_TYPE_NONE = 0,
	CONNECT_TYPE_NET = 1,
	CONNECT_TYPE_USB = 2
} ConnectionType;

typedef enum Single_Multi_RT
{
	SINGLE_RT_PX = 0,
	MULTI_RT_PX = 1
} Single_Multi_RT;

/* function prototypes */
#ifdef __cplusplus
extern "C" {
#endif

/*
Function GetDeviceType: Returns the device type (eg 4000PCI or EXC-1553UNET)

Valid returns are:
	eSuccessfulCompletion //  "Successful Completion [eSuccessfulCompletion]");
	eExcRegKeyNotFound; // "Could not LOCATE the Excalibur Device's registry key [eExcRegKeyNotFound]"
	eExcRegKeyNotOpened; // "Could not OPEN the Excalibur Device's registry key [eExcRegKeyNotOpened]"
	eExcRegValNotRead; // "Could not READ the Excalibur Device's registry value [eExcRegValNotRead]"
*/

#ifdef _WIN32
int GetDeviceType(int deviceNumber, char * pDeviceType);
#else
//The linux ftd2xx library has a function called GetDeviceType
int GetDeviceTypeReg(int deviceNumber, char * pDeviceType);
#endif


/*
Function IsRemoteDevice:	Returns TRUE if the device is a UNET device; FALSE otherwise
(also returns FALSE if there was an error finding the registry key OR reading the registry.)
*/

BOOL IsRemoteDevice (int deviceNumber);

/*
Function GetConnectionType: Returns the connection type (ie enum CONNECT_TYPE_NET or CONNECT_TYPE_USB)

Valid returns are:
	eSuccessfulCompletion //  "Successful Completion [eSuccessfulCompletion]");
	eExcRegKeyNotFound; // "Could not LOCATE the Excalibur Device's registry key [eExcRegKeyNotFound]" (-3291)
	eExcRegKeyNotOpened; // "Could not OPEN the Excalibur Device's registry key [eExcRegKeyNotOpened]" (-3292)
	eExcRegValNotRead; // "Could not READ the Excalibur Device's registry value [eExcRegValNotRead]" (-3293)
	eExcRegInvalidVal; // "The Excalibur Device's registry value is INVALID [eExcRegInvalidVal]" (-3294)
*/

int GetConnectionType (int deviceNumber, enum ConnectionType * pUNetType);

/*
Function GetNetInfo: Returns the specified MAC_Address, IP_Address, and UDP port number parameters

Valid returns are:
	eSuccessfulCompletion //  "Successful Completion [eSuccessfulCompletion]");
	eExcRegKeyNotFound; // "Could not LOCATE the Excalibur Device's registry key [eExcRegKeyNotFound]" (-3291)
	eExcRegKeyNotOpened; // "Could not OPEN the Excalibur Device's registry key [eExcRegKeyNotOpened]" (-3292)
	eExcRegValNotRead; // "Could not READ the Excalibur Device's registry value [eExcRegValNotRead]" (-3293)
*/

int GetNetInfo (int deviceNumber, char * pMacAddress, char * pIpAddress, unsigned short * pUdpPort);

/*
Function GetUsbInfo: Returns the specified Vendor_ID, Product_ID, and Serial Number parameters

Valid returns are:
	eSuccessfulCompletion //  "Successful Completion [eSuccessfulCompletion]");
	eExcRegKeyNotFound; // "Could not LOCATE the Excalibur Device's registry key [eExcRegKeyNotFound]" (-3291)
	eExcRegKeyNotOpened; // "Could not OPEN the Excalibur Device's registry key [eExcRegKeyNotOpened]" (-3292)
	eExcRegValNotRead; // "Could not READ the Excalibur Device's registry value [eExcRegValNotRead]" (-3293)
*/

int GetUsbInfo (int deviceNumber, char * pVendorId, char * pProductId, char * pSerialNum);

// Experimental functions; not for production use 
int GetSharedUsbInfo (int deviceNumber, char * pSharedGuid, unsigned short * pSharedUdpPort);
int SetSharedUsbInfo (int deviceNumber, char * pSharedGuid, unsigned short sharedUdpPort);

/*
// ReadUnetSerialId reads the serial number string from the registry; extracts the integer ID, and returns that ID to the user.
int ReadUnetSerialId  (int deviceNumber, int * pSerialId);

// WriteUnetSerialId first checks that the device is a UNet device, and if so converts the SerialID into a Unet Serial ID string
int WriteUnetSerialId  (int deviceNumber, int serialId);

// Two conversion functions for the serial number IDs/Strings (returns error if illegal value(s))
int UnetSerialIdStringEncode (int deviceId, enum Single_Multi_RT smFlag, int serialId, char * pSerialString);
int UnetSerialIdStringDecode (char * pSerialString, int * pDeviceId, enum * Single_Multi_RT pSmFlag, int * pSerialId);
*/

/*
Function IsDebugDeviceEnabled:	Returns TRUE if any of the device's modules have debug turned on; FALSE otherwise
(also returns FALSE if there was an errror finding the registry key OR reading the registry.)
*/
 
BOOL IsDebugDeviceEnabled(int deviceNumber);

/*
Function IsDebugModuleEnabled:	Returns TRUE if the specified device & module (pair) has debug turned on; FALSE otherwise
(also returns FALSE if there was an errror finding the registry key OR reading the registry.)
*/

BOOL IsDebugModuleEnabled(int deviceNumber, int moduleNumber);

// A few internal support functions
int GetRegVal_DWORD(int deviceNumber, char * pValName, DWORD * pValVal_DWORD);
int GetRegVal_SZ(int deviceNumber, char * pValName, int valValSz_Size, char * pValVal_SZ);

int SetRegVal_DWORD(int deviceNumber, char * pValName, DWORD valVal_DWORD);
int SetRegVal_SZ(int deviceNumber, char * pValName, char * pValVal_SZ);

BOOL Is64Bit();
BOOL IsWow64();


#ifdef __cplusplus
}
#endif


