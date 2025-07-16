#ifndef __EXC_LINUX_DEF_H
#define __EXC_LINUX_DEF_H

#ifndef __WINDOWS_TYPES__ //The FTD WinTypes.h
// For Linux-Windows compatibility issues
#define BYTE   unsigned char
#define UINT	unsigned short int
#define INT    int
#define WORD   unsigned short int
#define DWORD  unsigned int
#define DWORD_PTR  unsigned long int //We don't need any #ifdef for this, since unsigned long is 64 bits in 64-bit Linux
#define HANDLE int
#define ULONG  unsigned long int
#define ULONG_PTR unsigned long
#define LONG unsigned long int 
#define LPSTR	char *
#define LPVOID void *
#define __int64 long long

#define LARGE_INTEGER long long
#define WINAPI 

#define BOOL   int
#define TRUE   1
#define FALSE  0

#ifndef NULL
#define NULL   0
#endif

#define LOBYTE(x) (x & 0xFF)

#endif // __WINDOWS_TYPES__
#include <sys/vlimit.h>
#define INFINITE INFINITY 
#include <unistd.h>
#define Sleep(x) (usleep(x*1000))

#ifndef min
#define min(a, b)	((a) < (b)? (a): (b))
#endif
#ifndef __cplusplus
#ifndef max
#define max(a, b)	((a) > (b)? (a): (b))
#endif
#endif

#endif






