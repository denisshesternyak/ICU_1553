#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#include <stdio.h>
#include "pxIncl.h"

extern INSTANCE_RTX cardrtx[];

#include "ExcUnetMs.h"
#define M4K429RTX_BASE_HANDLE 100
#define B2DRMHANDLE ((int) (M4K429RTX_BASE_HANDLE + handlertx))

static int daysInMonth[12] = {31,29,31,30,31,30,31,31,30,31,30,31};
static char MonthNames[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

/*
Convert BCD value (as listed in the Irig time extracted from memory) into an integer
*/
#define BCD_DIGITS_IN_INT 8
#define SIZE_OF_DIGIT 4
int BCDtoBinary(unsigned int BCDvalue){
	unsigned int BinaryValue = 0;
	int digit, digitmultiplyer = 1;
	for (digit = 0; digit < BCD_DIGITS_IN_INT; digit++) {
		BinaryValue += (BCDvalue & 0xf) * digitmultiplyer;
		BCDvalue >>= SIZE_OF_DIGIT;
		digitmultiplyer *= 10;
	}
	return BinaryValue;
}

/*
PrintIRIGtime

Description: Print (sprintf) the Irig ttag components into a string
For use with value returned from function Get_ModuleIrigTime_BCD_RTx

Input parameters (all components are in BCD):
	ttag1		number of the day from 01 Jan, where 1=01 Jan (1-366)
	ttag2		hours component, followed by minutes component
	ttag3		second component, followed by microseconds component
	ttag4		more of the microseconds component

timetag words are in BCD; see function PrintIRIGtime_RTx for timetag words in decimal representation

Output parameters:
	outString	a string containing the visual display of the Irig timetag value, in format:
			"day MonthName hour:minute:second.microseconds"

Return values:	none
*/

void borland_dll PrintIRIGtime(unsigned short ttag1, unsigned short ttag2, unsigned short ttag3, unsigned short ttag4, char * outString)
{
	unsigned int hour,minute,second,microseconds;
	int day, month;
	int i;
	int systemYear;
#ifdef _WIN32
	SYSTEMTIME SystemTime;

	GetLocalTime(&SystemTime);
	systemYear = SystemTime.wYear;
#else
	time_t timer;
	time (&timer);
	systemYear = localtime (&timer)->tm_year;
#endif

	//use Windows time to decide if it's a leapyear.
	if ((systemYear % 4) != 0)
		daysInMonth[1] = 28;

	day = 		BCDtoBinary(ttag1 & 0xfff);
	hour = 		BCDtoBinary(ttag2 >> 8);
	minute = 		BCDtoBinary(ttag2 & 0xff);
	second = 		BCDtoBinary((unsigned int)ttag3 >> 8);
	microseconds = 	BCDtoBinary(((unsigned int)(ttag3 & 0xff) << 16) | ttag4);

	for (i=0; i < 12; i++) {
		if ((day - daysInMonth[i]) > 0)
			day -= daysInMonth[i];
		else break;
	}
	month = i;

	sprintf(outString,"%u %s %u:%02u:%02u.%06u",day,MonthNames[month],hour,minute,second,microseconds);
	return;
}


// ----------------------------------------------------------

/*
PrintIRIGtime_RTx

Description: Print (sprintf) the Irig ttag components into a string
For use with value returned from function Get_ModuleIrigTime_RTx

Input parameters (all components are in decimal):
	struct t_IrigOnModuleTime, with fields:
		IrigTime->days		number of the day from 01 Jan, where 1=01 Jan (1-366)
		IrigTime->hours		hours component
		IrigTime->minutes		minutes component
		IrigTime->seconds		second component
		IrigTime->microsecs	microseconds component

timetag words are in decimal; see function PrintIRIGtime_BCD_RTx for timetag words in BCD representation

Output parameters:
	outString	a string containing the visual display of the Irig timetag value, in format:
			"day MonthName hour:minute:second.microseconds"

Return values:	none
*/
void borland_dll PrintIRIGtime_RTx (t_IrigOnModuleTime *IrigTime, char * outString)
{
	unsigned int hour,minute,second,microseconds;
	int day, month;
	int i;
	int systemYear;
#ifdef _WIN32
	SYSTEMTIME SystemTime;

	GetLocalTime(&SystemTime);
	systemYear = SystemTime.wYear;
#else
	time_t timer;
	time (&timer);
	systemYear = localtime (&timer)->tm_year;
#endif

	//use Windows time to decide if it's a leapyear.
	if ((systemYear % 4) != 0)
		daysInMonth[1] = 28;

	day			= IrigTime->days;
	hour			= IrigTime->hours;
	minute		= IrigTime->minutes;
	second		= IrigTime->seconds;
	microseconds	= IrigTime->microsecs;

	for (i=0; i < 12; i++) {
		if ((day - daysInMonth[i]) > 0)
			day -= daysInMonth[i];
		else break;
	}
	month = i;

	sprintf(outString,"%u %s %u:%02u:%02u.%06u",day,MonthNames[month],hour,minute,second,microseconds);
	return;
}


/*
Build a BCD string for some integer value, for visual display in memory (DPR)
*/
unsigned int BuildBCD_RTx (unsigned int binaryValue, unsigned int bitOffset, unsigned int numDigits)
{
	#define BITS_IN_DIGIT	4
	unsigned int digitLoopCounter;
	unsigned int BcdValue;
	unsigned int currentBinaryDigit;

	BcdValue = 0;
	for (digitLoopCounter = 0; digitLoopCounter < numDigits; digitLoopCounter++) {
		currentBinaryDigit = binaryValue % 10;
		binaryValue = binaryValue / 10; //prepare for next digit
		BcdValue = BcdValue | (currentBinaryDigit << (digitLoopCounter * BITS_IN_DIGIT));
	}
	return (BcdValue << bitOffset);
}


/*
Preset_IrigTimeTag_RTx

Description: Load values [day, hours, minutes, seconds] into the Irig Preload registers

Input parameters:
	handlertx		Module handle returned from the earlier call to Init_Module_RTx
	Day		number of the day from 01 Jan, where 1=01 Jan (1-366)
	Hour		hour of the day (0-23)
	Minute	minute of the hour (0-59)
	Second	second of the minute (0-59)

Output parameters:	none

Return values:
	0				If successful
	ebadhandle			If handle parameter is out of range or not allocated
	ewrongprocessor		If module processor is not (PxIII) Nios-based
	efeature_not_supported	If IrigB Timetag feature is not supported on this module
	>0				successful, but some parameter(s) was input out of bounds & normalized internally
*/
/*
*********************************************
int borland_dll Preset_IrigTimeTag_RTx (int handlertx, usint Day, usint Hour, usint Minute, usint Second)
{
	#define IRIG_DAY_OFFSET	 	0	//bit offset of end of DAY field in Irig preset register
	#define IRIG_HR_OFFSET		8	//bit offset of end of HOUR field in Irig preset register
	#define IRIG_MIN_OFFSET		0	//bit offset of end of Minutes field in Irig preset register
	#define IRIG_SEC_OFFSET		8	//bit offset of end of SECONDS field in Irig preset register

	#define IRIG_DAY_NORMALIZED		0x8
	#define IRIG_HOUR_NORMALIZED		0x4
	#define IRIG_MINUTE_NORMALIZED		0x2
	#define IRIG_SECOND_NORMALIZED		0x1

	unsigned int IrigRegister;
	int status;

	// Verify that the value of module's handle is valid (i.e. between zero and NUMCARDS-1).
	if ((handlertx < 0) || (handlertx >= NUMCARDS)) return (ebadhandle);

	// Verify that the module's handle is allocated/initialized.
	if (cardrtx[handlertx].allocated != 1) return (ebadhandle);

	// Check that the processor is a NIOS II processor.
	//if (cardrtx[handlertx].processor != PROC_NIOS)	return (ewrongprocessor);

	// Check that the firmware supports the IrigB timestamp feature
	//if (cardrtx[handlertx].supportIrigModeFW != TRUE) return (efeature_not_supported);

	status = 0;
	// if parameters are out of bounds, normalize them
	if (Day > 366)
	{
		status |= IRIG_DAY_NORMALIZED;
		Day %= 366;
	}
	if (Hour > 23)
	{
		status |= IRIG_HOUR_NORMALIZED;
		Hour %= 24;
	}
	if (Minute > 59)
	{
		status |= IRIG_MINUTE_NORMALIZED;
		Minute %= 60;
	}
	if (Second > 59)
	{
		status |= IRIG_SECOND_NORMALIZED;
		Second %= 60;
	}

	IrigRegister = BuildBCD_RTx(Hour, IRIG_HR_OFFSET, 2);
	IrigRegister |= BuildBCD_RTx(Minute, IRIG_MIN_OFFSET, 2);

	// cardrtx[handlertx].glob->IrigPreload_days = BuildBCD_RTx(Day, IRIG_DAY_OFFSET, 3);
	WWORD_H2D( cardrtx[handlertx].glob->IrigPreload_days, BuildBCD_RTx(Day, IRIG_DAY_OFFSET, 3) );

	// cardrtx[handlertx].glob->IrigPreload_hr_min = IrigRegister;
	WWORD_H2D( cardrtx[handlertx].glob->IrigPreload_hr_min, IrigRegister );

	// cardrtx[handlertx].glob->IrigPreload_seconds = BuildBCD_RTx(Second, IRIG_SEC_OFFSET, 2);
	WWORD_H2D( cardrtx[handlertx].glob->IrigPreload_seconds, BuildBCD_RTx(Second, IRIG_SEC_OFFSET, 2) );

	// cardrtx[handlertx].glob->IrigPreload_load = 1;
	WWORD_H2D( cardrtx[handlertx].glob->IrigPreload_load, 1 );

	return (status);
}
*********************************************
*/

/* written April 2015 by David Koppel

Get_ModuleIrigTime_RTx

Description: Returns the current Module level IrigB timestamp, in decimal representation.

Note:
(a) This function is supported only in the PxIII (NiosII-based) modules, firmware released post Feb 2015.

Input parameters:
	handlertx	Module handle returned from the earlier call to Init_Module_RTx

Output parameters:
	IrigTime   	A structure, t_IrigOnModuleTime, containing the IrigB timestamp in decimal digits, defined in file pxIncl.h as
		typedef struct {
			unsigned short int days;
			unsigned short int hours;
			unsigned short int minutes;
			unsigned short int seconds;
			unsigned int microsecs;
		}t_IrigOnModuleTime;

Return values:
		0			If successful
		ebadhandle		If handle parameter is out of range or not allocated
		ewrongprocessor	If module processor is not (PxIII) Nios-based
		efeature_not_supported	If IrigB Timetag feature is not supported on this module

timetag words are in decimal; see function Get_ModuleIrigTime_BCD_RTx for timetag words in BCD representation

*/

int borland_dll Get_ModuleIrigTime_RTx (int handlertx, t_IrigOnModuleTime *IrigTime)
{
	int i;
	unsigned short int moduleTime[4];
	
	// Verify that the value of module's handle is valid (i.e. between zero and NUMCARDS-1).
	if ((handlertx < 0) || (handlertx >= NUMCARDS)) return (ebadhandle);

	// Verify that the module's handle is allocated/initialized.
	if (cardrtx[handlertx].allocated != 1) return (ebadhandle);

	// Check that the processor is a NIOS II processor.
	//if (cardrtx[handlertx].processor != PROC_NIOS)	return (ewrongprocessor);

	// Check that the firmware supports the IrigB timestamp feature
	if (cardrtx[handlertx].supportIrigModeFW != TRUE) return (efeature_not_supported);

	// Get the module Irig time; an array of 4 short ints; days; hours + mins; secs + usec; more usec
	for (i=0; i<4; i++) 
	{
		// read 0th element first, to freeze the ttag
		// moduleTime[3-i] = cardrtx[handlertx].glob->IRIGttag[i];
		moduleTime[3-i] = RWORD_D2H( cardrtx[handlertx].glob->IRIGttag[i] );
	}

	Convert_IrigTimetag_BCD_to_Decimal_RTx (moduleTime, IrigTime);
	return 0;
}


/* written June 2015

Convert_IrigTimetag_BCD_to_Decimal_RTx

Description: Converts Irig timetag from BCD representation (array of 4 words) to decimal representation (struct t_IrigOnModuleTime)

Utility function, not dependent on handle of module.
However, we call it internally in the DLL for functions Get_ModuleIrigTime_RTx

Input parameters:
	moduleTime	Array of 4 words - BCD representation of timetag

Output parameters:
	IrigTime   	A structure, t_IrigOnModuleTime, containing the IrigB timestamp in decimal digits, defined in file pxIncl.h as
		typedef struct {
			unsigned short int days;
			unsigned short int hours;
			unsigned short int minutes;
			unsigned short int seconds;
			unsigned int microsecs;
		}t_IrigOnModuleTime;

Return values:
		0			If successful
*/

int borland_dll Convert_IrigTimetag_BCD_to_Decimal_RTx (unsigned short int *moduleTime, t_IrigOnModuleTime *IrigTime)
{
	IrigTime->days = (((moduleTime[0] & 0x0F00) >> 8) * 100);	/* first digit */
	IrigTime->days += (((moduleTime[0]  & 0x00F0) >> 4) * 10);	/* second digit */
	IrigTime->days += (moduleTime[0]  & 0x000F);		/* lowest digit */
	
	IrigTime->hours = (((moduleTime[1]  & 0xF000) >> 12) * 10);
	IrigTime->hours += ((moduleTime[1]  & 0x0F00) >> 8);

	IrigTime->minutes = (((moduleTime[1]  & 0x00F0) >> 4) * 10);
	IrigTime->minutes += (moduleTime[1]  & 0x000F);

	IrigTime->seconds = (((moduleTime[2]  & 0xF000) >> 12) * 10);
	IrigTime->seconds += ((moduleTime[2]  & 0x0F00) >> 8);

	IrigTime->microsecs = (unsigned int)(((moduleTime[2]  & 0x00F0) >> 4) * 100000);
	IrigTime->microsecs += (unsigned int)((moduleTime[2]  & 0x000F) * 10000);

	IrigTime->microsecs += (unsigned int)(((moduleTime[3]  & 0xF000) >> 12) * 1000);
	IrigTime->microsecs += (unsigned int)(((moduleTime[3]  & 0x0F00) >> 8) * 100);

	IrigTime->microsecs += (unsigned int)(((moduleTime[3]  & 0x00F0) >> 4) * 10);
	IrigTime->microsecs += (unsigned int)(moduleTime[3]  & 0x000F);
	
	IrigTime->seconds++;  // need to add 1 to compensate for the fact that the module Irig timetag returned by hardware is one behind the actual current irig time
	if (IrigTime->seconds == 60)
	{
		IrigTime->seconds = 0;
		IrigTime->minutes++;
		if (IrigTime->minutes == 60)
		{
			IrigTime->minutes = 0;
			IrigTime->hours++;
			if (IrigTime->hours == 24)
			{
				IrigTime->hours = 0;
				IrigTime->days++;
				if ((IrigTime->days == 366) || (IrigTime->days == 367))
					IrigTime->days = 0;
			}
		}
	}

	return 0;
}
	
