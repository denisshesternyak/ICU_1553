#include <stdio.h>
#include "pxIncl.h"

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)

extern INSTANCE_PX cardpx[];
static usint tmpval;

/*
Restart

Description: Continue processing following Set_Stop_on_Error.
Input parameters:  none
Output parameters: none
Return values:	0
*/

int borland_dll Restart (void)
{
	return Restart_Px (g_handlepx);
}
int borland_dll Restart_Px (int handlepx)
{
	unsigned char tmpStart;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	cardpx[handlepx].exc_bd_bc->start |= 0x01;
	tmpStart = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start );
	tmpStart |= 0x01;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start , tmpStart );

	return (0);
}


/*
Set_Mode

Description: Sets the mode in which the board is to operate. It performs a
reset, clearing all memory in the board and initializes the board to its
default values for each mode. This routine must be called prior to any mode
specific routines.

Note: Set_Mode may be called just for reset purposes although Clear_Card
or Clear_Frame are faster in BC mode. (Clear_Card and Clear_Frame reset the
driver variables only, not the card.)

Input parameters:  mode   RT_MODE      For Remote Terminal Mode
			  BC_RT_MODE   For BC/Concurrent RT Mode (default)
			  MON_BLOCK    For Sequential Monitor Mode
			  MON_LOOKUP   For Look Up Table Monitor
			  BC_MODE      obsolete - used BC_RT_MODE

Output parameters:  none
Return values:	einval	 If parameter is out of range
			emonmode If the board/module only works in monitor mode
		0	 If successful
*/

int borland_dll Set_Mode (int mode)
{
	return Set_Mode_Px (g_handlepx, mode);
}
int borland_dll Set_Mode_Px (int handlepx, int mode)
/* Note that this function is dependent on the Windows API Timer (hence it uses the LARGE_INTEGER type).
	Since this function is dependent on a Windows API call, it is not portable to other operating systems as written.	*/
{
	int i, efound;

#ifdef _WIN32
	LARGE_INTEGER startime;
	int d;
#endif
	usint *dataptr;
	usint tempWord;
	usint local_more_board_options;
	usint local_moduleFunction;
	
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ((mode != BC_MODE) && (mode != RT_MODE) && (mode != BC_RT_MODE) &&
		(mode != MON_BLOCK) && (mode != MON_LOOKUP) && (mode != LL_0018_MODE) )
		return (einval);
		
	local_more_board_options = RWORD_D2H( cardpx[handlepx].exc_bd_bc->more_board_options );
//	if ((cardpx[handlepx].processor == PROC_NIOS) &&
//		((cardpx[handlepx].exc_bd_bc->more_board_options & MONITOR_ONLY_MODE) == MONITOR_ONLY_MODE) &&
//		((mode != MON_BLOCK) && (mode != MON_LOOKUP)) )
	if ((cardpx[handlepx].processor == PROC_NIOS) &&
		((local_more_board_options & MONITOR_ONLY_MODE) == MONITOR_ONLY_MODE) &&
		((mode != MON_BLOCK) && (mode != MON_LOOKUP)) )
		return (emonmode);

	/* get value of rt from register to put in rtNumber of INSTANCE_PX for single function */
	if (cardpx[handlepx].singleFunction)
		Update_RT_From_Reg(handlepx);

//	cardpx[handlepx].exc_bd_bc->board_id = ' ';  /* initialize board to garbage */
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_id,  ' ' ); /* initialize board to garbage */
//	cardpx[handlepx].exc_bd_bc->board_status = 0x00; /* board reset will fix it */
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_status, 0x00 ); /* board reset will fix it */
	Reset_Card_Px(handlepx);

	if (cardpx[handlepx].card_addr == SIMULATE)
	{
		cardpx[handlepx].exc_bd_bc->board_id = 'E';
		cardpx[handlepx].exc_bd_bc->board_status = 0xf;
	}
	/* wait for reset */
	efound = 0;
#ifdef _WIN32
	StartTimer_Px(&startime);

#ifdef MMSI
	/* RESET_DELAY_MMSI is set to 25*100 ms*/
	while(EndTimer_Px(startime, RESET_DELAY_MMSI) != 1)
#else
	/* RESET_DELAY_PX is set to 25*100 ms*/
	while(EndTimer_Px(startime, RESET_DELAY_PX) != 1)
#endif
//		if (cardpx[handlepx].exc_bd_bc->board_id=='E')
		if (RBYTE_D2H(cardpx[handlepx].exc_bd_bc->board_id) == 'E')
		{
			efound = 1;
			break;
		}
	if (!efound) return(etimeoutreset);

	for(d=0;d<2;d++)
	{
//		tmpval =  cardpx[handlepx].exc_bd_bc->board_status;
		tmpval = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status );
		if ((tmpval & 0xf)==0xf) break;
		StartTimer_Px(&startime);
#ifdef MMSI
		/* RESET_DELAY_MMSI is set to 25*100 ms*/
		while(EndTimer_Px(startime, RESET_DELAY_MMSI) != 1);
#else
		/* RESET_DELAY_PX is set to 25*100 ms*/
		while(EndTimer_Px(startime, RESET_DELAY_PX) != 1);
#endif
	}
	if (d==2) return(etimeoutreset);
#else
	for (i=0; i<TIMEOUT_VAL; i++)
	{
		Sleep(50L);
//		if (cardpx[handlepx].exc_bd_bc->board_id=='E')
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_id ) == 'E')
		{
			efound = TRUE;
			break;
		}
	}
	if (!efound) return(etimeoutreset);
	for (i=0; i<TIMEOUT_VAL; i++)
	{
		Sleep(50L);
//		tmpval =  cardpx[handlepx].exc_bd_bc->board_status;
		tmpval =  RWORD_D2H( cardpx[handlepx].exc_bd_bc->board_status );
		if ((tmpval & 0xf)==0xf)
		{
			efound = TRUE;
			break;
		}
	}
	if (!efound) return(etimeoutreset);
#endif // _WIN32

	/* maybe need this line for vme cards when vme interrupts will work
	cardpx[handlepx].exc_bd_bc->reset_interrupt = 1; */

	cardpx[handlepx].board_config = (unsigned char)mode;	//write to instance for fast checking in other routines
//	cardpx[handlepx].exc_bd_bc->board_config = (unsigned char)mode;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_config,  mode );

	// For some reason this line was duplicated; we're commenting it out
	// cardpx[handlepx].board_config = (unsigned char)mode;

	cardpx[handlepx].univmode = 0;
	cardpx[handlepx].mBufManagement.numMsgsInLclBuffer = 0;
	cardpx[handlepx].prevCounterInMsg = 0;
	if (mode == RT_MODE)
	{
		cardpx[handlepx].next_rt_entry = 0;
		cardpx[handlepx].next_rt_entry_old = 0;

//		cardpx[handlepx].exc_bd_rt->var_amplitude = 0xff;	/* default is 7.5 volts */
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->var_amplitude, 0xff );	/* default is 7.5 volts */

		/* set default RT status return to legal status */
		/* set sync tables to 0 */

		if (cardpx[handlepx].singleFunction)
		{
			cardpx[handlepx].rtid_brd_ctrl = (unsigned char *)&(cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[0x800]);
//			cardpx[handlepx].exc_bd_rt->status[0] = (usint)(cardpx[handlepx].rtNumber << 11);
			WWORD_H2D( cardpx[handlepx].exc_bd_rt->status[0] , (cardpx[handlepx].rtNumber << 11) );

			cardpx[handlepx].rt_sync_timetag[0] = 0;
			cardpx[handlepx].rt_sync_data[0] = 0;

		}
		else
		{
			for(i=0; i < 32; i++)
			{
//				cardpx[handlepx].exc_bd_rt->status[i] = (usint)(i << 11);
				WWORD_H2D( cardpx[handlepx].exc_bd_rt->status[i] , (i << 11) );
				cardpx[handlepx].rt_sync_timetag[i] = 0;
				cardpx[handlepx].rt_sync_data[i] = 0;
			}
		}
		if (cardpx[handlepx].processor == PROC_AMD)
		{
			cardpx[handlepx].rtid_ctrl = cardpx[handlepx].exc_bd_rt->cmonIP_rtidAP;
			cardpx[handlepx].cmon_stack = (struct MON_STACK *) cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP;
			cardpx[handlepx].cmon_stack_size = CMON_STACK_SIZE_AP;

		}
		else
		{
			cardpx[handlepx].rtid_ctrl = cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP;
			cardpx[handlepx].cmon_stack = (struct MON_STACK *) cardpx[handlepx].exc_bd_rt->cmonIP_rtidAP;
			cardpx[handlepx].cmon_stack_size = CMON_STACK_SIZE;
		}

	}

	if ((mode == BC_MODE) || (mode == BC_RT_MODE))
	{
		/* Set arbitrary default value for frame time, response time, var amplitude - BC mode */
#ifdef MMSI
//		cardpx[handlepx].exc_bd_bc->frametime_resolution = 0;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->frametime_resolution, 0 );

//		cardpx[handlepx].exc_bd_bc->frametime = 0x30d4;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->frametime, 0x30d4 );

//		if (cardpx[handlepx].exc_bd_bc->more_board_options & MMSI_TIMING)
		if (local_more_board_options & MMSI_TIMING)
		{
			// cardpx[handlepx].exc_bd_bc->resptime = 160;   // default is ~8 microseconds, as per spec paragraph 3.3.3.9
			WBYTE_H2D( cardpx[handlepx].exc_bd_bc->resptime, 160 );   // default is ~8 microseconds, as per spec paragraph 3.3.3.9
		}
		else
		{
			// cardpx[handlepx].exc_bd_bc->resptime = 0xFF;   // compensates for older Nios firmware
			WBYTE_H2D( cardpx[handlepx].exc_bd_bc->resptime, 0xFF );   // compensates for older Nios firmware
		}
#else
//		cardpx[handlepx].exc_bd_bc->frametime_resolution = 0xc99;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->frametime_resolution, 0xc99 );
//		cardpx[handlepx].exc_bd_bc->frametime = 0x0;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->frametime, 0x0);
//		cardpx[handlepx].exc_bd_bc->internal_mon_connect  = 0;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->internal_mon_connect, 0 );
#endif

//		cardpx[handlepx].exc_bd_bc->resptime = 0x61;	 /* default is ~15 microseconds */
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->resptime, 0x61 );	 /* default is ~15 microseconds */
//		cardpx[handlepx].exc_bd_bc->amplitude = 0xff;	 /* default is 7.5 volts */
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->amplitude, 0xff );	 /* default is 7.5 volts */
//		cardpx[handlepx].exc_bd_bc->bit = 3;             /* no error */
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->bit, 3 );             /* no error */

		cardpx[handlepx].frequencyModeON = FALSE;
		cardpx[handlepx].nextid = 1;
		cardpx[handlepx].nextframe = 0;
		cardpx[handlepx].next_MON_entry = 0;
		cardpx[handlepx].lastmsgttag = 0;
		cardpx[handlepx].nextmsg = cardpx[handlepx].exc_bd_bc->msg_block1;
		cardpx[handlepx].msg_block = 1;
		cardpx[handlepx].top_stack = &(cardpx[handlepx].exc_bd_bc->stack[0]);
		cardpx[handlepx].nextstack = cardpx[handlepx].top_stack;
		for (i=0 ; i < MSGMAX; i++)
			cardpx[handlepx].msgalloc[i] = 0;

		if (cardpx[handlepx].processor == PROC_NIOS)
		{

// MMSI:	cardpx[handlepx].exc_bd_bc->bcProtocolOptions |= BC_PROT_IMG_250N_RES | BC_PROT_SKIP_WITH_IMG;
// PX:		cardpx[handlepx].exc_bd_bc->bcProtocolOptions |= BC_PROT_IMG_250N_RES;
			tempWord = RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions );
#ifdef MMSI
			tempWord |= BC_PROT_IMG_250N_RES | BC_PROT_SKIP_WITH_IMG;
#else
			tempWord |= BC_PROT_IMG_250N_RES;
#endif
			WWORD_H2D( cardpx[handlepx].exc_bd_bc->bcProtocolOptions, tempWord );
		}
		
		if (cardpx[handlepx].processor == PROC_AMD)
		{
			cardpx[handlepx].cmon_stack = (struct MON_STACK *) cardpx[handlepx].exc_bd_bc->msgblock2IP_cmonAP;
			cardpx[handlepx].cmon_stack_size = CMON_STACK_SIZE_AP;
			cardpx[handlepx].msg_block2 = cardpx[handlepx].exc_bd_bc->msgblock2AP;
			cardpx[handlepx].msg_block2_size = MSG_BLOCK2_SIZE_AP;
		
			if (cardpx[handlepx].isPCMCIA == 1)
			{
				/* AMD on PCMCIA/EP memory goes until 0x8000 */
				cardpx[handlepx].msg_block3 = NULL;
				cardpx[handlepx].msg_block3_size = 0;
			}
			else
			{
				/* AMD on M4K1553Px (I) module memory goes until 0xFFFF */
				cardpx[handlepx].msg_block3 = cardpx[handlepx].exc_bd_bc->cmonIP;
				cardpx[handlepx].msg_block3_size = MSG_BLOCK3_SIZE_IP_AMD;
			}

			cardpx[handlepx].msg_block4_size = 0;
		}
		else
		{
			cardpx[handlepx].cmon_stack = (struct MON_STACK *) cardpx[handlepx].exc_bd_bc->cmonIP;
			cardpx[handlepx].cmon_stack_size = CMON_STACK_SIZE;
			cardpx[handlepx].msg_block2 = cardpx[handlepx].exc_bd_bc->msgblock2IP_cmonAP;
			cardpx[handlepx].msg_block2_size = MSG_BLOCK2_SIZE_IP;
			cardpx[handlepx].msg_block3 = cardpx[handlepx].exc_bd_bc->msgblock2AP;
			cardpx[handlepx].msg_block3_size = MSG_BLOCK3_SIZE_IP;
			cardpx[handlepx].msg_block4 = cardpx[handlepx].exc_bd_bc->cmonIP;
			cardpx[handlepx].msg_block4_size = MSG_BLOCK4_SIZE_IP;
		}

	}

	if ((mode == MON_BLOCK) || (mode == MON_LOOKUP) || (mode == LL_0018_MODE))
	{
		local_moduleFunction = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->moduleFunction );

#ifdef MMSI
		if (cardpx[handlepx].processor == PROC_NIOS)
//			if ((cardpx[handlepx].exc_bd_bc->more_board_options & MMSI_TIMING) != MMSI_TIMING)
//				cardpx[handlepx].exc_bd_monseq->response_time *= 10;  // NIOS bit is 100 nano instead of 1 micro
			if ((local_more_board_options & MMSI_TIMING) != MMSI_TIMING)
			{
				tempWord = RWORD_D2H(cardpx[handlepx].exc_bd_monseq->response_time) * 10;
				WWORD_H2D(cardpx[handlepx].exc_bd_monseq->response_time, tempWord);  // NIOS bit is 100 nano instead of 1 micro
				// older Nios firmware without the timing fix, needs a *10
			}
#endif
		
		cardpx[handlepx].nextblock = 0;
		cardpx[handlepx].ignoreoverrun = 0;
		cardpx[handlepx].hiMsgCount = 0;


		if (cardpx[handlepx].enhancedMONblockFW)
		{
//			if (cardpx[handlepx].exc_bd_monseq->moduleFunction & ENHANCED_MON_MODE)
			if (local_moduleFunction & ENHANCED_MON_MODE)
				cardpx[handlepx].isEnhancedMon = 1;
			else
				cardpx[handlepx].isEnhancedMon = 0;
		}
		else
			cardpx[handlepx].isEnhancedMon = 0;

		if (cardpx[handlepx].expandedMONblockFW)
		{
//			if (cardpx[handlepx].exc_bd_monseq->moduleFunction & EXPANDED_BLOCK_MODE)
			if (local_moduleFunction & EXPANDED_BLOCK_MODE)
				cardpx[handlepx].isExpandedMon = 1;
			else
				cardpx[handlepx].isExpandedMon = 0;
		}
		else
			cardpx[handlepx].isExpandedMon = 0;

		/* Regular Mode - 200 Blocks */
		cardpx[handlepx].lastblock = MAXMONBLOCKS_OLD;

		/* Expanded Monitor - 800 Blocks */
		if (cardpx[handlepx].isExpandedMon)
			cardpx[handlepx].lastblock = MAXMONBLOCKS_EXPANDED;

		/* Enhanced Monitor - 400 Block (+400 status table); Enhanced supercedes Expanded mode */
		if 	(cardpx[handlepx].isEnhancedMon)
			cardpx[handlepx].lastblock = MAXMONBLOCKS_ENHANCED;

		/* 	Add the code from Load_Datablk to extract a pointer to the beginning of the data block area.
		Then set the blknum entry into the .msgalloc array to point to the beginning of this block.
		Then, the user can directly access the block. */

		for (i=0; i<200; i++) {
			dataptr = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block[i]);
			cardpx[handlepx].msgalloc[i] = dataptr ;   /* address where the block sits */
		}

		for (i=0; i<153; i++) {
			dataptr = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block2[i]);
			cardpx[handlepx].msgalloc[200+i] = dataptr ;   /* address where the block sits */
		}

		for (i=0; i<47; i++) {
			dataptr = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block3[i]);
			cardpx[handlepx].msgalloc[353+i] = dataptr ;   /* address where the block sits */
		}

		for (i=0; i<400; i++) {
			dataptr = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block4[i]);
			cardpx[handlepx].msgalloc[400+i] = dataptr ;   /* address where the block sits */
		}

		// move setting of cardpx[handlepx].lastCountRead to here
		cardpx[handlepx].lastCountRead = 0;
	}
	
	return (0);
}


/*
Stop

Description: Stops the current operation of the board in all modes.

Input parameters:  none
Output parameters: none
Return values:	0	  Always
*/

int borland_dll Stop (void)
{
	return Stop_Px (g_handlepx);
}
int borland_dll Stop_Px (int handlepx)
{
	unsigned char tmpStart;
	unsigned int  tmpBoard_options;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	cardpx[handlepx].exc_bd_bc->start &= ~0x01;
	tmpStart = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start );
	tmpStart &= ~0x01;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start , tmpStart); 

	/* Moved to Run_RT
	cardpx[handlepx].next_rt_entry = 0;
	cardpx[handlepx].next_rt_entry_old = 0;
	*/
	
	/* flag bit 0x800 in board_options register is shared between PROC_AMD and currently in REPLAY_MODE (since PROC_INTEL) */
//	if ((cardpx[handlepx].exc_bd_bc->replay == ENABLE) && (cardpx[handlepx].processor != PROC_AMD))
//		cardpx[handlepx].exc_bd_bc->board_options &= ~REPLAY_MODE;
	if ((RBYTE_D2H( cardpx[handlepx].exc_bd_bc->replay ) == ENABLE) && (cardpx[handlepx].processor != PROC_AMD))
	{
		tmpBoard_options = RWORD_D2H( cardpx[handlepx].exc_bd_bc->board_options );
		tmpBoard_options &= ~REPLAY_MODE;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->board_options, tmpBoard_options);
	}

	return 0;
}

/*
Set_Header_Exists - for 1760 cards

Description: Set to indicate that there is an assigned header word value
	     for this RT Subaddress.

Input parameters: int sa      the Subaddress number for this setting

for BC and RT modes:
		 int enable  HEADER_ENABLE, HEADER_DISABLE
for Monitor mode:
		 int enable  HEADER_ENABLE to enable both xmt and rcv
                             HEADER_ENABLE_XMT to enable xmt only
			     HEADER_ENABLE_RCV to enable rcv only
			     HEADER_DISABLE
Output parameters: none
Return values:	0	   success
		rterror    bad Subaddress number
*/

int borland_dll Set_Header_Exists (int sa, int enable)
{
	return Set_Header_Exists_Px (g_handlepx, sa, enable);
}

int borland_dll Set_Header_Exists_Px (int handlepx, int sa, int enable)
{
	int mode, enable_val = 0;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ( (sa < 0) || (sa > 31) )
		return (rterror);

//	mode = cardpx[handlepx].exc_bd_bc->board_config;
//	mode = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_config);
	mode = cardpx[handlepx].board_config;
	if ((mode == MON_BLOCK) || (mode == MON_LOOKUP))
	{
		if ((enable < HEADER_DISABLE) || (enable > HEADER_ENABLE_RCV))
			return einval;

		if (enable == HEADER_ENABLE_XMT)
			enable_val = 0x100;
		else if (enable == HEADER_ENABLE_RCV)
			enable_val = 0x1;
		else if (enable == HEADER_ENABLE)
			enable_val = 0x101;
		else if (enable == HEADER_DISABLE)
			enable_val = 0;
	}
	else
	{
		if ((enable != HEADER_ENABLE) && (enable != HEADER_DISABLE))
			return einval;

		if ((mode == BC_MODE) || (mode == BC_RT_MODE))
			enable_val = enable << 8; /* upper byte */
		else /* RT_MODE */
			enable_val = enable;  /* lower byte */
	}

//	cardpx[handlepx].exc_bd_bc->header_exists[sa] = enable_val;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->header_exists[sa],enable_val );
	return(0);
}

/*
Set_Header_Value - for 1760 cards

Description: Set the header word value to be assigned for this RT Subaddress.
Input parameters: int sa      the Subaddress number for this setting
		  usint header_value
			      the header value to be assigned
Output parameters: none
Return values:	0	   success
		rterror    bad Subaddress number
*/

int borland_dll Set_Header_Value (int sa, int direction, usint header_value)
{
	return Set_Header_Value_Px (g_handlepx, sa, direction, header_value);
}

int borland_dll Set_Header_Value_Px (int handlepx, int sa, int direction, usint header_value)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ( (sa < 0) || (sa > 31) )
		return (rterror);

//	if ((cardpx[handlepx].exc_bd_bc->board_config == BC_MODE)
//		|| (cardpx[handlepx].exc_bd_bc->board_config == BC_RT_MODE))
//		cardpx[handlepx].exc_bd_bc->header_val[sa] = header_value;
//	else if (cardpx[handlepx].exc_bd_bc->board_config == RT_MODE)
//		cardpx[handlepx].exc_bd_rt->header_val[sa] = header_value;

	if ((cardpx[handlepx].board_config == BC_MODE) ||
		(cardpx[handlepx].board_config == BC_RT_MODE))
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->header_val[sa], header_value );
	else if (cardpx[handlepx].board_config == RT_MODE)
		WWORD_H2D( cardpx[handlepx].exc_bd_rt->header_val[sa], header_value );

	else /* monitor */
	{
		if (direction == TRANSMIT) /* select upper block of header vals */
//			cardpx[handlepx].exc_bd_monseq->header_val[0x20 + sa] = header_value;
			WWORD_H2D( cardpx[handlepx].exc_bd_monseq->header_val[0x20 + sa] , header_value );
		else if (direction == RECEIVE) /* select lower block of header vals */
//			cardpx[handlepx].exc_bd_monseq->header_val[sa] = header_value;
			WWORD_H2D( cardpx[handlepx].exc_bd_monseq->header_val[sa] , header_value );
		else
			return einval;
	}
	return 0;
}

int borland_dll Enable_1553A_Support (int enableflag)
{
	return Enable_1553A_Support_Px (g_handlepx, enableflag);
}
int borland_dll Enable_1553A_Support_Px (int handlepx, int enableflag)
{
	unsigned char tmpRtProtocolOptions;

#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ((enableflag != ENABLE) && (enableflag != DISABLE))
		return einval;

//	if (cardpx[handlepx].exc_bd_bc->board_config == BC_RT_MODE)
	if (cardpx[handlepx].board_config == BC_RT_MODE)
	{
//		tmpval = cardpx[handlepx].exc_bd_bc->bcProtocolOptions;
		tmpval = RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions );
		if (enableflag == ENABLE)
			tmpval |= BC1553A_BIT;
		else
			tmpval &= ~BC1553A_BIT;
//		cardpx[handlepx].exc_bd_bc->bcProtocolOptions = tmpval;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->bcProtocolOptions ,tmpval );
	}

//	else if ((cardpx[handlepx].exc_bd_bc->board_config == MON_LOOKUP) ||
//		(cardpx[handlepx].exc_bd_bc->board_config == MON_BLOCK))
//		cardpx[handlepx].exc_bd_monseq->x1553a = enableflag;

	else if ((cardpx[handlepx].board_config == MON_LOOKUP) ||
		(cardpx[handlepx].board_config == MON_BLOCK))
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->x1553a ,enableflag );

//	else if (cardpx[handlepx].exc_bd_bc->board_config == RT_MODE)
	else if (cardpx[handlepx].board_config == RT_MODE)
	{
//		if (enableflag == ENABLE)
//			cardpx[handlepx].exc_bd_rt->rtProtocolOptions |= RT1553A_BIT;
//		else
//			cardpx[handlepx].exc_bd_rt->rtProtocolOptions &= ~RT1553A_BIT;

		tmpRtProtocolOptions = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->rtProtocolOptions );
		if (enableflag == ENABLE)
			tmpRtProtocolOptions  |= RT1553A_BIT;
		else
			tmpRtProtocolOptions &= ~RT1553A_BIT;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->rtProtocolOptions , tmpRtProtocolOptions);

	}
	else
		return emode;
	return 0;
#endif
}

int borland_dll Set_Timetag_Res(usint resolution)
{
	return Set_Timetag_Res_Px(g_handlepx, resolution);
}
int borland_dll Set_Timetag_Res_Px(int handlepx, usint resolution)
{
	usint resval;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_rt->board_config != RT_MODE) &&
//		(cardpx[handlepx].exc_bd_rt->board_config != MON_LOOKUP) &&
//		(cardpx[handlepx].exc_bd_rt->board_config != MON_BLOCK))

	if ((cardpx[handlepx].board_config != RT_MODE) &&
		(cardpx[handlepx].board_config != MON_LOOKUP) &&
		(cardpx[handlepx].board_config != MON_BLOCK))
		return (emode);

	if ((resolution < 4) || (resolution >1024))
		return (einval);
	resval = (usint)(((resolution + 2)/4)-1);

//	cardpx[handlepx].exc_bd_rt->time_res = (unsigned char) resval;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->time_res ,(unsigned char) resval );
	return 0;
}

/*
Clear_Timetag_Sync

int borland_dll Clear_Timetag_Sync(int modecode, int flag);

Description: Clear the timetag when the BC sends out a Mode Code
Synchronize message (MC-1 or MC-17).

Input parameters:   modecode    The selected mode code, 1 or 17
                     flag        Legal flags are:
                                 ENABLE - to enable the feature for this
mode code
                                 DISABLE - to disable the feature for this
mode code

Output parameters:  none

Return values:  ebadhandle      If invalid handle specified
                 einval          If parameter is out of range
                 0               If successful

*/

int borland_dll Clear_Timetag_Sync (int modecode, int flag)
{
	return Clear_Timetag_Sync_Px (g_handlepx, modecode, flag);
}
#ifdef VME4000
#define MC1_BIT 1
#define MC17_BIT 0
#else
#define MC1_BIT 0
#define MC17_BIT 1
#endif
int borland_dll Clear_Timetag_Sync_Px (int handlepx, int modecode, int flag)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_monseq->board_config == MON_BLOCK) ||
//		(cardpx[handlepx].exc_bd_monseq->board_config == MON_LOOKUP))
	if ((cardpx[handlepx].board_config == MON_BLOCK) ||
		(cardpx[handlepx].board_config == MON_LOOKUP))
	{
		if (cardpx[handlepx].processor == PROC_AMD)
		{
			if (cardpx[handlepx].exc_bd_bc->revision_level < 0x69)
				return func_invalid;
		}
		else if (cardpx[handlepx].processor != PROC_NIOS) // intel old Firmware
		{
//			if ((cardpx[handlepx].exc_bd_bc->revision_level < 0x21) && (cardpx[handlepx].processor != PROC_NIOS))
			if (cardpx[handlepx].exc_bd_bc->revision_level < 0x21)
				return func_invalid;
		}
	}

	if ((flag != ENABLE) && (flag != DISABLE))
		return einval;

	if (modecode == 1)
//		cardpx[handlepx].exc_bd_bc->ttagclrsync[MC1_BIT] = (unsigned char)flag;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->ttagclrsync[MC1_BIT],(unsigned char)flag );
	else if (modecode == 17)
//		cardpx[handlepx].exc_bd_bc->ttagclrsync[MC17_BIT] = (unsigned char)flag;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->ttagclrsync[MC17_BIT],(unsigned char)flag );
	else
		return einval;

	return 0;
}

#define HUB_MASK 0x3
int borland_dll Set_Hubmode_MMSI_Px(int handlepx, int hubmode)
{
#ifndef MMSI
	return (func_invalid);
#else
	int i;
	unsigned char tempByte;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* allow this function to operate in all three modes: BC/RT/Mon
	for MON mode, this will indicate that someone else on the bus is in a specific HUB mode,
	and the Monitor should perhaps mask out BAD_RT_ADDR bit in Message Status Word (MSW) */

	if ((hubmode != HUB_SWITCH) && (hubmode != HUB_LINK) && (hubmode != HUB_SPEC))
		return einval;

//	if	((cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != RT_MODE))
	if ((cardpx[handlepx].board_config != BC_RT_MODE) &&
		(cardpx[handlepx].board_config != RT_MODE))
	{
// replaced by line at end of this if statement
//		cardpx[handlepx].hubmode_MMSI &= ~HUB_MASK;
//		cardpx[handlepx].hubmode_MMSI |= hubmode;

		// new feature in MMSI-Nios firmware rev 3.5 & later
		// Monitor mode firmware will write the RT number into the command word in LINK hub mode

//		cardpx[handlepx].exc_bd_bc->hubmode_MMSI &= ~HUB_MASK;
//		cardpx[handlepx].exc_bd_bc->hubmode_MMSI |= hubmode;
		tempByte = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->hubmode_MMSI );
		tempByte = ((tempByte & ~HUB_MASK) | hubmode );
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->hubmode_MMSI, tempByte );

		cardpx[handlepx].hubmode_MMSI = tempByte & HUB_MASK;
	}
	else
	{
		//in LINK mode the RT should always return a status with RT address = 0
//		if ((cardpx[handlepx].exc_bd_bc->board_config == RT_MODE) && (hubmode == HUB_LINK) ) {
		if ((cardpx[handlepx].board_config == RT_MODE) && (hubmode == HUB_LINK) ) {
			for(i=0; i < 32; i++)	{
//				cardpx[handlepx].exc_bd_rt->status[i] = 0;
				WWORD_H2D( cardpx[handlepx].exc_bd_rt->status[i], 0 );
			}
		}
//		cardpx[handlepx].exc_bd_bc->hubmode_MMSI &= ~HUB_MASK;
//		cardpx[handlepx].exc_bd_bc->hubmode_MMSI |= hubmode;
		tempByte = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->hubmode_MMSI );
		tempByte = ((tempByte & ~HUB_MASK) | hubmode );
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->hubmode_MMSI, tempByte );
	}

	return 0;
#endif
}

/*
Description		Ignore_Timetag_Overrun_Px is used in conjunction with a user initiated
				Time tag reset (Reset_Time_Tag_Px,Clear_Timetag_Sync_Px, or SetIrig_4000 with the flag
				IRIG_TIME_RESET) to stop Get_Next_Message_xxx_Px from returning an overrun error even when the Time tag
				associated with the message following the Time tag reset is earlier than the previously read message.

Syntax			Ignore_Timetag_Overrun_Px (int handle, int enableflag)
Input Parameters 	handle 		The handle designated by Init_Module_Px
					enableflag	ENABLE Ignores overrun error [0001 H]
								DISABLE Does not ignore overrun error [0000 H]
Output Parameters	none

Return Values		ebadhandle 	If an invalid handle was specified; should be value returned by Init_Module_Px
					einval		If an invalid parameter was used as an input
					0 			If successful
*/
int borland_dll Ignore_Timetag_Overrun_Px(int handlepx, int enableflag)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((enableflag != ENABLE) && (enableflag != DISABLE))
		return einval;

	cardpx[handlepx].ignoreoverrun = enableflag;

	return 0;
}

/*
Set_ModuleTime_Px				Coded 19-Nov-2006 by YY Berlin

Description: Loads the M4K1553PxIII Module Time register with a user-
			 supplied value.  (Note that the Module Time register (3ea2h-3ea5h)
			 is different from the TimeTag (7008h-700bh) register).

Warning:  This function will reset the module and all data will be lost.

Processor required: NIOS II

Input parameters:

	handlepx		Module handle returned from the earlier call to Init_Module
	moduleTime		32-bit Module Time

Output parameters:

	(none)

Return values:

	ebadhandle		If handle is invalid
	ewrongprocessor	If module processor does not support this function
	etimeout		If the driver timed out waiting for BOARD_HALTED
	etimeoutreset	If the driver timed out waiting for reset
	esetmoduletime	If could not set the Module Time
	ewrongprocessor	If the processor is not a NIOS II processor
	0				If successful

*/

#define STOP_MODULE_DELAY 2500
#define SET_MODULETIME_DELAY 2500

int borland_dll Set_ModuleTime_Px (int handlepx, unsigned int moduleTime)
/* Note that this function is dependent on the Windows API Timer (hence it uses the LARGE_INTEGER type).
	Since this function is dependent on a Windows API call, it is not portable to other operating systems as written.	*/
{
	unsigned char  tmpStart;

	int				timedOut;			/* (boolean-like) flag indicating if timed out waiting
										for operation to complete			*/
#ifdef _WIN32
	LARGE_INTEGER		startTime;			/* time (number of ticks since system boot) at begining of
											the timeout loop				*/
#endif

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

	/* Stop the card (in case it was running).
	*/
//	cardpx[handlepx].exc_bd_bc->start &= ~0x01;
	tmpStart = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->start );
	tmpStart &= ~0x01;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start, tmpStart);

	timedOut = 1;
#ifdef _WIN32
	StartTimer_Px(&startTime);
	while(EndTimer_Px(startTime, STOP_MODULE_DELAY) != 1)
	{
//		if (cardpx[handlepx].exc_bd_bc->board_status & BOARD_HALTED )
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status) & BOARD_HALTED )
		{
			timedOut = 0;
			break;
		}
	}
#else
        int i;
	for (i=0; i<TIMEOUT_VAL; i++)
	{
		Sleep(50L);
//		if (cardpx[handlepx].exc_bd_bc->board_status & BOARD_HALTED )
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status ) & BOARD_HALTED )
		{
			timedOut = FALSE;
			break;
		}
	}
#endif // _WIN32
	if (timedOut)
		return etimeout;

	/*	Put the board into Download Mode; zero the Download mode operation-succeeded
	register; set the module time; and check the status register to make sure
	the operation was successful.
	*/
//	cardpx[handlepx].exc_bd_dwnld->board_config = DWNLD_MODE;
	WBYTE_H2D( cardpx[handlepx].exc_bd_dwnld->board_config, DWNLD_MODE);
	cardpx[handlepx].board_config = DWNLD_MODE;
//	cardpx[handlepx].exc_bd_dwnld->opSucceeded = 0x00;
	WBYTE_H2D( cardpx[handlepx].exc_bd_dwnld->opSucceeded ,0x00 );

//	cardpx[handlepx].exc_bd_dwnld->moduleTimeHi = (usint)(moduleTime >> 16);
	WWORD_H2D( cardpx[handlepx].exc_bd_dwnld->moduleTimeHi ,(usint)(moduleTime >> 16) );
//	cardpx[handlepx].exc_bd_dwnld->moduleTimeLo = (usint)(moduleTime);
	WWORD_H2D( cardpx[handlepx].exc_bd_dwnld->moduleTimeLo ,(usint)(moduleTime) );

//	cardpx[handlepx].exc_bd_bc->start = 0x05;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start ,0x05 );

	timedOut = 1;
#ifdef _WIN32
	StartTimer_Px(&startTime);
	while(EndTimer_Px(startTime, SET_MODULETIME_DELAY) != 1)
	{
//		if (cardpx[handlepx].exc_bd_bc->board_status == 0x9F)
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status ) == 0x9F)
		{
			timedOut = 0;
			break;
		}
	}
#else
	for (i=0; i<50; i++)
	{
		Sleep(50L);
//		if (cardpx[handlepx].exc_bd_bc->board_status == 0x9F)
		if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status ) == 0x9F)
		{
			timedOut = FALSE;
			break;
		}
	}
#endif // _WIN32
	if (timedOut == 1)
	{
		return etimeoutreset;
	}

//	if (cardpx[handlepx].exc_bd_dwnld->opSucceeded != 0x00)
	if (RBYTE_D2H( cardpx[handlepx].exc_bd_dwnld->opSucceeded ) != 0x00)
	{
		return (esetmoduletime);
	}

	/*	All done....
	*/
	return (0);

}  /* end function Set_ModuleTime_Px */

/* For all modes */

/* written Dec 2014

Set_IRIG_TimeTag_Mode_Px

Description: Sets the module to generate IrigB format timetags (4 words)
in each message received in Sequential Monitor mode.

In this mode, use function Get_Next_Mon_IRIG_Message_Px
(instead of function Get_Next_Message_Px) to read new messages.

Note:
This function is supported only in the PxIII (NiosII-based) modules, and on a 4000PCI[e]
carrier that supports receiving the IrigB time from an external source.

Input parameters: 
	handlepx		Module handle returned from the earlier call to Init_Module_Px
	flag
		ENABLE	Enable Irig Timetag mode [0001 H]
		DISABLE	Disable Irig Timetag mode [0000 H]

Output parameters: none

Return values:
		0			If successful
		emode		If not in Sequential Monitor mode
		einval		If illegal input value (for flag parameter)
		ebadhandle	If handle parameter is out of range or not allocated
		efeature_not_supported_PX	If IrigB Timetag feature is not supported on this module
		efeature_not_supported_MMSI	If IrigB Timetag feature is not supported on this module
*/

int borland_dll Set_IRIG_TimeTag_Mode_Px (int handlepx, int flag)
{
	unsigned short tempModFunc;
	usint local_moduleFunction;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].irigTtagModeSupported == FALSE)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

//	if (cardpx[handlepx].exc_bd_bc->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return emode;

	local_moduleFunction = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->moduleFunction );
	if (flag == ENABLE)
	{
//		cardpx[handlepx].exc_bd_monseq->moduleFunction |= IRIG_TIMETAG_MODE;
		tempModFunc = local_moduleFunction | IRIG_TIMETAG_MODE;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->moduleFunction, tempModFunc );

//		if ((cardpx[handlepx].exc_bd_monseq->moduleFunction & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE)
		if ((tempModFunc & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE)
			cardpx[handlepx].lastblock = MAX_IRIGMON_BLOCKS;
		else
			cardpx[handlepx].lastblock = MAX_IRIGMON_REGULAR_SIZE_BLOCKS;

		cardpx[handlepx].irigModeEnabled = TRUE;
	}
	else if (flag == DISABLE)
	{
//		cardpx[handlepx].exc_bd_monseq->moduleFunction &= ~((usint)IRIG_TIMETAG_MODE);
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->moduleFunction, (local_moduleFunction & ~((usint)IRIG_TIMETAG_MODE) ) );

		cardpx[handlepx].irigModeEnabled = FALSE;

	}
	else
		return einval;

	return 0;
}

int borland_dll Set_Expanded_Block_Mode_Px (int handlepx, int flag)
{
	unsigned short tmp;

#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* RT Mode */
//	if (cardpx[handlepx].exc_bd_rt->board_config == RT_MODE)
	if (cardpx[handlepx].board_config == RT_MODE)
	{
		if (!cardpx[handlepx].expandedRTblockFW)
#ifdef MMSI
			return efeature_not_supported_MMSI;
#else
			return efeature_not_supported_PX;
#endif

		if (flag == ENABLE)
		{
//			cardpx[handlepx].exc_bd_rt->moduleFunction |= EXPANDED_BLOCK_MODE;
			tmp = RWORD_D2H( cardpx[handlepx].exc_bd_rt->moduleFunction );
			tmp  |= EXPANDED_BLOCK_MODE;
			WWORD_H2D( cardpx[handlepx].exc_bd_rt->moduleFunction, tmp );
			cardpx[handlepx].userRequestedRTexpanded = 1;
			cardpx[handlepx].cmon_stack_size = CMON_STACK_SIZE_EBMODE;
		}
		else if (flag == DISABLE)
		{
//			cardpx[handlepx].exc_bd_rt->moduleFunction &= ~((usint)EXPANDED_BLOCK_MODE);
			tmp = RWORD_D2H( cardpx[handlepx].exc_bd_rt->moduleFunction );
			tmp  &= ~((usint)EXPANDED_BLOCK_MODE);
			WWORD_H2D( cardpx[handlepx].exc_bd_rt->moduleFunction, tmp );
			cardpx[handlepx].userRequestedRTexpanded = 0;
			cardpx[handlepx].cmon_stack_size = CMON_STACK_SIZE;
		}
		else
			return einval;
	}
	/* Sequential Monitor Mode */
//	else if (cardpx[handlepx].exc_bd_monseq->board_config == MON_BLOCK)
	else if (cardpx[handlepx].board_config == MON_BLOCK)
	{
		if (!cardpx[handlepx].expandedMONblockFW)
#ifdef MMSI
			return efeature_not_supported_MMSI;
#else
			return efeature_not_supported_PX;
#endif

		if (flag == ENABLE)
		{
//			cardpx[handlepx].exc_bd_monseq->moduleFunction |= EXPANDED_BLOCK_MODE;
			tmp = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->moduleFunction );
			tmp |= EXPANDED_BLOCK_MODE;
			WWORD_H2D( cardpx[handlepx].exc_bd_monseq->moduleFunction , tmp );
		}
		else if (flag == DISABLE)
		{
//			cardpx[handlepx].exc_bd_monseq->moduleFunction &= ~((usint)EXPANDED_BLOCK_MODE);
			tmp = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->moduleFunction );
			tmp &= ~((usint)EXPANDED_BLOCK_MODE);;
			WWORD_H2D( cardpx[handlepx].exc_bd_monseq->moduleFunction , tmp );
		}
		else
			return einval;
	}
	/* BC Mode */
//	else if (cardpx[handlepx].exc_bd_monseq->board_config == BC_RT_MODE) 
	else if (cardpx[handlepx].board_config == BC_RT_MODE)
	{
		if (!cardpx[handlepx].expandedBCblockFW)
#ifdef MMSI
			return efeature_not_supported_MMSI;
#else
			return efeature_not_supported_PX;
#endif

		if (flag == ENABLE)
		{
//			cardpx[handlepx].exc_bd_bc->expandedBlock |= EXPANDED_BLOCK_MODE;
			tmp = RWORD_D2H( cardpx[handlepx].exc_bd_bc->expandedBlock );
			tmp |= EXPANDED_BLOCK_MODE;
			WWORD_H2D( cardpx[handlepx].exc_bd_bc->expandedBlock, tmp);
		}
		else if (flag == DISABLE)
		{
//			cardpx[handlepx].exc_bd_bc->expandedBlock &= ~((usint)EXPANDED_BLOCK_MODE);
			tmp = RWORD_D2H( cardpx[handlepx].exc_bd_bc->expandedBlock );
			tmp &= ~((usint)EXPANDED_BLOCK_MODE);
			WWORD_H2D( cardpx[handlepx].exc_bd_bc->expandedBlock, tmp);
		}
		else
			return einval;

	}
	else
		return (emode);


	return 0;
#endif
}

/** ------------------------------------------------------------------------------------
Set_UseDmaIfAvailable_Px()

Description: Returns an indicator as to the availablility of DMA for transfer data
between the host memory and the Px module memory.

Applies to Modes: Mode Independent

Input parameters:
	nodeHandle	the handle obtained from Init_Module_Px() when
				the node was initialized.

	useDmaFlag	a flag indicating that DMA memory access
				should be used if available.
				Valid values are:
					ENABLE
					DISABLE
Output parameters:
	none

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

int borland_dll Set_UseDmaIfAvailable_Px(int handlepx, int useDmaFlag)
{
	/* Check that the module handle is valid. Don't bother checking the mode because it
	doesn't matter - the reads and writes will only happen if the mode is correct. */
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((useDmaFlag != ENABLE) && (useDmaFlag != DISABLE))
		return einval;

	/* Set the instance flag. */
	cardpx[handlepx].useDmaIfAvailable = useDmaFlag;

	return Successful_Completion_Exc;

}  /* -----  end of Set_UseDmaIfAvailable_Px()  ----- */

// UMMR = UNet Multi-Message-Read
int borland_dll Set_UseUMMRIfAvailable_Px(int handlepx, int useUMMRFlag)
{
	/* Check that the module handle is valid. Don't bother checking the mode because it
	doesn't matter - the reads and writes will only happen if the mode is correct. */
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((useUMMRFlag != ENABLE) && (useUMMRFlag != DISABLE))
		return einval;

	/* Set the instance flag. */
	cardpx[handlepx].useUnetMultiMsgReadIfAvailable = useUMMRFlag;

	return Successful_Completion_Exc;

}  /* -----  end of Set_UseUMMRIfAvailable_Px()  ----- */

