// excsysio.h
//
// Define control codes for Excalbr driver
//

#include "WinTypes.h"
#include <asm/ioctl.h>
#include "deviceio.h"

#ifndef __excsysio__h_
#define __excsysio__h_

#define EXC_BUFFER_SIZE	256
#define EXC_BUFFER_SIZE_LARGE 512

#define MAX_REGISTRY_DEVICES	16
#define MAX_DEVICES				32
#define MAX_MODULES				9
#define MAX_BANKS				MAX_MODULES

// Structures for IOCTL passing
typedef struct tagExcBankInfo {
  unsigned int dwBank;
  unsigned long dwAddress;
  unsigned int dwLength;
} ExcBankInfo;


typedef struct tagExcIRQRequest{
	int nModule;
	int timeout;
	BOOL timedOut;
}ExcIRQRequest;

typedef struct tagExcMultipleIRQRequest{
	WORD moduleBitfield;
	WORD modulesInterrupting;
	int timeout;
	BOOL timedOut;
}ExcMultipleIRQRequest;

typedef struct tagExcMultipleIRQMultipleDeviceRequest{
	t_ExcDevModPair DevModPairs[sizeof(DWORD)*8];
	int numpairs;
	unsigned int modulesInterrupting;
	int timeout;
	BOOL timedOut;
}ExcMultipleIRQMultipleDeviceRequest;

#define EXC_IOCTL_MAGIC 'E'


#define EXC_IOCTL_GETBANKINFO  (_IOWR(EXC_IOCTL_MAGIC, 1, ExcBankInfo))
#define EXC_IOCTL_GET_INTERRUPT_COUNT (_IOR(EXC_IOCTL_MAGIC, 2, unsigned int))
#define EXC_IOCTL_GET_MCH_INT_COUNT (_IOWR(EXC_IOCTL_MAGIC, 3, unsigned int))
#define EXC_IOCTL_GET_MCH_SHADOW (_IOR(EXC_IOCTL_MAGIC, 4, char))
#define EXC_IOCTL_GET_IRQ_NUMBER (_IOR(EXC_IOCTL_MAGIC, 5, int))
#define EXC_IOCTL_WAIT_FOR_IRQ (_IO(EXC_IOCTL_MAGIC, 6))  
#define EXC_IOCTL_WAIT_FOR_MOD_IRQ (_IOWR(EXC_IOCTL_MAGIC, 8, ExcIRQRequest))
#define EXC_IOCTL_WAIT_FOR_MULT_IRQ_MULT_DEV (_IOWR(EXC_IOCTL_MAGIC, 9, ExcMultipleIRQMultipleDeviceRequest))
#define EXC_IOCTL_MAX 9

// memory banks
#define EXC_BANK_DUALPORTRAM	0
#define EXC_BANK_DUALPORTRAM2	3
#define EXC_BANK_CONTROL		1
#define EXC_BANK_ATTRIB			2
#define EXC_BANK_DUALPORTRAM2   3

// device number defines
#define MAX_REGISTRY_DEVICES	16
#define MAX_DEVICES				32

#define EXC_1553PCI				MAX_REGISTRY_DEVICES+2
#define EXC_1553PCIPX           MAX_REGISTRY_DEVICES+3
#define DAS_429PCI				MAX_REGISTRY_DEVICES+4
#define EXC_3910PCI				MAX_REGISTRY_DEVICES+5
#define EXC_4000PCI 			MAX_REGISTRY_DEVICES+9

#define EXC_1553PCMCIA			MAX_REGISTRY_DEVICES+10
#define EXC_1553PCMCIAEP		MAX_REGISTRY_DEVICES+11
#define EXC_429PCMCIA			MAX_REGISTRY_DEVICES+12

#define EXC_SOCKET_AUTO		0xFFFF
#define EXC_MULTI_DEVICE_NUM 	12 

// these two defines no longer used in Win2000
#define EXC_ANYSOCKET			99
#define EXC_DEVICE_ANYPCMCIA	MAX_REGISTRY_DEVICES+1

// EXC-4000PCI Board Types
#define EXC4000_BRDTYPE_PCI			0x4000
#define EXC4000_BRDTYPE_CPCI		0x4001
#define EXC4000_BRDTYPE_MCH_PCI		0x4002
#define EXC4000_BRDTYPE_MCH_CPCI	0x4003
#define EXC4000_BRDTYPE_MCH_PMC		0x4004
#define EXC4000_BRDTYPE_429_PMC		0x4005
#define EXC2000_BRDTYPE_PCI			0x4006
#define EXC4000_BRDTYPE_P104P		0x4007
#define EXC4000_BRDTYPE_PCIHC		0x4008
#define EXC4000_BRDTYPE_708_PMC		0x4009
#define EXC4000_BRDTYPE_1553PX_PMC	0x400A
#define EXC4000_BRDTYPE_DISCR_PCI   0x400D
#define EXC4000_BRDTYPE_PCIE		0xE400
#define EXC2000_BRDTYPE_PCIE		0xE406
#define EXCARD_BRDTYPE_1553PX		0xE401
#define EXCARD_BRDTYPE_429RTX		0xE402
#define MINIPCIE_BRDTYPE_1553PX		0xE403
#define MINIPCIE_BRDTYPE_429RTX		0xE404
#define EXC4500_BRDTYPE_PCIE_VPX	0xE450
#define EXC4000_BRDTYPE_PCIE64		0xE464
#define EXC_BRDTYPE_664PCIE			0xE664
#define EXC_BRDTYPE_1394PCI			0x1394
#define EXC_BRDTYPE_1394PCIE		0xEF00
#define EXC_BRDTYPE_UNET		0x5502
#define EXC_BRDTYPE_RNET		0x5505

// EXC-4000PCI Module Types
#define EXC_BRDTYPE_UNDEFINED		0
#define EXC_BRDTYPE_PCEP			1
#define EXC_BRDTYPE_PCMCH			2
#define EXC_BRDTYPE_1553PCIEP		3
#define EXC_BRDTYPE_PCIPX			4
#define EXC_BRDTYPE_429MX			5
#define EXC_BRDTYPE_MAGIC			6
#define EXC_BRDTYPE_429PCI			7
#define EXC_BRDTYPE_429RXTX			8
#define EXC_BRDTYPE_PCHC			9
#define EXC_BRDTYPE_3900			10
#define EXC_BRDTYPE_429PCMCIA		11
#define EXC_BRDTYPE_1553PCMCIA		12	
#define EXC_BRDTYPE_1553PCMCIAEP	13
#define EXC_BRDTYPE_HOO9			14
#define EXC_BRDTYPE_3910PCI			15
#define EXC_BRDTYPE_PCPX			16
#define EXC_BRDTYPE_4000PCI			17
#define EXC_BRDTYPE_PCIMCH			18
#define EXC_BRDTYPE_1760MMI			19
#define EXC_BRDTYPE_1553PCMCIAPX	20
#define EXC_BRDTYPE_1553PCMCIAEPPX	21 //recognizes EP or PX
#define EXC_BRDTYPE_AFDX			22
#define EXC_BRDTYPE_1394			32
#define EXC_BRDTYPE_4000ETH			64

/*
 * These  are the old defines for Linux; new ones copied from Windows below:
#define EXC4000_MODTYPE_SERIAL		2
#define EXC4000_MODTYPE_MCH			3
#define EXC4000_MODTYPE_RTX			4
#define EXC4000_MODTYPE_PX			5
#define EXC4000_MODTYPE_MMSI		6
#define EXC4000_MODTYPE_708			7
#define EXC4000_MODTYPE_MA			8
#define EXC4000_MODTYPE_CAN825		0x28	//CAN825 only comes in 32 bit access
#define EXC4000_MODTYPE_CAN			0xc
#define EXC4000_MODTYPE_DIO			0xd
#define EXC4000_MODTYPE_H009		0x9
#define EXC4000_MODTYPE_M4KETHERNET		0x0E
#define EXC4000_MODTYPE_SERIAL_PLUS	0x12
#define EXC4000_MODTYPE_AFDX_TX		0x1C
#define EXC4000_MODTYPE_AFDX_RX		0x1A
#define EXC4000_MODTYPE_NONE		0x1F
#define EXC4000_MODTYPE_ETHERNET		0x1B
*/

// EXC-4000PCI Module Types
#define EXC4000_MODTYPE_NOMOD		0		// When looking at the moduleInfoSecondGroup on older boards, we find values of 0x00, corresponding to no module; hence we'll use EXC4000_MODTYPE_NOMOD as another no-module indicator
#define EXC4000_MODTYPE_561			0x1
#define EXC4000_MODTYPE_SERIAL		0x2
#define EXC4000_MODTYPE_MCH			0x3
#define EXC4000_MODTYPE_RTX			0x4
#define EXC4000_MODTYPE_PX			0x5
#define EXC4000_MODTYPE_MMSI		0x6
#define EXC4000_MODTYPE_708			0x7
#define EXC4000_MODTYPE_825CAN		0x8		//CAN825 module type is 8 (MODTYPE_MA is retired); however
#define EXC4000_MODTYPE_CAN825		0x28	//CAN825 module type is frequently handled as 0x28; we'll reserve both to avoid breaking any software
#define EXC4000_MODTYPE_H009		0x9
#define EXC4000_MODTYPE_ADDA		0xA
#define EXC4000_MODTYPE_CAN			0xC
#define EXC4000_MODTYPE_DIO			0xD
#define EXC4000_MODTYPE_M4KETHERNET	0x0E
#define EXC4000_MODTYPE_MEM			0x0F
#define EXC4000_MODTYPE_SERIAL_PLUS	0x12
#define EXC4000_MODTYPE_717			0x17
#define EXC4000_MODTYPE_AFDX_RX		0x1A
#define EXC4000_MODTYPE_ETHERNET	0x1B
#define EXC4000_MODTYPE_AFDX_TX		0x1C
#define EXC4000_MODTYPE_NONE		0x1F	// both EXC4000_MODTYPE_NOMOD (00) and EXC4000_MODTYPE_NONE (1F) are indicators of no module



typedef struct tagExcWriteIOPacket
{
	ULONG	offset;
	BYTE	value;
} _ExcWriteIOPacket;

typedef struct tagExcReadIOPacket
{
	ULONG	offset;
	BYTE	*pValue;
} _ExcReadIOPacket;

typedef struct tagExcWriteAttribPacket
{
	ULONG	offset;
	WORD	value;
} _ExcWriteAttribPacket;

typedef struct tagExcReadAttribPacket
{
	ULONG	offset;
	WORD	*pValue;
} _ExcReadAttribPacket;


typedef struct tagExcDevInfoPacket {
	DWORD	fWorkingState;
	DWORD	fRegistryDeviceValid;
	DWORD	fFlag3;
	DWORD	fFlag4;
	DWORD	fFlag5;
	DWORD	fFlag6;
	DWORD	dwBusType;
	DWORD	dwBoardType;
	DWORD	dwSocketNumber;
	DWORD	dwRegistryDevice;
	DWORD	dwDword5;
	DWORD	dwDword6;
	DWORD	dwDword7;
	DWORD	dwDword8;
} _ExcDevInfoPacket;

typedef struct tagExcRegistryInfoPacket {
	char	szDeviceType[EXC_BUFFER_SIZE];
	DWORD	dwMemBase[MAX_BANKS];
	DWORD	dwMemLength[MAX_BANKS];
	DWORD	wMemHandle[MAX_BANKS];
	DWORD	dwMemBanks;
	DWORD	fUsingIRQ;
	DWORD	dwIrqNumber;
	DWORD	fUsingIO;
	DWORD	pIORange;
	DWORD	lIORangeLength;
	DWORD	dwSocketNumber;
} _ExcRegistryInfoPacket;

#endif












