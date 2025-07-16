#include "pxIncl.h"
extern INSTANCE_PX cardpx[];
int globalTemp;

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)

/*
Insert_Msg_Err

Description: Selects various errors to inject into previously defined
messages.

Input parameters:   id      Message identifier returned from a prior call to
                            Create_Message
                    flags   Legal flags are:
                            *WD_CNT_ERR     SYNC_ERR
                            GAP_ERR         PARITY_ERR_PX / PARITY_ERR_MMSI
                            *BIT_CNT_ERR
                            For rev 1.1 and up:
                            DATA_ERR    For BC to RT commands the SYNC_ERR
                                        and PARITY_ERR_PX / PARITY_ERR_MMSI may be accompanied
                                        by this flag to cause the error to
                                        be inserted in the data words rather
                                        than the command word.

Note: *CNT errors are only valid if preceded by calls to Set_Wd_Cnt or
Set_Bit_Cnt as appropriate.

Output parameters:  none

Return values:   emode   If not in BC mode
                 ebadid  If message id is not a valid message id
                 0       If successful

*/

int borland_dll Insert_Msg_Err (int id, int flags)
{
	return Insert_Msg_Err_Px(g_handlepx, id, flags);
}

int borland_dll Insert_Msg_Err_Px (int handlepx, int id, int flags)
{
	usint controlword, *msgptr, cmdtype, commandword;
	int modecodeRcv = 0;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);

	if (cardpx[handlepx].singleFunction)
		return (enotforsinglefuncerror);

	msgptr = cardpx[handlepx].msgalloc[id];
	/* cmdtype = (usint)(*msgptr & 7); */
//	controlword = *msgptr;
	controlword = RWORD_D2H( msgptr[0] );
	
	cmdtype = controlword & 7;
	if (cmdtype == SKIP)
		cmdtype = cardpx[handlepx].commandtype[id];

	if (cmdtype == JUMP)
		return ejump;

	if ((cmdtype == MODE) || (cmdtype == BRD_MODE))
	{
//		commandword = *(msgptr+1);
		commandword = RWORD_D2H( msgptr[1] );
		if ((commandword & 0x1f) == 17)  /* checking if modecode 17 */
			modecodeRcv = 1;
	}

	if (flags == NO_ERR_IN_MSG)
		controlword &= ~ERRFLAGS; /* no 0x100 in any case */
	else if ((cmdtype != BC2RT) && (cmdtype != BRD_RCV)  && (!modecodeRcv))
		controlword = (usint)((controlword & ~ERRFLAGS) | flags | 0x100);
	else
		controlword = (usint)((controlword & ~ERRFLAGS) | flags);

//	*msgptr = controlword;
	WWORD_H2D( msgptr[0], controlword);
	return(0);
} /* end function Insert_Msg_Err */



/*
Reset_BC_Status

Description:  Clears the BC status so that the next call to Get_BC_Status
is meaningful. Essentially, Get_BC_Status returns status flags generated
since the last call to Reset_BC_Status.

Input parameters:   none

Output parameters:  none

Return values:   emode  If not in BC mode
                 0      If successful

*/

int borland_dll Reset_BC_Status (void)
{
        return Reset_BC_Status_Px(g_handlepx);
}

int borland_dll Reset_BC_Status_Px (int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//      if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//          (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
 	if ((cardpx[handlepx].board_config != BC_MODE) &&
    	(cardpx[handlepx].board_config != BC_RT_MODE))
            return (emode);

//	cardpx[handlepx].exc_bd_bc->message_status = 0;
    WBYTE_H2D( cardpx[handlepx].exc_bd_bc->message_status , 0 );

	return (0);
} /* end function Reset_BC_Status */



/*
Run_BC

Description:  Causes the Bus Controller to start transmitting messages. All
data structures must be set up before calling this routine.

Input parameters:   type   0 for continuous operation or 1-255 for the
                           number of times to execute the current frame

Output parameters:  none

Return values:  emode        If not in BC mode
                bcr_erange   If called with type > 255
		etimeout     If timed out waiting for BOARD_HALTED
                0            If successful

*/

int borland_dll Run_BC(int type) /* type = 0 for continuous operation or 1-255 for the*/
{                     /* number of times to execute the current frame */
	return Run_BC_Px(g_handlepx, type);
}


int borland_dll Run_BC_Px(int handlepx, int type)
{
	int timeout=0;
#ifdef _WIN32
	LARGE_INTEGER startime;
#endif

#define LOCAL_RESET_DELAY 2 // millisec delay
// max legitimate wait is 700 microsec, which is max 1553 message size (wait for current message to complete transmission)

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE) )
		return (emode);
	if ((type > 255) || (type < 0))
		return (bcr_erange);

#ifdef _WIN32
	timeout = 1;
	StartTimer_Px(&startime);
	while (EndTimer_Px(startime, LOCAL_RESET_DELAY) != 1)
	{
		if ( Get_Board_Status_Px(handlepx) & BOARD_HALTED )
		{
			timeout = 0;
			break;
		}
	}
	if (timeout == 1)
		return (etimeout);
#else
	while (timeout < 10000)  /* changed from 10, and replace Sleep w/busy loop */
	{
		int i;
		if ( Get_Board_Status_Px(handlepx) & BOARD_HALTED )
			break;

		/* Sleep(1L); - replace with busy loop, 03 Feb 02
		access DPR, to give the f/w time to write the HALTED bit
		reading from DPR takes about 150 nanosec, on any processor */

		for (i=0; i<3; i++)
//			globalTemp = cardpx[handlepx].exc_bd_bc->revision_level;
			globalTemp = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->revision_level );

		timeout++;
	}
	if (timeout == 10000)   /* changed from 10, and replace Sleep w/busy loop */
		return (etimeout);
#endif //_WIN32

	if (type != 1)
	{     /* loop mode */
//		cardpx[handlepx].exc_bd_bc->loop_count = (unsigned char)type;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->loop_count, (unsigned char)type );
//		cardpx[handlepx].exc_bd_bc->start = 5;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start, 5 );
	}
	else      /* single shot mode */
//		cardpx[handlepx].exc_bd_bc->start = 1;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->start, 1 );
	return (0);
} /* end function Run_BC */



/*
Set_BC_Resp

Description: Sets the amount of time the BC will wait for a response before
declaring the RT late.

Input parameters:   time    Time in nanoseconds, minimum value is 2000,
                            maximum is 32000

Output parameters:  none

Return values:   emode   If not in BC mode
                 einval  If parameter is out of range
                 0       If successful

*/

int borland_dll Set_BC_Resp (int time) /* time in nanoseconds */
{
	return Set_BC_Resp_Px(g_handlepx, time);
}

int borland_dll Set_BC_Resp_Px (int handlepx, int time) /* time in nanoseconds */
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);


#ifdef MMSI
	if ((time < 1000) || (time > 12000))
		return (einval);
	cardpx[handlepx].exc_bd_bc->resptime = (unsigned char)(time/50);
#else

	if (cardpx[handlepx].singleFunction)
	{
		if ((time < 2000) || (time > 12000))
			return eresptimeerror;
	}
	else
	{
		if ((time < 2000) || (time > 32000))
			return (einval);
	}

//	cardpx[handlepx].exc_bd_bc->resptime = (unsigned char)(time/155);
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->resptime, (unsigned char)(time/155) );
#endif

	return (0);
}  /* end function Set_BC_Resp */



/*
Set_Bit_Cnt     Coded 28.02.96 by D. Koppel

Description: Determines how many bits will be sent for messages that have
requested BIT_CNT_ERR errors. The number sent will be the usual number of
bits in a word (20) + offset.

Input parameters:   offset   Number of bits to add or subtract from the
                             correct number of bits in a 1553 word (20).
                             Valid values are -3 through +3.
                            
Output parameters:  none

Return values:   emode   If not in BC mode
                 einval  If offset is less than -3 or greater than 3
                 0       If successful

*/

int borland_dll Set_Bit_Cnt (int offset)  /* offset from normal 20 bit word size */
{
	return Set_Bit_Cnt_Px(g_handlepx, offset);
}

int borland_dll Set_Bit_Cnt_Px (int handlepx, int offset)  /* offset from normal 20 bit word size */
{
	unsigned char tmpBit;

#define ZERO_CROSS_MASK 0xf0
	int count=0;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].singleFunction)
		return (enotforsinglefuncerror);

//	if ((cardpx[handlepx].exc_bd_bc->board_config == BC_MODE) ||
//		(cardpx[handlepx].exc_bd_bc->board_config == BC_RT_MODE) )
	if ((cardpx[handlepx].board_config == BC_MODE) ||
		(cardpx[handlepx].board_config == BC_RT_MODE) )
		/* in BC MODEs valid bit offset is -3,-2,-1,0,1,2,3 */
	{
		if ((offset < -3) || (offset > 3) )
			return (einval);
		else{
//			cardpx[handlepx].exc_bd_bc->bit = (cardpx[handlepx].exc_bd_bc->bit & ZERO_CROSS_MASK) 
//			| (unsigned char)(offset + 3);
			tmpBit = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->bit );
			tmpBit = (tmpBit & ZERO_CROSS_MASK) | (unsigned char)(offset + 3);
			WBYTE_H2D( cardpx[handlepx].exc_bd_bc->bit, tmpBit );
		}
	}

//	else if (cardpx[handlepx].exc_bd_bc->board_config == RT_MODE)          
	else if (cardpx[handlepx].board_config == RT_MODE)          
	{

            switch(offset)
            {
              case -3 : count= 5  ;
                        break;
              case -2 : count=  0 ;
                        break;
              case -1 : count=  2 ;
                        break;
              case  0 : count=  3 ;
                        break;
              case 1 :  count=  4 ;
                        break;
              case 2 :  count=  1 ;
                        break;
              case 3 :  count=  6 ;
                        break;
            }
//		cardpx[handlepx].exc_bd_rt->bit = (unsigned char)count;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->bit,(unsigned char)count );

	}
	else return (emode);
	return (0);
} /* end function Set_Bit_Cnt */



/*
Set_Bus

Description: Sets the  bus over which a particular message will
be transmitted.

Input parameters:  id       Message identifier returned from a prior call to
                            Create_Message
                   bus      Valid values are: XMT_BUS_A (default), XMT_BUS_B

Output parameters: none

Return values:   emode     If not in BC mode
                 ebadid    If message id is not a valid message id
                 ebadchan  If tried to set bus to illegal value
                 0         If successful

*/
int borland_dll Set_Bus (int id, int channel)
{
	return Set_Bus_Px (g_handlepx, id, channel);
}

int borland_dll Set_Bus_Px (int handlepx, int id, int channel)
{
	usint * msgptr,CtlWord,newCtlWord;

#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE) )
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);
	if ((channel != XMT_BUS_A) && (channel != XMT_BUS_B))
		return (ebadchan);
	msgptr = cardpx[handlepx].msgalloc[id];
	

	/* first turn off bus bit then set new bit */
//	*msgptr = (usint)((*msgptr &(~XMT_BUS_A)) | channel);
	CtlWord = RWORD_D2H(msgptr[0]);
	newCtlWord = (usint)((CtlWord &(~XMT_BUS_A)) | channel); 
	WWORD_H2D( msgptr[0], newCtlWord );
	return(0);
#endif
} /* end function Set_Bus */

/* for backward compatibility only */
int borland_dll Set_Channel (int id, int channel)
{
	return Set_Bus_Px (g_handlepx, id, channel);
}


/*
Set_Continue

Description: Restarts transmission that has been halted after a call to
Set_Halt.

Input parameters:   id   Message identifier returned from a prior call to
                         Create_Message

Output parameters:  none

Return values:   emode   If not in BC mode
                 ebadid  If message id is not a valid message id
                 0       If successful

*/

int borland_dll Set_Continue (int id)
{
	return Set_Continue_Px (g_handlepx, id);
}


int borland_dll Set_Continue_Px (int handlepx, int id)
{
	usint *msgptr;
	usint tempWord;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE) )
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);
	msgptr = cardpx[handlepx].msgalloc[id];

	/* turn off HALT bit */
//	tempWord = *msgptr;
//	tempWord &= ~HALT_CONT;
//	*msgptr = tempWord;

	tempWord = RWORD_D2H( msgptr[0] );
	tempWord &= ~HALT_CONT;
	WWORD_H2D( msgptr[0], tempWord );

	return(0);
} /* end function Set_Continue */



/*
Set_Frame_Time

Description: Determines the time each frame will take when the frame is to
be executed more than once. This value is the minimum time in microseconds
from the start of the frame to the start of the next repetition of the frame.

Input parameters:   time   Frame time in microseconds

Output parameters:  none

Return values:   emode  If not in BC mode
                 0      If successful

Note: (Not for MMSI): Frame time is composed of frametime resolution times (*) frametime.
The frame time resolution register has a value of 155 nanoseconds per bit.
The maximum resolution is therefore 155 * ffff (65535) = 10157925
nanoseconds or 10158 microseconds.  The card works best when the resolution
is maximized and the frametime is minimized. The algorithm first figures out
the minimum frame time and then finds the appropriate resolution.


*/

int borland_dll Set_Frame_Time (long time)   /* time in microseconds */
{
	return Set_Frame_Time_Px (g_handlepx, time);
}

int borland_dll Set_Frame_Time_Px (int handlepx, long time)   /* time in microseconds */
{
	long resolution;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

#ifdef MMSI

//	cardpx[handlepx].exc_bd_bc->frametime_resolution = (usint)(time >> 18);
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->frametime_resolution, (usint)(time >> 18) );

//	cardpx[handlepx].exc_bd_bc->frametime = (usint)(time >> 2);
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->frametime, (usint)(time >> 2) );

#else

//	cardpx[handlepx].exc_bd_bc->frametime = (usint)(time / 10158);
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->frametime, (usint)(time / 10158) );
//	resolution = (( time / (cardpx[handlepx].exc_bd_bc->frametime + 1)) * 1000) / 155;
	resolution = (( time / ( RWORD_D2H( cardpx[handlepx].exc_bd_bc->frametime ) + 1)) * 1000) / 155;
//	cardpx[handlepx].exc_bd_bc->frametime_resolution = (usint)resolution;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->frametime_resolution , (usint)resolution );

#endif

	return (0);
} /* end function Set_Frame_Time */



/*
Set_Halt

Description:  Halts transmission upon reaching this message. Transmission
will recommence when Set_Continue is called.

Input parameters:   id   Message identifier returned from a prior call to
                         Create_Message

Output parameters:  none

Return values:   emode   If not in BC mode
                 ebadid  If message id is not a valid message id
                 0       If successful

*/

int borland_dll Set_Halt (int id)
{
	return Set_Halt_Px (g_handlepx, id);
}

int borland_dll Set_Halt_Px (int handlepx, int id)
{
	usint *msgptr;
	usint tempWord;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);
	msgptr = cardpx[handlepx].msgalloc[id];

	/* turn on HALT bit */
//	tempWord = *msgptr;
//	tempWord |= HALT_CONT;
//	*msgptr = tempWord;
	tempWord = RWORD_D2H( msgptr[0] );
	tempWord |= HALT_CONT;
	WWORD_H2D( msgptr[0], tempWord );

	return(0);
} /* end function Set_Halt */



/*
Set_Interrupt    modified July 31, 97 by Josh Waxman
                 Fixed logic. Before elses were matching wrong ifs
                 (closest ones) by adding {}s, I forced the elses to their
                 proper ifs.

Description: Describes the conditions under which the board is to generate
an interrupt. Set_Int_Level must also be called before interrupts will be
generated. Multiple conditions (flags) may be ORed together.

Input parameters:  flag
                Mode:         Flag:
                -----         -----
                BCRT_MODE     MSG_CMPLT, END_OF_FRAME, MSG_ERR, END_MINOR_FRAME, SRQ_MSG
                RT_MODE       MSG_CMPLT
                MON_BLOCK     TRIG_RCVD,MSG_IN_PROGRESS,CNT_TRIG_MATCH,MSG_INTERVAL
                MON_LOOKUP    MSG_IN_PROGRESS

Output parameters: none

Return values:  einval  If interrupt is not appropriate to the current mode
                emode   If the board mode is not recognized
                0       If successful

Note: In RT mode, interrupts can also be generated using Set_RT_Interrupt.

*/

int borland_dll Set_Interrupt (int flag)
{
	return Set_Interrupt_Px (g_handlepx, flag);
}


int borland_dll Set_Interrupt_Px (int handlepx, int flag)
{
	int mode;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if(cardpx[handlepx].intnum == 0)
		return(noirqset);

	/* check for invalid flags, each mode has its own interrupts */
//	mode = cardpx[handlepx].exc_bd_bc->board_config;
	mode = cardpx[handlepx].board_config ;

	if (mode == BC_RT_MODE)
	{
		if (flag & ~VALID_BC_RT_INT)
			return (eintr);
	}
	else if (mode == RT_MODE)
	{
		/* if (flag != MSG_CMPLT) */
		if (flag & ~VALID_RT_INT)
			return (eintr);
	}
	else if (mode == MON_LOOKUP)
	{
		//if (flag != MSG_IN_PROGRESS)  - changed to line below so that can set interrupt OFF, with flag=0
		if (flag & ~MSG_IN_PROGRESS)
			return (eintr);
	}
	else if (mode == MON_BLOCK)
	{
		if (flag & ~VALID_MON_INT)
			return (eintr);
	}
	else
		return(emode);

	/* cardpx[handlepx].exc_bd_bc->interpt = (unsigned char)flag; */
	if (mode != RT_MODE)
//		cardpx[handlepx].exc_bd_bc->interpt = (unsigned char)flag;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->interpt,(unsigned char)flag );
	else
//		cardpx[handlepx].exc_bd_rt->int_cond = (unsigned char)flag;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->int_cond,(unsigned char)flag );

	return (0);
} /* end function Set_Interrupt */



/*
Set_Jump

Description: Sets up the board to jump to a new frame when the board gets
to the selected message.

Note: The original message denoted by id is destroyed by this routine.

Input parameters:  id       Message identifier returned from a prior call to
                            Create_Message for jumping message
                   frameid  Frame identifier returned from a prior call to
                            Create_Frame for frame to jump to
                   intcnt   Number of instructions in new frame to execute,
                            FULLFRAME for all

Output parameters: none

Return values:   emode          If not in BC mode
                 ebadid         If message id is not a valid message id
                 frm_erange     If frameid is not a valid frame id
                 frm_erangecnt  If intcnt is greater than the number of
                                messages in the frame
		 enoalter	If message is now in middle of being sent
                 0              If successful

*/

int borland_dll Set_Jump(int id, int frameid, int intcnt)
{
	return Set_Jump_Px (g_handlepx, id, frameid, intcnt);
}

int borland_dll Set_Jump_Px (int handlepx, int id, int frameid, int intcnt)
{
	usint volatile *msgptr;
	usint origCtrlword;
	int status, wasHalted = 0;
	usint tempWord;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check proper mode */
//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return (emode);

	/* range check message id */
	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);

	/* range check frame id */
	if ((frameid < 0) || (frameid >= cardpx[handlepx].nextframe))
		return (frm_erange);

	/* range check instruction count */
	if (intcnt > cardpx[handlepx].framealloc[frameid].msgcnt)
		return (frm_erangecnt);

	/* set up frame pointer and instruction count */
	msgptr = cardpx[handlepx].msgalloc[id];
//	origCtrlword = *msgptr;
	origCtrlword = RWORD_D2H( msgptr[0] );
	tempWord = origCtrlword & ~COMMAND_CODE;
	tempWord |= JUMP | HALT_CONT;  /*halt until all jump updates are complete */
//	*msgptr = tempWord;
	 WWORD_H2D( msgptr[0] , tempWord );

	if (origCtrlword & HALT_CONT)
		wasHalted = 1;

	status = BC_Check_Alter_Msg_Px (handlepx, id);
	if (status == enoalter)
	{
//		*msgptr = origCtrlword;
		WWORD_H2D( msgptr[0], origCtrlword );
		return enoalter;
	}

	msgptr++;
//	*msgptr = (usint)cardpx[handlepx].framealloc[frameid].frameptr;
	 WWORD_H2D( msgptr[0], (usint)cardpx[handlepx].framealloc[frameid].frameptr );

	/* mark the command type as a 'jump' */
	cardpx[handlepx].commandtype[id] = JUMP;

	msgptr++;
	if (intcnt == FULLFRAME)
//		*msgptr = (usint)cardpx[handlepx].framealloc[frameid].msgcnt;
		 WWORD_H2D( msgptr[0],(usint)cardpx[handlepx].framealloc[frameid].msgcnt );
	else
//		*msgptr = (usint)intcnt;
		WWORD_H2D( msgptr[0],(usint)intcnt );

	msgptr = cardpx[handlepx].msgalloc[id];
	if (!wasHalted)
//		*msgptr = tempWord & ~HALT_CONT; /*continue with jump in place */
		WWORD_H2D( msgptr[0], (tempWord & ~HALT_CONT) ); /*continue with jump in place */

	return(0);
} /* end function Set_Jump */

/*
Set_Retry

Description: Selects the number and type of retries to attempt in the event
of an error.

Input parameters: id       Message identifier returned from a prior call to
                           Create_Message
                  num      Number of retries to attempt:
                           0		For no retries
                           1-3	Number of valid retries
                  altchan  Whether to retry on the initial bus or to
                           alternate buses for each retry.
                           Legal flags are:
                           RETRY_SAME_BUS  Retries should be on original bus
                           RETRY_ALT_BUS   Retires should alternate buses

Output parameters: none

Return values:  emode   If not in BC mode
                ebadid  If message id is not a valid message id
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_Retry (int id, int num, int altchan)
{
	return Set_Retry_Px (g_handlepx, id, num, altchan);
}


int borland_dll Set_Retry_Px (int handlepx, int id, int num, int altchan)
{
	usint * msgptr;
	usint  tmpAutochannelandRetry;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);

	if ((num < 0) || (num > 3))
		return (einval);

#ifdef MMSI
	altchan = RETRY_SAME_BUS;
#else
	if ((altchan != RETRY_SAME_BUS) && (altchan != RETRY_ALT_BUS))
		return (einval);
#endif

	msgptr = cardpx[handlepx].msgalloc[id];

	/* set auto channel and auto retry bits. */
	/* 4 is magic number for auto retry placement in control word */

//	*msgptr = (usint)((*msgptr &(~RETRY_MASK)) | altchan | (num << 4));
	tmpAutochannelandRetry = RWORD_D2H( msgptr[0] );
	WWORD_H2D( msgptr[0],(usint)((tmpAutochannelandRetry &(~RETRY_MASK)) | altchan | (num << 4)) );
	return(0);
} /* end function Set_Retry */



/*
Set_Stop_On_Error

Description: Sets up the board to halt transmission upon reaching a message
for which this routine has been called and which returns an error status
after all retry attempts.

Input parameters:  id   Message identifier returned from a prior call to
                        Create_Message

Output parameters: none

Return values:  emode   If not in BC mode
                ebadid  If message id is not a valid message id
                0       If successful
*/

int borland_dll Set_Stop_On_Error (int id)
{
	return Set_Stop_On_Error_Px (g_handlepx, id);
}


int borland_dll Set_Stop_On_Error_Px (int handlepx, int id)
{
	usint * msgptr;
	usint tempWord;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);
	msgptr = cardpx[handlepx].msgalloc[id];
	/* turn on STOP_ON_ERROR bit */
//	tempWord = *msgptr;
	tempWord = RWORD_D2H( msgptr[0] );
	tempWord |= STOP_ON_ERROR;
//	*msgptr = tempWord;
	WWORD_H2D( msgptr[0],tempWord );
	return(0);
} /* end function Set_Stop_On_Error */



/*
Set_Var_Amp   

Description: If this option is implemented it lets you set the amplitude of
the 1553 output signal.

Input parameters:  millivolts   Amplitude in millivolts. Value will be
                                rounded up to the nearest 30 millivolts.
                                The range is from zero to 7.5 volts.

Output parameters: none

Return values:  einvamp   If tried to set invalid amplitude
                0         If successful

*/

int borland_dll Set_Var_Amp (int millivolts)
{
	return Set_Var_Amp_Px (g_handlepx, millivolts);
}

int borland_dll Set_Var_Amp_Px (int handlepx, int millivolts)
{
#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].isPCMCIA == 1)
		return func_invalid;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != RT_MODE))
	if ((cardpx[handlepx].board_config != BC_RT_MODE) &&
		(cardpx[handlepx].board_config != RT_MODE))
		return (emode);

	if (cardpx[handlepx].singleFunction)
		return(enotforsinglefuncerror);

	if (((millivolts/30) > 255) || (millivolts < 0))
		return einvamp;
//	cardpx[handlepx].exc_bd_bc->amplitude = (unsigned char)(millivolts/30);
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->amplitude,(unsigned char)(millivolts/30) );
	return (0);
#endif
}  /* end function Set_Var_Amp */


/*
Set_Word_Cnt

Description: Sets the number of words to send for messages that have
requested WD_CNT_ERR errors. The number sent will be the actual number of
words in the message + offset.

Input parameters:  offset   Offset from correct word count. Valid values
                            are -3 through +3.

Output parameters: none

Return values:  einval  If parameter is out of range
                emode   If not in BC mode
                0       If successful
*/

int borland_dll Set_Word_Cnt (int offset)  /* offset from correct word count */
{
	return Set_Word_Cnt_Px (g_handlepx, offset);  /* offset from correct word count */
}

int borland_dll Set_Word_Cnt_Px (int handlepx, int offset)  /* offset from correct word count */
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].singleFunction)
			return (enotforsinglefuncerror);

//	if ((cardpx[handlepx].exc_bd_bc->board_config == BC_MODE) ||
//		(cardpx[handlepx].exc_bd_bc->board_config == BC_RT_MODE))
	if ((cardpx[handlepx].board_config == BC_MODE) ||
		(cardpx[handlepx].board_config == BC_RT_MODE))
		/* in BC MODEs valid bit offset is -3,-2,-1,0,1,2,3 */
	{
		if ((offset < -3) || (offset > 3))
			return (einval);
		else
//			cardpx[handlepx].exc_bd_bc->word = (unsigned char)offset;
			WBYTE_H2D( cardpx[handlepx].exc_bd_bc->word ,(unsigned char)offset );
	}
	else
		return (emode);
	return (0);
}  /* end function Set_Word_Cnt */



/*
Clear_Card     Coded 02.01.95 by D. Koppel   ecp d 1

Description: Clears out all messages and message frames (and the Galahad
variable associated with them) from the memory of the current module. It
leaves the drivers in the same state as following a call to Set_Mode
(BC_MODE). This routine is used to get rid of old unneeded messages to
make room for new messages. It may be called to undo calls to Create_Message
and Create_Frame.

Input parameters:   none

Output parameters:  none

Return values:      0      If successful

*/

int borland_dll Clear_Card (void)
{
	return  Clear_Card_Px (g_handlepx);
}

int borland_dll Clear_Card_Px (int handlepx)
{
	int i;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	cardpx[handlepx].nextid = 1;
	cardpx[handlepx].nextframe = 0;

	/* now put msgs in the msg_block areas */
	cardpx[handlepx].nextmsg = cardpx[handlepx].exc_bd_bc->msg_block1;
	cardpx[handlepx].msg_block = 1;
	/* initstack is now 0, stack is just the instruction stack and not the msg block area */
	cardpx[handlepx].top_stack = &(cardpx[handlepx].exc_bd_bc->stack[0]);
	cardpx[handlepx].nextstack = cardpx[handlepx].top_stack;
	for (i=0 ; i < MSGMAX; i++)
		cardpx[handlepx].msgalloc[i] = 0;
	return (0);
}  /* end function Clear_Card */


/*
Clear Frame     Coded 11.01.96 by D. Koppel

Description: Clears out all message frames previously created via
Create_Frame while leaving messages created via Create_Message. May be used
in situations when a limited number of messages must be reformulated into
different frames in real time.

Note: It is often more efficient to use the Set_Skip and Set_Restore
functions for this situation.

Input parameters:   none

Output parameters:  none

Return values:      0     If successful

*/

int borland_dll Clear_Frame (void)
{
	return Clear_Frame_Px (g_handlepx);
}


int borland_dll Clear_Frame_Px (int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	cardpx[handlepx].nextframe = 0;
	cardpx[handlepx].nextstack = cardpx[handlepx].top_stack;
	return (0);
} /* end function Clear_Frame */



/*
Set_Skip     Coded 11.01.96 by D. Koppel

Description: Causes a message within a specified frame to be skipped. The
message can be restored later by calling Set_Restore.

Input parameters:  id   Message identifier returned from a prior call to
                        Create_Message

Output parameters: none

Return values:  emode   If not in BC mode
                ebadid  If message id is not a valid message id
                0       If successful
*/

int borland_dll Set_Skip (int id)
{
	return Set_Skip_Px (g_handlepx, id);
}

int borland_dll Set_Skip_Px (int handlepx, int id)
{
	usint * msgptr;
	usint tempWord;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);
	msgptr = cardpx[handlepx].msgalloc[id];
	/* turn on SKIP bit */
//	tempWord = *msgptr;
	tempWord = RWORD_D2H( msgptr[0] );
	tempWord &= ~COMMAND_CODE;
	tempWord |= SKIP;
//	*msgptr = tempWord;
	WWORD_H2D( msgptr[0] , tempWord);

	return(0);
} /* end function Set_Skip */



/*
Set_Restore     Coded 11.01.96 by D. Koppel

Description: Restores a message that was set to skip or to jump to be sent
as usual.

Input parameters: id        Message identifier returned from a prior call to
                            Create_Message
                  msgtype   One of the following:
                            RT2BC       RT to BC message
                            RT2RT       RT to RT message
                            BC2RT       BC to RT message
                            MODE        Mode command
                            BRD_RCV     Broadcast BC to RT
                            BRD_RT2RT   Broadcast RT to RT
                            BRD_MODE    Broadcast mode code

Note: Msgtype must be the same as was used in Create_1553_Message. Set_Skip
destroys this information so Set_Restore must supply it again.

Output parameters: none

Return values:  emode     If not in BC mode
                ebadid    If message id is not a valid message id
                einval    If parameter is out of range
                enoskip   If tried to restore a message that wasn't skipped
                0         If successful
*/

int borland_dll Set_Restore (int id, int msgtype)
{
	return Set_Restore_Px (g_handlepx, id, msgtype);
}

int borland_dll Set_Restore_Px (int handlepx, int id, int msgtype)
{
	usint * msgptr;
	usint tempWord;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);
	if (msgtype >= SKIP) 
		return (einval);
	/* this also checks that commandtype wasn't changed to JUMP */
	if (msgtype != cardpx[handlepx].commandtype[id])
		return (einval);
	msgptr = cardpx[handlepx].msgalloc[id];
	/* message was not a SKIP type */
//	tempWord = *msgptr;
	tempWord = RWORD_D2H( msgptr[0] );
	if ((tempWord & COMMAND_CODE) != SKIP)
		return (enoskip);

	/* replace command code */
	tempWord &= ~COMMAND_CODE;
	tempWord |= (usint)msgtype;
//	*msgptr = tempWord;
	WWORD_H2D( msgptr[0],tempWord);

	return(0);
} /* end function Set_Restore */



/*
Decription:
This function is valid only if Frequency mode is supported on this module, and it is turned ON.
Send a message once, asynchronously, while in Frequency Mode.
The message id should be created, but should not be added to the FRAME structure.

See also function Set_Frequency_Mode_Px which must be called before this function.
*/
int borland_dll Send_Msg_Once_Px (int handlepx, int id, int msgtype)
{
	usint * msgptr;
	usint tempWord;
	
#ifdef MMSI
	return (func_invalid);
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	if (cardpx[handlepx].frequencyModeON == FALSE)
		return efrequencymodemustbeon;

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);
	if (msgtype >= SKIP) 
		return (einval);
	/* this also checks that commandtype wasn't changed to JUMP */
	if (msgtype != cardpx[handlepx].commandtype[id])
		return (einval);
	msgptr = cardpx[handlepx].msgalloc[id];
	/* message was not a SKIP type */
	tempWord = RWORD_D2H( msgptr[0] );
	if ((tempWord & COMMAND_CODE) != SKIP)
		return (enoskip);

	/* replace command code */
	tempWord &= ~COMMAND_CODE;
	tempWord |= (usint)msgtype | CTL_SEND_ONCE;
	WWORD_H2D( msgptr[0],tempWord);

	return(0);
} /* end function Send_Msg_Once_Px */

/*
Set_Minor_Frame_Time - Coded 29 Dec 97 by A. Meth

Description: Sets the minor frame time & its resolution - for a MINOR_FRAME
type of message. This message functions as a "delay time" message. The time
given is from beginning of message sequence to end of first minor frame,
and from beginning of a minor frame to beginning of the next minor frame.

The way to observe how this works is to run a Bus Monitor on the data bus
and to observe the time tag for each message. The MINOR_FRAME message does
*not* appear as a real message on the data bus (on the Bus Monitor side).

Input parameters:   micro_sec    minor frame time in micro-seconds
                                 maximum legal value is 800,000
                                 (which translates to 800 millisec)
Output parameters:  none

Return values:   emode  If not in BC or BC/RT mode
                 0      If successful

Note: This routine is valid in BC/Concurrent RT mode only.
*/

int borland_dll Set_Minor_Frame_Time (long micro_sec)
{
	return Set_Minor_Frame_Time_Px (g_handlepx, micro_sec);
}

int borland_dll Set_Minor_Frame_Time_Px (int handlepx, long micro_sec)
{
	usint multiplier;
	long value;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return (emode);

	multiplier = (usint)((micro_sec-1) / (long)(0xffff)) + 1;
	value = (micro_sec / multiplier);

//	cardpx[handlepx].exc_bd_bc->minor_frametime = (usint)(value);
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->minor_frametime , ((usint)(value)) );
//	cardpx[handlepx].exc_bd_bc->minor_frame_resolution = multiplier;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->minor_frame_resolution , multiplier );

	return 0;
} /* end function Set_Minor_Frame_Time */

int borland_dll Set_Replay (int flag)
{
	return Set_Replay_Px (g_handlepx, flag);
}

int borland_dll Set_Replay_Px (int handlepx, int flag)
{
#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_MODE))
	if ((cardpx[handlepx].board_config != BC_RT_MODE) &&
		(cardpx[handlepx].board_config != BC_MODE))
		return (emode);

	if ((flag != ENABLE) && (flag != DISABLE))
		return einval;

//	cardpx[handlepx].exc_bd_bc->replay = (unsigned char)flag;
	WBYTE_H2D( cardpx[handlepx].exc_bd_bc->replay , ((unsigned char)flag) );
	return 0;
#endif
}


/*
Send_Timetag_Sync

int borland_dll Send_Timetag_Sync(int flag);

Description: Send the current timetag as the single data word when the BC 
sends out a Mode Code Synchronize
with Data message (MC-17). The timetag is sent out with resolution of 64 
microseconds.

Input parameters:   flag        Legal flags are:
                                 ENABLE - to enable the feature
                                 DISABLE - to disable the feature

Output parameters:  none

Return values:  ebadhandle      If invalid handle specified
                  emode          If board is not in BC or BC_RT mode
                 einval          If parameter is out of range
                 0               If successful
*/
int borland_dll Send_Timetag_Sync (int flag)
{
	return Send_Timetag_Sync_Px (g_handlepx, flag);
}

int borland_dll Send_Timetag_Sync_Px (int handlepx, int flag)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_MODE))
	if ((cardpx[handlepx].board_config != BC_RT_MODE) &&
		(cardpx[handlepx].board_config != BC_MODE))
		return (emode);

	if ((flag != ENABLE) && (flag != DISABLE))
		return einval;

//	cardpx[handlepx].exc_bd_bc->ttagsendsync = (unsigned char)flag;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->ttagsendsync , (unsigned char)flag );
	return 0;
}

int borland_dll Enable_SRQ_Support_Px (int handlepx, int enableflag)
{
	usint tempWord;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ((enableflag != ENABLE) && (enableflag != DISABLE))
		return einval;

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return emode;

//	tempWord = cardpx[handlepx].exc_bd_bc->bcProtocolOptions;
	tempWord = RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions );
	if (enableflag == ENABLE)
		tempWord &= ~NOSRQ_BIT;
	else
		tempWord |= NOSRQ_BIT;

//	cardpx[handlepx].exc_bd_bc->bcProtocolOptions = tempWord;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->bcProtocolOptions , tempWord );
	return 0;
}


/*
Decription:
New mode to set up a bus list where messages are transmitted at a specified frequency.
The frequency of each message (in microseconds) in this bus list is assigned to the gaptime field in the FRAME structure.

See also function Send_Msg_Once_Px which can be used in this mode to transmit a message asynchronously.
*/
int borland_dll Set_Frequency_Mode_Px (int handlepx, int enableflag)
{
	usint tempWord;

#ifdef MMSI
	return (func_invalid);
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ((enableflag != ENABLE) && (enableflag != DISABLE))
		return einval;

	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return emode;

	if (cardpx[handlepx].frequencyModeSupported == FALSE)
		return efrequencymodenotsupported;

	tempWord = RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions );
	if (enableflag == DISABLE)
	{
		tempWord &= ~BC_PROT_FREQUENCY;
		cardpx[handlepx].frequencyModeON = FALSE;
	}
	else
	{
		tempWord |= BC_PROT_FREQUENCY;
		cardpx[handlepx].frequencyModeON = TRUE;
	}

	WWORD_H2D( cardpx[handlepx].exc_bd_bc->bcProtocolOptions , tempWord );
	return 0;
}

/*
Set_Sync_Pattern     Coded 23.05.04 by D. Koppel

Description: Determines the pattern the sync error will take. Each of the low six bits represents
		1/6 of the sync time

Input parameters:   pattern  
                            
Output parameters:  none

Return values:   emode   If not in BC mode
                 einval  If pattern uses more than 6 bits
                 0       If successful

*/


int borland_dll Set_Sync_Pattern_Px (int handlepx, int pattern)  
{
	usint tmpProtocolOptions;

#define PROTOCOL_REG_ERROR_ENABLE	4
#define VALID_SYNC_PATTERN_MASK		0x3f


#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if (cardpx[handlepx].processor == PROC_AMD)
		return func_invalid;
//	if (cardpx[handlepx].exc_bd_bc->revision_level < 0x1a)
	if ( RBYTE_D2H( cardpx[handlepx].exc_bd_bc->revision_level ) < 0x1a)
		return func_invalid;

//	if (cardpx[handlepx].exc_bd_bc->board_config == BC_RT_MODE) {
	if (cardpx[handlepx].board_config == BC_RT_MODE)
	{
		if (pattern & ~VALID_SYNC_PATTERN_MASK )  /* only bits 0 through 5 should be used */
			return (einval);
		else
		{
//			cardpx[handlepx].exc_bd_bc->bcProtocolOptions |= PROTOCOL_REG_ERROR_ENABLE;
			tmpProtocolOptions = RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions );
			tmpProtocolOptions |= PROTOCOL_REG_ERROR_ENABLE;
			WWORD_H2D( cardpx[handlepx].exc_bd_bc->bcProtocolOptions , tmpProtocolOptions );

//			cardpx[handlepx].exc_bd_bc->sync_pattern = pattern;
			WWORD_H2D( cardpx[handlepx].exc_bd_bc->sync_pattern , pattern );
		}
	}
	else
		return (emode);

	return (0);
#endif
} /* end function Set_Sync_Pattern */

/*
Set_Error_Location     Coded 23.05.04 by D. Koppel

Description: Determines which word and which error bit errors should be inserted into. Only zero cross errors
			 make use of bit position

Input parameters:   errorWord	word number (0 = first word) to inject error into
                    errorBit	bit number  (0 = first bit) to inject bit errors into

Output parameters:  none

Return values:   emode   If not in BC mode
                 einval  If pattern uses more than 6 bits
                 0       If successful

*/


int borland_dll Set_Error_Location_Px (int handlepx, unsigned int errorWord, unsigned int zcerrorBit)  
{

#ifdef MMSI
	return (func_invalid);
#else
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].processor == PROC_AMD)
		return func_invalid;
//	if (cardpx[handlepx].exc_bd_bc->revision_level < 0x1a)
	if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->revision_level ) < 0x1a)
		return func_invalid;

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return emode;
	if (errorWord > 31) 
		return (einval);
	if (zcerrorBit > 16)
		return (einval);
	
	if (cardpx[handlepx].singleFunction)
		return (enotforsinglefuncerror);

//	cardpx[handlepx].exc_bd_bc->err_word_index = errorWord;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->err_word_index , errorWord );
//	cardpx[handlepx].exc_bd_bc->err_bit_index = zcerrorBit;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->err_bit_index , zcerrorBit );
	return (0);
#endif
} /* end function Set_Error_Location */

int borland_dll Set_Zero_Cross_Px (int handlepx, unsigned int zctype)  /* zero cross error pattern */
{
	unsigned char tmpBit;

#define BIT_CNT_MASK 0xf
#ifdef _WIN32
	int count=0;
#endif

#ifdef MMSI
	return (func_invalid);
#else
	if (cardpx[handlepx].processor == PROC_AMD)
		return func_invalid;
//	if (cardpx[handlepx].exc_bd_bc->revision_level < 0x1a)
	if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->revision_level ) < 0x1a)
		return func_invalid;
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].singleFunction)
			return (enotforsinglefuncerror);

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return emode;
	/* in BC MODEs valid patterns are 0 through 11 */
	if ((zctype > 10 ) || (zctype == 7))
		return (einval);
	else{
//		cardpx[handlepx].exc_bd_bc->bit = (cardpx[handlepx].exc_bd_bc->bit & BIT_CNT_MASK)
//		|(unsigned char)(zctype << 4);
		tmpBit = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->bit );
		tmpBit = (tmpBit & BIT_CNT_MASK) | (unsigned char)(zctype << 4);
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->bit , tmpBit );
	}
	return 0;
#endif
}

/*
Set_IMG_On_Skip_Px - for Nios cards

Description: Set the module to wait for IMGs even for SKIP messages
Input parameters: 
		handlepx		handle from Init_Module_Px
		enableflag      ENABLE, DISABLE
Output parameters: none
Return values:	0 		success
		ebadhandle    	bad handle
		func_invalid	invalid for this processor
		emode			wrong mode for this function (not BC-RT mode)
*/

int borland_dll Set_IMG_On_Skip_Px (int handlepx, int enableflag)  
{
	usint tempWord;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((cardpx[handlepx].processor == PROC_AMD) || (cardpx[handlepx].processor == PROC_INTEL))
		return func_invalid;

//	if (cardpx[handlepx].exc_bd_bc->board_config == BC_RT_MODE)
	if (cardpx[handlepx].board_config == BC_RT_MODE)

	{
//		tempWord = cardpx[handlepx].exc_bd_bc->bcProtocolOptions;
		tempWord = RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions );

		if (enableflag == ENABLE)
			tempWord |= BC_PROT_SKIP_WITH_IMG;
		else
			tempWord &= ~BC_PROT_SKIP_WITH_IMG;

//		cardpx[handlepx].exc_bd_bc->bcProtocolOptions = tempWord;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->bcProtocolOptions , tempWord );
	}
	else return (emode);

	return (0);
} /* end function Set_IMG_On_Skip_Px */

// These functions were moved to file bcframe.c .
// The next stage is to merge these functions with the corresponding Px functions.
// int borland_dll Enable_Checksum_MMSI_Px(int hanlepx, int id, int enable);
// int borland_dll Enable_Checksum_Error_MMSI_Px(int handlepx, int id, int enable);
