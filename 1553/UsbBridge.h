#pragma once

#include "Host2Dpr.h"
#ifdef _WIN32
#include "LibUsbK3Bridge.h"
#endif


class CUsbBridge : public CHost2Dpr {
	public:

		CUsbBridge (
			unsigned short vendorID,
			unsigned short deviceID
		);
		CUsbBridge (
			unsigned short vendorID,
			unsigned short deviceID,
			unsigned char * pInstanceID
		);
		~CUsbBridge() {mDisConnect(); mCloseDerived();}


	protected:


	private:

		unsigned short mVendorID;
		unsigned short mProductID;
		unsigned char mInstanceID[KLST_STRING_MAX_LEN];
		EXC_USB_HANDLE mEUHandle;

		// Initialize the device and prepare it for communication.
		virtual void mOpen ();

		// Close the device.
		virtual void mCloseDerived ();

		virtual void mWriteMessageToDpr(unsigned int numCharsToWrite);

		virtual void mReadMessageFromDpr(unsigned int numCharsToRead);

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
};

