#ifndef __EXC_LINUX_DEF_H
#define __EXC_LINUX_DEF_H

// For Linux-Windows compatibility issues
#define BYTE   char
#define UINT	unsigned short int
#define INT    int
#define WORD   unsigned short int
#define DWORD  unsigned long int
#define HANDLE int
#define ULONG  unsigned long int
#define ULONG_PTR unsigned long
#define LONG unsigned long int 
#define LPSTR	char *

#define LARGE_INTEGER DWORD

#define BOOL   int
#define TRUE   1
#define FALSE  0

#ifndef NULL
#define NULL   0
#endif

#define LOBYTE(x) (x & 0xFF)


#define Sleep(x) (usleep(x*1000))

#endif

