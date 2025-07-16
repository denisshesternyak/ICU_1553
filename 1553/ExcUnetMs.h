#pragma once

//
// UNet Public Interface Prototype Functions
//
// The bulk of these functions define a bridge interface between the SW tools and the target raw memory of the device
// that enables a raw reading and writing of bytes (8 bit) and words (16 bits) to and from the raw memory of the device
//

#ifdef _WIN32
#include <windows.h>
#else
#include "WinTypes.h"
#endif
#include "RegTools.h"

//Definitions for old compilers
#ifndef ULONG_PTR
#if defined(_WIN64)
    typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;
#else
    typedef unsigned long ULONG_PTR, *PULONG_PTR;
#endif
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
#endif

#define MACC_COMMAND_PORT 0x1234

#ifndef NUMUNETCARDS
/* These flags are used internally for compilation of galahad dll */
#define NUMUNETBOARDS 16  
#define NUMUNETMODULES 16
#define NUMUNETCARDS ((NUMUNETBOARDS * NUMUNETMODULES) + 1) // as defined in flags_px.h (with the "+ 1"
#endif

#ifndef eboardnotalloc
#define eboardnotalloc	-24
#endif

#define MODULE_0 0
#define MODULE_1 1
#define MODULE_2 2
#define MODULE_3 3
#define MODULE_4 4

#ifndef BYTES_PER_WORD
#define BYTES_PER_WORD 2
#endif

#ifndef BYTES_PER_DWORD
#define BYTES_PER_DWORD 4
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32
#define __declspec(dllexport) 
#endif
void __declspec(dllexport) Get_Error_String_UNET(int errorNum, int errStringLen, char *pErrorString);

// Bridge2DeviceRawMemory

void			__declspec(dllexport) b2drmSetBaseAddress( DWORD_PTR address, int remoteModuleHandle );
void			__declspec(dllexport) b2drmSetTestMacroWPciBaseAddress( unsigned int address, int remoteModuleHandle );
unsigned char	__declspec(dllexport) b2drmReadByteFromDpr( DWORD_PTR offsetAddress, const char * paramName, int remoteModuleHandle  );
unsigned short	__declspec(dllexport) b2drmReadWordFromDpr( DWORD_PTR offsetAddress, const char * paramName, int remoteModuleHandle  );
unsigned int	__declspec(dllexport) b2drmReadDWordFromDpr( DWORD_PTR offsetAddress, const char * paramName, int remoteModuleHandle  );
void			__declspec(dllexport) b2drmWriteByteToDpr ( DWORD_PTR offsetAddress,
										unsigned char dataByte,
										const char * paramName,
										int remoteModuleHandle );
void			__declspec(dllexport) b2drmWriteWordToDpr ( DWORD_PTR offsetAddress,
										unsigned short dataWord,
										const char * paramName,
										int remoteModuleHandle );
void			__declspec(dllexport) b2drmWriteDWordToDpr ( DWORD_PTR offsetAddress,
										unsigned int dataDWord,
										const char * paramName,
										int remoteModuleHandle );
void			__declspec(dllexport) b2drmReadWordBufferFromDpr( DWORD_PTR offsetAddress,
										unsigned short * wordBufferToFill,
										unsigned int numWordsToRead,
										const char * paramName,
										int remoteModuleHandle );
void			__declspec(dllexport) b2drmReadDWordBufferFromDpr( DWORD_PTR offsetAddress,
									 unsigned int * dwordBufferToFill,
									 unsigned int numDWordsToRead,
									 const char * paramName,
									 int remoteModuleHandle );
void			 __declspec(dllexport) b2drmReadWordBufferFromDpr_AndClearEOMStatus( DWORD_PTR offsetAddress,
										unsigned short * wordBufferToFill,
										unsigned int numWordsToRead,
										const char * paramName,
										int remoteModuleHandle );
void			__declspec(dllexport) b2drmWriteWordBufferToDpr ( DWORD_PTR offsetAddress,
										unsigned short * wordBufferToWrite,
										unsigned int numWordsToWrite,
										const char * paramName,
										int remoteModuleHandle );
void			__declspec(dllexport) b2drmWriteDWordBufferToDpr ( DWORD_PTR offsetAddress,
										unsigned int * dwordBufferToWrite,
										unsigned int numDWordsToWrite,
										const char * paramName,
										int remoteModuleHandle );
void			__declspec(dllexport) b2drmReadWordFifoFromDpr( DWORD_PTR offsetAddress,
										unsigned short * wordBufferToFill,
										unsigned int numWordsToRead,
										const char * paramName,
										int remoteModuleHandle,
										int * pErrStatus );
void			__declspec(dllexport) b2drmWriteWordFifoToDpr ( DWORD_PTR offsetAddress,
										unsigned short * wordBufferToWrite,
										unsigned int numWordsToWrite,
										const char * paramName,
										int remoteModuleHandle,
										int * pErrStatus );
int				__declspec(dllexport) b2drmOpenReadWORDClose (
										int deviceNum,
										int moduleNum,
										DWORD_PTR offset,
										char *paramName,
										unsigned short *pData,
										int remoteModuleHandle );
int				__declspec(dllexport) b2drmOpenReadWORDBufferClose (
										int deviceNum,
										int moduleNum,
										DWORD_PTR offset,
										char *paramName,
										unsigned short *pData,
										unsigned int numWordsToRead,
										int remoteModuleHandle );
int				 __declspec(dllexport) b2drmOpenWriteWORDClose (
										int deviceNum,
										int moduleNum,
										DWORD_PTR offset,
										const char * paramName,
										unsigned short data,
										int remoteModuleHandle );
int				__declspec(dllexport) b2drmOpenWriteWORDBufferClose (
										int deviceNum,
										int moduleNum,
										DWORD_PTR offset,
										const char * paramName,
										unsigned short *pData,
										unsigned int numWordsToWrite,
										int remoteModuleHandle );

void			__declspec(dllexport) b2drmStartDebug(int remoteModuleHandle);
void			__declspec(dllexport) b2drmStopDebug(int remoteModuleHandle);
void			__declspec(dllexport) b2drmStartLocalAccessToPx(int remoteModuleHandle);
int				__declspec(dllexport) b2drmIsRemoteAccessFlag(int remoteModuleHandle);
BOOL			__declspec(dllexport) b2drmIsDeviceDefined (unsigned short Device);
int				__declspec(dllexport) b2drmIsRemoteDeviceReachable (int deviceNumber);
BOOL			__declspec(dllexport) b2drmIsCmdClearEOMStatusSupported (int deviceNumber);
BOOL			__declspec(dllexport) b2drmIsUnetFifoRWSupported (int deviceNumber);


	// b2drmIsRemoteDevice & b2drmIsRemote/b2drmDeviceIsRemote are just wrappers for RegTools IsRemoteDevice(int deviceNumber)
	// However, b2drmIsRemoteDevice is different from b2drmIsRemote in that:
	// (1) b2drmIsRemote takes remoteModuleHandle variable rather than a device number
	// (2) b2drmIsRemote uses remoteModuleHandle to do some logging, while b2drmIsRemoteDevice does no logging.
	// Returns TRUE if the device is a UNET device

BOOL			__declspec(dllexport) b2drmIsRemoteDevice (unsigned short Device);
BOOL			__declspec(dllexport) b2drmIsNetDevice (unsigned short Device);
BOOL			__declspec(dllexport) b2drmIsUsbDevice (unsigned short Device);
int				__declspec(dllexport) b2drmGetDeviceType (unsigned short Device, char * pDeviceType);
int				__declspec(dllexport) b2drmGetConnectionType (unsigned short Device, ConnectionType * pUNetType);
int				__declspec(dllexport) b2drmReCreateReciprocalDevice (unsigned short originalDevice, unsigned short reciprocalDevice);


// Deprecated function: b2drmDeviceIsRemote
int				__declspec(dllexport) b2drmIsRemote(int remoteModuleHandle);
int				__declspec(dllexport) b2drmDeviceIsRemote(unsigned short Device, int remoteModuleHandle); // { return b2drmIsRemote(remoteModuleHandle); }

BOOL			__declspec(dllexport) b2drmIsUSB2xx(int remoteModuleHandle );
BOOL			__declspec(dllexport) b2drmIsUSB3xx(int remoteModuleHandle );
BOOL			__declspec(dllexport) b2drmIsUSB(int remoteModuleHandle );

// b2drmInitRemoteAccessToDevice is deprecated and should only be used internally;
// use b2drmInitRemoteDeviceModule instead (& keep track of the handle it generates).
int				__declspec(dllexport) b2drmInitRemoteAccessToDevice( unsigned short deviceNum, unsigned short moduleNum, int remoteModuleHandle);
int				__declspec(dllexport) b2drmInitRemoteDeviceModule( unsigned short deviceNum, unsigned short moduleNum, int *pRemoteModuleHandle );

#ifndef _WIN32
int				__declspec(dllexport) b2drmInitNonRemoteAccessToDevice(unsigned short deviceNum, unsigned short moduleNum, int moduleHandle);
#endif
void			__declspec(dllexport) b2drmReleaseRemoteAccessToDevice(unsigned short deviceNum, unsigned short moduleNum, int remoteModuleHandle);

int				__declspec(dllexport) b2drmWaitForModuleInterrupt(int remoteModuleHandle, unsigned int timeOutInMilliSeconds);
int				__declspec(dllexport) b2drmStopInterruptWait ( int remoteModuleHandle );
int				__declspec(dllexport) b2drmGetModuleInterruptCount(int remoteModuleHandle, unsigned int *pInterruptCount);

int				__declspec(dllexport) b2drmExc_Request_Interrupt_Notification(int nDevice, int nModule, HANDLE hEvent, int remoteModuleHandle);
int				__declspec(dllexport) b2drmExc_Cancel_Interrupt_Notification(int nDevice, int nModule, int remoteModuleHandle);

int				__declspec(dllexport) b2drmGetEventHandleForModule(int deviceNum, int moduleNum, HANDLE * phEvent);
#ifndef _WIN32
int __declspec(dllexport) b2drmWait_For_Multiple_Interrupts_Px(int numints, int *handle_array, unsigned int timeout, unsigned long *pInterrupt_Bitfield);
#endif


// Some functions to access UNets by device. Open/Release UNet Device opens/releases module 4, but we can use it to read from / write to any module
int				__declspec(dllexport) b2drmOpenUnetDevice ( unsigned short Device ) ;// {return b2drmInitRemoteAccessToDevice ( Device, MODULE_4, (NUMUNETCARDS + (int) Device) );}
void			__declspec(dllexport) b2drmReleaseUnetDevice ( unsigned short Device ) ;// {return b2drmReleaseRemoteAccessToDevice ( Device, MODULE_4, (NUMUNETCARDS + (int) Device) );}

int			__declspec(dllexport) b2drmReadWordBufferFromUnetDevice(
										unsigned short Device,
										unsigned short Module,
										DWORD_PTR offsetAddress,
										unsigned short * wordBufferToFill,
										unsigned int numWordsToRead);

int			__declspec(dllexport) b2drmWriteWordBufferToUnetDevice (
										unsigned short Device,
										unsigned short Module,
										DWORD_PTR offsetAddress,
										unsigned short * wordBufferToWrite,
										unsigned int numWordsToWrite);

int			__declspec(dllexport) b2drmSendMaccCommand  (
										unsigned short Device,
										int portNum,
										char * pQueryText,
										unsigned int timeoutMilliSec,
										char * pResponseText,
										int responseTextSize,
										char * pWinsockErrorText,
										int winsockErrorTextSize);

// This was for an old unsuccessful attempt at support for b2drmSendMaccCommand
int			__declspec(dllexport) b2drmOpenWRCharBufferClose_H2ND (
										unsigned short Device,
										int portNum,
										char * pCommandText,
										unsigned int timeoutMilliSec,
										char * pCommandResponse);


// BEWARE: Do not use the Read/Write BYTE Buffer functions for now.
// We have found suspicious problems when using them, but have not
// isolated exactly how and when!
// Use the Read/Write WORD Buffer functions instead!
void			b2drmReadByteBufferFromDpr( DWORD_PTR offsetAddress,
											unsigned char * byteBufferToFill,
											unsigned int numBytesToRead,
											const char * paramName,
											int remoteModuleHandle );
void			b2drmWriteByteBufferToDpr ( DWORD_PTR offsetAddress,
											unsigned char * byteBufferToWrite,
											unsigned int numBytesToWrite,
											const char * paramName,
											int remoteModuleHandle );

int				b2drmLog_AddLogEntry (char * pLogEntryString);

#ifdef __cplusplus
} //of extern "C"
#endif

//
//  Device Raw Memory Access Macros
//

// Read and return BYTE data from Device Raw Memory to Host
#define RBYTE_D2H( MemoryMapField )  b2drmReadByteFromDpr( (DWORD_PTR) &( MemoryMapField ) , #MemoryMapField, B2DRMHANDLE )

// Write BYTE data from Host to Device Raw Memory
#define WBYTE_H2D( MemoryMapField , byteData)  b2drmWriteByteToDpr( (DWORD_PTR) &( MemoryMapField ) , (unsigned char) (byteData) , #MemoryMapField, B2DRMHANDLE  )

// Read and return WORD data from Device Raw Memory to Host
#define RWORD_D2H( MemoryMapField )  b2drmReadWordFromDpr( (DWORD_PTR) &( MemoryMapField ) , #MemoryMapField, B2DRMHANDLE )

// Write WORD data from Host to Device Raw Memory
#define WWORD_H2D( MemoryMapField , wordData)  b2drmWriteWordToDpr( (DWORD_PTR) &( MemoryMapField ) , (unsigned short) (wordData) , #MemoryMapField, B2DRMHANDLE  )

// Read and return DWORD data from Device Raw Memory to Host
#define RDWORD_D2H( MemoryMapField )  b2drmReadDWordFromDpr( (DWORD_PTR) &( MemoryMapField ) , #MemoryMapField, B2DRMHANDLE )

// Write DWORD data from Host to Device Raw Memory
#define WDWORD_H2D( MemoryMapField , dwordData)  b2drmWriteDWordToDpr( (DWORD_PTR) &( MemoryMapField ) , (unsigned int) (dwordData) , #MemoryMapField, B2DRMHANDLE  )

// OpenUNet-ReadWORD-CloseUNet data from Device MODULE 4 (EXC4000 interface) to Host (returns an errorNumber)
#define ORWordC_M42H( Device, MemoryMapField, pWordData ) b2drmOpenReadWORDClose ( Device, MODULE_4, (DWORD_PTR) &( MemoryMapField ), #MemoryMapField, pWordData, (NUMUNETCARDS - 1) - Device )

// OpenUNet-WriteWORD-CloseUNet data from Device MODULE 4 (EXC4000 interface) to Host (returns an errorNumber)
#define OWWordC_H2M4( Device, MemoryMapField, wordData) b2drmOpenWriteWORDClose ( Device, MODULE_4, (DWORD_PTR) &( MemoryMapField ), #MemoryMapField, wordData, (NUMUNETCARDS - 1) - Device )

