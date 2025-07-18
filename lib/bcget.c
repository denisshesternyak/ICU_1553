#include "pxIncl.h"
extern INSTANCE_PX cardpx[];

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)


/*
Get BC related Information - Coded 30.10.90 by D. Koppel

Description: These routines are called to get various parameters from the
Excalibur card. They all return the requested information via the function
return value.

Input:  None

Output: Board information

*/



/*
Get_BC_Status

Description:  Indicates what condition triggered an interrupt. It may also
be used for polling applications. It returns a status word that should be
checked for the following flags: MSG_CMPLT, END_OF_FRAME, MSG_ERR,
WAIT_FOR_CONTINUE.

Note: You must reset this status with the Reset_BC_Status command to clear
these flags.

Input parameters:   none

Output parameters:  none

Return values:   emode       If not in BC mode
                 0 or more of the following flags:
                 MSG_CMPLT   END_OF_FRAME
                 MSG_ERR     WAIT_FOR_CONTINUE
	         	END_MINOR_FRAME
		 		SRQ_MSG

*/

int borland_dll Get_BC_Status (void)
{
	return Get_BC_Status_Px(g_handlepx); 
}


int borland_dll Get_BC_Status_Px (int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	return (cardpx[handlepx].exc_bd_bc->message_status);
	return ( RBYTE_D2H( cardpx[handlepx].exc_bd_bc->message_status ) );

}  /* end function Get_BC_Status */



/*
Get_Msg_Status

Description: Returns a status word associated with a selected message
transmitted by the Bus Controller. If a valid message status is returned,
the routine clears the status.

Input parameters:   id    Message identifier returned from a prior call to
                          Create_Message

Output parameters:  none

Return values:  On success, returns 0 or more of the following flags:
                   END_OF_MSG    INVALID_WORD
                   BAD_BUS       WD_CNT_HI
                   ME_SET        WD_CNT_LO
                   BAD_STATUS    BAD_SYNC
                   INVALID_MSG   BAD_RT_ADDR
                   LATE_RESP     MSG_ERROR
                   SELFTEST_ERR  BAD_GAP
                ebadid     If message id is not a valid message id
                frm_nostack   if Start_Frame was not yet called to set up the message stack

*/

int borland_dll Get_Msg_Status (int id)
{
	return Get_Msg_Status_Px (g_handlepx, id);
}

int borland_dll Get_Msg_Status_Px (int handlepx, int id)
{
	usint *msgptr;
	usint temp_status;
	usint temp_mbptr;
	struct instruction_stack * stackptr;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ( (cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE) )
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);

	msgptr = (usint *)(cardpx[handlepx].msgalloc[id]);

	/* if the curstack pointer is NULL, then we have not yet called Start_Frame; return error msg; */
	/* added 26 Apr 99 by A. Meth, in case call this function before call to Start_Frame;  */
	/* this could be as a result of calling BC_Check_Alter_Msg too early */

	if (cardpx[handlepx].curstack == NULL) return(frm_nostack);

#ifdef MMSI
	for (stackptr = cardpx[handlepx].curstack; stackptr < (struct instruction_stack *)&(cardpx[handlepx].exc_bd_bc->stack[STACKSIZE_MMSI-1]);stackptr++)
#else
	for (stackptr = cardpx[handlepx].curstack; stackptr < (struct instruction_stack *)&(cardpx[handlepx].exc_bd_bc->stack[STACKSIZE_PX-1]);stackptr++)
#endif

	{
//		if (stackptr->message_block_pointer == ((char *)msgptr- (char*)cardpx[handlepx].exc_bd_bc))
		temp_mbptr = RWORD_D2H( stackptr->message_block_pointer );
		if (temp_mbptr == ((char *)msgptr- (char*)cardpx[handlepx].exc_bd_bc))
			break;
	}

//	temp_status = stackptr->message_status_word;
	temp_status = RWORD_D2H( stackptr->message_status_word );

	if ((temp_status != 0) && (temp_status != NO_ALTER))
//		stackptr->message_status_word = 0;
		WWORD_H2D( stackptr->message_status_word, 0 );

	#ifdef MMSI
	/* if LINK mode, and MSW indicates wrong RT address, and there are no other error bits, then this is not an error; turn off ERR bit 00 */
	if (cardpx[handlepx].exc_bd_bc->hubmode_MMSI == HUB_LINK)
	{
		if ((temp_status & BAD_RT_ADDR) == BAD_RT_ADDR)
		{
			if ((temp_status & ERROR_BITS_NO_RT_ADDR) == 0)
			{
				temp_status = temp_status & ~0x1; /* mask out Error bit */
			}
		}
	}
	#endif

	return (temp_status);
}   /* end function Get_MSG_Status */


/*
Get_Minor_Frame_Time - Coded 09 Jul 98 by A. Meth

Description: Gets the previously stored minor frame time in microseconds.
             This value was set in a call to Set_Minor_Frame_time.


Input parameters:   none
Output parameters:  none

Return values:      minor frame time in microseconds

Note: This routine is valid in BC/Concurrent RT mode only.
*/
int borland_dll Get_Minor_Frame_Time (long *frame_time)
{
	return Get_Minor_Frame_Time_Px(g_handlepx, frame_time);
}

int borland_dll Get_Minor_Frame_Time_Px (int handlepx, long *frame_time)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return (emode);

//	*frame_time =
//		cardpx[handlepx].exc_bd_bc->minor_frametime * cardpx[handlepx].exc_bd_bc->minor_frame_resolution;
	*frame_time =
		RWORD_D2H( cardpx[handlepx].exc_bd_bc->minor_frametime ) * RWORD_D2H( cardpx[handlepx].exc_bd_bc->minor_frame_resolution );
	return 0;

} /* end function Get_Minor_Frame_Time */

/*
Get_Msgentry_Status

Description: Returns a status word transmitted by the Bus Controller associated with a 
selected message in a selected frame.
If a valid message status is returned, the routine clears the status.

Input parameters:  frameid      Frame identifier returned from a prior call
                                to Create_Frame
                   msgentry     Entry within the frame, i.e., 0 for the
                                first message in the frame, 1 for the second
                                message etc.

Output parameters: msgstatusword
On success, 0 or more of the following flags:
                   END_OF_MSG    INVALID_WORD
                   BAD_BUS       WD_CNT_HI
                   ME_SET        WD_CNT_LO
                   BAD_STATUS    BAD_SYNC
                   INVALID_MSG   BAD_RT_ADDR
                   LATE_RESP     MSG_ERROR
                   SELFTEST_ERR  BAD_GAP

Return values:  
		ebadhandle
		emode  
		frm_erange     If frameid is not a valid frame id
		frm_erangecnt  If msgentry is out of range
		0              If successful

*/
#define MSGENTRY_SIZE 4

int borland_dll Get_Msgentry_Status (int frameid, int msgentry, usint *msgstatusword)
{
	return Get_Msgentry_Status_Px (g_handlepx, frameid, msgentry,msgstatusword);
}

int borland_dll Get_Msgentry_Status_Px (int handlepx, int frameid, int msgentry, usint *msgstatusword)
{
	usint *p_msgstatusword;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE) )
		return (emode);

	/* range check frame id */
	if ((frameid < 0) || (frameid >= cardpx[handlepx].nextframe))
		return (frm_erange);

	/* range check msgentry */
	if (msgentry <0)
		return einval;

	if (msgentry >= cardpx[handlepx].framealloc[frameid].msgcnt)
		return (frm_erangecnt);

	/* get pointer to beginning of instruction stack for given frame */
	p_msgstatusword = (usint *)(cardpx[handlepx].exc_bd_bc) +
		((cardpx[handlepx].framealloc[frameid].frameptr) / sizeof(usint));

	/* point to message within frame */
	p_msgstatusword += (msgentry * MSGENTRY_SIZE);

	/* point to msg status word within message */
	p_msgstatusword += 3;
//	*msgstatusword = *p_msgstatusword;
//	*msgstatusword =  b2drmReadWordFromDpr( (unsigned int) p_msgstatusword, "Get_Msgentry_Status_Px::p_msgstatusword" );
	*msgstatusword = RWORD_D2H(p_msgstatusword[0]);

	/* reset the msg_status_word */
	if ((*msgstatusword != 0) && (*msgstatusword != NO_ALTER))
//		*p_msgstatusword = 0;
//		b2drmWriteWordToDpr( (unsigned int) p_msgstatusword , (unsigned short) 0 , "Get_Msgentry_Status_Px::p_msgstatusword"  );	
		WWORD_H2D(p_msgstatusword[0], 0);

	#ifdef MMSI
	/* if LINK mode, and MSW indicates wrong RT address, and there are no other error bits, then this is not an error; turn off ERR bit 00 */
	if (cardpx[handlepx].exc_bd_bc->hubmode_MMSI == HUB_LINK)
	{
		if ((*msgstatusword & BAD_RT_ADDR) == BAD_RT_ADDR)
		{
			if ((*msgstatusword & ERROR_BITS_NO_RT_ADDR) == 0)
			{
				*msgstatusword = *msgstatusword & ~0x1; /* mask out Error bit */
			}
		}
	}
	#endif

	return (0);
}   /* end function Get_Msgentry_Status */


 
/*
Get_Instruction_Counter_Px 			Coded 11-Sep-14 by Asher Meth

Description: Reads the current value of the instruction counter

Processor required: any

Input parameters:
	handlepx		Module handle returned from the earlier call to Init_Module_Px

Output parameters:
	instructionCounter	instruction counter current value

Return values:
	ebadhandle		If handle is invalid
	emode			If not in BC mode
	0			If successful
*/

int borland_dll Get_Instruction_Counter_Px (int handlepx, usint *instructionCounter)
{
	usint tempInstructionCounter;
	
	// Verify that the value of module's handle is valid (i.e. between zero and NUMCARDS-1).
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return (ebadhandle);

	// Verify that the module's handle is allocated/initialized.
	if (cardpx[handlepx].allocated != 1) return (ebadhandle);

	// Verify that we are in BC or BC/RT mode
//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
//		return (emode);
	if ( (cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE) )
		return (emode);

	// Get the instruction counter value. It is stored in two registers, 0x3FEB followed by 0x3FEA
//	*instructionCounter = (cardpx[handlepx].exc_bd_bc->instruction_counter >> 8) +
//		((cardpx[handlepx].exc_bd_bc->instruction_counter & 0xFF) << 8);

	tempInstructionCounter = RWORD_D2H(cardpx[handlepx].exc_bd_bc->instruction_counter);
	*instructionCounter = (tempInstructionCounter >> 8) + ((tempInstructionCounter & 0xFF) << 8);
	
	return (0);
}  // end function Get_Instruction_Counter_Px

