#pragma once


#ifndef _WIN32
#define __declspec(dllexport) 
#include "WinTypes.h"
typedef union {long long QuadPart;} LARGE_INTEGER ;
void QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);
void QueryPerformanceCounter(LARGE_INTEGER* ticks);
#define MAX_PATH 260
#endif
#define MAX_STEPS 100

typedef struct t_Steps {
	int stepDataValid;
	unsigned int step_Elapsed_MilliSec;
	char stepDescription[MAX_PATH];
} t_Steps;

typedef struct t_DebugTiming {
	LARGE_INTEGER stepTimer;
	int currentStep;
	t_Steps debugSteps[MAX_STEPS];
} t_DebugTiming;


#ifdef __cplusplus
extern "C" {
#endif

void __declspec(dllexport) GenDateTimeString(char * pString);
void __declspec(dllexport) GenRunningNDeltaString(char * pString);
void __declspec(dllexport) GetRunningNDeltaTime(double * pRunningTimeSec, double * pDeltaTimeMillisec);
int __declspec(dllexport) TimeStampFPrint(LPCSTR lpszFormat, ...);
int  __declspec(dllexport) TimeStampErrorFPrint(int errorNum, LPCSTR lpszFormat, ...);
int __declspec(dllexport) JustFPrint(LPCSTR lpszFormat, ...);
int __declspec(dllexport) CloseTimeStampLog();

void __declspec(dllexport) StartWinTimer(LARGE_INTEGER * pWinTimer);
unsigned long long __declspec(dllexport) GetElapsedWinTime_MicroSec(LARGE_INTEGER * pWinTimer);
void __declspec(dllexport) InitDebugTimer(t_DebugTiming * pDebugTimer);
void __declspec(dllexport) SnapshotDebugTimer(t_DebugTiming * pDebugTimer, const char * pStepDescription);
unsigned int __declspec(dllexport) TotalMillisecDebugTimer(t_DebugTiming * pDebugTimer);
void __declspec(dllexport) PrintStepsDebugTimer(t_DebugTiming * pDebugTimer, char * pSummaryDescription);

#ifdef __cplusplus
} // end of extern "C"
#endif

