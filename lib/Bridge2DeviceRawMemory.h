#ifndef BRIDGE_TO_DEVICE_RAW_MEMORY_H
#define BRIDGE_TO_DEVICE_RAW_MEMORY_H
//
// Bridge2DeviceRawMemory.h : Defines a bridge Interface between the SW tools and the target raw memory of the device
//							  that enables a raw reading and writing of bytes (8 bit) and words (16 bits) to and
//							  from the raw memeory of the device
//


//
// Public Interface Prototype Functions
//
#ifdef __cplusplus
extern "C" {
#endif

void			b2drmSetBaseAddress( unsigned int address, int remoteModuleHandle );
unsigned char	b2drmReadByteFromDpr( unsigned int offsetAddress, char * paramName, int remoteModuleHandle  );
unsigned short	b2drmReadWordFromDpr( unsigned int offsetAddress, char * paramName, int remoteModuleHandle  );
void			b2drmWriteByteToDpr ( unsigned int	offsetAddress,
									  unsigned char dataByte,
									  char * paramName,
									  int remoteModuleHandle );
void			b2drmWriteWordToDpr ( unsigned int	offsetAddress,
									  unsigned short dataWord,
									  char * paramName,
									  int remoteModuleHandle );
void			b2drmReadWordBufferFromDpr( unsigned int offsetAddress,
											unsigned short * wordBufferToFill,
											unsigned int numWordsToRead,
											char * paramName,
											int remoteModuleHandle );
void			b2drmWriteWordBufferToDpr ( unsigned int offsetAddress,
											unsigned short * wordBufferToWrite,
											unsigned int numWordsToWrite,
											char * paramName,
											int remoteModuleHandle );
void			b2drmStartDebug(int remoteModuleHandle);
void			b2drmStopDebug(int remoteModuleHandle);
void			b2drmStartLocalAccessToPx(int remoteModuleHandle);

int				b2drmIsRemoteAccessFlag(int remoteModuleHandle);

// b2drmIsRemoteDevice() is just a wrapper for RegTools's IsRemoteDevice().
// b2drmDeviceIsRemote() does more.
BOOL			b2drmIsRemoteDevice (int deviceNumber);
int				b2drmIsRemoteDeviceReachable (int deviceNumber);

int				b2drmDeviceIsRemote(unsigned short deviceNum, int remoteModuleHandle);

/* Not sure if we should keep these
// b2drmGetNetInfo() is just a wrapper for RegTools's GetNetInfo().
int				b2drmGetNetInfo (int deviceNumber, char * pMacAddress, char * pIpAddress, unsigned short * pUdpPort);

// b2drmGetUsbInfo() is just a wrapper for RegTools's GetUsbInfo().
int				b2drmGetUsbInfo (int deviceNumber, char * pVendorId, char * pProductId, char * pSerialNum);
*/

int				b2drmInitRemoteAccessToDevice(unsigned short deviceNum, unsigned short moduleNum, int remoteModuleHandle);
int				b2drmInitNonRemoteAccessToDevice(unsigned short deviceNum, unsigned short moduleNum, int moduleHandle);
void			b2drmReleaseRemoteAccessToDevice(unsigned short deviceNum, unsigned short moduleNum, int remoteModuleHandle);

int				b2drmWaitForModuleInterrupt(int remoteModuleHandle, unsigned int timeOutInMilliSeconds);
int				b2drmGetModuleInterruptCount(int remoteModuleHandle, unsigned int *pInterruptCount);

// BEWARE: Do not use the Read/Write BYTE Buffer functions for now.
// We have found suspicious problems when using them, but have not
// isolated exactly how and when!
// Use the Read/Write WORD Buffer functions instead!
void			b2drmReadByteBufferFromDpr( unsigned int offsetAddress,
											unsigned char * byteBufferToFill,
											unsigned int numBytesToRead,
											char * paramName,
											int remoteModuleHandle );
void			b2drmWriteByteBufferToDpr ( unsigned int offsetAddress,
											unsigned char * byteBufferToWrite,
											unsigned int numBytesToWrite,
											char * paramName,
											int remoteModuleHandle );


#ifdef __cplusplus
} //of extern "C"
#endif


//
//  Device Raw Memory Access Macros
//

// Read and return BYTE data from Device Raw Memory to Host
#define RBYTE_D2H( MemoryMapField )  b2drmReadByteFromDpr( (unsigned int) &( MemoryMapField ) , #MemoryMapField, handlepx )

// Write BYTE data form Host to Device Raw Memory
#define WBYTE_H2D( MemoryMapField , byteData)  b2drmWriteByteToDpr( (unsigned int) &( MemoryMapField ) , (unsigned char) (byteData) , #MemoryMapField, handlepx  )

// Read and return WORD data from Device Raw Memory to Host
#define RWORD_D2H( MemoryMapField )  b2drmReadWordFromDpr( (unsigned int) &( MemoryMapField ) , #MemoryMapField, handlepx )

// Write WORD data form Host to Device Raw Memory
#define WWORD_H2D( MemoryMapField , wordData)  b2drmWriteWordToDpr( (unsigned int) &( MemoryMapField ) , (unsigned short) (wordData) , #MemoryMapField, handlepx  )

#endif // of BRIDGE_TO_DEVICE_RAW_MEMORY_H
