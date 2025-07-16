// DeviceIO_Px.C for PX Module windows95/nt
// Routines for interacting with excalibur device driver for PX Module

#include "pxIncl.h"
extern INSTANCE_PX cardpx[];
#include "deviceio.h"
#include "excsysio.h"

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)
#include <sys/vlimit.h>
#define INFINITE INFINITY 

/* CR1 */
#ifdef __linux__
int borland_dll Wait_For_Interrupt_Device_Px(int nCurrentDevice)
{
	return Exc_Wait_For_Interrupt(nCurrentDevice);
}
#endif // __linux__

int borland_dll Wait_For_Interrupt_15EP(int handlepx)
{
	return Wait_For_Interrupt_Px(g_handlepx, INFINITE);
}

int borland_dll Wait_For_Interrupt_Px(int handlepx, unsigned int timeout)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].card_addr == SIMULATE)
		return(0);
	else
	{
		// Wait for a classic Exc4000 based module, or a remote-device (ie UNET1553) module

		// .. the classic module
		if (b2drmIsRemoteAccessFlag(B2DRMHANDLE) == FALSE)
			return Exc_Wait_For_Module_Interrupt(cardpx[handlepx].card_addr, cardpx[handlepx].module, timeout);

		// .. the remote-device (ie UNET1553) module
		else
			return b2drmWaitForModuleInterrupt(B2DRMHANDLE, timeout); // timeout is in milliseconds
	}
}

int borland_dll Get_Interrupt_Count_Px(int handlepx, unsigned int *Sys_Interrupts_Ptr)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].card_addr == SIMULATE)
		return(0);
	else

	{
		// Get Interrupt count from a classic Exc4000 based module, or a remote-device (ie UNET1553) module

		// .. the classic module
		if (b2drmIsRemoteAccessFlag(B2DRMHANDLE) == FALSE)
			return Exc_Get_Channel_Interrupt_Count(cardpx[handlepx].card_addr, cardpx[handlepx].module, Sys_Interrupts_Ptr);

		// .. the remote-device (ie UNET1553) module
		else
			return b2drmGetModuleInterruptCount(B2DRMHANDLE, Sys_Interrupts_Ptr);
	}

}

#ifdef __linux__
int borland_dll Get_Interrupt_Count_Device_Px(int nCurrentDevice, unsigned int *Sys_Interrupts_Ptr)
{
	return Exc_Get_Interrupt_Count(nCurrentDevice, Sys_Interrupts_Ptr);
}
#endif // __linux__

/* CR1 */

#ifdef _WIN32
/*
	// Replaced this to support UNet without making the appropriate changes to deviceio.c - hence
	// we don't affect ALL of Software Tools (by making them all UNet-aware) yet.

int borland_dll Wait_For_Multiple_Interrupts_Px(int numints, int *handle_array, unsigned int timeout, unsigned long *Interrupt_Bitfield)
{
	int i, intindex;
	t_ExcDevModPair devmodpair[NUMCARDS];

	for (i=0; i<numints; i++)
	{
		intindex = handle_array[i];
		if ((intindex) <0 || (intindex >= NUMCARDS)) return ebadhandle;
		if (cardpx[intindex].allocated != 1) return ebadhandle;
		if (cardpx[intindex].card_addr == SIMULATE)
			return(0);
		devmodpair[i].nDevice = cardpx[intindex].card_addr;
		devmodpair[i].nModule = cardpx[intindex].module;
	}
	return Exc_Wait_For_Multiple_Interrupts(numints, devmodpair, timeout, Interrupt_Bitfield);
}
*/

int borland_dll Wait_For_Multiple_Interrupts_Px(int numints, int *handle_array, unsigned int timeout, unsigned long *pInterrupt_Bitfield)
{
	// Wait for an interrupt on any one of the specified modules
	// The location of each module is specified via a device/module pair
	// When returning, provide a bitfield of which of the specified modules have interrupted 
	// (note that more than one may have interrupted simultaenously)

	HANDLE			hEvents[MAX_DEVICES * MAX_MODULES];
	DWORD			dwBitfield;
	int				i;
	int				intindex;
	int				status;
	DWORD			dwReturn;

	// Validate number of modules is within our defines
	if (numints <= 0 || numints > MAX_DEVICES * MAX_MODULES)
		return ekernelbadparam;

	// Validate that the bitfield will fit in the DWORD bitfield
	if (numints > sizeof(DWORD)*8 ) 
		return ekernelbadparam;

	// Validate output buffer
	if (IsBadWritePtr(pInterrupt_Bitfield, sizeof(DWORD)))
		return ekernelbadpointer;

	for (i=0; i < numints; i++) {

		intindex = handle_array[i];

		// Validate dev and mod parameters
		if (cardpx[intindex].card_addr < 0 || cardpx[intindex].card_addr >= MAX_DEVICES) 
			return ekernelbadparam;
		if ( (cardpx[intindex].module < 0 || cardpx[intindex].module >= MAX_MODULES) && (cardpx[intindex].module != SINGLE_MODULE_CARD) )
			return ekernelbadparam;

		// Handle non-UNet devices first
		if (!(b2drmIsRemoteDevice(cardpx[intindex].card_addr)))
		{
			intindex = handle_array[i];
			if ((intindex) <0 || (intindex >= NUMCARDS)) return ebadhandle;
			if (cardpx[intindex].allocated != 1) return ebadhandle;
			if (cardpx[intindex].card_addr == SIMULATE)
				return(0);

			// Get Semaphore
			if (cardpx[intindex].module == SINGLE_MODULE_CARD)
			{
				status = DevioGetEventHandle(cardpx[intindex].card_addr, &hEvents[i]);
			}
			else
			{
				status = DevioGetEventHandleForModule(cardpx[intindex].card_addr, cardpx[intindex].module, &hEvents[i]);
			}

			if (status < 0) {
				return status;
			}
		}

		else // Device is aUNet
		{
			status = b2drmGetEventHandleForModule(cardpx[intindex].card_addr, cardpx[intindex].module, &hEvents[i]);
			if (status == eboardnotalloc)
				return ekerneldevicenotopen;
		}

	} // end loop over all devices in the wait-for list

	dwReturn = WaitForMultipleObjects(numints, hEvents, FALSE, timeout);
	if (dwReturn == WAIT_TIMEOUT) {
		*pInterrupt_Bitfield = 0;
		return ekerneltimeout;
	}

	// Build bitfield to send back
	// Note that multiple objects may signal at the same time during a WaitForMultipleObjects call
	dwBitfield = 0;
	for (i=0; i < numints; i++) {

		DWORD dwWait;

		// Checking the state of a synch object in the WinAPI
		// is performed via a call to WaitForSingleObj with a timeout value of 0.
		dwWait = WaitForSingleObject(hEvents[i], 0);

		if (dwWait == WAIT_OBJECT_0) {

			// object is signalled 
			dwBitfield |= (1<<i);

			// reset object
			ResetEvent(hEvents[i]);
		}
	}

	*pInterrupt_Bitfield = dwBitfield;
	return (0);
}

int borland_dll  InitializeInterrupt_Px(int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1 ) return ebadhandle;

	if (cardpx[handlepx].card_addr == SIMULATE)
		return(0);

	if (cardpx[handlepx].isUNet == TRUE)
		return(0);

	return Exc_Initialize_Interrupt_For_Module(cardpx[handlepx].card_addr, cardpx[handlepx].module);
}

int borland_dll Request_Interrupt_Notification_Px (int handlepx, HANDLE hEvent)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if (cardpx[handlepx].card_addr == SIMULATE)
		return(0);

	else if (b2drmIsRemoteDevice((int) cardpx[handlepx].card_addr) == TRUE)
		return b2drmExc_Request_Interrupt_Notification(cardpx[handlepx].card_addr, 
				cardpx[handlepx].module, hEvent, B2DRMHANDLE);

	else
		return Exc_Request_Interrupt_Notification(cardpx[handlepx].card_addr, 
				cardpx[handlepx].module, hEvent);
}

int borland_dll Cancel_Interrupt_Notification_Px(int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if (cardpx[handlepx].card_addr == SIMULATE)
		return(0);

	else if (b2drmIsRemoteDevice((int) cardpx[handlepx].card_addr) == TRUE)
		return b2drmExc_Cancel_Interrupt_Notification(cardpx[handlepx].card_addr, 
				cardpx[handlepx].module, B2DRMHANDLE);

	else
		return Exc_Cancel_Interrupt_Notification(cardpx[handlepx].card_addr, 
			cardpx[handlepx].module);
}
#else
int borland_dll Wait_For_Multiple_Interrupts_Px(int numints, int *handle_array, unsigned int timeout, unsigned long *pInterrupt_Bitfield)
{
	return b2drmWait_For_Multiple_Interrupts_Px(numints, handle_array, timeout, pInterrupt_Bitfield);
}
#endif
