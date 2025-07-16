#include "pxIncl.h"

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)

extern INSTANCE_PX cardpx[];
usint nothing;

int globalTemp1;
usint tmpval2;

/*
Clear_Msg_Blks     Coded 04.12.90 by D. Koppel

Description: Clears out all 1553 messages previously received and stored in
memory. The Monitor Status register is also cleared. If the card is running,
it is stopped, cleared and restarted.

Input parameters:  none

Output parameters: none

Return values:  emode  If board is not in Monitor mode
                0      If successful
*/

#define CLEARMEM 8 /* bit in start register */
int borland_dll Clear_Msg_Blks (void)
{
	return Clear_Msg_Blks_Px (g_handlepx);
}


int borland_dll Clear_Msg_Blks_Px (int handlepx)
{
	char oldstart;
	long i;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if ((cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) &&
//		(cardpx[handlepx].exc_bd_monseq->board_config != MON_LOOKUP))
	if ((cardpx[handlepx].board_config != MON_BLOCK) &&
		(cardpx[handlepx].board_config != MON_LOOKUP))

		return (emode);

	/* if card is running: stop the card. Zero out the memory */
//	oldstart = cardpx[handlepx].exc_bd_monseq->start;
	oldstart = RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->start );

//	cardpx[handlepx].exc_bd_monseq->start = 0;
	WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->start, 0 );

	/*      while ((cardpx[handlepx].exc_bd_bc->board_status & BOARD_HALTED) != BOARD_HALTED); */
	do
	{
//		tmpval2 = cardpx[handlepx].exc_bd_bc->board_status;
		tmpval2 = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status );
	}while ((tmpval2 & BOARD_HALTED) != BOARD_HALTED);

	// .. clear 0x0000h-3e7fh - First Message block area  (200 blocks)
	for (i=0; i<MONSEQ_MSGBLOCK_AREA_1; i++)
//		cardpx[handlepx].exc_bd_monseq->msg_block[i] = (usint)0;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->msg_block[i], (usint)0 );

	// These blocks are not used in MON_LOOKUP mode; they're marked
	//  "Data Block Look-up Table" or "Reserved" in the memory map; hence clearing the second
	//  message block area will wipe out the MON_LOOKUP's Data Block Look-up Table!

	if (cardpx[handlepx].board_config != MON_LOOKUP)
	{
		// .. clear 4000h - 6fcfh Second message block (153 blocks)
		for (i=0; i<MONSEQ_MSGBLOCK_AREA_2; i++)
//			cardpx[handlepx].exc_bd_monseq->msg_block2[i] = (usint)0;
			WWORD_H2D( cardpx[handlepx].exc_bd_monseq->msg_block2[i], (usint)0 );

		// .. clear 7100h - 7fafh Third message block (47 blocks)
		for (i=0; i<MONSEQ_MSGBLOCK_AREA_3; i++)
//			cardpx[handlepx].exc_bd_monseq->msg_block3[i] = (usint)0;
			WWORD_H2D( cardpx[handlepx].exc_bd_monseq->msg_block3[i], (usint)0 );

		// .. clear 7fb0h - fcafh Fourth message block (400 blocks)
		for (i=0; i<MONSEQ_MSGBLOCK_AREA_4; i++)
//			cardpx[handlepx].exc_bd_monseq->msg_block4[i] = (usint)0;
			WWORD_H2D( cardpx[handlepx].exc_bd_monseq->msg_block4[i], (usint)0 );
	} // end if not MON_LOOKUP mode

	/* 14mar07: we need to reset the nextblock pointer to the position where the firmware
	will write the next message */

	/* For expanded mode in sequential monitor */
//	if ((cardpx[handlepx].exc_bd_monseq->board_config == MON_BLOCK) && 
	if ((cardpx[handlepx].board_config == MON_BLOCK) && 
		(cardpx[handlepx].isExpandedMon))
//		cardpx[handlepx].nextblock = cardpx[handlepx].exc_bd_monseq->ExpandedCurrentBlock;
		cardpx[handlepx].nextblock = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->ExpandedCurrentBlock );
	else  
//		cardpx[handlepx].nextblock = cardpx[handlepx].exc_bd_monseq->msg_counter;
		cardpx[handlepx].nextblock = RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->msg_counter );

	/* 14mar07: For AMD processor, the msg_counter (at the firmware level) is one less than
	that for the newer processors; we are NOT using the prior-used feature of the CLEARMEM bit
	in the start register */
	if (cardpx[handlepx].processor == PROC_AMD) {
		if (cardpx[handlepx].exc_bd_monseq->msg_counter < (MAXMONBLOCKS_OLD-1))
			cardpx[handlepx].nextblock += 1;
		else
			cardpx[handlepx].nextblock = 0;
	}

	/* restart monitoring if was started */
//	cardpx[handlepx].exc_bd_monseq->start = oldstart;
	WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->start, oldstart );
	return(0);
}


/*
Get_Message     Coded 04.12.90 by D. Koppel

Description: Retrieves a specified message from the Message Block Area.
The legal range is 0-127.

Input parameters: int msgnum;  Range is 0-127. The blknum for the most
                               recent message is obtained via the
                               Get_Last_Blknum routine.
                  struct MONMSG *msgptr
                               Pointer to the structure defined below in
                               which to return the data. Space should
                               always be allocated for 36 words of data to
                               accommodate the maximum case of RT2RT
                               transmission of 32 data words + 2 status
                               words + 2 command words.
                  typedef struct {
                  int msgstatus; Status word containing the following flags:
                                 END_OF_MSG     RT2RT
                                 INVALID_MSG    BUS_A_XFER
                                 INVALID_WORD   WD_CNT_HI
                                 WD_CNT_LO      BAD_RT_ADDR
                                 BAD_SYNC       BAD_GAP
                                 LATE_RESP      ERROR
                  long elapsedtime;  A time tag. If the card set up for 32
                                     bit time tagging the full 32 bits will
                                     be recorded with a precision of 4
                                     microseconds per bit. In 16 bit mode
                                     the precision is 15.5 microseconds
                                     per bit.
                  int *words;        A pointer to an array of 1553 words.

Output parameters: none

Return values:  emode   If board is not in Monitor mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Get_Message (int blknum, struct MONMSG *msgptr)
{
	return Get_Message_Px (g_handlepx, blknum, msgptr);
}

int borland_dll Get_Message_Px (int handlepx, int blknum, struct MONMSG *msgptr)
{
	usint *blkptr;
	struct MONMSGPACKED msgCopy1packed;
	struct MONMSGPACKED msgCopy2packed;
	unsigned long ttaglast;
	int wordLoopCounter;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if ((cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) &&
//		(cardpx[handlepx].exc_bd_monseq->board_config != MON_LOOKUP))
	if ((cardpx[handlepx].board_config != MON_BLOCK) &&
		(cardpx[handlepx].board_config != MON_LOOKUP))
		return (emode);

	if ((blknum > 199) ||
//		((cardpx[handlepx].exc_bd_monseq->board_config == MON_LOOKUP) && (blknum > 127)) )
		((cardpx[handlepx].board_config == MON_LOOKUP) && (blknum > 127)) )
		return (einval);

	blkptr = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block[blknum*MSGSIZE]);

	// zero out the status word before beginning
	msgCopy1packed.msgstatus = 0;

	b2drmReadWordBufferFromDpr( (DWORD_PTR) blkptr,
								(unsigned short *) &msgCopy1packed,
								sizeof(struct MONMSGPACKED) / sizeof(unsigned short),
								"Get_Message_Px",
								B2DRMHANDLE );

	// help speed up message reads when there is nothing to read; if the EOM flag is not set, then there is no reason to re-read the message just yet
	if ((msgCopy1packed.msgstatus & END_OF_MSG) != END_OF_MSG)
		return enomsg;

	//get a second copy so we can check if any changes were made while we were in mid read
	b2drmReadWordBufferFromDpr( (DWORD_PTR) blkptr,
								(unsigned short *) &msgCopy2packed,
								sizeof(struct MONMSGPACKED) / sizeof(unsigned short),
								"Get_Message_Px",
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

	// If we're in MON_LOOKUP mode, we leave the timetag value as is;
	// but if we're in MON_BLOCK ? mode, then we return the elapsed usec
	// between this message and the previous one.
//	if (cardpx[handlepx].exc_bd_monseq->board_config == MON_BLOCK)
	if (cardpx[handlepx].board_config == MON_BLOCK)
	{
		if (blknum > 0)
		{
//			ttaglast = cardpx[handlepx].exc_bd_monseq->msg_block[(blknum-1)*MSGSIZE] |
//				((long)(cardpx[handlepx].exc_bd_monseq->msg_block[(blknum-1)*MSGSIZE+1]) << 16);
			ttaglast = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->msg_block[(blknum-1)*MSGSIZE] ) |			
				((long)(RWORD_D2H( cardpx[handlepx].exc_bd_monseq->msg_block[(blknum-1)*MSGSIZE+1] )) << 16);
		}
		else
		{
//			ttaglast = cardpx[handlepx].exc_bd_monseq->msg_block[199*MSGSIZE] |
//				((long)(cardpx[handlepx].exc_bd_monseq->msg_block[199*MSGSIZE+1]) << 16);
			ttaglast = RWORD_D2H( cardpx[handlepx].exc_bd_monseq->msg_block[199*MSGSIZE] ) |
				((long)(RWORD_D2H( cardpx[handlepx].exc_bd_monseq->msg_block[199*MSGSIZE+1] )) << 16);
		}
		msgptr->elapsedtime = (msgptr->elapsedtime - ttaglast) * 4;
	}

	return(0);
}

/*
Get_MON_Status     Coded 06.12.90 by D. Koppel

Description: Returns the status of the Monitor. Useful in determining what
condition triggered an interrupt.

Input parameters:  none

Output parameters: none

Return values: Status word to check for the following flags:
               MSG_IN_PROGRESS A message is in the process of being received
               TRIG_RCVD       A message which matched trigger1 or trigger2
                               has been received
               CNT_TRIG_MATCH  The number of messages recorded by the board
                               has reached the number set via Set_Cnt_Trig
	       MSG_INTERVAL    If 'interval' number of messages was received
               emode           If board is not in Monitor mode
               0               If successful

Note: This status is automatically reset each time it is read.

*/

int borland_dll Get_MON_Status (void)
{
	return Get_MON_Status_Px (g_handlepx);
}


int borland_dll Get_MON_Status_Px (int handlepx)
{
	unsigned char status;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if ((cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) &&
//		(cardpx[handlepx].exc_bd_monseq->board_config != MON_LOOKUP))
	if ((cardpx[handlepx].board_config != MON_BLOCK) &&
		(cardpx[handlepx].board_config != MON_LOOKUP))
		return (emode);

//	status = cardpx[handlepx].exc_bd_monseq->msg_status;
	status = RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->msg_status );

//	cardpx[handlepx].exc_bd_monseq->msg_status = 0;
	WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->msg_status, 0 );

	return(status);
}

/*
Run_MON     Coded 06.12.90 by D. Koppel

Description: Causes Monitor function to start.

Input parameters:  none

Output parameters: none

Return values:  emode  If board is not in Monitor mode
		etimeout     If timed out waiting for BOARD_HALTED
                0      If successful
*/

#define TRIG_OPTIONS (STORE_ONLY | STORE_AFTER)
int borland_dll Run_MON (void)
{
	return Run_MON_Px (g_handlepx);
}

int borland_dll Run_MON_Px (int handlepx)
{
	int i;
	int timeout=0;
	usint moduleFunction;

	unsigned char startValue;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if ((cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) &&
//		(cardpx[handlepx].exc_bd_monseq->board_config != MON_LOOKUP))
	if ((cardpx[handlepx].board_config != MON_BLOCK) &&
		(cardpx[handlepx].board_config != MON_LOOKUP))
		return (emode);

	while (timeout < 10000)
	{
		if ( Get_Board_Status_Px(handlepx) & BOARD_HALTED )
			break;

		/* Sleep(1L); - replace with busy loop, 03 Feb 02 
		access DPR, to give the f/w time to write the HALTED bit
		reading from DPR takes about 150 nanosec, on any processor */
		for (i=0; i<3; i++)
//			globalTemp1 = cardpx[handlepx].exc_bd_bc->revision_level;
			globalTemp1 = RBYTE_D2H(cardpx[handlepx].exc_bd_bc->revision_level);
		timeout++;
	}
	if (timeout == 10000)
		return (etimeout);

	/* if the board is set up to wait for a trigger, let Get_Next_Msg know */
//	if ((cardpx[handlepx].exc_bd_monseq->trig_ctl & TRIG_OPTIONS) &&
//		(cardpx[handlepx].exc_bd_monseq->trig_ctl & 3))
	if ((RBYTE_D2H(cardpx[handlepx].exc_bd_monseq->trig_ctl) & TRIG_OPTIONS) &&
		(RBYTE_D2H(cardpx[handlepx].exc_bd_monseq->trig_ctl) & 3))
		cardpx[handlepx].wait_for_trigger = 1;
	else
		cardpx[handlepx].wait_for_trigger = 0;

	cardpx[handlepx].lastmsgttag = 0;

	// Let's just read moduleFunction once
	moduleFunction = RWORD_D2H(cardpx[handlepx].exc_bd_monseq->moduleFunction);

	if (cardpx[handlepx].enhancedMONblockFW)
	{
//		if (cardpx[handlepx].exc_bd_monseq->moduleFunction & ENHANCED_MON_MODE)
//		if (RWORD_D2H( cardpx[handlepx].exc_bd_monseq->moduleFunction) & ENHANCED_MON_MODE)
		if (moduleFunction & ENHANCED_MON_MODE)
			cardpx[handlepx].isEnhancedMon = 1;
		else
			cardpx[handlepx].isEnhancedMon = 0;
	}
	else
		cardpx[handlepx].isEnhancedMon = 0;



	if (cardpx[handlepx].expandedMONblockFW)
	{
//		if (cardpx[handlepx].exc_bd_monseq->moduleFunction & EXPANDED_BLOCK_MODE)
//		if (RWORD_D2H(cardpx[handlepx].exc_bd_monseq->moduleFunction) & EXPANDED_BLOCK_MODE)
		if (moduleFunction & EXPANDED_BLOCK_MODE)
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

//	if 	(cardpx[handlepx].exc_bd_monseq->moduleFunction & IRIG_TIMETAG_MODE)
	if (moduleFunction & IRIG_TIMETAG_MODE)
	{
//		if ((cardpx[handlepx].exc_bd_monseq->moduleFunction & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE)
		if ((moduleFunction & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE)
			cardpx[handlepx].lastblock = MAX_IRIGMON_BLOCKS;
		else
			cardpx[handlepx].lastblock = MAX_IRIGMON_REGULAR_SIZE_BLOCKS;
	}

	// Initialize the wrap counter variables to trigger a re-read of the wrap counter
	// (Any value of msg counter will look like a wrap-around)
	cardpx[handlepx].prevMonMsgCount = 0xFFFF;

//	cardpx[handlepx].exc_bd_monseq->start |= 1;
	startValue = RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->start );
	startValue |= 1;
	WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->start , startValue );

	return(0);
}

/*
Set_Broad_Ctl     Coded 04 May 98 by D. Koppel & A. Meth

Description:  Set the broadcast control register (3FE8 h) to specify whether
RT address 31 should be regarded as a valid RT number (flag = 0) or as the
Broadcast address (flag = 1).

Input parameters:  flag    1 = turn the broadcast control register on
                           0 = no broadcast messages recognized

Output parameters: none

Return values:  emode   If board is not in Monitor mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_Broad_Ctl (int flag)
{
	return Set_Broad_Ctl_Px (g_handlepx, flag);
}

int borland_dll Set_Broad_Ctl_Px (int handlepx, int flag)
{
#ifdef MMSI
	return (func_invalid);
#else
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;


	/* check if board is configured as a Monitor */
//	if ((cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) &&
//		(cardpx[handlepx].exc_bd_monseq->board_config != MON_LOOKUP))
	if ((cardpx[handlepx].board_config != MON_BLOCK) &&
		(cardpx[handlepx].board_config != MON_LOOKUP))
		return (emode);

	if (flag == 1)
//		cardpx[handlepx].exc_bd_monseq->broad_ctl = 1;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->broad_ctl, 1 );

	else if (flag == 0)
//		cardpx[handlepx].exc_bd_monseq->broad_ctl = 0;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->broad_ctl, 0 );

	else
		return einval;

	return 0;
#endif
}

/*
Set_MON_Concurrent

Description: The 1553PX boards have a concurrent monitor option
on modules 1 and 3 (and 5 for vme/px), that can be used to monitor, respectively,
modules 0 and 2 (and 4 for vme/px) without attaching to the bus.
When calling Set_MON_Concurent the module must be in monitor mode.

Input parameters:  flag
		   ENABLE
		   DISABLE
Output parameters: none
Return values:	einval	          If parameter is out of range
                econcurrmonmodule If module 1 or 3 (or 5 for vme/px) not currently selected
		0	          If successful

*/

int borland_dll Set_MON_Concurrent (int flag)
{
	return Set_MON_Concurrent_Px (g_handlepx, flag);
}

int borland_dll Set_MON_Concurrent_Px (int handlepx, int flag)
{
#ifdef MMSI
	return (func_invalid);
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* PCMCIA/PxIII does have MON_Concurrent feature */
//	if ( ((cardpx[handlepx].exc_bd_monseq->board_options & BOARD_PCMCIAEP) == BOARD_PCMCIAEP) &&
	if ( ((RWORD_D2H( cardpx[handlepx].exc_bd_monseq->board_options ) & BOARD_PCMCIAEP) == BOARD_PCMCIAEP) &&
		(cardpx[handlepx].processor != PROC_NIOS) )
		return func_invalid;

	/* check if board is configured as a Monitor */
//	if ((cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) &&
//		(cardpx[handlepx].exc_bd_monseq->board_config != MON_LOOKUP))
	if ((cardpx[handlepx].board_config != MON_BLOCK) &&
		(cardpx[handlepx].board_config != MON_LOOKUP))
		return (emode);

	/* module 5 applies to vme/px */
	if ((cardpx[handlepx].module != 1) && (cardpx[handlepx].module != 3) && (cardpx[handlepx].module !=5))
		return (econcurrmonmodule);

	if ((flag != ENABLE) && (flag != DISABLE))
		return (einval);

	else
	{
		if(flag == ENABLE)
//			cardpx[handlepx].exc_bd_bc->internal_mon_connect = (unsigned char) 0x02;
			WBYTE_H2D( cardpx[handlepx].exc_bd_bc->internal_mon_connect, (unsigned char) 0x02 );

		else
//			cardpx[handlepx].exc_bd_bc->internal_mon_connect = (unsigned char) 0x00;
			WBYTE_H2D( cardpx[handlepx].exc_bd_bc->internal_mon_connect, (unsigned char) 0x00 );
	}

	return(0);
}

/* *****************************   
   NOTE: LL-0018 functionality was a feature added only in old EP/Px modules for customer in UK.
   It predates PxIII modules. (Maybe even PxI modules ?) Hence, regular memory accesses are performed.
   *****************************    */

/*

   Run_LL_0018      Aug 97 by A. Meth

   Description - This routine resets ll_pointer and starts execution of the card
   Inputs:  none
   Outputs: 0 for success  <0 for failure
*/

int borland_dll Run_LL_0018(void)
{
	return Run_LL_0018_Px(g_handlepx);
}
int borland_dll Run_LL_0018_Px(int handlepx)
{
#ifdef MMSI
	return (func_invalid);
#else
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_monseq->board_config != LL_0018_MODE)
	if (cardpx[handlepx].board_config != LL_0018_MODE)
		return(emode);

	/* set ll_pointer initially to FFFF to indicate that we have not yet
	received any 0018-type messages; so that when the value is 0, this
	indicates that the next message is found at location 0 */

	cardpx[handlepx].exc_bd_monseq->ll_pointer = 0xFFFF;

	cardpx[handlepx].exc_bd_monseq->start = 1;
	return 0;
#endif
}


/*
   Get_0018_Message	      Aug 97 by A. Meth

   Description
      This routine retrieves the next 0018-type message in the linked list.

      We use the "next message pointer" to decide which datablock to read.
      However, this will be restricted by the information found in the
      "wrap around bit" and in the "overrun bit".

      *********************************************

      Get the LL pointer - pointer to next message.

      NOTE: Initially, the function Run_LL_0018 sets the value at this
      location to 0xFFFF, which is an illegal location for received data.
      Hence, if this value is 0xFFFF, then this means that we have not
      yet received any data, & return with a status value that indicates this.

      if (wrap_around != 1) then the message is good at ll_pointer; extract it
      else if (overrun == 1) then
	 display message that we have double wrap_around
	 reset wrap_around & overrun pointers
      else - single wrap around
	 check if end pointer of the wrap around is not passed our pointer
	 if wrap around is past ll_pointer, then reset ll_pointer to top,
	    and display message
	 else
	    it is OK to get the next message

      Get message from top.

      Display status messages about this message:
	 GET_BUS	     0x100
	 BAD_CHECKSUM	     0x200
	 NOT_FFFF	     0x400
	 LL_INVALID_WORD     0x800

      If we have BAD_CHECKSUM, then the next message may be messed up ...
	 If checksum has bit #4 (76543210) set, then this is taken as a
	 mode code with subaddress 16, which means mode code with data word.
	 So, the firmware sets up another message, with data word of 0000.


   Output: msgptr - the message is returned in a structure of type MONMSG pointed to by msgptr

   Return value: block number for success, <0 for error

 */

int borland_dll Get_0018_Message (struct MONMSG *msgptr, int *numwords)
{
	return Get_0018_Message_Px (g_handlepx, msgptr, numwords);

}
int borland_dll Get_0018_Message_Px (int handlepx, struct MONMSG *msgptr, int *numwords)
{
	int i, next_msgptr, nx_msgptr;
	struct MONLLMSG
	{
		usint ll_ptr;
		usint msgstatus;
		usint elapsedtime_lo;
		usint elapsedtime_hi;
		usint words[36];
	} *llmsg;
	usint driv_ptr, fw_ptr, wrap, overrun;

#ifdef MMSI
	return (func_invalid);
#else
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a LL_0018 */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != LL_0018_MODE)
	if (cardpx[handlepx].board_config != LL_0018_MODE)
		return (emode);

	driv_ptr = cardpx[handlepx].exc_bd_monseq->ll_pointer;
	fw_ptr   = cardpx[handlepx].exc_bd_monseq->current_PS;
	wrap     = cardpx[handlepx].exc_bd_monseq->wrap_around;
	overrun  = cardpx[handlepx].exc_bd_monseq->overrun;

	if (wrap == 1)
	{
		/* reset pointers and indicators to 0, as necessary */
		if (overrun == 1)
		{
			/* wrap & overrun (twice wrapped w/o being reset); reset all */
			cardpx[handlepx].exc_bd_monseq->ll_pointer  = fw_ptr;
			cardpx[handlepx].exc_bd_monseq->wrap_around = 0;
			cardpx[handlepx].exc_bd_monseq->overrun     = 0;
			/* return ewrapNoverrun; */
			return eoverrun;
		}
		else if (fw_ptr > driv_ptr)
		{
			/* wrap & fw_ptr passed the driv_ptr; data has been clobbered; reset */
			cardpx[handlepx].exc_bd_monseq->ll_pointer  = 0;
			cardpx[handlepx].exc_bd_monseq->wrap_around = 0;
			/* return ewrapNclobber; */
			return eoverrun;
		}
	}
	/* if we have gotten here, then our pointers are OK and the data is *not*
	clobbered; continue with regular processing to extract the next message */

	nx_msgptr = cardpx[handlepx].exc_bd_monseq->ll_pointer;
	if (nx_msgptr == 0xFFFF)
		return enomsg;

	next_msgptr = (int) nx_msgptr;
	llmsg = (struct MONLLMSG *)&cardpx[handlepx].exc_bd_monseq->msg_block[next_msgptr / sizeof(usint)];

	if (llmsg->ll_ptr == 0x00FF)
		return enomsg;

	tmpval2 = llmsg->msgstatus;
	*numwords = (tmpval2 & 0x00ff);		  /* lower byte */
	msgptr->msgstatus = (tmpval2 & 0xff00);	  /* upper byte */

	msgptr->elapsedtime = (llmsg->elapsedtime_hi << 16) | (llmsg->elapsedtime_lo);
	for (i=0; i < *numwords; i++)
		msgptr->words[i] = (int)(llmsg->words[i]);

	if (llmsg->ll_ptr == 0x0000)
	{
		/*
		next message starts at 0
		check if wrap_around pointer is set
		if yes, and NOT overrun, then reset wrap to 0
		*/
		if (wrap == 1)
		{
			if (overrun == 0)
				cardpx[handlepx].exc_bd_monseq->wrap_around = 0;
			else
				/* if wrap AND overrun, then "how did we manage to get here !?" error */
				/* retval = ebadEXECpath; */
				return eoverrun;
		}
	}

	cardpx[handlepx].exc_bd_monseq->ll_pointer = llmsg->ll_ptr;
	return 0;
#endif
}

int borland_dll Create_0018_Word(usint cmdword, short int *handle)
{
	return Create_0018_Word_Px(g_handlepx, cmdword, handle);
}

int borland_dll Create_0018_Word_Px(int handlepx, usint cmdword, short int *id)
{
	int status;

#ifdef MMSI
	return (func_invalid);
#else
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	status = Create_1553_Message_Px(handlepx, BRD_MODE, &cmdword, id);
	if (status <0)
		return status;
	status = Insert_Msg_Err_Px(handlepx, *id, SYNC_ERR);
	return status;
#endif
}

int borland_dll Set_Mon_Response_Time(usint rtime)
{
	return Set_Mon_Response_Time_Px(g_handlepx, rtime);
}

int borland_dll Set_Mon_Response_Time_Px(int handlepx, usint rtime)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if ((cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) &&
//		(cardpx[handlepx].exc_bd_monseq->board_config != MON_LOOKUP))
	if ((cardpx[handlepx].board_config != MON_BLOCK) &&
		(cardpx[handlepx].board_config != MON_LOOKUP))
		return (emode);

#ifdef MMSI
	if (cardpx[handlepx].processor == PROC_NIOS)
//		cardpx[handlepx].exc_bd_monseq->response_time = rtime * 10;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->response_time, (rtime * 10) );

	else
#endif
//		cardpx[handlepx].exc_bd_monseq->response_time = rtime;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->response_time, rtime );

	return 0;
}

/*
Set_Mon_Response_Time_1553A_Px

Description: Use this function, in Monitor mode, to set the expected RT response time for messages
for RTs that heve been set up to conform to the 1553A spec (using function Enable_Mon_1553A_Support_Px).

Input Parameter:
	rtime		the desired RT response time

Return Values:	func_invalid [MMSI], ebadhandle, emode, efeature_not_supported_PX, efeature_not_supported_MMSI, 0 (success)

NOTES:
- For RTs set up to conform to the 1553B spec, use function call Set_Mon_Response_Time_Px
to set the expected RT response time for messages to these RTs
- Hence, we have one response time for all those RTs set up to conform to the 1553A spec,
and another response time for all those RTs set up to conform to the 1553B spec,
*/

int borland_dll Set_Mon_Response_Time_1553A(usint rtime)
{
	return Set_Mon_Response_Time_1553A_Px(g_handlepx, rtime);
}

int borland_dll Set_Mon_Response_Time_1553A_Px(int handlepx, usint rtime)
{
#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	// check if board is configured as a Monitor
//	if ((cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) &&
//		(cardpx[handlepx].exc_bd_monseq->board_config != MON_LOOKUP))
	if ((cardpx[handlepx].board_config != MON_BLOCK) &&
		(cardpx[handlepx].board_config != MON_LOOKUP))
		return (emode);

	// Verify that the firmware supports the "Mode per RT" feature
	if (!cardpx[handlepx].isModePerRTSupported)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

//	cardpx[handlepx].exc_bd_monseq->resptime_1553a = rtime;
	WWORD_H2D( cardpx[handlepx].exc_bd_monseq->resptime_1553a, rtime );

	return 0;
#endif
}
