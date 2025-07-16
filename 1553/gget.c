#include <stdio.h>
#include "pxIncl.h"

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)

extern INSTANCE_PX cardpx[];
#define LOOPBACK_DELAY 2500
#ifndef _WIN32
#include <time.h>
#endif

usint boardoptions;
int daysInMonth[12] = {31,29,31,30,31,30,31,31,30,31,30,31};
char MonthNames[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


/*
Get_Board_Status

  Description: Indicates the status of the board.
  Input parameters:  none
  Output parameters: none
  Return values:	Zero or more of the following flags:
		BOARD_READY
		RAM_OK
		TIMERS_OK
		SELF_TEST_OK
		BOARD_HALTED  The board is stopped

		  Bit 7 is always set to 1.
*/

int borland_dll Get_Board_Status (void)
{
	return Get_Board_Status_Px (g_handlepx);
}


int borland_dll Get_Board_Status_Px (int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	return (cardpx[handlepx].exc_bd_bc->board_status);
	return ( RBYTE_D2H(cardpx[handlepx].exc_bd_bc->board_status) );
}



/*
Get_Id

  Description: Returns the board ID.
  Input parameters:  none
  Output parameters: none
  Return values: BOARD_ID

*/

int borland_dll Get_Id (void)
{
	return Get_Id_Px (g_handlepx);
}

int borland_dll Get_Id_Px (int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	return ( cardpx[handlepx].exc_bd_bc->board_id );
	return ( RBYTE_D2H(cardpx[handlepx].exc_bd_bc->board_id) );
}



/*
Get_Mode

  Description: Indicates the current mode of the card, reflecting the last
  call on Set_Mode.
  Input parameters:  none
  Output parameters: none

	Return values: One of the following:
	RT_MODE
	BCRT_MODE
	MON_BLOCK
	MON_LOOKUP
*/


int borland_dll Get_Mode (void)
{
	return Get_Mode_Px (g_handlepx);
}

int borland_dll Get_Mode_Px (int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	return (cardpx[handlepx].exc_bd_bc->board_config);
	return (cardpx[handlepx].board_config);
}


#define RT_PARITY_BIT_ERR 0x80
#define RT_LOCK_BIT 0x40
#define RT_NUM_BITS 0x1f

int borland_dll Get_RT_Info (char *pRtNum, char *pRtLock)
{
	return Get_RT_Info_Px(g_handlepx, pRtNum, pRtLock);
}

int borland_dll Get_RT_Info_Px (int handlepx, char *pRtNum, char *pRtLock)
{
	unsigned short rtRegister;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;


//	rtRegister = cardpx[handlepx].exc_bd_rt->rtNumSingle;
	rtRegister = RWORD_D2H( cardpx[handlepx].exc_bd_rt->rtNumSingle );

	if ((rtRegister & RT_PARITY_BIT_ERR) && (rtRegister & RT_LOCK_BIT))
		*pRtLock = 2; //parity error
	else
	{
		if (rtRegister & RT_LOCK_BIT)
			*pRtLock = 1;
		else
			*pRtLock = 0;
	}

	*pRtNum = rtRegister & RT_NUM_BITS;

	return rtRegister;

}
/*
Get_Rev_Level

  Description: Indicates the current revision level of the firmware. Each
  hex nibble (i.e. 4 bits) reflects a level so the 0x11 should be interpreted
  as revision 1.1

	Input parameters:  none
	Output parameters: none
	Return values:	Firmware revision level
*/

int borland_dll Get_Rev_Level (void)
{
	return Get_Rev_Level_Px (g_handlepx);
}

int borland_dll Get_Rev_Level_Px (int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	return ( cardpx[handlepx].exc_bd_bc->revision_level );
	return ( RBYTE_D2H( cardpx[handlepx].exc_bd_bc->revision_level ) );
}



int borland_dll Read_Start_Reg(usint *startval)
{
	return Read_Start_Reg_Px(g_handlepx, startval);
}

int borland_dll Read_Start_Reg_Px(int handlepx, usint *startval)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	*startval = cardpx[handlepx].exc_bd_bc->start;
	*startval = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start );
	return (0);
}

/*
Get_Card_Type

  Description: Indicates the board firmware type, either 1553 or 1760.

	Input parameters:  none
	Output parameters: board firmware type: 0x1553, 0x1760
	Return values:	0

*/

int borland_dll Get_Card_Type (usint *cardtype)
{
	return Get_Card_Type_Px (g_handlepx, cardtype);
}

int borland_dll Get_Card_Type_Px (int handlepx, usint *cardtype)
{
	usint moreboardoptions;
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	boardoptions = cardpx[handlepx].exc_bd_bc->board_options;
	boardoptions = RWORD_D2H( cardpx[handlepx].exc_bd_bc->board_options ); 

//	moreboardoptions = cardpx[handlepx].exc_bd_bc->more_board_options;
	moreboardoptions = RWORD_D2H( cardpx[handlepx].exc_bd_bc->more_board_options );

	if ((boardoptions & MOD_TYPE_MASK) == MOD_TYPE_1760) /* 1760 Family */
	{
		if ((moreboardoptions & RT_SINGLE_FUNCTION_MASK) == RT_SINGLE_FUNCTION_MASK)
			*cardtype = MOD_1760_SF; /* 1760 PxS */
		else if ((moreboardoptions & MONITOR_ONLY_MODE_MASK) == MONITOR_ONLY_MODE_MASK)
			*cardtype = MOD_1760_MON_ONLY; /* 1760 PxM */
		else
			*cardtype = MOD_1760; /* 1760 */
	}
	else if  ((boardoptions & MOD_TYPE_MASK) == MOD_TYPE_1553) /* 1553 Family */
	{
		if ((moreboardoptions & RT_SINGLE_FUNCTION_MASK) == RT_SINGLE_FUNCTION_MASK)
			*cardtype = MOD_1553_SF; /* 1553 PxS */
		else if ((moreboardoptions & MONITOR_ONLY_MODE_MASK) == MONITOR_ONLY_MODE_MASK)
			*cardtype = MOD_1553_MON_ONLY; /* 1553 PxM */
		else
			*cardtype = MOD_1553; /* 1553 */
	}
	else
		*cardtype = MOD_NOT_ACTIVE;

	return 0;
}

/*
Get_Board_Options_Lo

  Description: Indicates whether the board has Concurrent Monitor option

	Input parameters:  none
	Output parameters: concurrent monitor option:
	P - PCMCIA/EP  or Px  card
	M - PCMCIA/EPM or PMx card
	Return values:	0

*/

int borland_dll Get_Board_Options_Lo (int *boardopt)
{
	return Get_Board_Options_Lo_Px (g_handlepx, boardopt);
}

int borland_dll Get_Board_Options_Lo_Px (int handlepx, int *boardopt)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	boardoptions =  (int)(cardpx[handlepx].exc_bd_bc->board_options);
	boardoptions =  (int)(RWORD_D2H( cardpx[handlepx].exc_bd_bc->board_options));
	boardoptions &= 0x00ff;
	*boardopt = boardoptions;
	return 0;
}



/*
Get_Board_Options_Hi

  Description: Indicates some of the board options.
  Input parameters:  none
  Output parameters: the board options register, hi nibble
  indicating type of firmware on the card

	boardopt is a combination of the following values:

	  0x04: Intel processor [If not set, AMD processor]
	  0x10: EXC-1553PCMCIA/EPII  [If not set, M4k-1553px module]
	  0x40: Always set

		bits 0 and 1: 0x01: Firmware for 1553
	       0x02: Firmware for 1760


			   Return value: 0
*/

int borland_dll Get_Board_Options_Hi (int *boardopt)
{
	return Get_Board_Options_Hi_Px (g_handlepx, boardopt);
}

int borland_dll Get_Board_Options_Hi_Px (int handlepx, int *boardopt)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* *boardopt =  (int)((cardpx[handlepx].exc_bd_bc->board_options & 0xff00) >> 8); */
//	boardoptions = cardpx[handlepx].exc_bd_bc->board_options;
	boardoptions = RWORD_D2H( cardpx[handlepx].exc_bd_bc->board_options );
	boardoptions= (boardoptions & 0xff00) >> 8;
	*boardopt = boardoptions;
	return 0;
}


/*
Get_Header_Exists - for 1760 cards

  Description: Check to see if this Subaddress has an assigned header word value.

	Input parameters: int sa	 the Subaddress number for this setting
	Output parameters: int enable	 1 means enabled, 0 means disabled
	Return values:	0	   success
	rterror    bad Subaddress number

*/

int borland_dll Get_Header_Exists (int sa, int *enable)
{
	return Get_Header_Exists_Px (g_handlepx, sa, enable);
}

int borland_dll Get_Header_Exists_Px (int handlepx, int sa, int *enable)
{
	int mode, enable_val;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ( (sa < 0) || (sa > 31) )
		return(rterror);
//	enable_val = cardpx[handlepx].exc_bd_bc->header_exists[sa];
	enable_val = RWORD_D2H( cardpx[handlepx].exc_bd_bc->header_exists[sa] );

//	mode = cardpx[handlepx].exc_bd_bc->board_config;
	mode = cardpx[handlepx].board_config;
	if ((mode == MON_BLOCK) || (mode == MON_LOOKUP))
	{
		if (enable_val == 0x100)
			*enable = HEADER_ENABLE_XMT;
		else if (enable_val == 0x1)
			*enable = HEADER_ENABLE_RCV;
		else if (enable_val == 0x101)
			*enable = HEADER_ENABLE;
		else if (enable_val == 0)
			*enable = HEADER_DISABLE;
	}
	else if ((mode == BC_MODE) || (mode == BC_RT_MODE))
	{
		*enable = enable_val >> 8;
	}
	else /* RT_MODE */
		*enable = enable_val & 0xff;
	return(0);
}


/*
Get_Header_Value - for 1760 cards

  Description: Get the header word value assigned this subaddress.
  Input parameters:  int sa     the Subaddress number for this setting
  Output parameters: usint header_value
  the assigned header value

	Return values:	0	   success
	rterror    bad Subaddress number
*/

int borland_dll Get_Header_Value (int sa, int direction, usint *header_value)
{
	return Get_Header_Value_Px (g_handlepx, sa, direction, header_value);
}

int borland_dll Get_Header_Value_Px (int handlepx, int sa, int direction, usint *header_value)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ( (sa < 0) || (sa > 31) )
		return (rterror);

//	if ((cardpx[handlepx].exc_bd_bc->board_config == BC_MODE)
//		|| (cardpx[handlepx].exc_bd_bc->board_config == BC_RT_MODE))
//		*header_value = cardpx[handlepx].exc_bd_bc->header_val[sa];
//	else if (cardpx[handlepx].exc_bd_bc->board_config == RT_MODE)
//		*header_value = cardpx[handlepx].exc_bd_rt->header_val[sa];

	if ((cardpx[handlepx].board_config == BC_MODE) ||
		(cardpx[handlepx].board_config == BC_RT_MODE))
		*header_value = RWORD_D2H( cardpx[handlepx].exc_bd_bc->header_val[sa]);
	else if (cardpx[handlepx].board_config == RT_MODE)
		*header_value = RWORD_D2H( cardpx[handlepx].exc_bd_rt->header_val[sa] );
	else /* monitor */
	{
		if (direction == TRANSMIT) /* select upper block of header vals */
//			*header_value = cardpx[handlepx].exc_bd_monseq->header_val[0x20+sa];
			*header_value = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->header_val[0x20+sa]);
		else if (direction == RECEIVE) /* select lower block of header vals */
//			*header_value = cardpx[handlepx].exc_bd_monseq->header_val[sa];
			*header_value = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->header_val[sa] );
		else
			return einval;
	}
	return 0;
}


int borland_dll Internal_Loopback(struct i_loopback *ilbvals)
{
	return Internal_Loopback_Px(g_handlepx,ilbvals);
}
int borland_dll Internal_Loopback_Px(int handlepx, struct i_loopback *ilbvals)
{
	int i, retval;
#ifdef _WIN32
	LARGE_INTEGER startime;
#endif
	BOOL efound;
	usint *data2;
	unsigned char tmpStart;

	data2 = (usint *)ilbvals;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* stop card, in case it was started */
//	cardpx[handlepx].exc_bd_bc->start = 0;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start , 0);

	/* set board_config to internal loopback setting */
//	cardpx[handlepx].exc_bd_bc->board_config = 0xed;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_config , 0xed );
	cardpx[handlepx].board_config = 0xed;



	/* make sure card is stopped */
	efound = FALSE;
	
#ifdef _WIN32
	StartTimer_Px(&startime);

	/* LOOPBACK_DELAY is 50 * 50 */
	while(EndTimer_Px(startime, LOOPBACK_DELAY) != 1)
	{
//		if (cardpx[handlepx].exc_bd_bc->board_status & BOARD_HALTED )
		if ( RBYTE_D2H(cardpx[handlepx].exc_bd_bc->board_status) & BOARD_HALTED )
		{
			efound = TRUE;
			break;
		}
	}
	
#else

	for (i=0; i<TIMEOUT_VAL; i++)
	{
		Sleep(50L);
//		if (cardpx[handlepx].exc_bd_bc->board_status & BOARD_HALTED )
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status ) & BOARD_HALTED )
		{
			efound = TRUE;
			break;
		}
	}
	if (!efound)
		return etimeout;
	
#endif /* _WIN32 */
	
	/* now that card is stopped, start it and loopback should take effect */
//	cardpx[handlepx].exc_bd_bc->start |= 0x01;
	tmpStart = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start );
	tmpStart |= 0x01;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start , tmpStart );

	/* wait until loopback test finished, and card is stopped */
	efound = FALSE;
	
#ifdef _WIN32

	StartTimer_Px(&startime);
	/* LOOPBACK_DELAY is 50 * 50 */
	while(EndTimer_Px(startime, LOOPBACK_DELAY) != 1)
	{
//		if (cardpx[handlepx].exc_bd_bc->start == 0)
		if ( RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start ) == 0)
		{
			efound = TRUE;
			break;
		}
	}

#else

	for (i=0; i<TIMEOUT_VAL; i++)
	{
		Sleep(50L);
//		if (cardpx[handlepx].exc_bd_bc->start == 0 )
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start ) == 0 )
		{
			efound = TRUE;
			break;
		}
	}

#endif /* _WIN32 */
	if (!efound)
		return etimeout;
	else
	{
		for (i=0; i < 12; i++)
//			data2[i] = cardpx[handlepx].exc_bd_bc->stack[i];
			data2[i] = RWORD_D2H( cardpx[handlepx].exc_bd_bc->stack[i] );

		retval = 0;
		if (ilbvals->frame_status != 0x8000)  retval = ilbfailure;
     	 	else if (ilbvals->resp_status != 0x8000) retval = ilbfailure;
#ifndef MMSI
		else if ((ilbvals->early_val & 0x003F ) != 0x15) retval = ilbfailure;
/* #else 
		else if ((ilbvals->early_val & 0x03FF ) != 0x295) retval = ilbfailure;
*/
#endif
      		else if (ilbvals->status_1 != 0x8000) retval = ilbfailure;
		else if (ilbvals->status_2 != 0x8000) retval = ilbfailure;
#ifndef MMSI
		else if (ilbvals->mc_status != 0x8000) retval = ilbfailure;
#endif
		else if (ilbvals->ttag_status != 0x8000) retval = ilbfailure;
	}

	return retval;
}

int borland_dll External_Loopback(struct e_loopback *elbvals)
{
	return External_Loopback_Px(g_handlepx,elbvals);
}
int borland_dll External_Loopback_Px(int handlepx, struct e_loopback *elbvals)
{
	unsigned char tmpStart;

#ifdef _WIN32
	LARGE_INTEGER startime;
#endif

#ifndef MMSI
	int i, retval;
	BOOL efound;
	usint *data2;
#endif

#ifdef MMSI
	return (func_invalid);
#else
	data2 = (usint *)elbvals;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((cardpx[handlepx].processor == PROC_NIOS) &&
//		((cardpx[handlepx].exc_bd_bc->more_board_options & MONITOR_ONLY_MODE) == MONITOR_ONLY_MODE))
		((RWORD_D2H( cardpx[handlepx].exc_bd_bc->more_board_options) & MONITOR_ONLY_MODE) == MONITOR_ONLY_MODE))
		return (emonmode);

	/* stop card, in case it was started */
//	cardpx[handlepx].exc_bd_bc->start = 0;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start, 0 );
	/* set board_config to external loopback setting */
//	cardpx[handlepx].exc_bd_bc->board_config = 0xff;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_config ,0xff );
	cardpx[handlepx].board_config = 0xff;

	/* make sure card is stopped */
	efound = FALSE;
	
#ifdef _WIN32

	StartTimer_Px(&startime);

	/* LOOPBACK_DELAY is 50 * 50 */
	while(EndTimer_Px(startime, LOOPBACK_DELAY) != 1)
	{
//		if (cardpx[handlepx].exc_bd_bc->board_status & BOARD_HALTED )
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status) & BOARD_HALTED )
		{
			efound = TRUE;
			break;
		}
	}

#else

	for (i=0; i<TIMEOUT_VAL; i++)
	{
		Sleep(50L);
//		if (cardpx[handlepx].exc_bd_bc->board_status & BOARD_HALTED )
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status ) & BOARD_HALTED )
		{
			efound = TRUE;
			break;
		}
	}

#endif /* _WIN32 */
	if (!efound)
		return etimeout;
	
	/* now that card is stopped, start it and loopback should take effect */
//	cardpx[handlepx].exc_bd_bc->start |= 0x01;
	tmpStart = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start );
	tmpStart |= 0x01;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start , tmpStart );

	/* wait until loopback test finished, and card is stopped */
	efound = FALSE;
	
#ifdef _WIN32

	StartTimer_Px(&startime);

	/* LOOPBACK_DELAY is 50 * 50 */
	while(EndTimer_Px(startime, LOOPBACK_DELAY) != 1)
	{
//		if (cardpx[handlepx].exc_bd_bc->start == 0)
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start ) == 0)
		{
			efound = TRUE;
			break;
		}
	}

#else

	for (i=0; i<TIMEOUT_VAL; i++)
	{
		Sleep(50L);
//		if (cardpx[handlepx].exc_bd_bc->start == 0 )
		if (RBYTE_D2H(cardpx[handlepx].exc_bd_bc->start) == 0 )
		{
			efound = TRUE;
			break;
		}
	}
#endif /* _WIN32 */

	if (!efound)
		return etimeout;
	else
	{
		for (i=0; i < 13; i++)
//			data2[i] = cardpx[handlepx].exc_bd_bc->stack[i];
			data2[i] = RWORD_D2H( cardpx[handlepx].exc_bd_bc->stack[i] );

		retval = 0;
		if (elbvals->frame_status != 0x8000)  retval = elbfailure;
		else if (elbvals->cmd_send[1] != 0x8000) retval = elbfailure;
		else if (elbvals->cmd_send[3] != 0x8000) retval = elbfailure;
		else if (elbvals->cmd_send[5] != 0x8000) retval = elbfailure;
		else if (elbvals->cmd_send[7] != 0x8000) retval = elbfailure;
		else if (elbvals->ttag_status != 0x8000) retval = elbfailure;
	}
	return retval;
#endif
}

int borland_dll External_Loopback_MMSI_Px(int handlepx, struct e_loopback_MMSI *elbvals)
{

#ifndef MMSI
	return (func_invalid);
#else

	int i;
#ifdef _WIN32
	LARGE_INTEGER startime;
#endif
	BOOL efound;
	usint *data2;
	unsigned char numPorts;
	int status;
	unsigned char tmpStart;

	data2 = (usint *)elbvals;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* stop card, in case it was started */
//	cardpx[handlepx].exc_bd_bc->start = 0;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start, 0 );

	/* set board_config to external loopback setting */
//	cardpx[handlepx].exc_bd_bc->board_config = 0xff;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_config, 0xff );
	cardpx[handlepx].board_config = 0xff;
	
	/* make sure card is stopped */
	efound = FALSE;

#ifdef _WIN32

	StartTimer_Px(&startime);

	/* LOOPBACK_DELAY is 50 * 50 */
	while(EndTimer_Px(startime, LOOPBACK_DELAY) != 1)
	{
//		if (cardpx[handlepx].exc_bd_bc->board_status & BOARD_HALTED )
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status ) & BOARD_HALTED )

		{
			efound = TRUE;
			break;
		}
	}
	
#else

	for (i=0; i<TIMEOUT_VAL; i++)
	{
		Sleep(50L);
//		if (cardpx[handlepx].exc_bd_bc->board_status & BOARD_HALTED )
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status ) & BOARD_HALTED )

		{
			efound = TRUE;
			break;
		}
	}

#endif /* _WIN32 */

	if (!efound)
		return etimeout;
	
	/* now that card is stopped, start it and loopback should take effect */
//	cardpx[handlepx].exc_bd_bc->start |= 0x01;
	tmpStart = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start );
	tmpStart |= 0x01;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start, tmpStart );
	
	/* wait until loopback test finished, and card is stopped */
	efound = FALSE;
	
#ifdef _WIN32

	StartTimer_Px(&startime);

	/* LOOPBACK_DELAY is 50 * 50 */
	while(EndTimer_Px(startime, LOOPBACK_DELAY) != 1)
	{
//		if (cardpx[handlepx].exc_bd_bc->start == 0)
		if ( RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start ) == 0)
		{
			efound = TRUE;
			break;
		}
	}
	
#else

	for (i=0; i<TIMEOUT_VAL; i++)
	{
		Sleep(50L);
//		if (cardpx[handlepx].exc_bd_bc->start == 0 )
		if ( RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start ) == 0)
		{
			efound = TRUE;
			break;
		}
	}

#endif /* _WIN32 */

	if (!efound)
		return etimeout;
	else
	{
		for (i=0; i < 50; i++)
//			data2[i] = cardpx[handlepx].exc_bd_bc->stack[i];
			data2[i] = RWORD_D2H( cardpx[handlepx].exc_bd_bc->stack[i] );

		if (elbvals->ttag_status != 0x8000) return elbfailure;
		if (elbvals->frame_status != 0x8000)  return elbfailure;

		status = Get_Num_Ports_MMSI_Px (handlepx, &numPorts);
		if (numPorts == 6)
			numPorts = 5; //6 channel module has no Composite BM, so only check channels 0 through 5 
		for (i=0; i <= numPorts; i++)
		{
			if (elbvals->status_1[i] != 0x8000)
				return elbfailure;
			if (elbvals->status_2[i] != 0x8000)
				return elbfailure;
		}
		return 0;
	}
#endif
}


int borland_dll OnBoard_Loopback_Px(int handlepx, struct e_loopback *oblbvals)
{
	int status;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return (ebadhandle);
	if (cardpx[handlepx].allocated != 1) return (ebadhandle);
	if (cardpx[handlepx].processor != PROC_NIOS) return (ewrongprocessor);
//	if ((cardpx[handlepx].exc_bd_bc->more_board_options & ON_BOARD_LOOPBACK) != ON_BOARD_LOOPBACK)
	if ((RWORD_D2H(cardpx[handlepx].exc_bd_bc->more_board_options) & ON_BOARD_LOOPBACK) != ON_BOARD_LOOPBACK)
		return (enoonboardloopback);

//	cardpx[handlepx].exc_bd_rt->options_select = 0x4;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->options_select,0x4 );
	/* this causes External_Loopback to run on the OnBoard relays */

	DelayMicroSec_Px(handlepx, 1000);
	status = External_Loopback_Px(handlepx, oblbvals);
	DelayMicroSec_Px(handlepx, 1000);

//	cardpx[handlepx].exc_bd_rt->options_select = 0;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->options_select ,0 );
	return status;
}

/*
Get_SerialNumber_Px				Coded 16-Nov-2006 by YY Berlin

  Description: Reads and reports the M4K1553PxIII Module's serial number.

	Processor required: NIOS II

	  Input parameters:

		handlepx		Module handle returned from the earlier call to Init_Card.

		  Output parameters:

			serialNum		Module serial number

			  Return values:

				ebadhandle		If handle is invalid
				ewrongprocessor	If the processor is not a NIOS II processor
				0				If successful

*/

int borland_dll Get_SerialNumber_Px (int handlepx, usint * serialNum)
{
/*	Verify that the value of module's handle is valid (i.e. between
zero and NUMCARDS-1).
	*/
	if ((handlepx < 0) || (handlepx >= NUMCARDS))
	{
		return (ebadhandle);
	}

	/*	Verify that the module's handle is allocated/initialized.
	*/
	if (cardpx[handlepx].allocated != 1)
	{
		return (ebadhandle);
	}

	/*	Check that the processor is a NIOS II processor.
	*/
	if (cardpx[handlepx].processor != PROC_NIOS)
	{
		return (ewrongprocessor);
	}

	/*	Get the serial number.
	*/
//	*serialNum = cardpx[handlepx].exc_bd_dwnld->serialNum;
	*serialNum = RWORD_D2H( cardpx[handlepx].exc_bd_dwnld->serialNum );

	/*	All done....
	*/
	return (0);

}  /* end function Get_SerialNumber_Px */

/*
   Get_ModuleTime_Px				Coded 16-Nov-2006 by YY Berlin

	 Description: Reads and reports the M4K1553PxIII Module Time register
	 (Note that the Module Time register (3ea2h-3ea5h) is different
	 from the TimeTag (7008h-700bh) register).

	   Processor required: NIOS II

		 Input parameters:

		   handlepx		Module handle returned from the earlier call to Init_Card.

			 Output parameters:

			   moduleTime		32-bit Module Time

				 Return values:

				   ebadhandle		If handle is invalid
				   ewrongprocessor	If the processor is not a NIOS II processor
				   0				If successful

*/

int borland_dll Get_ModuleTime_Px (int handlepx, unsigned int * moduleTime)
{

/*	Verify that the value of module's handle is valid (i.e. between
zero and NUMCARDS-1).
	*/
	if ((handlepx < 0) || (handlepx >= NUMCARDS))
	{
		return (ebadhandle);
	}

	/*	Verify that the module's handle is allocated/initialized.
	*/
	if (cardpx[handlepx].allocated != 1)
	{
		return (ebadhandle);
	}

	/*	Check that the processor is a NIOS II processor.
	*/
	if (cardpx[handlepx].processor != PROC_NIOS)
	{
		return (ewrongprocessor);
	}

	/*	Get the module time.
	*/
//	*moduleTime = ((unsigned int)(cardpx[handlepx].exc_bd_dwnld->moduleTimeHi) << 16)
//		+ (unsigned int)(cardpx[handlepx].exc_bd_dwnld->moduleTimeLo);
	*moduleTime = ((unsigned int)(RWORD_D2H( cardpx[handlepx].exc_bd_dwnld->moduleTimeHi)) << 16)
		+ (unsigned int)(RWORD_D2H( cardpx[handlepx].exc_bd_dwnld->moduleTimeLo));

		/*	All done....
	*/
	return (0);

}  /* end function Get_ModuleTime_Px */

/*
   function Parse_CommandWord_Px

Description: Take a command word, and parse it into its components

Input:		cmdWord - a 1553 command word

Output:		RT - the RT number
			SA - the SubAddress
			direction -	1 for Transmit, 0 for Receive
			wordCount - number of words in the message; for Mode Codes, this is the mode code
			RTid - the MSB 11 bits o the command word, comprising RT-direction-SA

Return value:	none

*/
void borland_dll Parse_CommandWord_Px (usint cmdWord, usint *RT, usint *SA, usint *direction, usint *wordCount, usint *RTid)
{
	*RT = cmdWord >> 11;
	*SA = (cmdWord & 0x03E0) >> 5;
	*direction = (cmdWord & 0x0400) >> 10;
	*wordCount = cmdWord & 0x001F;
	*RTid = cmdWord >> 5;
}

/* written March 2007

Get_Next_Mon_Message_Px

Description: Reads the message block following the block read in the previous
call to Get_Next_Mon_Message_Px.

This function merges the functionality of Get_Next_Message_Px (Monitor mode) with
that of Get_Next_Message_BCM_Px and Get_Next_Message_RTM_Px (Internal Concurrent
Monitor in BC or RT modes).

The first call to Get_Next_Mon_Message_Px will return block 0.

Note:
(a) Currently, this function is supported in the PxIII (NiosII-based) modules.
(b) If the next block does not contain a new message an error will be
returned.

Input parameters: none

Output parameters:
		msgptr   Pointer to the structure defined below in which to return the message
		  struct MONMSG {
			int msgstatus; Status word containing the flags as defined in the corresponding
						function for each mode [ see Get_Next_Message_Px in Monitor mode,
						Get_Next_Message_BCM_Px and Get_Next_Message_RTM_Px (Internal Concurrent
						Monitor in BC or RT modes) ]
			unsigned long elapsedtime; The timetag associated with the
					     message with a precision of 4 microseconds per bit
			unsigned int words [36];   A pointer to an array of 1553
					     words in the sequence they were received over the bus
		  }

        msg_counter		A 32-bit serial message count associated with this message.

Return values:
		block number	If successful
		emode			If board is not configured as one of: BC/Concurrent RT mode, RT mode, Sequential Monitor mode
		enomsg			If there is no message to return
		ewrongprocessor	If not PxIII module
		ebadhandle		If handle parameter is out of range or not allocated
		eoverrun		If message was overwritten in middle of being read here
		func_invalid	If the module is an M4KMMSI module

Change history:
-----------------
8-Sep-2008	(YYB)
-	added DMA support

10-Dec-2008	(YYB)
- corrected a pointer assignment when DMA is not available
	(the sense of an "if" test was backwards)

*/

int borland_dll Get_Next_Mon_Message_Px (int handlepx, struct MONMSG *msgptr, unsigned long *msg_counter)
{
	int rt2rtFlag;
	struct MONMSGPACKED msgCopy1packed;
	struct MONMSGPACKED msgCopy2packed;

	usint * pDpram = NULL;
	int wordLoopCounter;

	usint * pDmaCopy;
	struct MONMSGPACKED  * pMsgDmaCopyPacked;
	int		dmaCopyIsValid;
	int	status = Successful_Completion_Exc;


	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

#ifdef MMSI
	// There is no internal concurrent monitor in MMSI RT_Mode
	if (cardpx[handlepx].board_config == RT_MODE)
		return func_invalid;
#endif

	if (cardpx[handlepx].processor != PROC_NIOS) return (ewrongprocessor);

	/* check if board is configured as an RT or BC or MON*/
//	if ((cardpx[handlepx].exc_bd_rt->board_config	  != RT_MODE)		&&
//		(cardpx[handlepx].exc_bd_bc->board_config	  != BC_RT_MODE)	&&
//		(cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK))
	if ((cardpx[handlepx].board_config != RT_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE) &&
		(cardpx[handlepx].board_config != MON_BLOCK))
	{
		return (emode);
	}

//	if 	(cardpx[handlepx].exc_bd_monseq->board_config == MON_BLOCK)
	if (cardpx[handlepx].board_config == MON_BLOCK)
	{

		if (cardpx[handlepx].irigModeEnabled == TRUE)
#ifdef MMSI
			return eIrigTimetagSet_MMSI;
#else
			return eIrigTimetagSet_PX;
#endif

		/* handle wraparound of buffer */
		cardpx[handlepx].nextblock %= cardpx[handlepx].lastblock;

		/* 0 - 199 and for non expanded mode */
		if (cardpx[handlepx].nextblock < EXPANDED_MON_BLOCK_NUM_1) 
			pDpram = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block[cardpx[handlepx].nextblock * MSGSIZE]);
		/* 200 - 352 */
		else if ((cardpx[handlepx].nextblock >= EXPANDED_MON_BLOCK_NUM_1) && (cardpx[handlepx].nextblock < EXPANDED_MON_BLOCK_NUM_2))
			pDpram = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block2[(cardpx[handlepx].nextblock - EXPANDED_MON_BLOCK_NUM_1) * MSGSIZE]);
		/* 353 - 400 */
		else if ((cardpx[handlepx].nextblock >= EXPANDED_MON_BLOCK_NUM_2) && (cardpx[handlepx].nextblock < EXPANDED_MON_BLOCK_NUM_3))
			pDpram = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block3[(cardpx[handlepx].nextblock - EXPANDED_MON_BLOCK_NUM_2) * MSGSIZE]);
		/* 400 - 800 (Expanded Monitor) */
		else if (!cardpx[handlepx].isEnhancedMon) 
			pDpram = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block4[(cardpx[handlepx].nextblock - EXPANDED_MON_BLOCK_NUM_3) * MSGSIZE]);

		rt2rtFlag = RT2RT_MSG;
	}
	else /* concMon BC and concMon RT */
	{
		/* handle wraparound of buffer */
		cardpx[handlepx].next_MON_entry %= cardpx[handlepx].cmon_stack_size;
		pDpram = (usint *)&(cardpx[handlepx].cmon_stack[cardpx[handlepx].next_MON_entry]);
		rt2rtFlag = RT2RT_MSG_CONCM;
	}

	// UNet takes advantage of this function too. Though initially written for DMA, it was
	// extended and generalized to cover devices/modules that can support multi-message reads.
	status = Get_DmaMonBlockCopy_Px(handlepx, pDpram, &pDmaCopy, &dmaCopyIsValid);

	// dmaCopyIsValid does not indicate that there is a message in the dmacache;
	// All it means is that the DMA/UNet read did not result in an error. So we still need to check for a valid message
	if (dmaCopyIsValid == TRUE)
	// Extract a message from the "DMA" cache (which was filled by UNet or DMA multi-message reads).
	{
		pMsgDmaCopyPacked = (struct MONMSGPACKED *) pDmaCopy;

		// check the "DMA" cache for no message
		if ((pMsgDmaCopyPacked->msgstatus & END_OF_MSG) != END_OF_MSG)
			return enomsg;

		msgptr->msgstatus = pMsgDmaCopyPacked->msgstatus;
		msgptr->elapsedtime = pMsgDmaCopyPacked->elapsedtime;
		//instead of 36 we could figure out the correct number based on the command word and Px status
		for (wordLoopCounter = 0; wordLoopCounter < 36; wordLoopCounter++) {
			msgptr->words[wordLoopCounter] = pMsgDmaCopyPacked->words[wordLoopCounter];
		}
		msgptr->msgCounter = pMsgDmaCopyPacked->msgCounter;
	}

	else
	// Just get a single message directly from the module.
	{
		// zero out the status word before beginning
		msgCopy1packed.msgstatus = 0;

		b2drmReadWordBufferFromDpr( (DWORD_PTR) pDpram,
										(unsigned short *) &msgCopy1packed,
										sizeof(struct MONMSGPACKED) / sizeof(unsigned short),
										"Get_Next_Mon_Message_Px",
										B2DRMHANDLE );

		// help speed up message reads when there is nothing to read; if the EOM flag is not set, then there is no reason to re-read the message just yet
		if ((msgCopy1packed.msgstatus & END_OF_MSG) != END_OF_MSG)
			return enomsg;

		//get a second copy of the header so we can check if any changes were made while we were in mid read
		b2drmReadWordBufferFromDpr( (DWORD_PTR) pDpram,
									(unsigned short *) &msgCopy2packed,
									MONMSG_HEADER_SIZE / sizeof(unsigned short),
									"Get_Next_Mon_Message_Px",
									B2DRMHANDLE );

		if (msgCopy1packed.msgstatus != msgCopy2packed.msgstatus)
			return eoverrunEOMflag;
		if (msgCopy1packed.elapsedtime != msgCopy2packed.elapsedtime)
			return eoverrunTtag;

		msgptr->msgstatus = msgCopy1packed.msgstatus;
		msgptr->elapsedtime = msgCopy1packed.elapsedtime;
		//instead of 36 we could figure out the correct number based on the command word and Px status
		for (wordLoopCounter = 0; wordLoopCounter < 36; wordLoopCounter++) {
			msgptr->words[wordLoopCounter] = msgCopy1packed.words[wordLoopCounter];
		}
		msgptr->msgCounter = msgCopy1packed.msgCounter;

		// after reading, set the EOM bit (ON THE MODULE) to 0
//		*pDpram = msgptr->msgstatus & (usint)(~END_OF_MSG);
		WWORD_H2D( pDpram[0], (usint)(msgptr->msgstatus & (usint)(~END_OF_MSG)) );

	} // End 'Just get a single message directly from the module.'

	// If the timetag difference is significant, assume a wrap-around occured.
	// If the timetag difference is NOT significant, then what probably happened is the messages
	// wrapped around and overwrote some of the messages that we are currently retriving.
	// (Hence we could look at this as a few super-new messages overwrote a few of the new messages.)
	// When we finish the super-new messages, and continue on to the new messages, the timetag will
	// appear to go backwards a NOT significant anount.
	// If this message wrap-around occurs, we will return "eoverrunTtag" to alert the user.


	if (msgptr->elapsedtime < cardpx[handlepx].lastmsgttag)
	{
		if (cardpx[handlepx].lastmsgttag - msgptr->elapsedtime < 0x10000000)
		{
			/* added 26oct06, as per request from Holland */
			if (!cardpx[handlepx].ignoreoverrun)
				return eoverrunTtag;
		}
		/* otherwise, it means that the timetag reached its maximum value and wrapped to 0 */
	}
	cardpx[handlepx].lastmsgttag = msgptr->elapsedtime;

	/*
		For Mon Mode Only -
		if we are waiting for a trigger, a valid message exists only if the
		TRIGGER_FOUND bit is set in addition to the END_OF_MSG bit.
	*/

//	if ((cardpx[handlepx].exc_bd_monseq->board_config == MON_BLOCK) &&
	if ( (cardpx[handlepx].board_config == MON_BLOCK) &&
		(cardpx[handlepx].wait_for_trigger) )
	{
		if (msgptr->msgstatus & TRIGGER_FOUND)
		{
//			if (cardpx[handlepx].exc_bd_monseq->trig_ctl & STORE_AFTER)
			if ( RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->trig_ctl ) & STORE_AFTER )
				cardpx[handlepx].wait_for_trigger = 0;
		}
		else
			return (enomsg);
	}

#ifdef MMSI
	/* if LINK mode, and MSW indicates wrong RT address, and there are no other error bits, then this is not an error; turn off ERR bit 00 */
	if (cardpx[handlepx].hubmode_MMSI == HUB_LINK)
	{
		if ((msgptr->msgstatus & BAD_RT_ADDR) == BAD_RT_ADDR)
		{
			if ((msgptr->msgstatus & ERROR_BITS_NO_MON_ADDR) == 0)
			{
				msgptr->msgstatus = msgptr->msgstatus & ~0x1; /* mask out Error bit */
			}
		}
	}
#endif

//	if (cardpx[handlepx].exc_bd_monseq->board_config == MON_BLOCK)
	if (cardpx[handlepx].board_config == MON_BLOCK)
	{
		// if the msgCounter wrapped around, reread the wrapCounter, otherwise just used the cached value
		if (msgptr->msgCounter < cardpx[handlepx].prevMonMsgCount)
			cardpx[handlepx].monWrapCount = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->monWrapCount );

//		*msg_counter = ((cardpx[handlepx].exc_bd_monseq->monWrapCount * 0x10000) + (*(blkptr+39)));
//		*msg_counter = ((RWORD_D2H( cardpx[handlepx].exc_bd_monseq->monWrapCount ) * 0x10000) + msgptr->msgCounter ); // don't need another read for (blkptr+39) since we have it in msgptr->msgCounter
		*msg_counter = ((cardpx[handlepx].monWrapCount * 0x10000) + msgptr->msgCounter );

		// update the previous message count
		cardpx[handlepx].prevMonMsgCount = msgptr->msgCounter;

		cardpx[handlepx].nextblock++;
		return (cardpx[handlepx].nextblock - 1);
	}
	else
	{
		// if the msgCounter wrapped around, reread the wrapCounter, otherwise just used the cached value
		if (msgptr->msgCounter < cardpx[handlepx].prevMonMsgCount)
			cardpx[handlepx].monWrapCount = RWORD_D2H( cardpx[handlepx].exc_bd_bc->concMonWrapCount );

//		*msg_counter = ((cardpx[handlepx].exc_bd_bc->concMonWrapCount * 0x10000) + (*(blkptr+39)));
//		*msg_counter = ((RWORD_D2H( cardpx[handlepx].exc_bd_bc->concMonWrapCount ) * 0x10000) + msgptr->msgCounter );  // don't need another read for (blkptr+39) since we have it in msgptr->msgCounter
		*msg_counter = ((cardpx[handlepx].monWrapCount * 0x10000) + msgptr->msgCounter );

		// update the previous message count
		cardpx[handlepx].prevMonMsgCount = msgptr->msgCounter;

		cardpx[handlepx].next_MON_entry++;
		return (cardpx[handlepx].next_MON_entry - 1);
	}
}

/** ------------------------------------------------------------------------------------
Get_UseDmaIfAvailable_Px()

Description: Returns the indicator as to whether DMA should be used (for data transfer
between the host memory and the Px module memory) if DMA is available.

Applies to Modes: Mode Independent

Input parameters:
	handlepx		the handle obtained from Init_Module_Px() when
				the node was initialized.

Output parameters:
	pUseDmaFlag	(pointer to) a flag indicating if DMA memory
				access to DPRAM is available.
				Valid values are:
					ENABLE
					DISABLE

Return values:
	Successful_Completion_Exc
	ebadhandle

Design/Notes:

Coding history:
-----------------
1-Sep-2008	(YYB)
-	original coding

11-Sep-2008	(YYB)
-	corrected the reference to the instance variable name.

*/

int borland_dll Get_UseDmaIfAvailable_Px(int handlepx, int * pUseDmaFlag)
{
	/* Check that the module handle is valid. Don't bother checking the mode because it
		doesn't matter - the reads and writes will only happen if the mode is correct. */
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
		
	/* Get the output parameter. */
	if(cardpx[handlepx].useDmaIfAvailable == TRUE)
		*pUseDmaFlag = ENABLE;
	else
		*pUseDmaFlag = DISABLE;
	
	return Successful_Completion_Exc;

}  /* -----  end of Get_UseDmaIfAvailable_Px()  ----- */


// UMMR = UNet Multi-Message-Read
int borland_dll Get_UseUMMRIfAvailable_Px(int handlepx, int * pUseUMMRFlag)
{
	/* Check that the module handle is valid. Don't bother checking the mode because it
		doesn't matter - the reads and writes will only happen if the mode is correct. */
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* Get the output parameter. */
	if(cardpx[handlepx].useUnetMultiMsgReadIfAvailable == TRUE)
		*pUseUMMRFlag = ENABLE;
	else
		*pUseUMMRFlag = DISABLE;

	return Successful_Completion_Exc;

}  /* -----  end of Get_UseUMMRIfAvailable_Px()  ----- */





/* Not recommended for users - get pointer to beginning of "messages" area. Not documented*/
int borland_dll Get_MsgArrayPtr_Px(int handlepx, usint **msgarrayptr)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	*msgarrayptr = (usint *)cardpx[handlepx].msgalloc;

	return 0;
}

#ifdef DMA_SUPPORTED
/** ------------------------------------------------------------------------------------
Get_DmaMonBlockCopy_Px()

Description: This function gets an absolute pointer (as opposed to a DPRAM offset)
to host memory that contains a DMA-read copy of memory at the specified (DPRAM)
pointer (not offset).  If there is no valid copy, the function attempts another DMA-Read
to refill its DMA-clone buffer.

Applies to Modes: RT_MODE, BC_RT_MODE, MON_BLOCK

Available while running: Yes

Input parameters:
	int handlepx		the handle obtained from Init_Module_Px() when
					the module was initialized.
	usint * pDpram		an absolute pointer (as opposed to an offset) to the
					desired block in DPRAM.

Output parameters:
	usint ** ppDmaCopy	a pointer to an absolute pointer to a location in
						(an array of usints in) host memory that contains a
						DMA-read copy of the desired message block
	int * pDmaCopyIsValid	flag indicating if there is a valid copy of the
						DPRAM message block in the host memory (array)
	
Return values:
	Successful_Completion_Exc	if the routine completed successfully
	edmareadfail				if the DMA read operation failed

Design/Notes:

Coding history:
-----------------
8-Sep-2008	(YYB)
-	original coding

8-Dec-2008	(YYB)
- corrected the calculation of cardpx[handlepx].dmaBuffer.endDpramOffset

9-Mar-2009	(YYB)
- corrected the detection of RT-mode Expanded-Data-Block mode. Previously
	the code checked for firmware support of Expanded-Data-Block mode, rather
	than checking to see that it was actually turned on.

APR-MAY-2014 (DK & YYB)
- added suport for multimessage reads from UNet devices

*/

int Get_DmaMonBlockCopy_Px(int handlepx, usint * pDpram, usint ** ppDmaCopy, int * pDmaCopyIsValid)
{
	int	status = Successful_Completion_Exc;
	DWORD_PTR	datablockOffset;
	int i;
	int	numBytesToRead;
	int maxBlocksToRead;
	struct MONMSGPACKED  * pOrigMsgInDPRAM;
	struct MONMSGPACKED  * pMsgsInCache;

	/* Put in the obligatory input checking, even though this routine is internal. */
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_rt->board_config != RT_MODE) &&
//		(cardpx[handlepx].exc_bd_rt->board_config != BC_RT_MODE) &&
//		(cardpx[handlepx].exc_bd_rt->board_config != MON_BLOCK))
	if ((cardpx[handlepx].board_config != RT_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE) &&
		(cardpx[handlepx].board_config != MON_BLOCK))

        return (emode);

	// Read the test as:
	// If ((we're not going to do a DMA read) -AND- (we're not going to do a UNet multi-message read)) then skidaddle!

	// i.e.

	// If ((there is no DMA) or (we're not to use DMA)) -AND- ((device is not a UNet) or (we're not reading Multi-Msgs))
	// then we don't belong here; so skidaddle

	if (	( (cardpx[handlepx].dmaAvailable == FALSE) || (cardpx[handlepx].useDmaIfAvailable == FALSE) )	&&
			( (cardpx[handlepx].isUNet == FALSE) || (cardpx[handlepx].isUnetMultiMsgReadSupported == FALSE) || (cardpx[handlepx].useUnetMultiMsgReadIfAvailable == FALSE) )	)
	{
		*pDmaCopyIsValid = FALSE;
		*ppDmaCopy = NULL;
		return status;
	}

	/* translate the pDpram into an offset from the begining of the card */
	datablockOffset = ((DWORD_PTR)pDpram - (DWORD_PTR)(cardpx[handlepx].exc_bd_bc));

	/* First check if there is a DPRAM copy, next check that if we already had a block, it's the block we want;
		finally check if we have an unread message at that location. */
	if (cardpx[handlepx].dmaBuffer.dmaCopyIsValid == TRUE)
	{
		if ((datablockOffset >= cardpx[handlepx].dmaBuffer.beginDpramOffset) && (datablockOffset < cardpx[handlepx].dmaBuffer.endDpramOffset))
		{
			*ppDmaCopy = (usint *) ((DWORD_PTR) &(cardpx[handlepx].dmaBuffer.msgBlockData[0]) + datablockOffset - (DWORD_PTR) cardpx[handlepx].dmaBuffer.beginDpramOffset);
			if ((**ppDmaCopy & END_OF_MSG) == END_OF_MSG)
			{
				*pDmaCopyIsValid = TRUE;
				return status;
			}
		}
	}

	/* If we are here, then we did not have a useable copy of the DPRAM block in the dmaBuffer,
		so go get a new one */
	cardpx[handlepx].dmaBuffer.beginDpramOffset = datablockOffset;

	/* Handle the BC_RT mode - which means we're looking to get
		a copy of the concurrent monitor space at 0x8000 - 0xFFFF */
//	if (cardpx[handlepx].exc_bd_rt->board_config == BC_RT_MODE)
	if (cardpx[handlepx].board_config == BC_RT_MODE)
	{
		if (cardpx[handlepx].isUNet == FALSE) // NOT a UNet device, hence we'll be doing DMA
		{
			cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON) - 1), END_CMON_BANK_BCRT);
		}
		else
		{
			if (b2drmIsUSB(B2DRMHANDLE))
				maxBlocksToRead = MAX_NUMBER_BLOCKS_IN_USB_READ;
			else
				maxBlocksToRead = MAX_NUMBER_BLOCKS_IN_ETH_READ;

			cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (maxBlocksToRead * MSGSIZEBYTES_MON) - 1), END_CMON_BANK_BCRT);

		} // end UNet device
	}

	/* Handle the RT mode - which means we're looking to get a copy of the
		concurrent monitor space at 0x8000 - 0xFFCF (default concurrent
		monitor space) or 0x8000 - 0xBFBF (when using the Expanded Data
		Block option. */
//	else if (cardpx[handlepx].exc_bd_rt->board_config == RT_MODE)
	else if (cardpx[handlepx].board_config == RT_MODE)
	{
		if (cardpx[handlepx].isUNet == FALSE) // NOT a UNet device, hence we'll be doing DMA
		{
			if ((cardpx[handlepx].exc_bd_rt->moduleFunction & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE)
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON) - 1), END_CMON_BANK_RT_EXP);

			else /* module is not in expanded-block mode */
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON - 1), END_CMON_BANK_BCRT);
		}
		else
		{
			if (b2drmIsUSB(B2DRMHANDLE))
				maxBlocksToRead = MAX_NUMBER_BLOCKS_IN_USB_READ;
			else
				maxBlocksToRead = MAX_NUMBER_BLOCKS_IN_ETH_READ;

//			if ((cardpx[handlepx].exc_bd_rt->moduleFunction & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE)
			if (( RWORD_D2H(cardpx[handlepx].exc_bd_rt->moduleFunction) & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE)
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (maxBlocksToRead * MSGSIZEBYTES_MON) - 1), END_CMON_BANK_RT_EXP);

			else /* module is not in expanded-block mode */
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + maxBlocksToRead * MSGSIZEBYTES_MON - 1), END_CMON_BANK_BCRT);

		} // end UNet device
	}

	/* The last case is that we're dealing with fixed block regular monitor mode, with blocks distributed in four different "banks". */

//	else if (cardpx[handlepx].exc_bd_rt->board_config == MON_BLOCK)
	else if (cardpx[handlepx].board_config == MON_BLOCK)
	{
		if (cardpx[handlepx].isUNet == FALSE) // NOT a UNet device, hence we'll be doing DMA
		{
			/* blocks 0 - 199 */
			if (cardpx[handlepx].dmaBuffer.beginDpramOffset < END_MON_BANK_1)
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON) - 1), END_MON_BANK_1);

			/* 200 - 352 */
			else if (cardpx[handlepx].dmaBuffer.beginDpramOffset < END_MON_BANK_2)
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON) - 1), END_MON_BANK_2);

			/* 353 - 400 */
			else if (cardpx[handlepx].dmaBuffer.beginDpramOffset < END_MON_BANK_3)
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON) - 1), END_MON_BANK_3);

			/* 400 - 800 (Expanded Monitor, but not Enhanced Monitor) */
			else if (!cardpx[handlepx].isEnhancedMon) 
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON) - 1), END_MON_BANK_4);

		} // end non-UNet device

		else // IS a UNet device
		{
			if (b2drmIsUSB(B2DRMHANDLE))
				maxBlocksToRead = MAX_NUMBER_BLOCKS_IN_USB_READ;
			else
				maxBlocksToRead = MAX_NUMBER_BLOCKS_IN_ETH_READ;

			/* blocks 0 - 199 */
			if (cardpx[handlepx].dmaBuffer.beginDpramOffset < END_MON_BANK_1)
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (maxBlocksToRead * MSGSIZEBYTES_MON) - 1), END_MON_BANK_1);

			/* 200 - 352 */
			else if (cardpx[handlepx].dmaBuffer.beginDpramOffset < END_MON_BANK_2)
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (maxBlocksToRead * MSGSIZEBYTES_MON) - 1), END_MON_BANK_2);

			/* 353 - 400 */
			else if (cardpx[handlepx].dmaBuffer.beginDpramOffset < END_MON_BANK_3)
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (maxBlocksToRead * MSGSIZEBYTES_MON) - 1), END_MON_BANK_3);

			/* 400 - 800 (Expanded Monitor, but not Enhanced Monitor) */
			else if (!cardpx[handlepx].isEnhancedMon) 
				cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (maxBlocksToRead * MSGSIZEBYTES_MON) - 1), END_MON_BANK_4);

		} // end UNet device
	}

	/* Based on the begin and end, calculate the size of the block to read; then do the read. */
	numBytesToRead = (int)(cardpx[handlepx].dmaBuffer.endDpramOffset - cardpx[handlepx].dmaBuffer.beginDpramOffset + 1);

	if (cardpx[handlepx].isUNet == FALSE) // NOT a UNet device, hence we'll be doing DMA
	{

#ifdef DMA_SUPPORTED
		status = PerformDMARead(cardpx[handlepx].card_addr, cardpx[handlepx].module,
			&(cardpx[handlepx].dmaBuffer.msgBlockData[0]), numBytesToRead, (void *) datablockOffset);
#else
		*pDmaCopyIsValid = FALSE;
		*ppDmaCopy = NULL;
		status = edmanotsupported;
		return status;
#endif

		// Scan the DMA buffer for EOMs and clear the EOMs on the actual board.
		pMsgsInCache = (struct MONMSGPACKED *) &(cardpx[handlepx].dmaBuffer.msgBlockData[0]);
		pOrigMsgInDPRAM = (struct MONMSGPACKED *) ((char *)(cardpx[handlepx].exc_bd_bc) + datablockOffset);
		for (i=0; i<(numBytesToRead/(int)sizeof(struct MONMSGPACKED)); i++)
		{
			if (pMsgsInCache[i].msgstatus & (usint)(END_OF_MSG))
			{
				pOrigMsgInDPRAM[i].msgstatus &= (usint)(~END_OF_MSG);
			}
			else
				break;
		}

	}

	else // IS a UNet device
	{
		// This function is a void, but we initialized status to success above.
		b2drmReadWordBufferFromDpr_AndClearEOMStatus (
			(DWORD_PTR) pDpram,
			(unsigned short *) &cardpx[handlepx].dmaBuffer.msgBlockData[0],
			numBytesToRead/2,
			"Get_DmaMonBlockCopy_Px",
			B2DRMHANDLE );
	}

	/* Fill out the dmaBuffer struct and the output parameters, based on the success of the DMA read. */
	if (status == Successful_Completion_Exc)
	{
		cardpx[handlepx].dmaBuffer.dmaCopyIsValid = TRUE;
		*pDmaCopyIsValid = TRUE;
		*ppDmaCopy = (usint *) ((void *) &(cardpx[handlepx].dmaBuffer.msgBlockData[0]));
	}

	else
	{
		cardpx[handlepx].dmaBuffer.dmaCopyIsValid = FALSE;
		*pDmaCopyIsValid = FALSE;
		*ppDmaCopy = NULL;
		status = edmareadfail;
	}

	return status;
}

int Get_DmaMonIRIGBlockCopy_Px(int handlepx, usint * pDpram, usint ** ppDmaCopy, int * pDmaCopyIsValid)
{
	int	status = Successful_Completion_Exc;
	DWORD_PTR	datablockOffset;
	int	numBytesToRead;

	/* Put in the obligatory input checking, even though this routine is internal. */
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].irigTtagModeSupported == FALSE)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

//	if (cardpx[handlepx].exc_bd_rt->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	/* If there is no DMA or we're not to use it, then there is nothing to talk about */
	if ((cardpx[handlepx].dmaAvailable == FALSE) || (cardpx[handlepx].useDmaIfAvailable == FALSE))
	{
		*pDmaCopyIsValid = FALSE;
		*ppDmaCopy = NULL;
		return status;
	}

	/* translate the pDpram into an offset from the begining of the card */
	datablockOffset = ((DWORD_PTR)pDpram - (DWORD_PTR)(cardpx[handlepx].exc_bd_bc));

	/* First check if there is a DPRAM copy, next check that if we already had a block, it's the block we want;
		finally check if we have an unread message at that location. */
	if (cardpx[handlepx].dmaBuffer.dmaCopyIsValid == TRUE)
	{
		if ((datablockOffset >= cardpx[handlepx].dmaBuffer.beginDpramOffset) && (datablockOffset < cardpx[handlepx].dmaBuffer.endDpramOffset))
		{
			*ppDmaCopy = (usint *) ((DWORD_PTR) &(cardpx[handlepx].dmaBuffer.msgBlockData[0]) + datablockOffset - (DWORD_PTR) cardpx[handlepx].dmaBuffer.beginDpramOffset);
			if ((**ppDmaCopy & END_OF_MSG) == END_OF_MSG)
			{
				*pDmaCopyIsValid = TRUE;
				return status;
			}
		}
	}

	/* If we are here, then we did not have a useable copy of the DPRAM block in the dmaBuffer,
		so go get a new one */
	cardpx[handlepx].dmaBuffer.beginDpramOffset = datablockOffset;

	/* Handle the BC_RT mode - which means we're looking to get
		a copy of the concurrent monitor space at 0x8000 - 0xFFFF */
//	if (cardpx[handlepx].exc_bd_rt->board_config == MON_BLOCK)
	if (cardpx[handlepx].board_config == MON_BLOCK)
	{

		/* blocks 0 - 199 */
		if (cardpx[handlepx].dmaBuffer.beginDpramOffset < END_MON_BANK_1)
			cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON_IRIG) - 1), END_MON_BANK_1);

		/* 200 - 352 */
		else if (cardpx[handlepx].dmaBuffer.beginDpramOffset < END_MON_BANK_2)
			cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON_IRIG) - 1), END_MON_BANK_2);

		/* 353 - 400 */
		else if (cardpx[handlepx].dmaBuffer.beginDpramOffset < END_MON_BANK_3)
			cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON_IRIG) - 1), END_MON_BANK_3);

		/* 400 - 800 (Expanded Monitor, but not Enhanced Monitor) */
		else if (!cardpx[handlepx].isEnhancedMon) 
			cardpx[handlepx].dmaBuffer.endDpramOffset = min((cardpx[handlepx].dmaBuffer.beginDpramOffset + (MAX_NUMBER_BLOCKS_IN_DMA_READ * MSGSIZEBYTES_MON_IRIG) - 1), END_MON_BANK_4);
	}

	/* Based on the begin and end, calculate the size of the block to read; then do the read. */
	numBytesToRead = (int)(cardpx[handlepx].dmaBuffer.endDpramOffset - cardpx[handlepx].dmaBuffer.beginDpramOffset + 1);
	status = PerformDMARead(cardpx[handlepx].card_addr, cardpx[handlepx].module,
		&(cardpx[handlepx].dmaBuffer.msgBlockData[0]), numBytesToRead, (void *) datablockOffset);

	/* Fill out the dmaBuffer struct and the output parameters, based on the success of the DMA read. */
	if (status == Successful_Completion_Exc)
	{
		cardpx[handlepx].dmaBuffer.dmaCopyIsValid = TRUE;
		*pDmaCopyIsValid = TRUE;
		*ppDmaCopy = (usint *) ((void *) &(cardpx[handlepx].dmaBuffer.msgBlockData[0]));
	}

	else
	{
		cardpx[handlepx].dmaBuffer.dmaCopyIsValid = FALSE;
		*pDmaCopyIsValid = FALSE;
		*ppDmaCopy = NULL;
		status = edmareadfail;
	}

	return status;
}
#endif /* End of DMA_SUPPORTED */

/*
Get_Message_Count_Px

  Description: Returns the running message count
  Input parameters:  none
  Output parameters: msgCount - how many messages were transmitted or received
  Return values: 0
*/
int borland_dll Get_Message_Count_Px (int handlepx, unsigned int *msgCount)
{
	usint lclMsgCount[2];
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	*msgCount = ((unsigned int)(cardpx[handlepx].exc_bd_monseq->msgCount[0])<<16) + cardpx[handlepx].exc_bd_monseq->msgCount[1];
//	*msgCount = ((unsigned int)(RWORD_D2H( cardpx[handlepx].exc_bd_monseq->msgCount[0]))<<16) + RWORD_D2H( cardpx[handlepx].exc_bd_monseq->msgCount[1]);

	b2drmReadWordBufferFromDpr( (DWORD_PTR) &cardpx[handlepx].exc_bd_monseq->msgCount[0],
			(unsigned short *) &lclMsgCount,
			2,
			"Get_Message_Count_Px",
			B2DRMHANDLE );
	//firmware update the high before the low so moving from 1ffff to 20000 has an interim value of 2ffff for a few nanoseconds
	//doing a second read if the low is ffff takes care of his rare anamoly
	if (lclMsgCount[1] == 0xffff)
		b2drmReadWordBufferFromDpr( (DWORD_PTR) &cardpx[handlepx].exc_bd_monseq->msgCount[0],
			(unsigned short *) &lclMsgCount,
			2,
			"Get_Message_Count_Px",
			B2DRMHANDLE );

	*msgCount = (lclMsgCount[0] << 16) + lclMsgCount[1];
	return 0;
}

/*
Is_Expanded_Mode_Supported_Px

Description: For the given BC/RT/Mon mode, does this module support Expanded mode 
Output Parameter: 	isFeatureSupported	TRUE/FALSE
Return Values:	func_invalid, ebadhandle, emode, 0 (success)

NOTE: Usage: Call this function to determine if the feature is supported.
If TRUE, then you can call the function Set_Expanded_Block_Mode_Px .
*/

int borland_dll Is_Expanded_Mode_Supported_Px(int handlepx, int *isFeatureSupported)
{
#ifdef MMSI
	return (func_invalid);
#else

	*isFeatureSupported = FALSE;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* RT Mode */
//	if (cardpx[handlepx].exc_bd_rt->board_config == RT_MODE)
	if (cardpx[handlepx].board_config == RT_MODE)

	{
		*isFeatureSupported = cardpx[handlepx].expandedRTblockFW;
	}

	/* Sequential Monitor Mode */
//	else if (cardpx[handlepx].exc_bd_monseq->board_config == MON_BLOCK) 
	else if (cardpx[handlepx].board_config == MON_BLOCK)
	{
		*isFeatureSupported = cardpx[handlepx].expandedMONblockFW;
	}

	/* BC Mode */
//	else if (cardpx[handlepx].exc_bd_monseq->board_config == BC_RT_MODE) 
	else if (cardpx[handlepx].board_config == BC_RT_MODE) 
	{
		*isFeatureSupported = cardpx[handlepx].expandedBCblockFW;
	}

	else
		return (emode);

	return 0;
#endif
}

/* written May 2015 by David Koppel

Is_IrigSignal_Present_Px

Description: Returns indication telling us if the Irig signal is present on the card

Note:
(a) This function is supported only in the PxIII (NiosII-based) modules, firmware released post Feb 2015.

Input parameters:
	handlepx		Module handle returned from the earlier call to Init_Module_Px

Output parameters:
	isIrigSignalPresent   1=Irig signal is present, 0=Irig signal is not present

Return values:
		0			If successful
		ebadhandle		If handle parameter is out of range or not allocated
		ewrongprocessor	If module processor is not (PxIII) Nios-based

		efeature_not_supported_PX	If IrigB Timetag feature is not supported on this module
		efeature_not_supported_MMSI	If IrigB Timetag feature is not supported on this module

*/

int borland_dll Is_IrigSignal_Present_Px (int handlepx, unsigned short *isIrigSignalPresent)
{
	unsigned short int i, moduleTime[4];

	// Verify that the value of module's handle is valid (i.e. between zero and NUMCARDS-1).
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return (ebadhandle);

	// Verify that the module's handle is allocated/initialized.
	if (cardpx[handlepx].allocated != 1) return (ebadhandle);

	// Check that the processor is a NIOS II processor.
	if (cardpx[handlepx].processor != PROC_NIOS)	return (ewrongprocessor);

	// Check that the firmware supports the IrigB timestamp feature
	if (cardpx[handlepx].irigTtagModeSupported != TRUE)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

	// Get the module Irig time; an array of 4 short ints; days; hours + mins; secs + usec; more usec
	for (i=0; i<4; i++) 

//		moduleTime[3-i] = cardpx[handlepx].exc_bd_bc->IRIGttag[i];
		moduleTime[3-i] = RWORD_D2H( cardpx[handlepx].exc_bd_bc->IRIGttag[i] );

		// read 0th element first, to freeze the ttag
	
	// if the hi bit of the Irig time is set to 1, then no Irig signal is present; if 0, then Irig signal is present
	//*isIrigSignalPresent = ((moduleTime[0] >> 15) == 0);
	*isIrigSignalPresent = ((moduleTime[0] & 0x8000) == 0);
	return (0);
}  // end function Is_IrigSignal_Present_Px

/* written April 2015 by David Koppel

Get_ModuleIrigTime_BCD_Px

Description: Returns the current Module level IrigB timestamp, in BCD representation.

Note:
(a) This function is supported only in the PxIII (NiosII-based) modules, firmware released post Feb 2015.

Input parameters:
	handlepx		Module handle returned from the earlier call to Init_Module_Px

Output parameters:
	moduleTime   An array of four words, containing the IrigB timestamp in BCD
		days		(bits 11-00, three BCD digits; bit 15=1 means "No Irig signal")
		hours + mins	(hours: bits 15-08, two BCD digits; mins: bits 07-00, two BCD digits)
		secs + usec	(secs: bits 15-08, two BCD digits; usecs: bits 07-00, two hi BCD digits)
		more usec	(usecs: bits 15-00, four low BCD digits)

Return values:
		0			If successful
		ebadhandle		If handle parameter is out of range or not allocated
		ewrongprocessor	If module processor is not (PxIII) Nios-based
		efeature_not_supported_PX	If IrigB Timetag feature is not supported on this module
		efeature_not_supported_MMSI	If IrigB Timetag feature is not supported on this module

timetag words are in BCD; see function Get_ModuleIrigTime_Px for timetag words in decimal representation

*/

int borland_dll Get_ModuleIrigTime_BCD_Px (int handlepx, unsigned short *moduleTime)
{
	int i;

	// Verify that the value of module's handle is valid (i.e. between zero and NUMCARDS-1).
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return (ebadhandle);

	// Verify that the module's handle is allocated/initialized.
	if (cardpx[handlepx].allocated != 1) return (ebadhandle);

	// Check that the processor is a NIOS II processor.
	if (cardpx[handlepx].processor != PROC_NIOS)	return (ewrongprocessor);

	// Check that the firmware supports the IrigB timestamp feature
	if (cardpx[handlepx].irigTtagModeSupported != TRUE)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

	// Get the module Irig time; an array of 4 short ints; days; hours + mins; secs + usec; more usec
	for (i=0; i<4; i++) 
//		*(moduleTime+(3-i)) = cardpx[handlepx].exc_bd_bc->IRIGttag[i];
		*(moduleTime+(3-i)) = RWORD_D2H( cardpx[handlepx].exc_bd_bc->IRIGttag[i] );

		// read 0th element first, to freeze the ttag

	return (0);

}  // end function Get_ModuleIrigTime_BCD_Px


/* written April 2015 by David Koppel

Get_ModuleIrigTime_Px

Description: Returns the current Module level IrigB timestamp, in decimal representation.

Note:
(a) This function is supported only in the PxIII (NiosII-based) modules, firmware released post Feb 2015.

Input parameters:
	handlepx	Module handle returned from the earlier call to Init_Module_Px

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
		efeature_not_supported_PX	If IrigB Timetag feature is not supported on this module
		efeature_not_supported_MMSI	If IrigB Timetag feature is not supported on this module

timetag words are in decimal; see function Get_ModuleIrigTime_BCD_Px for timetag words in BCD representation

*/

int borland_dll Get_ModuleIrigTime_Px (int handlepx, t_IrigOnModuleTime *IrigTime)
{
	int i;
	unsigned short int moduleTime[4];
	
	// Verify that the value of module's handle is valid (i.e. between zero and NUMCARDS-1).
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return (ebadhandle);

	// Verify that the module's handle is allocated/initialized.
	if (cardpx[handlepx].allocated != 1) return (ebadhandle);

	// Check that the processor is a NIOS II processor.
	if (cardpx[handlepx].processor != PROC_NIOS)	return (ewrongprocessor);

	// Check that the firmware supports the IrigB timestamp feature
	if (cardpx[handlepx].irigTtagModeSupported != TRUE)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

	// Get the module Irig time; an array of 4 short ints; days; hours + mins; secs + usec; more usec
	for (i=0; i<4; i++) 

		// read 0th element first, to freeze the ttag
//		moduleTime[3-i] = cardpx[handlepx].exc_bd_bc->IRIGttag[i];
		moduleTime[3-i] = RWORD_D2H( cardpx[handlepx].exc_bd_bc->IRIGttag[i] );
	
	Convert_IrigTimetag_BCD_to_Decimal_Px (moduleTime, IrigTime);
	return 0;
}


/* written June 2015

Convert_IrigTimetag_BCD_to_Decimal_Px

Description: Converts Irig timetag from BCD representation (array of 4 words) to decimal representation (struct t_IrigOnModuleTime)

Utility function, not dependent on handle of module.
However, we call it internally in the DLL for functions Get_ModuleIrigTime_Px and Get_Next_Mon_IRIGttag_Message_Px

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

int borland_dll Convert_IrigTimetag_BCD_to_Decimal_Px (unsigned short int *moduleTime, t_IrigOnModuleTime *IrigTime)
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
	
/*
Build a BCD string for some integer value, for visual display in memory (DPR)
*/
unsigned int BuildBCD_Px (unsigned int binaryValue, unsigned int bitOffset, unsigned int numDigits)
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
Preset_IrigTimeTag_Px

Description: Load values [day, hours, minutes, seconds] into the Irig Preload registers

Input parameters:
	handlepx		Module handle returned from the earlier call to Init_Module_Px
	Day		number of the day from 01 Jan, where 1=01 Jan (1-366)
	Hour		hour of the day (0-23)
	Minute	minute of the hour (0-59)
	Second	second of the minute (0-59)

Output parameters:	none

Return values:
	0				If successful
	ebadhandle			If handle parameter is out of range or not allocated
	ewrongprocessor		If module processor is not (PxIII) Nios-based
	efeature_not_supported_PX	If IrigB Timetag feature is not supported on this module
	efeature_not_supported_MMSI	If IrigB Timetag feature is not supported on this module
	>0				successful, but some parameter(s) was input out of bounds & normalized internally
*/
int borland_dll Preset_IrigTimeTag_Px (int handlepx, usint Day, usint Hour, usint Minute, usint Second)
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
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return (ebadhandle);

	// Verify that the module's handle is allocated/initialized.
	if (cardpx[handlepx].allocated != 1) return (ebadhandle);

	// Check that the processor is a NIOS II processor.
	if (cardpx[handlepx].processor != PROC_NIOS)	return (ewrongprocessor);

	// Check that the firmware supports the IrigB timestamp feature
	if (cardpx[handlepx].irigTtagModeSupported != TRUE)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

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

	IrigRegister = BuildBCD_Px(Hour, IRIG_HR_OFFSET, 2);
	IrigRegister |= BuildBCD_Px(Minute, IRIG_MIN_OFFSET, 2);

//	cardpx[handlepx].exc_bd_bc->IrigPreload_days = BuildBCD_Px(Day, IRIG_DAY_OFFSET, 3);
//	cardpx[handlepx].exc_bd_bc->IrigPreload_hr_min = IrigRegister;
//	cardpx[handlepx].exc_bd_bc->IrigPreload_seconds = BuildBCD_Px(Second, IRIG_SEC_OFFSET, 2);
//	cardpx[handlepx].exc_bd_bc->IrigPreload_load = 1;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->IrigPreload_days, BuildBCD_Px(Day, IRIG_DAY_OFFSET, 3) );
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->IrigPreload_hr_min, IrigRegister );
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->IrigPreload_seconds, BuildBCD_Px(Second, IRIG_SEC_OFFSET, 2) );
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->IrigPreload_load, 1 );

	return (status);
}

/*
Convert BCD value (as listed in the Irig time extracted from memory) into an integer
*/
int BCDtoBinary_Px (unsigned int BCDvalue)
{
#define BCD_DIGITS_IN_INT 8
#define SIZE_OF_DIGIT 4

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
PrintIRIGtime_BCD_Px

Description: Print (sprintf) the Irig ttag components into a string
For use with value returned from function Get_ModuleIrigTime_BCD_Px

Input parameters (all components are in BCD):
	ttag1		number of the day from 01 Jan, where 1=01 Jan (1-366)
	ttag2		hours component, followed by minutes component
	ttag3		second component, followed by microseconds component
	ttag4		more of the microseconds component

timetag words are in BCD; see function PrintIRIGtime_Px for timetag words in decimal representation

Output parameters:
	outString	a string containing the visual display of the Irig timetag value, in format:
			"day MonthName hour:minute:second.microseconds"

Return values:	none
*/
void borland_dll PrintIRIGtime_BCD_Px (unsigned short int ttag1, unsigned short int ttag2, unsigned short int ttag3, unsigned short int ttag4, char * outString)
{
	unsigned int hour,minute,second,microseconds;
	int day, month;
	int i;
	int systemYear;
#ifdef _WIN32
	SYSTEMTIME SystemTime;
#else
	time_t currtime = time(NULL);
#endif


#ifdef _WIN32
	GetLocalTime(&SystemTime);
	systemYear = SystemTime.wYear;
#else
	systemYear = localtime(&currtime)->tm_year;
#endif

	//use Windows time to decide if it's a leapyear.
	if ((systemYear % 4) != 0)
		daysInMonth[1] = 28;

	day				= BCDtoBinary_Px(ttag1 & 0xfff);
	hour			= BCDtoBinary_Px(ttag2 >> 8);
	minute			= BCDtoBinary_Px(ttag2 & 0xff);
	second			= BCDtoBinary_Px((unsigned int)ttag3 >> 8);
	microseconds	= BCDtoBinary_Px(((unsigned int)(ttag3 & 0xff) << 16) | ttag4);

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
PrintIRIGtime_Px

Description: Print (sprintf) the Irig ttag components into a string
For use with value returned from function Get_ModuleIrigTime_Px

Input parameters (all components are in decimal):
	struct t_IrigOnModuleTime, with fields:
		IrigTime->days		number of the day from 01 Jan, where 1=01 Jan (1-366)
		IrigTime->hours		hours component
		IrigTime->minutes		minutes component
		IrigTime->seconds		second component
		IrigTime->microsecs	microseconds component

timetag words are in decimal; see function PrintIRIGtime_BCD_Px for timetag words in BCD representation

Output parameters:
	outString	a string containing the visual display of the Irig timetag value, in format:
			"day MonthName hour:minute:second.microseconds"

Return values:	none
*/
void borland_dll PrintIRIGtime_Px (t_IrigOnModuleTime *IrigTime, char * outString)
{
	unsigned int hour,minute,second,microseconds;
	int day, month;
	int i;
	int systemYear;
#ifdef _WIN32
	SYSTEMTIME SystemTime;
#else
	time_t currtime = time(NULL);
#endif


#ifdef _WIN32
	GetLocalTime(&SystemTime);
	systemYear = SystemTime.wYear;
#else
	systemYear = localtime(&currtime)->tm_year;
#endif

	//use Windows time to decide if it's a leapyear.
	if ((systemYear % 4) != 0)
		daysInMonth[1] = 28;

	day			= IrigTime->days;
	hour		= IrigTime->hours;
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
Get_Num_Ports_MMSI_Px

  Description: Returns the number of MMSI ports available on this module
  Input parameters: none
  Output parameter: pNumPorts - how many ports exist on this module (1-8)
  Return values: 0
*/

int borland_dll Get_Num_Ports_MMSI_Px (int handlepx, unsigned char *pNumPorts)
{
#ifndef MMSI
	return (func_invalid);
#else

	unsigned char numberOfPorts;
	
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	numberOfPorts = (unsigned char)(cardpx[handlepx].exc_bd_bc->NumChannels);
	numberOfPorts = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->NumChannels );

	if (numberOfPorts == 0xFF)
		numberOfPorts = 8;
	
	*pNumPorts = numberOfPorts;
	return 0;
#endif
}

