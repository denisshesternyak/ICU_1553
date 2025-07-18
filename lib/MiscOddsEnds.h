#pragma once

// #include <windows.h>
// #include <stdio.h>

#define __declspec(dllexport) 
#ifdef __cplusplus
extern "C" {
#endif

void __declspec(dllexport) HexByteString(const unsigned char * pHexBytes, int numHexBytes, char * pHexString, int hexStringLen);
void __declspec(dllexport) HexWordString(const unsigned short * pHexWords, int numHexWords, char * pHexString, int hexStringLen);
void __declspec(dllexport) HeaderString(char * pHeader, char * pHeaderString, int headerStringLen);

#ifdef __cplusplus
} // end of extern "C"
#endif

