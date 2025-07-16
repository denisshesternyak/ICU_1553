#pragma once

typedef int eUNetStatus;

/*
enum eUNetStatus {
	enStatOk							= 0  // Status OK - no error
};
*/

enum eUNetUDPport {
	enInvalidPort = 0,
	en1553UDPport = 0x1553,
	en429UDPport = 0x429
};

// The next couple of enums are used to build 

enum eUsbConnector {
	enInvalidConnector = 0,
	enUsbPowerConnector = 80, // 'P',
	enUsbDataConnector = 67, // 'D'
};

enum eUNetProtocol {
	enInvalidProtocol = 0,
	en1553Protocol = 0x1553,
	en429Protocol = 0x429
};

