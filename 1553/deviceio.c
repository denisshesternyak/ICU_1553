// DeviceIO.C for Linux
// Routines for interacting with our kernel driver

#define  _LARGEFILE64_SOURCE //For lseek64 and off64_t
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "error_devio.h"
#include "deviceio.h"
#include "excsysio.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include "exc4000.h"

#ifndef DWORD_PTR  
#define DWORD_PTR  unsigned long int //We don't need any #ifdef for this, since unsigned long is 64 bits in 64-bit Linux
#endif
static int mPerformDMARead(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard, int repeatCode);
static int mPerformDMAWrite(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard, int repeatCode);
static int GetModuleType(WORD device_num, WORD module_num, WORD *modtype);

// Defines for this linux module

struct t_devinfo {
  void     *allocatedMem[MAX_BANKS];
  void     *physMem[MAX_BANKS];
  DWORD    offsetFromStartOfCard[MAX_BANKS];
  DWORD    dwPhysAdjustFactor[MAX_BANKS];
  unsigned long dwLength[MAX_BANKS];
  DWORD    krnlError;
  BOOL     fDeviceOpen;
  DWORD    dwRefCount;
  int      iHandle;
  DWORD    dwDPRLength;
  DWORD	   dwDPRType[MAX_BANKS];

};

static struct t_devinfo DevInfo[MAX_DEVICES];
static BOOL fInitialized = FALSE;

static void mInitialize();

int Get_Event_Handle(int dev, HANDLE *phEvent)
{
	return (0);
}
int Release_Event_Handle(int dev)
{
        return(0);
}

int Exc_Wait_For_Interrupt(int dev) {

  int iRet;

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  iRet = ioctl(DevInfo[dev].iHandle, EXC_IOCTL_WAIT_FOR_IRQ);
  if (iRet != 0) {
    return egeteventhand1;
  }

  return 0;
}


/*
	Exc_Get_Interrupt_Count()
	Exported Function

	Description:
		Returns total interrupt count from the time interrupt notification was
		enabled with Get_Event_Handle().

	Input:
		Pointer to buffer to receive total interrupts

	Output:
		Returns total interrupts in provided buffer
		Returns error message or 0 for success.
*/

int Exc_Get_Interrupt_Count(int dev, unsigned int *dwTotal) {

  int iRet;

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  iRet = ioctl(DevInfo[dev].iHandle, EXC_IOCTL_GET_INTERRUPT_COUNT, dwTotal);
  if (iRet != 0) {
    return egetintcount;
  }

  return(0);

}


int Exc_Get_Channel_Interrupt_Count(int dev, unsigned int iChannel, unsigned int *dwTotal) {

  int iRet;
  unsigned int ioctlBuffer;

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  ioctlBuffer = iChannel;
  iRet = ioctl(DevInfo[dev].iHandle, EXC_IOCTL_GET_MCH_INT_COUNT, &ioctlBuffer);
  if (iRet != 0) {
    return egetchintcount;
  }

  *dwTotal = ioctlBuffer;
  return(0);
}


int Exc_Get_Interrupt_Channels(int dev, BYTE *Shadow)
{  
  int iRet;

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  iRet = ioctl(DevInfo[dev].iHandle, EXC_IOCTL_GET_MCH_SHADOW, Shadow);
  if (iRet != 0) {
    return egetintchannels;
  }

  return(0);
}

/////////////////////////////////////////////////////////////////
// Get_IRQ_Number
//
// Makes a DeviceIOControl Call to the Kernel Mode driver
// returns the IRQ resource which has been allocated
// useful only for PCEP when ring3 must use Set_Int_Level routine to tell
// card which irq we are using
// if IRQ's are not being used, returns 0

int Get_IRQ_Number(int dev, int *iInterrupt)
{
  int iRet;
  unsigned int dwIrq;

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;
 
  iRet = ioctl(DevInfo[dev].iHandle, EXC_IOCTL_GET_IRQ_NUMBER, &dwIrq);
  if (iRet != 0) {
    return egetirq;
  }
  *iInterrupt = (int) dwIrq;

  return(0);
}


BOOL WriteIOByte(int dev, ULONG offset, BYTE value)
{
 
        return (0);
}

BOOL ReadIOByte(int dev, ULONG offset, BYTE *pValue)
{

        return (0);
}

int WriteAttributeMemory(int dev, ULONG offset, WORD value)
{
	return (0);
}

int ReadAttributeMemory(int dev, ULONG offset, WORD *pValue)
{
	return (0);
}


int MapMemory(int dev, void **pMemoryPointer, int iBank)
{

  void *pageStart;          // to contain start of virtual address to receive mapping
  void *physStartAdj;
  unsigned long physAddr;   // will contain physical address to be mapped
  int iRet;

  ExcBankInfo BankInfo;

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  // if we have a mapping already, just return that
  if (DevInfo[dev].physMem[iBank]) {
    // note that the physMem pointer is the actual page boundary beginning
    // we must add an offset for banks not on a page boundary
    *pMemoryPointer = DevInfo[dev].physMem[iBank] + DevInfo[dev].dwPhysAdjustFactor[iBank];
    return 0;
  }

  // First find the physical address of requested bank, via ioctl call to our driver
  BankInfo.dwBank = iBank;
  iRet = ioctl(DevInfo[dev].iHandle, EXC_IOCTL_GETBANKINFO, &BankInfo);
  if (iRet != 0) {
    return ekernelcantmap;
  }
  physAddr = BankInfo.dwAddress;

  // Allocate a virtual memory area
  // allocatedMem[iBank] receives pointer to allcocated virtual memory area
  // Add PAGE_SIZE-1 to account for the area before the page boundary
  // Add another PAGE_SIZE to account for the possible need to start the physical mapping on an earlier page boundary
  if (!(long)(DevInfo[dev].allocatedMem[iBank])) {
    DevInfo[dev].allocatedMem[iBank] = (void *)malloc(BankInfo.dwLength+(PAGE_SIZE-1)+PAGE_SIZE);
    if (!(DevInfo[dev].allocatedMem[iBank])) {
      return ekernelcantmap;
    }
  }

  // Figure out starting address for mapping (must be on a page boundary)
  // local param pageStart receives this value
  pageStart = DevInfo[dev].allocatedMem[iBank];
  if ((unsigned long) pageStart % (unsigned long) PAGE_SIZE) {
	unsigned long dwTemp = (unsigned long) pageStart;
	dwTemp += (PAGE_SIZE - ((unsigned long)pageStart % (unsigned long)PAGE_SIZE));
	pageStart = (void *) dwTemp;
  }

  // Figure out starting physical address for mapping (must be on a page boundary)
  DevInfo[dev].dwPhysAdjustFactor[iBank] = (physAddr % (unsigned long) PAGE_SIZE);
  physStartAdj =  (void*)(physAddr - DevInfo[dev].dwPhysAdjustFactor[iBank]);
  DevInfo[dev].dwLength[iBank] = BankInfo.dwLength + DevInfo[dev].dwPhysAdjustFactor[iBank];

  // Do the mapping
  // physMem[iBank] will contain final mapped address
  DevInfo[dev].physMem[iBank] = (unsigned short int *) mmap(pageStart, DevInfo[dev].dwLength[iBank], PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, DevInfo[dev].iHandle, (off_t)physStartAdj);

  if ((DevInfo[dev].physMem[iBank]) == MAP_FAILED) {

    // reset physMem so we don't mistake this as a real address later
    DevInfo[dev].physMem[iBank]=0;
    return ekernelcantmap;
  }

  // copy the final address into the output parameter, including adjustment
  *pMemoryPointer = (DevInfo[dev].physMem[iBank] + DevInfo[dev].dwPhysAdjustFactor[iBank]);

  //Now get the address of the start of the card and put the offset of this module in offsetFromStartOfCard
  BankInfo.dwBank = 0;
  iRet = ioctl(DevInfo[dev].iHandle, EXC_IOCTL_GETBANKINFO, &BankInfo);
  if (iRet != 0) {
    return ekernelcantmap;
  }
  DevInfo[dev].offsetFromStartOfCard[iBank] = physAddr - BankInfo.dwAddress ;

  return 0;
}

int UnMapMemory(int dev)
{
  // Unmap our allocations
  int i;

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  for (i=0; i<MAX_BANKS; i++) {
    if (DevInfo[dev].physMem[i]) {
      munmap((void *) DevInfo[dev].physMem[i], DevInfo[dev].dwLength[i]);
      DevInfo[dev].physMem[i]=0;
    }
  }
  return 0;
}

int GetRamSize(int dev, unsigned long *Size)
{

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  // return size of first bank
  *Size = DevInfo[dev].dwLength[0] - DevInfo[dev].dwPhysAdjustFactor[0];
  return 0;
}

int GetBankRamSize(int dev, unsigned long *pdwSize, int iBank) {

  int iRet;
  ExcBankInfo BankInfo;

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  // Call driver to get info for this bank           
  BankInfo.dwBank = iBank;
  iRet = ioctl(DevInfo[dev].iHandle, EXC_IOCTL_GETBANKINFO, &BankInfo);
  if (iRet != 0) {
    return ekernelcantmap;
  }

  *pdwSize = BankInfo.dwLength;
  return 0;
}

int OpenKernelDevice(int dev) {

  int i;
  char szFullDevName[100];
  unsigned long			dwRamSize;
  WORD			wModType;
  int mod;

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  // if already open, up device count and return 0
  if (DevInfo[dev].fDeviceOpen) {
    DevInfo[dev].dwRefCount++;
    return 0;
  }

  // Open the driver
  sprintf(szFullDevName, "/dev/excalbr%d", dev);
  DevInfo[dev].iHandle = open (szFullDevName, O_RDWR);
  if (DevInfo[dev].iHandle == -1) {
    return (eopenkernel);
  }
  DevInfo[dev].fDeviceOpen = TRUE;
  DevInfo[dev].dwRefCount++;

  // reset memory pointers
  for (i=0; i<MAX_BANKS; i++) {
    DevInfo[dev].physMem[i] = 0;
  }

  GetRamSize(dev, &dwRamSize);
  DevInfo[dev].dwDPRLength = dwRamSize;
  // Fill in module types - also used in DMA 
  for (mod = 0; mod < MAX_MODULES; mod++) {
	  GetModuleType((WORD)dev, (WORD)mod, &wModType);
	  DevInfo[dev].dwDPRType[mod] = wModType;
  }

  return 0;
}

int CloseKernelDevice(int dev) {

  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  // reduce ref count
  DevInfo[dev].dwRefCount--;

  if (DevInfo[dev].dwRefCount == 0) {
    // unmap any outstanding memory alloctions
    UnMapMemory(dev);
    
    // send the close
    close(DevInfo[dev].iHandle);
    DevInfo[dev].fDeviceOpen = FALSE;
  }
  
  return 0;
}

DWORD Exc_Get_Last_Kernel_Error(int dev)
{
  // Validations
  if ( !fInitialized )
    mInitialize();

  if ( (dev<0) || (dev >= MAX_DEVICES) )
    return ekernelbadparam;

  if (!DevInfo[dev].fDeviceOpen)
    return ekernelbadparam;

  return DevInfo[dev].krnlError;
}

int Exc_Request_Interrupt_Notification(int dev, int mod, HANDLE hEvent)
{
  // not implemented under linux - use WaitForInterrupt()
  return 0;
}

int Exc_Cancel_Interrupt_Notification(int dev, int mod)
{
  // not implemented under linux - use WaitForInterrupt()
  return 0;
}

int Exc_Wait_For_Interrupt_Timeout(int dev, unsigned int timeout) {

  // timeout not implemented under linux - just calls regular WaitForInterrupt() [with no timeout]
  return Exc_Wait_For_Interrupt(dev);
}

void mInitialize() {

  int dev;
  for (dev=0; dev<MAX_DEVICES; dev++) {
    DevInfo[dev].fDeviceOpen = FALSE;
    DevInfo[dev].dwRefCount = 0;
    DevInfo[dev].dwDPRLength = 0;

  }

  fInitialized = TRUE;
}


// Note: this method only supports multiple-module cards. For single
// module cards, use Release_Event_Handle()
int Release_Event_Handle_For_Module(int nDevice, int nModule)
{

 return(0);
 /*
	BOOL    bResult;
	ULONG   dwBytesRead;

if ( !fInitialized )
    mInitialize();




	// Validate input parameters
	if (nDevice < 0 || nDevice >= MAX_DEVICES) 
		return ekernelbadparam;
	if (nModule < 0 || nModule >= MAX_MODULES)
		return ekernelbadparam;

	// if device is not open, return error
	if (!DevInfo[dev].fDeviceOpen)
            return ekernelbadparam;
	
	if (mhEventForModule[nDevice][nModule]) {

		if (mdwOS == EXC_OS_WIN9X) {
		
			t_ExcDeviceIOPacket      pktDeviceIO;

			pktDeviceIO.nDevice = nDevice;
			pktDeviceIO.dwData = nModule;

			bResult = DeviceIoControl(mhDevice9X, EXC_IOCTL9X_CLEAR_NOTIFICATION_EVENT_MODULE,
				&pktDeviceIO, sizeof(pktDeviceIO), NULL, 0, &dwBytesRead, NULL);
		}
		
		else {

			bResult = DeviceIoControl(mhDeviceNT[nDevice], EXC_IOCTLNT_CLEAR_NOTIFICATION_EVENT_MODULE,
				&nModule, sizeof(&nModule), NULL, 0, &dwBytesRead, NULL);
		}

		if (!bResult) {
			return (ereleventhandle);
		}

		CloseHandle(mhEventForModule[nDevice][nModule]);
		mhEventForModule[nDevice][nModule] = NULL;
	}
	
	return(0);
 */
}

int Exc_Wait_For_Multiple_Interrupts(int numDevModulePairs, t_ExcDevModPair *DevModPairs, DWORD dwTimeout, DWORD *pdwInterruptBitfield)
{
	// Wait for an interrupt on any one of the specified modules
	// The location of each module is specified via a device/module pair
	// When returning, provide a bitfield of which of the specified modules have interrupted 
	// (note that more than one may have interrupted simultaenously)

	int				i;
	int nDevice = DevModPairs[0].nDevice;
	int iRet;
	ExcMultipleIRQMultipleDeviceRequest multirqreq;

	if ( !fInitialized )
		mInitialize();

	// Validate number of modules is within our defines
	if (numDevModulePairs <= 0 || numDevModulePairs > MAX_DEVICES * MAX_MODULES)
		return ekernelbadparam;

	// Validate that the bitfield will fit in the DWORD bitfield
	if (numDevModulePairs > sizeof(DWORD)*8 ) 
		return ekernelbadparam;

	for (i=0; i < numDevModulePairs; i++) {

		// Validate dev and mod parameters
		if (DevModPairs[i].nDevice < 0 || DevModPairs[i].nDevice >= MAX_DEVICES) 
			return ekernelbadparam;
		if ( (DevModPairs[i].nModule < 0 || DevModPairs[i].nModule >= MAX_MODULES) && (DevModPairs[i].nModule != SINGLE_MODULE_CARD) )
			return ekernelbadparam;

		// Validate that card is open
		if (!DevInfo[nDevice].fDeviceOpen)
			return ekerneldevicenotopen;

	}
	multirqreq.timeout = dwTimeout;
	multirqreq.modulesInterrupting = 0;
	multirqreq.numpairs = numDevModulePairs;
	for (i=0; i < sizeof(DWORD)*8; i++) {
		if(i < numDevModulePairs)
			multirqreq.DevModPairs[i] = DevModPairs[i];
		else{
			multirqreq.DevModPairs[i].nDevice = -1;
			multirqreq.DevModPairs[i].nModule = -1;
		}
	}

	iRet = ioctl(DevInfo[nDevice].iHandle, EXC_IOCTL_WAIT_FOR_MULT_IRQ_MULT_DEV, &multirqreq);
	if (multirqreq.timedOut) {
		*pdwInterruptBitfield = 0;
		return ekerneltimeout;
	}

	// Build bitfield to send back
	*pdwInterruptBitfield = multirqreq.modulesInterrupting;
	return (0);
}

int	  Exc_Initialize_Interrupt_For_Module(int nDevice, int nModule) {

	return (0);
}

int ChangeCurrentDevice(int i) {
	// obsolete function
	return 0;
}

int IsDMASupported(int nDevice) {

	// this facility is not yet enabled
	return TRUE;
}


int PerformDMARead(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard)
{
	return mPerformDMARead(nDevice, nModule, pBuffer, dwLengthInBytes, pAddressOnCard, EXC_DMA_REPEAT_CODE_NONE);
}
int PerformDMAWrite(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard)
{
	return mPerformDMAWrite(nDevice, nModule, pBuffer, dwLengthInBytes, pAddressOnCard, EXC_DMA_REPEAT_CODE_NONE);
}
int PerformRepetitiveDMARead(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard, int repeatCode)
{
	return mPerformDMARead(nDevice, nModule, pBuffer, dwLengthInBytes, pAddressOnCard, repeatCode);
}
int PerformRepetitiveDMAWrite(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard, int repeatCode)
{
	return mPerformDMAWrite(nDevice, nModule, pBuffer, dwLengthInBytes, pAddressOnCard, repeatCode);
}

static int mPerformDMARead(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard, int repeatCode) {

	ULONG   dwBytesRead;

	if (!fInitialized) {
		mInitialize();
	}

	// Validate input parameters
	if (nDevice < 0 || nDevice >= MAX_DEVICES) 
		return ekernelbadparam;
	if (nModule < 0 || nModule >= MAX_MODULES)
		return ekernelbadparam;

	if (repeatCode == EXC_DMA_REPEAT_CODE_BYTE)
		return einvaliddmaparam;
	else if (repeatCode == EXC_DMA_REPEAT_CODE_16BITS)
	{
		if ((dwLengthInBytes % 2) > 0)
			return einvaliddmaparam;
	}
	else if (repeatCode == EXC_DMA_REPEAT_CODE_32BITS)
	{
		if ((dwLengthInBytes % 4) > 0)
		return einvaliddmaparam;
	}
	else if ((dwLengthInBytes % 8) > 0)
		if((DevInfo[nDevice].dwDPRType[nModule] != EXC4000_MODTYPE_AFDX_RX) && (DevInfo[nDevice].dwDPRType[nModule] != EXC4000_MODTYPE_AFDX_TX))
			return einvaliddmaparam;

	// if device is not open, return error
	if (!DevInfo[nDevice].fDeviceOpen)
		return ekerneldevicenotopen;


	// figure out which address we want on the card
	DWORD_PTR dwModOffset = (DWORD_PTR) pAddressOnCard;
	DWORD dwleftoverbytes;
	off64_t dwAbsOffset = dwModOffset + DevInfo[nDevice].offsetFromStartOfCard[nModule];
	dwAbsOffset |= (off64_t)repeatCode << 32; //we pass the repeat code in the high DWORD of the offset

	/* DMA accesses must be in 64-bit units */
	switch (repeatCode)
	{
		case EXC_DMA_REPEAT_CODE_NONE:
			if (dwAbsOffset % 8)
			{
#if 0 //This was the original code for Linux
				unsigned long count = dwLengthInBytes % 8;
				unsigned char* pcBuffer = (unsigned char*)pBuffer;
				unsigned char* pcDPRAM = (unsigned char*)(DevInfo[nDevice].physMem[nModule]);
				pcDPRAM += dwModOffset;
				while(count > 0)
				{
					/* do regular reads */
					*pcBuffer++ = *pcDPRAM++;
					count--;
					dwLengthInBytes--;
					dwAbsOffset++;
				}
				pBuffer = (void*)pcBuffer;
#endif
				unsigned int dofullword = 0;
				DWORD bytesforRMAread = 8 - (dwAbsOffset % 8);
				unsigned int* pdwBuffer = (unsigned int*)pBuffer;
				unsigned char* pcDPRAM = (unsigned char*)(DevInfo[nDevice].physMem[nModule]);
				unsigned int* pdwDPRAM = (unsigned int*)pcDPRAM;
				DWORD localDword;
				char *pcBuffer;
				char *pclocal;
				DWORD i;
				pdwDPRAM += dwModOffset / sizeof (unsigned int);
				if (bytesforRMAread >= 4) { //We can do one 32-bit read without copying from any local DWORD
					dofullword = 1;
					bytesforRMAread -= 4;
				}
				//copy a dword from DPram 
				localDword = *pdwDPRAM;
				pcBuffer = (char *)pdwBuffer;
				pclocal = (char *)&localDword;
				for (i = 0; i < bytesforRMAread; i++)
				{
					*(pcBuffer+i) = *(pclocal+ i + (4 - bytesforRMAread)); //We have to read into the highest bytes in ascending order
				}
				pcBuffer += i; 
				if (dofullword)
				{
					if (bytesforRMAread > 0) { //If we originally needed exactly 4 bytes, we didn't do any reads yet, so we do it all with straight DWORD reads right into our buffer
						pdwDPRAM++;
					}
					pdwBuffer = (unsigned int*)pcBuffer;
					*pdwBuffer++ = *pdwDPRAM; 
					pcBuffer += 4; 
				}
				dwLengthInBytes -= 8 - (dwAbsOffset % 8);
				dwAbsOffset += 8 - (dwAbsOffset % 8);
				dwModOffset += 8 - (dwModOffset % 8);
				pBuffer = (void*)pcBuffer;
			}
			break;

		case EXC_DMA_REPEAT_CODE_BYTE:
			/* not implemented in hardware */
			return einvaliddmaparam;
			break;

		case EXC_DMA_REPEAT_CODE_16BITS:
			if (dwLengthInBytes % 8)
			{
				unsigned long count = (dwLengthInBytes % 8) / 2;
				WORD * pcBuffer = (WORD *)pBuffer;
				WORD* pcDPRAM = (WORD*)(DevInfo[nDevice].physMem[nModule]);
				pcDPRAM += dwModOffset;
				while(count > 0)
				{
					/* do regular reads */
					*pcBuffer++ = *pcDPRAM;
					count--;
					dwLengthInBytes -= 2;
				}
				pBuffer = (void*)pcBuffer;
			}
			break;

		case EXC_DMA_REPEAT_CODE_32BITS:
			if (dwLengthInBytes % 8)
			{
				unsigned long count = (dwLengthInBytes % 8) / 4;
				DWORD * pcBuffer = (DWORD *)pBuffer;
				unsigned int* pcDPRAM = (unsigned int*)(DevInfo[nDevice].physMem[nModule]);
				pcDPRAM += dwModOffset;
				while(count > 0)
				{
					/* do regular reads */
					*pcBuffer++ = *pcDPRAM;
					count--;
					dwLengthInBytes -= 4;
				}
				pBuffer = (void*)pcBuffer;
			}
			break;

		case EXC_DMA_REPEAT_CODE_64BITS:
			break;

		default:
			return einvaliddmaparam;
			break;
	}

	dwleftoverbytes = dwLengthInBytes % 8;
	dwLengthInBytes -= dwleftoverbytes; //do only an 8-byte aligned amount
	if(dwLengthInBytes){ //If we had less than 8 bytes to do, there is no need for the DMA (and it will fail with a size of 0)
		//	bResult = ReadFile(mhDeviceNT[nDevice], pBuffer, dwLengthInBytes, &dwBytesRead, &params);
		if(dwLengthInBytes < 16) //do regular reads since the DMA logic has problems with 8-byte DMAs
		{
			if((DevInfo[nDevice].dwDPRType[nModule] != EXC4000_MODTYPE_AFDX_RX) && (DevInfo[nDevice].dwDPRType[nModule] != EXC4000_MODTYPE_AFDX_TX))
			{
				unsigned int i;
				WORD* pwBuffer = (WORD*)pBuffer;
				WORD* pwDPRAM = (WORD*)(DevInfo[nDevice].physMem[nModule]);
				pwDPRAM += (dwModOffset) / sizeof(WORD);
				for(i = 0; i < dwLengthInBytes / sizeof(WORD); i++) { //this loop should always go 4 times
					if(repeatCode == EXC_DMA_REPEAT_CODE_NONE)
						*pwBuffer++ = *pwDPRAM++;
					else
						*pwBuffer++ = *pwDPRAM; //address of FIFO stays the same
				}
			}
			else //AFDX module prefers 32-bit accesses
			{
				unsigned int i;
				unsigned int* pdwBuffer = (unsigned int*)pBuffer;
				unsigned int* pdwDPRAM = (unsigned int*)(DevInfo[nDevice].physMem[nModule]);
				pdwDPRAM += (dwModOffset) / sizeof(unsigned int);
				for(i = 0; i < dwLengthInBytes / sizeof(unsigned int); i++) { //this loop should always go 2 times
					if(repeatCode == EXC_DMA_REPEAT_CODE_NONE)
						*pdwBuffer++ = *pdwDPRAM++;
					else
						*pdwBuffer++ = *pdwDPRAM; //address of FIFO stays the same
				}
			}
			dwBytesRead = dwLengthInBytes;
		}
		else
		{
			lseek64(DevInfo[nDevice].iHandle, dwAbsOffset, SEEK_SET);
			dwBytesRead = read(DevInfo[nDevice].iHandle, pBuffer, dwLengthInBytes);
		}
	}

	if (dwBytesRead != dwLengthInBytes) {
		return (edmareadfail);
	}

	if(dwleftoverbytes)
	{
		int i,byteinword;
		unsigned int dwtmp;
		unsigned char* pctmp;
		unsigned char* pcBuffer = (unsigned char*)pBuffer + dwLengthInBytes;
		unsigned char* pcDPRAM = (unsigned char*)(DevInfo[nDevice].physMem[nModule]);
		unsigned int* pdwDPRAM = (unsigned int*)pcDPRAM;
		pdwDPRAM += (dwModOffset + dwLengthInBytes) / sizeof(unsigned int);
		/* do two regular reads */
		for(i = 0; i < 2; i++) {
			dwtmp = *pdwDPRAM++;
			pctmp = (unsigned char*)&dwtmp;
			if (dwleftoverbytes > 4)
				byteinword = 4;
			else
				byteinword = dwleftoverbytes;
			dwleftoverbytes -= byteinword;
			while(byteinword > 0){
				*pcBuffer++ = *pctmp++;
				byteinword--;
			}
		}
	}
	return(0);
}
static int mPerformDMAWrite(int nDevice, int nModule, void *pBuffer, unsigned long dwLengthInBytes, void *pAddressOnCard, int repeatCode) {

	ULONG   dwBytesWritten;

	if (!fInitialized) {
		mInitialize();
	}

	// Validate input parameters
	if (nDevice < 0 || nDevice >= MAX_DEVICES) 
		return ekernelbadparam;
	if (nModule < 0 || nModule >= MAX_MODULES)
		return ekernelbadparam;

	if (repeatCode == EXC_DMA_REPEAT_CODE_BYTE)
		return einvaliddmaparam;
	else if (repeatCode == EXC_DMA_REPEAT_CODE_16BITS)
	{
		if ((dwLengthInBytes % 2) > 0)
			return einvaliddmaparam;
	}
	else if (repeatCode == EXC_DMA_REPEAT_CODE_32BITS)
	{
		if ((dwLengthInBytes % 4) > 0)
		return einvaliddmaparam;
	}
	else if ((dwLengthInBytes % 8) > 0)
		if((DevInfo[nDevice].dwDPRType[nModule] != EXC4000_MODTYPE_AFDX_RX) && (DevInfo[nDevice].dwDPRType[nModule] != EXC4000_MODTYPE_AFDX_TX))
			return einvaliddmaparam;

	// if device is not open, return error
	if (!DevInfo[nDevice].fDeviceOpen)
		return ekerneldevicenotopen;

	// figure out which address we want on the card
	DWORD_PTR dwModOffset = (DWORD_PTR) pAddressOnCard;
	DWORD dwleftoverbytes;
	off64_t dwAbsOffset = dwModOffset + DevInfo[nDevice].offsetFromStartOfCard[nModule];
	dwAbsOffset |= (off64_t)repeatCode << 32; //we pass the repeat code in the high DWORD of the offset

	/* DMA accesses must be in 64-bit units */
	switch (repeatCode)
	{
		case EXC_DMA_REPEAT_CODE_NONE:
			if (dwAbsOffset % 8)
			{
#if 0 //Original Linux code
				unsigned long count = dwLengthInBytes % 8;
				unsigned char* pcBuffer = (unsigned char*)pBuffer;
				unsigned char* pcDPRAM = (unsigned char*)(DevInfo[nDevice].physMem[nModule]);
				pcDPRAM += dwModOffset;
				while(count > 0)
				{
					/* do regular writes */
					*pcDPRAM = *pcBuffer++;
					count--;
					dwLengthInBytes--;
					dwAbsOffset++;
				}
				pBuffer = (void*)pcBuffer;
#endif
				unsigned int dofullword = 0;
				DWORD bytesforRMAwrite = 8 - (dwAbsOffset % 8);
				unsigned int* pdwBuffer = (unsigned int*)pBuffer;
				unsigned char* pcDPRAM = (unsigned char*)(DevInfo[nDevice].physMem[nModule]);
				unsigned int* pdwDPRAM = (unsigned int*)pcDPRAM;
				DWORD localDword;
				char *pcBuffer;
				char *pclocal;
				DWORD i;
				pdwDPRAM += dwModOffset / sizeof (unsigned int);
				if (bytesforRMAwrite >= 4) { //We can do one 32-bit write without copying into any local DWORD
					dofullword = 1;
					bytesforRMAwrite -= 4;
				}
				//copy a dword from DPram in case we need change only  a part of it
				localDword = *pdwDPRAM;
				pcBuffer = (char *)pdwBuffer;
				pclocal = (char *)&localDword;
				for (i = 0; i < bytesforRMAwrite; i++)
				{
					*(pclocal+ i + (4 - bytesforRMAwrite)) = *(pcBuffer+i); //We have to write into the highest bytes in ascending order
				}
				pcBuffer += i; 
				*pdwDPRAM = localDword;
				if (dofullword)
				{
					if (bytesforRMAwrite > 0) { //If we originally needed exactly 4 bytes, we didn't do any writes yet, so we do it all with straight DWORD writes right into our buffer
						pdwDPRAM++;
					}
					pdwBuffer = (unsigned int*)pcBuffer;
					*pdwDPRAM = *pdwBuffer; 
					pcBuffer += 4; 
				}
				dwLengthInBytes -= 8 - (dwAbsOffset % 8);
				dwAbsOffset += 8 - (dwAbsOffset % 8);
				dwModOffset += 8 - (dwModOffset % 8);
				pBuffer = (void*)pcBuffer;
			}
			break;

		case EXC_DMA_REPEAT_CODE_BYTE:
			/* not implemented in hardware */
			return einvaliddmaparam;
			break;

		case EXC_DMA_REPEAT_CODE_16BITS:
			if (dwLengthInBytes % 8)
			{
				unsigned long count = (dwLengthInBytes % 8) / 2;
				WORD * pcBuffer = (WORD *)pBuffer;
				WORD* pcDPRAM = (WORD*)(DevInfo[nDevice].physMem[nModule]);
				pcDPRAM += dwModOffset;
				while(count > 0)
				{
					/* do regular writes */
					*pcDPRAM = *pcBuffer++;
					count--;
					dwLengthInBytes -= 2;
				}
				pBuffer = (void*)pcBuffer;
			}
			break;

		case EXC_DMA_REPEAT_CODE_32BITS:
			if (dwLengthInBytes % 8)
			{
				unsigned long count = (dwLengthInBytes % 8) / 4;
				DWORD * pcBuffer = (DWORD *)pBuffer;
				unsigned int* pcDPRAM = (unsigned int*)(DevInfo[nDevice].physMem[nModule]);
				pcDPRAM += dwModOffset;
				while(count > 0)
				{
					/* do regular writes */
					*pcDPRAM = *pcBuffer++;
					count--;
					dwLengthInBytes -= 4;
				}
				pBuffer = (void*)pcBuffer;
			}
			break;

		case EXC_DMA_REPEAT_CODE_64BITS:
			break;

		default:
			return einvaliddmaparam;
			break;
	}

	dwleftoverbytes = dwLengthInBytes % 8;
	dwLengthInBytes -= dwleftoverbytes; //do only an 8-byte aligned amount
	if(dwLengthInBytes){ //If we had less than 8 bytes to do, there is no need for the DMA (and it will fail with a size of 0)
		//	bResult = ReadFile(mhDeviceNT[nDevice], pBuffer, dwLengthInBytes, &dwBytesWritten, &params);
		if(dwLengthInBytes < 16) //do regular writes since the DMA logic has problems with 8-byte DMAs
		{
			if((DevInfo[nDevice].dwDPRType[nModule] != EXC4000_MODTYPE_AFDX_RX) && (DevInfo[nDevice].dwDPRType[nModule] != EXC4000_MODTYPE_AFDX_TX))
			{
				unsigned int i;
				WORD* pwBuffer = (WORD*)pBuffer;
				WORD* pwDPRAM = (WORD*)(DevInfo[nDevice].physMem[nModule]);
				pwDPRAM += (dwModOffset) / sizeof(WORD);
				for(i = 0; i < dwLengthInBytes / sizeof(WORD); i++) { //this loop should always go 4 times
					if(repeatCode == EXC_DMA_REPEAT_CODE_NONE)
						*pwDPRAM++ = *pwBuffer++;
					else
						*pwDPRAM = *pwBuffer++; //address of FIFO stays the same
				}
			}
			else //AFDX module prefers 32-bit accesses
			{
				unsigned int i;
				unsigned int* pdwBuffer = (unsigned int*)pBuffer;
				unsigned int* pdwDPRAM = (unsigned int*)(DevInfo[nDevice].physMem[nModule]);
				pdwDPRAM += (dwModOffset) / sizeof(unsigned int);
				for(i = 0; i < dwLengthInBytes / sizeof(unsigned int); i++) { //this loop should always go 2 times
					if(repeatCode == EXC_DMA_REPEAT_CODE_NONE)
						*pdwDPRAM++ = *pdwBuffer++; 
					else
						*pdwDPRAM = *pdwBuffer++; //address of FIFO stays the same
				}
			}
			dwBytesWritten = dwLengthInBytes;
		}
		else
		{
			lseek64(DevInfo[nDevice].iHandle, dwAbsOffset, SEEK_SET);
			dwBytesWritten = write(DevInfo[nDevice].iHandle, pBuffer, dwLengthInBytes);
		}
	}

	if (dwBytesWritten != dwLengthInBytes) {
		return (edmareadfail);
	}

	if(dwleftoverbytes)
	{
		int i,byteinword;
		unsigned int dwtmp;
		unsigned char* pctmp;
		unsigned char* pcBuffer = (unsigned char*)pBuffer + dwLengthInBytes;
		unsigned char* pcDPRAM = (unsigned char*)(DevInfo[nDevice].physMem[nModule]);
		unsigned int* pdwDPRAM = (unsigned int*)pcDPRAM;
		pdwDPRAM += (dwModOffset + dwLengthInBytes) / sizeof(unsigned int);
		/* do two regular reads */
		for(i = 0; i < 2; i++)
		{
			dwtmp = *pdwDPRAM;
			pctmp = (unsigned char*)&dwtmp;
			if (dwleftoverbytes > 4)
			{
				dwleftoverbytes -= 4;
				byteinword = 4;
			}
			else
				byteinword = dwleftoverbytes;
			//Fix bytes in local DWORD
			while(byteinword > 0){
				*pctmp++ = *pcBuffer++;
				byteinword--;
			}
			//write the DWORD back
			*pdwDPRAM++ = dwtmp;
		}
	}
	return(0);
}

int Exc_Wait_For_Module_Interrupt(int nDevice, int nModule, DWORD dwTimeout)
{

  int iRet;
  ExcIRQRequest irqreq;

  // Validations
  if ( !fInitialized )
    mInitialize();

  // Validate input parameters
  if (nDevice < 0 || nDevice >= MAX_DEVICES) 
	  return ekernelbadparam;
  if ((nModule < 0 || nModule >= MAX_MODULES) && (nModule != SINGLE_MODULE_CARD) )
	  return ekernelbadparam;

  // if a single module card is specified, pass the call over to the proper
  // function (which will do its own validation)
  if (nModule == SINGLE_MODULE_CARD) {
	  return Exc_Wait_For_Interrupt_Timeout(nDevice, dwTimeout);
  }

  if (!DevInfo[nDevice].fDeviceOpen)
    return ekernelbadparam;

  irqreq.nModule = nModule;
  irqreq.timeout = dwTimeout;
  iRet = ioctl(DevInfo[nDevice].iHandle, EXC_IOCTL_WAIT_FOR_MOD_IRQ, &irqreq);
  if (iRet != 0) {
    return ekernelwaitinterrupt;
  }
  if (irqreq.timedOut) {
    return ekerneltimeout;
  }

  return 0;
}


static int GetModuleType(WORD device_num, WORD module_num, WORD *modtype)
{
	int iError;
	WORD wBoardType;
	t_globalRegs4000 *globreg;

	if (module_num >= MAX_MODULES_ON_BOARD)
		return emodnum;

	// if device is not open, return error
	if (!DevInfo[device_num].fDeviceOpen)
		return ekerneldevicenotopen;

        iError = MapMemory(device_num, (void **)&(globreg), GLOBALREG_BANK);
        if (iError)
        {
                return iError; /* error from mapmemory */
        }
        
	/* verify that this is indeed a 4000PCI or 4000PCIe (express card) or AFDX */
        wBoardType = (WORD)(globreg->boardSigId & EXC4000PCI_BOARDSIGMASK);

	if ((wBoardType != EXC4000PCI_BOARDSIGVALUE) && (wBoardType != EXC4000PCIE_BOARDSIGVALUE) && (wBoardType != EXCAFDXPCI_BOARDSIGVALUE)) 
        {
                return ekernelnot4000card;
        }

        if (module_num < 4) {
                *modtype = (WORD)(globreg->moduleInfo[module_num] & EXC4000PCI_MODULETYPEMASK);
        }
        else {
                *modtype = (WORD)(globreg->moduleInfoSecondGroup[module_num-4] & EXC4000PCI_MODULETYPEMASK);
        }

        UnMapMemory(device_num);
        return 0;
}

