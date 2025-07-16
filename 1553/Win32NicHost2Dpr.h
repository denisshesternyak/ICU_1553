#pragma once

#include "Host2Dpr.h"
// WinSock2.h MUST be included before WinSock.h !!!!
#include <winsock.h>
#include "WinGetLastEMsg.h"
#include "UNetTiming.h" // renamed from "Profiler.h"

#define BUF_SIZE_WIN32NIC 1471 // + 42 i.e. MAC + IP + UDP headers = 42

// Select Single socket for both Tx and Rx, or select seperate Tx and Rx Sockets
// If SOCKET_CONFIGURATION is not set to SINGLE_SOCKET, the code will behave as if set to SEPARATE_SOCKETS
#define SINGLE_SOCKET		1
#define SEPARATE_SOCKETS	2 // this will be default
#define SOCKET_CONFIGURATION SINGLE_SOCKET
#if SOCKET_CONFIGURATION != SINGLE_SOCKET
#define SOCKET_CONFIGURATION SEPARATE_SOCKETS
#endif

#define BOARD_ID_REGISTER_ADDR 0x3ffe
#define BOARD_ID_REGISTER_VALUE 0x45 // the Excalibur 'E'


void IP_UI2Str(unsigned int ipAddrNum, int maxIpStrLen, char * pIv4String);
void IP_Str2UI(char * pIv4String, unsigned int * pIpAddrNum);

class CWin32NicHost2Dpr : public CHost2Dpr {
	public:

		// This constructor will get the host's IP address and will respond to any IP address
		// that talks to it; it's GREAT for Space.
		CWin32NicHost2Dpr ();

		// This constructor defaults to get and use the host's IP address,
		// but if there are multiple NICs you'll have no control over which IP address we get.
		CWin32NicHost2Dpr (
			unsigned int destIpaddr,
			unsigned short destUdpport,
			unsigned int myIpaddr,
			unsigned short myUdpport
		);

		// This constructor defaults to get and use the host's IP address,
		// but if there are multiple NICs you'll have no control over which IP address we get.
		CWin32NicHost2Dpr (
			char * pDestIpaddrStr,
			unsigned short destUdpport,
			char * pMyIpaddrStr = "0.0.0.0",
			unsigned short myUdpport = 0
		);

		~CWin32NicHost2Dpr() {mCloseDerived();}
		// ~CWin32NicHost2Dpr() {mDisConnect(); mCloseDerived();}

		// Include this to let user know what port was bound to the object-instance
		// if they did not specify 'myUdpport'
		unsigned short GetMyUdpRcvPort() {return mMyUdpPort;}


	protected:


	private:

		// Storage for my own addresses
		unsigned int mMyIpAddr;
		unsigned short int mMyUdpPort;

		// Storage for my destination addresses
		unsigned int mDestIpAddr;
		unsigned short int mDestUdpPort;

		// WinSock variables

#if SOCKET_CONFIGURATION == SINGLE_SOCKET
		SOCKET mSocketDescriptorSaR;			// Socket descriptor for both send and receive 		for SINGLE_SOCKET
#endif

#if SOCKET_CONFIGURATION == SEPARATE_SOCKETS
		SOCKET mSocketDescriptorRcv;			// Socket descriptor for incoming packets			for SEPARATE_SOCKETS
		SOCKET mSocketDescriptorSend;			// Socket descriptor for outgoing packets			for SEPARATE_SOCKETS
#endif

		SOCKET mSocketDescriptorMarker;			// Socket descriptor for marker packets that help us sync with wireshark
		struct sockaddr_in mMarkerInfo;			// Information about the Marker fictitious destination
		int mMarkerInfo_Size;						// Size of the struct that contains the marker info
		unsigned short int mMarkerUdpPort;
		unsigned int mMarkerSequenceNum;
		char mMarkerText[255];

		struct sockaddr_in mMyInfo;			// Information about Me
		int mMyInfo_Size;					// Size of the struct that contains my info
		struct sockaddr_in mDestInfo;		// Information about the destination (ie the MaccBox)
		int mDestInfo_Size;					// Size of the struct that contains the destination info

		// If the user does not specify a destination address (ie he specifies ipaddr = 0) -
		// AND IF THE FEATURE IS ENABLED, then use the last ipaddr that we received from.
		// This feature is important for UT_Space.
		bool m_RESPONDS_TO_SENDER;

		// The bulk of the work in the constructor happens here!
		void mConstructPart2();


		// Initialize the device and prepare it for communication.
		virtual void mOpen ();

		// Close the device.
		virtual void mCloseDerived ();
		void mCloseConnections (); // This function is unique to the CWin32NicHost2Dpr class

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

		bool mCanIRead(int timeOutInMilliSeconds = 0);
};

