#ifndef __EXC4000_H_
#define __EXC4000_H_

#include "WinTypes.h"
#include "ExcUnetMs.h"
#define EXC4000_INTERNAL_CLOCK	0
#define EXC4000_EXTERNAL_CLOCK	1 
#pragma pack(4)
typedef struct
{
	unsigned short int request;					//0x140
	unsigned short int avgCurrent;				//0x142
	unsigned short int remainingCapacity;		//0x144
	unsigned short int fullCapacity;			//0x146
	unsigned short int status;					//0x148
	unsigned short int relativeCharge;			//0x14A
	unsigned short int cycleCount;				//0x14C
	unsigned short int temperature;				//0x14E
	unsigned short int powerStatus;				//0x150
	unsigned short int BatteryDate;				//0x152
	unsigned short int BatterySerialNum;		//0x154
	unsigned short int voltage;					//0x156; millivolts
	unsigned short int batteryType;				//0x158
	unsigned short int reserved[3];				//0x15A-0x15F
} t_BatteryInfo;
#pragma pack()

typedef struct t_globalRegs4000
	{
		WORD boardSigId;				// 0x0000
		WORD softwareReset;				// 0x0002
		WORD interruptStatus;			// 0x0004
		WORD interruptReset;			// 0x0006
		WORD moduleInfo[4];				// 0x0008 - 0x000F
		WORD clocksourceSelect;			// 0x0010
		WORD reserved;					// 0x0012 (Byte Swapping)
		WORD IRcommand;					// 0x0014 (Sync IRIG B / SBS Hi)
		WORD IRsecondsOfDay;			// 0x0016 (IRIG B Time SBS Low)
		WORD IRdaysHours;				// 0x0018 (IRIG B Time Days / Hours
		WORD IRminsSecs;				// 0x001A (IRIG B Time Minutes / Seconds
		WORD IRcontrol1;				// 0x001C (IRIG Control Functions Hi)
		WORD IRcontrol2;				// 0x001E (IRIG Control Functions Low)
		WORD fpgaRevision;				// 0x0020 (FPGA Revision)
		WORD TMprescale;				// 0x0022
		WORD TMpreload;					// 0x0024
		WORD TMcontrol;					// 0x0026
		WORD TMcounter;					// 0x0028
		WORD reserved_2[4];				// 0x002A - x0031

		WORD boardType;					// 0x0032
		WORD boardInfo;					// 0x0034 - feature/options
		WORD reserved_3;				// 0x0036
		WORD moduleInfoSecondGroup[4];	// 0x0038 - 3F
		WORD reserved_4[12];			// 0x0040 - 0x0057
		WORD moduleIntCounters[5];		// 0x0058 - 0x0061
		WORD featureRegister;			// 0x0062
		WORD reserved_5[38];			// 0x0064 - 0x00AF
		WORD IRGenPreloadSec;			// 0x00B0
		WORD IRGenPreloadHourMin;		// 0x00B2
		WORD IRGenPreloadDay;			// 0x00B4
		WORD IRGenPreloadCtrlLo;		// 0x00B6 - Control F Low
		WORD IRGenPreloadCtrlHi;		// 0x00B8 - Control F High
		WORD IRGenPreloadSbsLo;			// 0x00BA
		WORD IRGenPreloadSbsHi;			// 0x00BC
		WORD reserved_6;				// 0x00BE

		WORD IRGenStart;				// 0x00C0 - Write a '0x000B' here to start with current values; write anything else to stop.
		WORD reserved_7[7];				// 0x00C2 - 0x00CF

		WORD IRGenReadSec;				// 0x00D0
		WORD IRGenReadMin;				// 0x00D2
		WORD IRGenReadHour;				// 0x00D4
		WORD IRGenReadDay;				// 0x00D6
		WORD IRGenReadCtrlLo;			// 0x00D8 - Control F Low
		WORD IRGenReadCtrlHi;			// 0x00DA - Control F High
		WORD IRGenReadSbsLo;			// 0x00DC - SBS Low
		WORD IRGenReadSbsHi;			// 0x00DE - SBS High

		WORD reserved_8[16];			// 0x00E0 - 0x00FF

	// Additional UNet 4000 registers:

		volatile WORD Mode;						//0x0100 - 0x0101
		volatile WORD reserved_9;				//0x0102 - 0x0103
		volatile UINT CommOptions;				//0x0104 - 0x0107 (bits 0-2 max header version; rest are reserved)
		volatile WORD reserved_10[3];			//0x0108 - 0x010D
		volatile WORD SerialNumber;				//0x010E - 0x010F
		volatile WORD Start;					//0x0110 - 0x0111
		volatile WORD reserved_11[5];			//0x0112 - 0x011B
		volatile WORD Exc4000FwRevMajor;		//0x011C - 0x011D
		volatile WORD Exc4000FwRevMinor;		//0x011E - 0x011F
		volatile WORD reserved_12[8];			//0x0132 - 0x013f
		volatile WORD OptionsReg;				//0x0130 - 0x0131
		volatile WORD reserved_13[7];			//0x0132 - 0x013f
		volatile t_BatteryInfo BatteryInfo;		//0x0140 - 0x015f
		volatile WORD reserved_14[63];			//0x0160 - 0x01DD
		volatile WORD DebugLevel;				//0x01DE - 0x01DF
		volatile UINT MACaddress[2];			//0x01E0 - 0x01E7
		volatile WORD reserved_15[3];			//0x01E8 - 0x01FD
		volatile WORD Status;					//0x01EE - 0x01EF
		volatile UINT IPaddress;				//0x01F0 - 0x01F3
		volatile UINT reserved_16[2];			//0x01F4 - 0x01FB
		volatile UINT BoardID;					//0x01FC - 0x01FF

	} t_globalRegs4000;

typedef struct
{
	int device_num;
	int allocCounter;
	t_globalRegs4000 *globreg;

} INSTANCE_EXC4000;

typedef struct t_INSTANCE_EXTENDED_EXCUNETINFO
{
	UINT CommOptions;			//0x0104 - 0x0107 (bits 0-2 max header version; rest are reserved)
	WORD SerialNumber;			//0x010E - 0x010F
	char UNetControlFwStr[10];
	WORD OptionsReg;			//0x0130 - 0x0131 (bit 0 = supports CLEAR_EOM_STATUS; bit 1 = supports FIFO_RW; bit 2 = supports 32BIT_RW; rest are reserved)
	char MACaddrStr[20];		// actual length is 18
	char IPaddrStr[20];			// actual length is 16
	char reserved[50];
} t_INSTANCE_EXTENDED_EXCUNETINFO;

typedef struct t_INSTANCE_EXCUNETINFO
{
	WORD device_num;
	int allocatedUIhandle;
	int validUnetInfo;
	WORD boardSigId;			// 0x0000
	WORD clocksourceSelect;		// 0x0010
	WORD fpgaRevision;			// 0x0020
	WORD boardType;				// 0x0032
	WORD moduleInfo[8];			// from moduleInfo (0x0008 - 0x000F) & moduleInfoSecondGroup (0x0038 - 0x003F)

	t_INSTANCE_EXTENDED_EXCUNETINFO extendedUnetInfo;

} t_INSTANCE_EXCUNETINFO;

typedef struct INSTANCE_EXCUNETINFO
{
	WORD device_num;
	int allocatedUIhandle;
	int validUnetInfo;
	WORD boardSigId;			// 0x0000
	WORD clocksourceSelect;		// 0x0010
	WORD fpgaRevision;			// 0x0020
	WORD boardType;				// 0x0032
	WORD moduleInfo[8];			// from moduleInfo (0x0008 - 0x000F) & moduleInfoSecondGroup (0x0038 - 0x003F)

} INSTANCE_EXCUNETINFO;

#define MODULEINFO_NAMESTR_SIZE 30
typedef struct t_ModuleInfo
{
	WORD moduleType;								// e.g. 05 (Px), 04 (RTx)
	WORD numOfBitsAccess;							// e.g. 16, 32, 64
	WORD moduleFamily;								// e.g. 4, 8
	char moduleNameStr[MODULEINFO_NAMESTR_SIZE];	// e.g. "M4K1553Px"
	WORD reserved[13];
} t_ModuleInfo;

typedef struct t_ExcModuleInfo
{
	WORD moduleType;			// e.g. Px-05, RTx-04
	WORD numOfBitsAccsess;		// e.g. 16, 32, 64
	WORD moduleFamily;			// e.g. 4, 8
	WORD reserved[13];
} t_ExcModuleInfo;

/* Irig stuff - for user */
#define IRIG_TIME_AND_RESET	0x100
#define IRIG_TIME		0x200
#define IRIG_TIME_AVAIL			0x400
#define IRIG_TIME_CONTINUOUS_P104P009	0x101 // Only for custom P104P-009
// For IRIG-Time Generator
#define IRIG_GENERATOR_PRESENT_BIT 0x8000
#define IRIG_GENERATOR_RUNNING_BITS 0x000B
#define FEATURE_REG_MODULE_INTERRUPT_COUNTERS 0x01
#define TM_STARTBITVAL	1

#define TM_STARTBIT	0
#define TM_RELOADBIT	1
#define TM_INTERRUPTBIT 2
#define TM_GLOBRESETBIT 3

#define TIMER_RELOAD		1
#define TIMER_NO_RELOAD		0
#define TIMER_INTERRUPT		1
#define TIMER_NO_INTERRUPT	0
#define TIMER_GLOBRESET		1
#define TIMER_NO_GLOBRESET	0

/* the timer on M4K cards is treated as module 4 */
#define EXC4000_MODULE_TIMER	4

#ifndef MAX_MODULES_ON_BOARD
#define MAX_MODULES_ON_BOARD	8
#endif

#define GLOBALREG_BANK	4
#define EXC4000PCI_BOARDIDMASK		0x000F
#define EXC4000PCI_BOARDSIGMASK		0xFF00
#define EXC4000PCI_BOARDSIGVALUE	0x4000
#define EXC4000PCIE_BOARDSIGVALUE	0xE400
#define EXC8000PCIE_BOARDSIGVALUE	0xE800
#define EXCAFDXPCI_BOARDSIGVALUE	0xAF00
#define EXC4000PCI_MODULEINFOMASK	0x1F
#define EXC4000PCI_MODULETYPEMASK	0x1F
#define EXC4000PCI_MODULETYPE_32BITACCESS_FLAG	0x20
#define EXC4000PCI_GLOBAL_TTAG_RESET	0x0010

#define EXCMODID_REGADDR_NONE 	0x0000
#define EXCMODID_REGADDR_MCH	0xFFFA
#define EXCMODID_REGADDR_RTX	0xFFFC
#define EXCMODID_REGADDR_PX		0x70FC
#define EXCMODID_REGADDR_MMSI	0x70FC
#define EXCMODID_REGADDR_708	0x08FC
#define EXCMODID_REGADDR_825CAN	0x007C
#define EXCMODID_REGADDR_H009	0xFFFC
#define EXCMODID_REGADDR_DIO	0x00FC
#define EXCMODID_REGADDR_MEM	0x007C
#define EXCMODID_REGADDR_SER	0x10FC
#define EXCMODID_REGADDR_717	0x007C
#define EXCMODID_REGADDR_ADDA	0x007C

#define EXCMODID_REGADDR_MASK	0x0000FFFF
#define EXCMODID_FLAG_M8K		0x0008
#define FEATURE_REG_MODULE_INTERRUPT_COUNTERS 0x01
typedef struct{
	unsigned short int days;
	unsigned short int hours;
	unsigned short int minutes;
	unsigned short int seconds;
}t_IrigTime;

// UNet #defines / flags:
#define BOARD_ID 			0x4000BA5E
#define DOWNLOAD_NET_MODE	0xcdcd
#define START 				0x1
#define STOP 				0x0
#define WAIT_FOR_CMD		0x9F
#define RUNNING				0x8F

#define OPT_CLEAR_STS		1 //UNet Supports Clear status option (Monitor mode [and CMon] )

#define POWERSTATUS_POWERPORT_ENUMERATED_FLAG 0x0200 				// bit  9
#define POWERSTATUS_POWERPORT_HASDEDICATEDCHARGER_FLAG 0x0400 		// bit 10
#define POWERSTATUS_POWERPORT_HASPOWER_FLAG 0x0010					// bit  4
#define POWERSTATUS_POWERPORT_EXCEEDSSAFECURRENT_FLAG 0x0020		// bit  5
#define POWERSTATUS_POWERPORT_HASCHARGER_FLAG 0x0001				// bit  0
#define POWERSTATUS_POWERPORT_EXCEEDSSAFECHARGERCURRENT_FLAG 0x0002	// bit  1
#define POWERSTATUS_POWERPORT_HASPCUSB_FLAG 0x0004					// bit  2
#define POWERSTATUS_POWERPORT_EXCEEDSSAFEPCUSBCURRENT_FLAG 0x0008	// bit  3
#define POWERSTATUS_COMMPORT_ENUMERATED_FLAG 0x0100					// bit  8
#define POWERSTATUS_COMMPORT_HASPOWER_FLAG 0x0040					// bit  6
#define POWERSTATUS_COMMPORT_EXCEEDSSAFECURRENT_FLAG 0x0080			// bit  7
#define POWERSTATUS_COMMPORT_SUSPENDED_FLAG 0x0800					// bit 11
#define POWERSTATUS_BATTERY_INSTALLED_FLAG 0x4000					// bit 14
/* Prototypes  --  Exported */

#define MODULE_TYPES 32
#ifdef __cplusplus
extern "C"
{
#endif
	int  Get_4000Board_Type(WORD device_num, WORD *boardtype);
	int Get_4000Board_Name(WORD device_num, char * pBoardName);
	int  Get_4000Module_Type(WORD device_num, WORD module_num, WORD *modtype);
	int Get_4000Module_Info(WORD device_num, WORD module_num, t_ModuleInfo *pModuleInfo);
	int Get_Extended_UNet_Info(WORD device_num, t_INSTANCE_EXTENDED_EXCUNETINFO * pExtendedUnetInfo);
	int  Get_UniqueID_P4000(WORD device_num, WORD *pwUniqueID);
	int  Select_Time_Tag_Source_4000 (WORD device_num, WORD source);
	int  Get_Time_Tag_Source_4000 (WORD device_num, WORD *source);
	int  Reset_Module_4000(WORD device_num, WORD module_num);
	int  Reset_Timetags_On_All_Modules_4000(WORD device_num);

	int  Init_Timers_4000 (WORD device_num, int *handle);
	int  Release_Timers_4000 (int handle);

	int  SetIrig_4000(int handle, WORD flag);
	int  IsIrigTimeavail_4000(int handle, int *availFlag);
	int  GetIrigSeconds_4000(int handle, unsigned long *seconds);
	int  GetIrigTime_4000(int handle, t_IrigTime *IrigTime);
	int  GetIrigControl_4000(int handle, unsigned long *control);

	int  StartTimer_4000(int handle, unsigned long microsecsToCount, int reload_flag, int interrupt_flag, int globalreset_flag, int *timeoffset);
	int  StopTimer_4000(int handle, unsigned long *timervalue);
	int  ReadTimerValue_4000(int handle, unsigned long  *timervalue);
	BOOL  IsTimerRunning_4000(int handle, int *errorcondition);
	int  ResetWatchdogTimer_4000(int handle);

	int  InitializeInterrupt_P4000(int handle);
	int  Wait_for_Interrupt_P4000(int handle, unsigned int timeout);
	int  Get_Interrupt_Count_P4000(int handle, unsigned int *Sys_Interrupts_Ptr);

	int  IsDMASupported_4000(WORD device_num);
	int  IsRepetitiveDMASupported_4000(WORD device_num);
	int  IsPciExpress_4000(WORD wBoardType);
	BOOL IsUnetFamilyDevice_4000(WORD device_num);

	int  Get_Error_String_4000(int errcode,int errlen, char *errstring);
	int  Get_4000Interface_Rev(WORD device_num,WORD *interface_rev);
	int Get_PtrBoardInstance_4000(int handle, INSTANCE_EXC4000 ** ppBoardInstance);

	int PokeWORD_4000(WORD device_num, unsigned short offset,unsigned short data);
	int PeekWORD_4000(WORD device_num, unsigned short offset,unsigned short *pData);
	int PeekWORDBuffer_4000(WORD device_num, DWORD_PTR offset, unsigned short *pData, int numWordsToRead);

// Under development
	int IsIrigGeneratorAvail_4000(int handle, int *availFlag);
	int IsIrigGeneratorRunning_4000(int handle, int *runningFlag);
	int StartIrigGenerator_4000(int handle, t_IrigTime irigTime, unsigned long sbs, unsigned long control);
	int StopIrigGenerator_4000(int handle);
	int GetIrigGeneratorTime_4000(int handle, t_IrigTime *IrigTime);
	int GetIrigGeneratorSBS_4000(int handle, unsigned long * currentSBS);
	int GetIrigGeneratorControl_4000(int handle, unsigned long * currentCF);
	int IsPerModuleInterruptCount_Avail_4000(int handle, int *availFlag);
	int Get_PerModuleInterrupt_Count_4000(int handle, int module, unsigned long *Sys_Interrupts_Ptr);
	int Verify4000SeriesBoard(t_globalRegs4000 *globreg);
	int Get_ExcModule_Info_4000(WORD device_num, WORD module_num, t_ExcModuleInfo *pExcModuleInfo, char *pModuleNameStr, int moduleNameStrSize);

#ifdef __cplusplus
}
#endif

#endif

