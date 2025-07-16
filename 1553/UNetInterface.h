#pragma once
// #include <windows.h>
#ifdef _WIN32
#include "atlstr.h" // required for cstring.h
#else
#include <string>
using namespace std;
#endif
#include "Host2Dpr.h"
#include "UnetFlags.h"

// #include "Unet4000defines.h" retired!
#include "excsysio.h"
#include "exc4000.h"

#include "ftd2xx.h"

#define MAX_FTDI_DEVICES 10



#ifndef MODULE_0
#define MODULE_0 0
#endif

#ifndef MODULE_4
#define MODULE_4 4
#endif

typedef struct
{
//FUTURE	bool flag_PowerPortEnumerated;						// PWREN2n			(bit  9)
//FUTURE	bool flag_PowerPortHasDedicatedCharger;				// BCD				(bit 10)
			bool flag_PowerPortHasPower;						// dSTATb1n			(bit  4)
			bool flag_PowerPortExceedsSafeCurrent;				// dWRNb1n			(bit  5)
//FUTURE	bool flag_PowerPortHasCharger;						// dSTATa1n			(bit  0)
			bool flag_PowerPortExceedsSafeChargerCurrent;		// dWRNa1n			(bit  1)
//FUTURE	bool flag_PowerPortHasPcUsb;						// dSTATa2n			(bit  2)
			bool flag_PowerPortExceedsSafePcUsbCurrent;			// dWRNa2n			(bit  3)
			bool flag_CommPortEnumerated;						// PWREN1n			(bit  8)
			bool flag_CommPortHasPower;							// dSTATb2n			(bit  6)
			bool flag_CommPortExceedsSafeCurrent;				// dWRNb2n			(bit  7)
			bool flag_CommPortSuspended;						// SUSPND1n			(bit 11)
			bool flag_BatteryInstalled;							// batteryPRESENTn	(bit 14)
}t_UNetPowerFlags;


typedef struct 
{
#ifdef _WIN32
	CString Manufacturer; 
	CString ManufacturerId; 
	CString Description; 
	CString SerialNumber; 
#else
	string Manufacturer; 
	string ManufacturerId; 
	string Description; 
	string SerialNumber; 
#endif
}t_UNetDataInfo;

class CUNETInterface
{
public:
	CUNETInterface();
	~CUNETInterface();

	//Connect/Disconnect
	eUNetStatus ListUsbDeviceSerialNums_UNet(unsigned int maxSerialNumsLen, char *pSerialNums);
	eUNetStatus DevConnect_UNet(int deviceNumber);
	eUNetStatus USBConnect_UNet(char *pSerialNumUSB);
	eUNetStatus NETConnect_UNet(char *pIPAddr, eUNetUDPport UNetUDPport);
	eUNetStatus Disconnect_UNet();

	//Peek and Poke
	eUNetStatus PeekWORD_UNet(int moduleNum, unsigned short offset, unsigned short *pData);
	eUNetStatus PokeWORD_UNet(int moduleNum, unsigned short offset, unsigned short data);
	eUNetStatus PeekBYTE_UNet(int moduleNum, unsigned short offset, BYTE *pData);
	eUNetStatus PokeBYTE_UNet(int moduleNum, unsigned short offset, BYTE data);
	eUNetStatus PeekWORDBuffer_UNet(int moduleNum, unsigned short size, unsigned short offset, unsigned short *pData);
	eUNetStatus PokeWORDBuffer_UNet(int moduleNum, unsigned short size, unsigned short offset, unsigned short *pData);


	//MAC Address and IP Address
	eUNetStatus GetNetInfo_UNet(char *pIPAddr, char *pMACAddr, eUNetUDPport *pUNetUDPport);
	eUNetStatus SetNetInfo_UNet(char *pIPAddr, char *pMACAddr);

	//Serial Number
	eUNetStatus GetExcSerialNumber_UNet(unsigned short *pExcSerialNum);
	eUNetStatus ExcSerNumToUsbSerString(unsigned short excSerialNum, enum eUsbConnector unetUsbConnector, enum eUNetProtocol unetProtocol, char * pUsbSerialString);

	//Revisions
	eUNetStatus GetFirmwareRev_UNet(unsigned short *pFWrevMajor, unsigned short *pFWrevMinor);
	eUNetStatus GetHardwareRev_UNet(unsigned short *pHWrevMajor, unsigned short *pHWrevMinor);

	//Debug
	eUNetStatus GetDebugLevel_UNet(unsigned short *pDebugLevel);
	eUNetStatus SetDebugLevel_UNet(unsigned short debugLevel);

	//Unet status
	eUNetStatus GetUNETstatus_UNet(unsigned short *pUNETstatus);

	bool IsBatteryInstalled_UNet();
	eUNetStatus GetUnetBatteryStatus_UNet(t_BatteryInfo *pUnetBatteryStatus, t_UNetPowerFlags *pUnetPowerFlags=NULL);
	eUNetStatus GetUnetPowerFlags_UNet(t_UNetPowerFlags *pUnetPowerFlags);

	// GetLastErrorMsg_UNet is obsolete, being replaced by GetLastH2DErrorMsg_UNet
	void GetLastErrorMsg_UNet(char *pErrorMsg) {return (GetLastH2DErrorMsg_UNet(pErrorMsg));}
	void GetLastH2DErrorMsg_UNet(char *pErrorMsg);

	void UNetStartDebug(const char *pDebugFileName);
	void UNetStartDebug() {UNetStartDebug("UNetInterface.log");}
	void UNetStopDebug();
	bool UNetIsDebugOn() {return mDebugOn;}

	// Registry Info about device
	bool GetRegDeviceType(int deviceNumber, char * pDeviceType);
	bool GetRegConnectType(int deviceNumber, char * pConnectType);

	// Additional USB Functions for Dovi's PxTest
	bool isDataUsbCableConnected();
	bool isPowerUsbCableConnected();
	bool isMoreThanOneUNetUsbDevice();
	bool ReadUsbPowerInfo(FT_EEPROM_X_SERIES *pFt_eeprom_x_series, char *pManufacturer, char *pManufacturerId, char *pDescription, char *pSerialNumber);
	bool ReadUsbDataInfo(FT_EEPROM_2232H *pFt_eeprom_2232h,		char *pManufacturer, char *pManufacturerId, char *pDescription, char *pSerialNumber);
	bool GetUsbUNetDataInfo(int *pNumOfUNetDevices, t_UNetDataInfo *pUNetDataInfo);

	bool ResetUNetUsbPower(); 
	bool ResetUNetUsbData(); 


private:

	t_globalRegs4000 * mpUnetModuleFour;

	int mDeviceNum;
	char mSerialNumUSB[20];
	char mIPAddr[20];
	eUNetUDPport mUNetUDPport;

	bool mDebugOn;

	bool mConnected;

	CHost2Dpr * mpHostToDeviceRawMemory;

	unsigned int mWordEndian(unsigned int word) {return ((word & 0x0000FFFF) << 16) + ((word & 0xFFFF0000) >> 16);}

	// Additional USB (Data &) Functions for to support Dovi's PxTest functions above
	bool ScanForUnetDevices(DWORD *pNumDevs);
	bool GetDataDevNum(DWORD *pDataDevNum);
	bool GetData2DevNum(DWORD *pDataDevNum);
	bool GetPowerDevNum(DWORD *pPowerDevNum);
#ifdef _WIN32
	bool GetPowerDefaultInfo(FT_EEPROM_X_SERIES *pFt_eeprom_x_series, char *pManufacturer, char *pManufacturerId);
	bool GetPowerEmptyUNetInfo(FT_EEPROM_X_SERIES *pFt_eeprom_x_series, char *pManufacturer, char *pManufacturerId,char *pDescription, char *pSerialNumber);
	bool GetDataDefaultInfo(FT_EEPROM_2232H *pFt_eeprom_2232h, char *pManufacturer, char *pManufacturerId);
#endif
	bool GetDeviceInfoDetail(int devNum, t_UNetDataInfo *pUNetDataInfo, char *pConnectionType);

	void InterpretUnetPowerFlags_UNET(unsigned short powerStatus, t_UNetPowerFlags *pUnetPowerFlags);

	FT_DEVICE_LIST_INFO_NODE *mpDevInfo; 


};

