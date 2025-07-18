
// DeviceIO.C
// Routines for communicating with our kernel driver

#ifndef __DEVICEIO_H_
#define __DEVICEIO_H_

#include <unistd.h>
#include "WinTypes.h"

// Prototypes
// Exported

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    int nDevice;
    int nModule;
} t_ExcDevModPair;

int Exc_Get_Interrupt_Count(int, unsigned int*);
int Exc_Get_Interrupt_Channels(int, BYTE *);
int Exc_Get_Channel_Interrupt_Count(int, unsigned int, unsigned int*);
int Exc_Wait_For_Interrupt(int);
int Exc_Wait_For_Interrupt_Timeout(int dev, unsigned int timeout);
int   Exc_Wait_For_Multiple_Interrupts(int numDevModulePairs, t_ExcDevModPair *DevModPairs, DWORD dwTimeout, DWORD *pdwInterruptBitfield);
int Exc_Wait_For_Module_Interrupt(int nDevice, int nModule, DWORD dwTimeout);
int	  Exc_Initialize_Interrupt_For_Module(int nDevice, int nModule);
DWORD Exc_Get_Last_Kernel_Error(int);


// Not Exported
int Get_Event_Handle(int, HANDLE *);
int ChangeCurrentDevice(int nNewDevice);
int OpenKernelDevice(int device_number);
int CloseKernelDevice(int);
int MapMemory(int, void **pMemory, int iBank);
int UnMapMemory(int);
int Release_Event_Handle(int); 
int Release_Event_Handle_For_Module(int nDevice, int nModule);
BOOL WriteIOByte(int, ULONG, BYTE);
BOOL ReadIOByte(int, ULONG, BYTE *);
int GetRamSize(int, unsigned long *lRamSize);
int Get_IRQ_Number(int, INT *pnIRQ);
int WriteAttributeMemory(int nCurrentDevice, ULONG offset, WORD value);
int ReadAttributeMemory(int nCurrentDevice, ULONG offset, WORD *pValue);
int Exc_Request_Interrupt_Notification(int nCurrentDevice, int mod, HANDLE hEvent);
int Exc_Cancel_Interrupt_Notification(int nCurrentDevice, int mod);
int IsDMASupported(int nDevice);
int PerformDMARead(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard);
int PerformDMAWrite(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard);
int PerformRepetitiveDMARead(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard, int repeatCode);
int PerformRepetitiveDMAWrite(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard, int repeatCode);
int GetBankRamSize(int dev, unsigned long *pdwSize, int iBank);

#ifdef __cplusplus
}
#endif
// compatibility defines
#define SINGLE_MODULE_CARD   0xFF

#define EXC_DMA_REPEAT_CODE_NONE 0
#define EXC_DMA_REPEAT_CODE_BYTE 1
#define EXC_DMA_REPEAT_CODE_16BITS 2
#define EXC_DMA_REPEAT_CODE_32BITS 4
#define EXC_DMA_REPEAT_CODE_64BITS 8

#endif





