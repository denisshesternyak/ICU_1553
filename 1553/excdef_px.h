#ifndef __EXC_DEF_PX_H
#define __EXC_DEF_PX_H


/* *****************************************************************
	Defines used to differentiate between Excalibur products
	This file accompanies EXC-4000PCI PX Module        
   ***************************************************************** */

#define PCI4000
#define M4K1553PX
#define PX
#define PCIPX

#define DMA_SUPPORTED

#define UNET_DEVICE

//Definitions for old compilers
#ifndef ULONG_PTR
#if defined(_WIN64)
    typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;
#else
    typedef unsigned long ULONG_PTR, *PULONG_PTR;
#endif
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
#endif

#endif

