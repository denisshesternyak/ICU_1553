#ifndef EXCTYPES_H
#define EXCTYPES_H

#define UINT	unsigned short int
#define BYTE	unsigned char
#define BOOL	int
#define DWORD	unsigned long
#define WORD	unsigned short int
#define ULONG_PTR unsigned long
#define LONG unsigned long int 

#define LOBYTE(x) (x & 0xFF)

//typedef LARGE_INTEGER PHYSICAL_ADDRESS;

#define ULONG	unsigned long
#define LPSTR	char *

#define FALSE 0
#define TRUE 1

// device types
#define EXC_BOARD_429MX     1
#define EXC_BOARD_429PCIMX  2
#define EXC_BOARD_1553PCIPX 3
#define EXC_BOARD_4000PCI   4

// interface types
#define EXC_INTERFACE_AMCC  1
#define EXC_INTERFACE_CORE  2

// bus types
#define EXC_BUS_ISA  1
#define EXC_BUS_PCI  2

#endif


