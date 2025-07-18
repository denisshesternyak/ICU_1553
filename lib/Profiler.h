#pragma once

// #define PROFILING_ENABLED 1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32
#define __declspec(dllexport) 
#include "WinTypes.h"
typedef union {long long QuadPart;} LARGE_INTEGER ;
void QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);
void QueryPerformanceCounter(LARGE_INTEGER* ticks);
#endif
void __declspec(dllexport) GetRunningNDeltaTime(double * pRunningTimeSec, double * pDeltaTimeMillisec);
int __declspec(dllexport) TimeStampFPrint(LPCSTR lpszFormat, ...);
int __declspec(dllexport) JustFPrint(LPCSTR lpszFormat, ...);
int __declspec(dllexport) CloseTimeStampLog();

#ifdef __cplusplus
} // end of extern "C"
#endif

