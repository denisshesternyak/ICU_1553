#pragma once

#include "Host2Dpr.h"
#include "LibFt2232UsbPx.h"

#ifndef NUMCARDS
/* These flags are used internally for compilation of galahad dll */
#define NUMBOARDS 16  
#define NUMMODULES 16
#define NUMCARDS ((NUMBOARDS * NUMMODULES) + 1) // as defined in flags_px.h (with the "+ 1")
#endif

class CUsbFtd2Px : public CHost2Dpr {
	public:

		CUsbFtd2Px (
			unsigned short vendorID,
			unsigned short deviceID,
			unsigned char * pDeviceTypeString,
			unsigned char dataOrIntFlag = DATA_CHAN_FLAG // Channel 'A' for Data, Channel 'B' for Interrupts
		);
		CUsbFtd2Px (
			unsigned short vendorID,
			unsigned short deviceID,
			unsigned char * pSerialNum,
			unsigned char * pDeviceTypeString,
			unsigned char dataOrIntFlag = DATA_CHAN_FLAG // Channel 'A' for Data, Channel 'B' for Interrupts
		);
#ifdef _WIN32
		~CUsbFtd2Px() {mDisConnect(); mCloseDerived();}
#else //TODO: The Host2Dpr_DevChange file is not yet compiling
		~CUsbFtd2Px() {mCloseDerived();}
#endif


	protected:


	private:

		unsigned short mVendorID;
		unsigned short mProductID;
		unsigned char mSerialNum[SERIALNUM_SIZE];
		unsigned char mDeviceTypeString[DEVICETYPE_SIZE];
		unsigned char mDataOrIntFlag;
		EXC_USB_HANDLE mEUHandle;

		// More elaborate reads to allow shared Interupt construct to match the correct response to module
		unsigned char mReadIntMessage[NUMCARDS][MAX_MESSAGE_SIZE];
		bool mReadIntMessagePresentFlag[NUMCARDS];

		// The bulk of the work in the constructor happens here!
		void mConstructPart2();

		// Initialize the device and prepare it for communication.
		virtual void mOpen ();
		// Initialize instance variables in a single place to prevent the two constructors from getting out of sync.
		void mInitMemberVars ();

		// Close the device.
		virtual void mCloseDerived ();

		virtual void mWriteMessageToDpr(unsigned int numCharsToWrite);

		virtual void mReadMessageFromDpr(unsigned int numCharsToRead) {return mReadMessageFromDpr_Debug(numCharsToRead, NULL);}
		virtual void mReadMessageFromDpr_Debug(unsigned int numCharsToRead, t_DebugTiming * pDebugTimer);

		virtual void mWriteBufferToDevice (
			unsigned char * pBuffer,
			unsigned int numCharsToWrite,
			unsigned int * pNumCharsWritten
		);

		virtual bool mReadBufferFromDevice (
			unsigned char * pBuffer,
			unsigned int maxCharsToRead,
			unsigned int * pNumCharsRead
		);

		virtual void mSetRWTimeouts();
		virtual void mWait_For_Module_Interrupt(unsigned short moduleNum, unsigned int interruptEventReqOffset, unsigned int timeOutInMilliSeconds, int * pErrReturn);
		void mReadInterruptMessageFromDpr(unsigned int numCharsToRead, int * pErrReturn);
};

