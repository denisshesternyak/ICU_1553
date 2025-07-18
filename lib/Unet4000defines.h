#pragma once

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

#pragma pack(4)
typedef struct 
{
	volatile WORD boardSigId;				//0x0000
	volatile WORD softwareReset;			//0x0002
	volatile WORD interruptStatus;			//0x0004
	volatile WORD interruptReset;			//0x0006
	volatile WORD moduleInfo[4];			//0x0008 - 0x000F
	volatile WORD clocksourceSelect;		//0x0010
	volatile WORD reserved;					//0x0012
	volatile WORD IRcommand;				//0x0014
	volatile WORD IRsecondsOfDay;			//0x0016
	volatile WORD IRdaysHours;				//0x0018
	volatile WORD IRminsSecs;				//0x001A
	volatile WORD IRcontrol1;				//0x001C
	volatile WORD IRcontrol2;				//0x001E
	volatile WORD fpgaRevision;				//0x0020
	volatile WORD TMprescale;				//0x0022
	volatile WORD TMpreload;				//0x0024
	volatile WORD TMcontrol;				//0x0026
	volatile WORD TMcounter;				//0x0028
	volatile WORD reserved_2[4];			//0x002A - 0x0031
	volatile WORD boardType;				//0x0032
	volatile WORD boardInfo;				//0x0034
	volatile WORD reserved_3;				//0x0036
	volatile WORD moduleInfoSecondGroup[4];	//0x0038 - 0x003F
	volatile WORD reserved_4[96];			//0x0040 - 0x00FF
/////////////////////////////////////////////////////////////////
	volatile WORD Mode;						//0x0100 - 0x0101
	volatile WORD reserved_5[6];			//0x0102 - 0x010D
	volatile WORD SerialNumber;				//0x010E - 0x010F
	volatile WORD Start;					//0x0110 - 0x0111
	volatile WORD reserved_6[5];			//0x0112 - 0x011B
	volatile WORD Exc4000FwRevMajor;		//0x011C - 0x011D
	volatile WORD Exc4000FwRevMinor;		//0x011E - 0x011F
	volatile WORD reserved_7[8];			//0x0132 - 0x013f
	volatile WORD OptionsReg;				//0x0130 - 0x0131
	volatile WORD reserved_8[7];			//0x0132 - 0x013f
	volatile t_BatteryInfo BatteryInfo;		//0x0140 - 0x015f
	volatile WORD reserved_9[63];			//0x0160 - 0x01DD
	volatile WORD DebugLevel;				//0x01DE - 0x01DF
	volatile UINT MACaddress[2];			//0x01E0 - 0x01E7
	volatile WORD reserved_10[3];			//0x01E8 - 0x01FD
	volatile WORD Status;					//0x01EE - 0x01EF
	volatile UINT IPaddress;				//0x01F0 - 0x01F3
	volatile UINT reserved_11[2];			//0x01F4 - 0x01FB
	volatile UINT BoardID;					//0x01FC - 0x01FF
} t_Exc4000MemoryMap;
#pragma pack()

