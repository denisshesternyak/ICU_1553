#pragma once

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#define MAX_PATH 260
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#endif

typedef struct t_IniOptions {
	char iniFileName[MAX_PATH];
	int saveIniFile;

	int option_Debug_AlwaysOn;			// in Bridge2DeviceRawMemory.cpp; formerly OPTION_UNETLOG_ALWAYS_ON
	int option_Debug_MsgHeaders;		// in Host2Dpr.cpp; formerly OPTION_DEVELOPMENTAL_DEBUG
	int option_Debug_FTInterface;		// in LibFt2232UsbPx.c; formerly OPTION_DEVELOPMENTAL_DEBUG
	int option_Debug_LogHandleInfo;		// in Bridge2DeviceRawMemory.cpp; formerly OPTION_LOG_HANDLE_AND_ENTRYEXIT_INFO
	int option_Debug_WhoCalledInit;		// in Bridge2DeviceRawMemory.cpp; formerly OPTION_WHOCALLEDINIT_DEBUG
	int option_Debug_LogUsbIntTimeouts;	// in UsbFtd2Px.cpp; formerly OPTION_LOG_USB_WAITFORINT_TIMEOUTS

	int option_Debug_RWTiming_JustRWCalls;	// in LibFt2232UsbPx.c & Win32NicHost2Dpr.cpp
	int option_Debug_RWTiming_RWRoutines;	// in Host2Dpr
	int option_Debug_LogRWTBeforeMsg;		// in LibFt2232UsbPx.c & Win32NicHost2Dpr.cpp ;
	unsigned int option_Debug_LogRWTimeThreshold_millisec; // in LibFt2232UsbPx.c & Win32NicHost2Dpr.cpp ;

	int option_Debug_MeasureIntTiming;	// in UsbFtd2Px.cpp (& ?)

	int option_MsgHeader_VersionOverride;	// in ?; forces the use of the specified MsgHeader version, irregardless of the UNet's CommOptions register

	unsigned int option_IntNotification_Timeout_Millisec;			// in Bridge2DeviceRawMemory.cpp; formerly INTERRUPT_NOTIFICATION_LOOP_TIMEOUT
	unsigned int option_IntNotification_SleepOverride_Millisec;		// new addition to Bridge2DeviceRawMemory.cpp

} t_IniOptions;

#ifdef __cplusplus
extern "C" {
#endif

int InitOptions();
int LoadIniOptions();
int SaveIniOptions();

#ifdef __cplusplus
} // end of extern "C"
#endif


