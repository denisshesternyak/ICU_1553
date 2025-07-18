#include "pxIncl.h"

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)

extern INSTANCE_PX cardpx[];
usint statusval, tmpval3;

#define STORE_ALL	4	/* set_trigger1/2: Store everything-ignore triggers */
#define BYTES_IN_USINT	2
#define SIZE_OF_IRIG_COUNTER 4
#define SIZE_OF_STD_COUNTER	2

/*
Get_Counter     Coded 02.12.90 by D. Koppel

Description: Returns the number of the last block to be filled with a
monitored message.  As there are 200 blocks, this value will always be in
the range of 0-199.

Input parameters:  none

Output parameters: none

Return values:  emode     If board is not in Sequential Monitor mode
                counter   0-199 or 0-800
*/

int borland_dll Get_Counter (void)
{
	return Get_Counter_Px (g_handlepx);
}

int borland_dll Get_Counter_Px (int handlepx)
{
#ifdef MMSI
	return (func_invalid);
#else
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	/* Expanded mode in sequential monitor */
	if ((cardpx[handlepx].isExpandedMon) || (cardpx[handlepx].isEnhancedMon))
//		return(cardpx[handlepx].exc_bd_monseq->ExpandedCurrentBlock);
		return((int) RWORD_D2H( cardpx[handlepx].exc_bd_monseq->ExpandedCurrentBlock ));
	else 
//		return(cardpx[handlepx].exc_bd_monseq->msg_counter);
		return((int) RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->msg_counter ));

#endif
}

/*
Set_Cnt_Trig     Coded 02.12.90 by D. Koppel

Description: Sets a block number as the counter trigger. When this block is
filled, the CNT_TRIG_MATCH bit within the Monitor status word will be set
and, if requested via the Set_Interrupt routine, an interrupt will be
generated.

Input parameters: blknum   Valid block numbers are:
Regular mode: 0-199
Enhanced mode: 0- 400
Expanded mode: 0-800

Output parameters: none

Return values:  emode   If board is not in Sequential Monitor mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_Cnt_Trig (int blknum)
{
	return Set_Cnt_Trig_Px (g_handlepx, blknum);
}

int borland_dll Set_Cnt_Trig_Px (int handlepx, int blknum)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	if (blknum < 0)	return einval;

	/* Enhanced Mode: 0-400 blocks (+400 block for message info) */
	if ((cardpx[handlepx].isEnhancedMon) && (blknum > (EXPANDED_MON_BLOCK_NUM_3 - 1)))
		return einval;

	/* Expanded Monitor: 0 - 800 blocks */
	if ((cardpx[handlepx].isExpandedMon) && (blknum > (EXPANDED_MON_BLOCK_NUM_4 - 1)))
		return einval;

	/* Regular Mode 0 - 200 blocks */
	if ((!cardpx[handlepx].isExpandedMon) && 
		(!cardpx[handlepx].isEnhancedMon) && 
		(blknum > (EXPANDED_MON_BLOCK_NUM_1 - 1)))
		return einval;

//	cardpx[handlepx].exc_bd_monseq->cnt_trigger = (unsigned char)blknum;
	WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->cnt_trigger, (unsigned char)blknum );
	return(0);
}

/*
Set_Trigger1     Coded 02.12.90 by D. Koppel

Description: In Set_Trigger_Mode you can select a 1553 command word or a
Message status word for the Monitor to trigger on (via STATUS_TRIGGER).
When a message matching this parameter is encountered, the trigger is met
and the action specified in Set_Trigger_Mode is taken.

Note: These parameters are composed of two words, a 1553 command word or
Message Status Word (see STATUS_TRIGGER in Set_Trigger_Mode) and a 16 bit
mask. The mask specifies which bits in the trigger word are required to
match and which are "don't cares." For example, when triggering on command
words, Set_Trigger(0x0843, 0xF800) would trigger on all messages to/from
RT #1.

Input parameters:  trigger  16 bit trigger value
                   mask     Mask for trigger. A 16 bit word containing:
                            1 in required bit positions
                            0 in don't care bit positions
                            Mask of all 0s turns off trigger

Output parameters: none

Return values:  emode  If board is not in Sequential Monitor mode
                0      If successful
*/

int borland_dll Set_Trigger1 (int trigger, int mask)
{
	return Set_Trigger1_Px (g_handlepx, trigger, mask);
}

int borland_dll Set_Trigger1_Px (int handlepx, int trigger, int mask)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	/* if trigger is being disabled and trigger 2 is not in use STORE ALL */
	if (mask == 0)
	{
		cardpx[handlepx].triggers &= 2; /* remove trigger 2 in use bit from triggers */

//		cardpx[handlepx].exc_bd_monseq->trig_ctl = (unsigned char)(cardpx[handlepx].triggers | cardpx[handlepx].trig_mode );
		WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->trig_ctl, (unsigned char)(cardpx[handlepx].triggers | cardpx[handlepx].trig_mode) );

		if (cardpx[handlepx].triggers == 0)
//			cardpx[handlepx].exc_bd_monseq->trig_ctl = STORE_ALL;
			WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->trig_ctl, STORE_ALL );
	}

	if (mask != 0)
	{
		cardpx[handlepx].triggers |= 1;

//		cardpx[handlepx].exc_bd_monseq->trig_ctl = (unsigned char)(cardpx[handlepx].triggers | cardpx[handlepx].trig_mode);
		WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->trig_ctl, (unsigned char)(cardpx[handlepx].triggers | cardpx[handlepx].trig_mode) );

//		cardpx[handlepx].exc_bd_monseq->trigger1 = (usint)trigger;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->trigger1, (usint)trigger );

//		cardpx[handlepx].exc_bd_monseq->trig_mask1 = (usint)mask;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->trig_mask1, (usint)mask );
	}
	return(0);
}

/*
Set_Trigger2     Coded 02.12.90 by D. Koppel

Description: In Set_Trigger_Mode you can select a 1553 command word or a
Message status word for the Monitor to trigger on (via STATUS_TRIGGER).
When a message matching this parameter is encountered, the trigger is met
and the action specified in Set_Trigger_Mode is taken. A trigger is
considered to have occurred if either trigger1 or trigger2 has been
satisfied.

Note: These parameters are composed of two words, a 1553 command word or
Message Status Word (see STATUS_TRIGGER in Set_Trigger_Mode, page 5 8) and
a 16 bit mask. The mask specifies which bits in the trigger word are
required to match and which are "don't cares." For example, when triggering
on command words, Set_Trigger(0x0843, 0xF800) would trigger on all messages
to/from RT #1.

Input parameters:  trigger   16 bit trigger value
                   mask      Mask for trigger. A 16 bit word containing:
                             1 in required bit positions
                             0 in don't care bit positions
                             Mask of all 0s turns off trigger

Output parameters: none

Return values:  emode  If board is not in Sequential Monitor mode
                0      If successful
*/

int borland_dll Set_Trigger2 (int trigger, int mask)
{
	return Set_Trigger2_Px (g_handlepx, trigger, mask);
}

int borland_dll Set_Trigger2_Px (int handlepx, int trigger, int mask)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	/* if trigger is being disabled and trigger 1 is not in use STORE ALL */
	if (mask == 0)
	{
		cardpx[handlepx].triggers &= 1; /* remove trigger 1 in use bit from triggers */

//		cardpx[handlepx].exc_bd_monseq->trig_ctl = (unsigned char)(cardpx[handlepx].triggers | cardpx[handlepx].trig_mode );
		WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->trig_ctl, (unsigned char)(cardpx[handlepx].triggers | cardpx[handlepx].trig_mode) );

		if (cardpx[handlepx].triggers == 0)
//			cardpx[handlepx].exc_bd_monseq->trig_ctl = STORE_ALL;
			WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->trig_ctl, STORE_ALL );
	}

	if (mask != 0)
	{
		cardpx[handlepx].triggers |= 2;

//		cardpx[handlepx].exc_bd_monseq->trig_ctl = (unsigned char)(cardpx[handlepx].triggers | cardpx[handlepx].trig_mode);
		WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->trig_ctl, (unsigned char)(cardpx[handlepx].triggers | cardpx[handlepx].trig_mode) );

//		cardpx[handlepx].exc_bd_monseq->trigger2 = (usint)trigger;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->trigger2, (usint)trigger );

//		cardpx[handlepx].exc_bd_monseq->trig_mask2 = (usint)mask;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->trig_mask2, (usint)mask );

	}
	return(0);
}

/*
Set_Trigger_Mode     Coded 02.12.90 by D. Koppel

Description: Determines the action to be taken upon receiving a message
matching the trigger. The trigger message is always stored, and you can
choose to store all messages from this point on or only those message
matching the trigger.

Note: To turn the triggers off call Set_Trigger with a mask argument
of 0000.

Input parameters:  mode
                     STORE_ONLY       Causes only messages matching the
                                      trigger to be stored
                     STORE_AFTER      Default. Causes storage of messages to
                                      begin following the first occurrence
                                      of a trigger message
                     STATUS_TRIGGER   For firmware versions 1.1 and up.
                                      May be added to the modes described
                                      above to cause triggering to be based
                                      on the Message Status Word rather than
                                      the Command Word.

Output parameters: none

Return values: emode   If board is not in Sequential Monitor mode
               einval  If parameter is out of range
               0       If successful
*/

#define TRIG_OPTIONS (STORE_ONLY | STORE_AFTER)
#define MODE_OPTIONS (STORE_ONLY | STORE_AFTER | STATUS_TRIGGER)
int borland_dll Set_Trigger_Mode (int mode)
{
	return Set_Trigger_Mode_Px (g_handlepx, mode);
}

int borland_dll Set_Trigger_Mode_Px (int handlepx, int mode)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	/* one of STORE_ONLY and STORE_AFTER must be chosen */
	if (((mode & TRIG_OPTIONS) != STORE_ONLY) &&
		((mode & TRIG_OPTIONS) != STORE_AFTER))
		return(einval);

	/* no other flags are legal */
	if ((mode & ~MODE_OPTIONS) != 0)
		return(einval);

	/* retain active triggers and set new mode */
//	(cardpx[handlepx].exc_bd_monseq->trig_ctl) = (unsigned char)((cardpx[handlepx].exc_bd_monseq->trig_ctl & 0x03) | mode);
	WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->trig_ctl,  (unsigned char)(( RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->trig_ctl ) & 0x03) | mode) );

	cardpx[handlepx].trig_mode = (char)mode; /* remember mode in case triggers are turned off and on*/

	return(0);
}

/*
Get_Next_Message     Coded Nov 28, 93 by D. Koppel

Description: Reads the message block following the block read in the
previous call to Get_Next_Message. The first call on Get_Next_Message
will return block 0.

Note: If the next block does not contain a new message an error will be
returned. Note that if you fall 200 blocks (800 in expanded mode,
400 in Enhanced mode) behind the board it will be
impossible to know if the message returned is newer or older than the
previous message returned unless you check the timetag. The routine
therefore returns the timetag itself in the elapsed time field rather
than a delta value.

Input parameters: 
	msgptr   Pointer to the structure defined below in which to return the message
                  struct MONMSG {
                  int msgstatus; Status word containing the following flags:
                                 END_OF_MSG      RT2RT_MSG
                                 TRIGGER_FOUND   INVALID_MSG
                                 BUS_A_XFER      INVALID_WORD
                                 WD_CNT_HI       WD_CNT_LO
                                 BAD_RT_ADDR     BAD_SYNC
                                 BAD_GAP         MON_LATE_RESP
                                 MSG_ERROR
                  unsigned long elapsedtime; The timetag associated with the
                                             message. If the card set up for
                                             32 bit time tagging, the full
                                             32 bits will be recorded with a
                                             precision of 4 microseconds per
                                             bit. In 16 bit mode the
                                             precision is 15.5 microseconds
                                             per bit.
                  unsigned int words [36];   A pointer to an array of 1553
                                             words in the sequence they
                                             were received over the bus
                  }

Output parameters: none

Return values:  
	block number		If successful. Valid values are 0-199.
                emode         If board is not in Sequential Monitor mode
                enomsg        If there is no message to return
	eoverrunTtag		If message was overwritten in middle of being read here
	eoverrunEOMflag		If message was overwritten in middle of being read here
	eIrigTimetagSet		If IrigB Timetag feature is supported on this module, and was invoked using function Set_IRIG_TimeTag_Mode_Px.
						Use function Get_Next_Mon_IRIG_Message_Px instead of this function.

Coding History

24-nov-2008	(YYB)
- corrected a pointer assignment when DMA is not available
	(the sense of an "if" test was backwards)
*/

int borland_dll Get_Next_Message (struct MONMSG *msgptr)
{
	return Get_Next_Message_Px (g_handlepx, msgptr);
}

int borland_dll Get_Next_Message_Px (int handlepx, struct MONMSG *msgptr) {
	int overwritten;

	if ((cardpx[handlepx].processor != PROC_NIOS) || (cardpx[handlepx].RTentryCounterFW == FALSE))
		return Get_Next_Message_Old_Px ( handlepx,  msgptr);
	else {
		return Get_Next_Message_New_Px ( handlepx,  msgptr, &overwritten);
	}
}

int borland_dll Get_Next_Message_New_Px (int handlepx, struct MONMSG *msgptr, int *overwritten)
{
	#define SIZE_OF_STATUS	2
	usint entrySize;
	usint * pCounterInBuffer; //pointer to MsgCounter of last message in the block we just read
	int retStatus;

	if (cardpx[handlepx].processor != PROC_NIOS)
		return (func_invalid);

	// check if board is configured as a Monitor
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].processor != PROC_NIOS)
		return (func_invalid);

	entrySize =(usint) sizeof (struct MONMSGPACKED);

	if (cardpx[handlepx].mBufManagement.numMsgsInLclBuffer == 0) {
		cardpx[handlepx].mBufManagement.pNextMsgInLclBuffer = (struct MONMSGPACKED *)cardpx[handlepx].mBufManagement.buffer;
		retStatus = Read_Monitor_Block_Px(handlepx,(unsigned char **)&cardpx[handlepx].mBufManagement.pNextMsgInLclBuffer,  entrySize, &cardpx[handlepx].mBufManagement.numMsgsInLclBuffer, overwritten, 0xfcb0);
		if (cardpx[handlepx].mBufManagement.numMsgsInLclBuffer == 0) {
			if (*overwritten == DATA_OVERWRITTEN)
				return eoverrun;
			else
				return (enomsg);
		}
		pCounterInBuffer = (usint *)((char *)(cardpx[handlepx].mBufManagement.pNextMsgInLclBuffer) + (cardpx[handlepx].mBufManagement.numMsgsInLclBuffer * entrySize) - SIZE_OF_STD_COUNTER);
		// lastCountRead is a 32 bit value that includes the high word of the counter which is not in the message.
		// If the current 16 bit count is less than the previous one, 
		// it will be the same as the hi of the firmware counter, if not it will be the same as the last time the user read it.

		if ( *pCounterInBuffer < cardpx[handlepx].prevCounterInBuffer)
//			cardpx[handlepx].lastCountRead = ((uint)(*pCounterInBuffer) | (((uint)(cardpx[handlepx].exc_bd_monseq->msgCount[0])) << 16)) + 1; //add 1 because counter in msg starts at 0 and lastcountread starts at 1
			cardpx[handlepx].lastCountRead = ((uint)(*pCounterInBuffer) | (((uint) (RWORD_D2H( cardpx[handlepx].exc_bd_monseq->msgCount[0] ))) << 16)) + 1; //add 1 because counter in msg starts at 0 and lastcountread starts at 1
		else
			cardpx[handlepx].lastCountRead = ((uint)(*pCounterInBuffer) | (((uint)(cardpx[handlepx].hiMsgCount)) << 16)) + 1; //add 1 because counter in msg starts at 0 and lastcountread starts at 1
		cardpx[handlepx].prevCounterInBuffer = *pCounterInBuffer;
	}
	//copy the status separately because the user structure is unpacked and the board structure is packed
	msgptr->msgstatus = cardpx[handlepx].mBufManagement.pNextMsgInLclBuffer->msgstatus;
	myMemCopy((void *)&msgptr->elapsedtime, (void *)&cardpx[handlepx].mBufManagement.pNextMsgInLclBuffer->elapsedtime, entrySize - SIZE_OF_STATUS, cardpx[handlepx].is32bitAccess);
	cardpx[handlepx].mBufManagement.pNextMsgInLclBuffer++;
	cardpx[handlepx].mBufManagement.numMsgsInLclBuffer--;

	if (msgptr->msgCounter < cardpx[handlepx].prevCounterInMsg) {//we had a wraparound 
		//the new high 16 bits were already figured into the count of cardpx[handlepx].lastCountRead
		cardpx[handlepx].hiMsgCount = cardpx[handlepx].lastCountRead >> 16;
	}
	cardpx[handlepx].prevCounterInMsg = msgptr->msgCounter;
	return (( (uint)(msgptr->msgCounter) + ((uint)cardpx[handlepx].hiMsgCount << 16)) % cardpx[handlepx].lastblock);
}

int borland_dll Get_Next_Message_Old_Px (int handlepx, struct MONMSG *msgptr)
{
	usint *blkptr, *dataptr;
	int numwords, numwords2, i;
	usint * pDpram = NULL;

#ifdef DMA_SUPPORTED
	usint * pDmaCopy;
	int		dmaCopyIsValid;
#endif

	unsigned long lastTtag, currentTtag;
	int	status = Successful_Completion_Exc;
	unsigned long msg_counter;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].irigModeEnabled == TRUE)
#ifdef MMSI
		return eIrigTimetagSet_MMSI;
#else
		return eIrigTimetagSet_PX;
#endif

	// check if board is configured as a Monitor
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	// If the module has a NIOS processor, get the next message using more streamlined optimized code.
	if (cardpx[handlepx].processor == PROC_NIOS)
		return Get_Next_Mon_Message_Px (handlepx, msgptr, &msg_counter);

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
	/* 400 - 800  For Non Enhanced Monitor */
	else if (!cardpx[handlepx].isEnhancedMon)
		pDpram = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block4[(cardpx[handlepx].nextblock - EXPANDED_MON_BLOCK_NUM_3) * MSGSIZE]);
#ifdef DMA_SUPPORTED
	status = Get_DmaMonBlockCopy_Px(handlepx, pDpram, &pDmaCopy, &dmaCopyIsValid);
	if (dmaCopyIsValid == TRUE)
		blkptr = pDmaCopy;
	else
#endif
		blkptr = pDpram;

	statusval = *blkptr;
	if ((statusval & END_OF_MSG) == END_OF_MSG)
	{
		msgptr->elapsedtime = (long)*(blkptr+1);
		msgptr->elapsedtime |= ((long)(*(blkptr+2)) << 16);

		currentTtag = msgptr->elapsedtime;
		lastTtag = cardpx[handlepx].lastmsgttag;

		if (currentTtag  < lastTtag )
		{
			if (lastTtag  - currentTtag  < 0x10000000)
			{
				if (!cardpx[handlepx].ignoreoverrun)
					return eoverrunTtag;
			}
		   	/* otherwise, it means that the timetag reached its maximum value and wrapped to 0 */
		}
		cardpx[handlepx].lastmsgttag = currentTtag ;

		dataptr = blkptr+3;

		/* we send more words than necessary for mode commands */
		numwords = *dataptr & 0x1f;
		if (numwords == 0)
			numwords = 32;
		numwords +=2;  /* add 1 for status word and 1 for command word */

		if (statusval & RT2RT_MSG)
		{
			/*if the word count of the second command does not equal the word count of the first CW*/
			numwords2= (*(dataptr+1)  & 0x1f);
			if (numwords2 == 0)
				numwords2 = 32;

			numwords2 +=2;

			if (numwords2 > numwords)
				numwords = numwords2;

			numwords += 2;  /* for RT to RT message add 2 for extra command+status*/
		}
		for (i=0; i< numwords; i++)
			msgptr->words[i] = dataptr[i];

		msgptr->msgCounter = dataptr[36];
		/*
			if we are waiting for a trigger, a valid message exists only if the
			TRIGGER_FOUND bit is set in addition to the END_OF_MSG bit.
		*/
		msgptr->msgstatus = *blkptr;
		if (cardpx[handlepx].wait_for_trigger)
		{
			if (msgptr->msgstatus & TRIGGER_FOUND)
			{
				if (cardpx[handlepx].exc_bd_monseq->trig_ctl & STORE_AFTER)
					cardpx[handlepx].wait_for_trigger = 0;
			}
			else
				return (enomsg);
		}
		/*     *blkptr |= ALREADY_READ; */
		
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

		/* added this, to check for overrun */
		if ((msgptr->msgstatus & END_OF_MSG) != END_OF_MSG)
			return eoverrunEOMflag;

		/* after reading, set the EOM bit to 0 */
		*pDpram = msgptr->msgstatus & (usint)(~END_OF_MSG);
#ifdef DMA_SUPPORTED
		if (dmaCopyIsValid == TRUE)
			*pDmaCopy = msgptr->msgstatus & (usint)(~END_OF_MSG);
#endif
		cardpx[handlepx].nextblock++;
		return (cardpx[handlepx].nextblock - 1);
	}
	else
		return (enomsg);
}

/*
Get_Next_Message_32Bit_Px     ---     coded 29oct07

  Description: Similar to Get_Next_Message_Px.
  Reads the message block following the block read in the
  previous call to Get_Next_Message_32Bit_Px.
  
	NOTE: Read access is 32-bits, vs. 16-bits.
	For use with newer revision of EXC-4000[c]PCI gate array, which allows 32-bit read access.
	*/

int borland_dll Get_Next_Message_32Bit_Px (int handlepx, struct MONMSG *msgptr)
{
	unsigned int *blkptr;
	usint *beginBlkPtr;
	unsigned int wideWords[20];
	int numwords, i, wwIndex;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);
	/* handle wraparound of buffer */
	cardpx[handlepx].nextblock %= 200;

	blkptr = (unsigned int *)&(cardpx[handlepx].exc_bd_monseq->msg_block[cardpx[handlepx].nextblock * MSGSIZE]);
	beginBlkPtr = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block[cardpx[handlepx].nextblock * MSGSIZE]);

	for (i=0; i<20; i++)
	{
		wideWords[i] = *blkptr++;
	}


	statusval = wideWords[0] & 0xFFFF;

	if ((statusval & END_OF_MSG) == END_OF_MSG)
	{
		msgptr->elapsedtime = wideWords[0] >> 16;
		msgptr->elapsedtime |= (wideWords[1] & 0xFFFF) << 16;
		if (msgptr->elapsedtime < cardpx[handlepx].lastmsgttag)
		{
			if (cardpx[handlepx].lastmsgttag - msgptr->elapsedtime < 0x10000000)
			{
				if (!cardpx[handlepx].ignoreoverrun)
					return eoverrunTtag;
			}
			/* otherwise, it means that the timetag reached its maximum value and wrapped to 0 */
		}
		cardpx[handlepx].lastmsgttag = msgptr->elapsedtime;

		/* we send more words than necessary for mode commands */
		msgptr->words[0] = wideWords[1] >> 16;
		numwords = msgptr->words[0] & 0x1f;

		if (numwords == 0)
			numwords = 32;
		numwords +=2;  /* add 1 for status word and 1 for command word*/
		if (statusval & RT2RT_MSG)
			numwords += 2;  /* for RT to RT message add 2 for extra command+status*/

		for (i=1, wwIndex=2; i<numwords; i++)
		{
			if ((i%2) != 0)
				msgptr->words[i] = wideWords[wwIndex] & 0xFFFF;
			else
				msgptr->words[i] = wideWords[wwIndex++] >> 16;
		}

		/*
		if we are waiting for a trigger, a valid message exists only if the
		TRIGGER_FOUND bit is set in addition to the END_OF_MSG bit.
		*/
		msgptr->msgstatus = *beginBlkPtr;
		if (cardpx[handlepx].wait_for_trigger)
		{
			if (msgptr->msgstatus & TRIGGER_FOUND)
			{
//				if (cardpx[handlepx].exc_bd_monseq->trig_ctl & STORE_AFTER)
				if ( RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->trig_ctl ) & STORE_AFTER)
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

		/* added this, to check for overrun */
		if ((msgptr->msgstatus & END_OF_MSG) != END_OF_MSG)
			return eoverrunEOMflag;

		/* after reading, set the EOM bit to 0 */
		*beginBlkPtr = msgptr->msgstatus & (usint)(~END_OF_MSG);
		cardpx[handlepx].nextblock++;
		return (cardpx[handlepx].nextblock - 1);
	}
	else
		return (enomsg);
}



/*
Description		Set_Message_Interval_Interrupt_Value_Px is used to set a message
				status flag (MSG_INTERVAL) after a specific number of
				messages are received. In addition, an interrupt message can
				be sent when the specific number of messages are received
				(See Set_Interrupt_Px). Note that only messages that
				are saved in memory are counted, not messages that are filtered out.

Syntax			Set_Message_Interval_Interrupt_Value_Px (int handle, int interval)
Input Parameters	handle 		The handle designated by Init_Module_Px
					interval	The number of messages after which to generate an interrupt
								NO_MSG_INTERVAL_INTERRUPT Disables interval interrupts [0000 H]
Output Parameters	none

Return Values		ebadhandle	If an invalid handle was specified; should be
								value returned by Init_Module_Px
					emode		If module is not in Sequential Monitor mode
					0			If successful
*/

int borland_dll Set_Message_Interval_Interrupt_Value_Px(int handlepx, usint interval)
{
	// This function is no longer supported
//	return func_invalid;

#ifdef MMSI
	return func_invalid;
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Sequential Monitor */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	if (cardpx[handlepx].processor == PROC_AMD)
		return func_invalid;

	// Intel & NIOS
//	if (cardpx[handlepx].exc_bd_bc->revision_level < 0x24)
	if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->revision_level ) < 0x24)
		return func_invalid;

//	cardpx[handlepx].exc_bd_monseq->interval_int = interval;
	WWORD_H2D( cardpx[handlepx].exc_bd_monseq->interval_int, interval );

	return 0;

#endif

}

int borland_dll Get_Message_Info_Px (int handlepx, int blknum, struct MONMSGINFO *msgInfoPtr)
{
	usint *blkptr, *wordsptr,statusValue;
	int numwords, i;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a Monitor */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	if (!cardpx[handlepx].isEnhancedMon) 
		return func_invalid;

	if ((blknum < 0) || (blknum > (EXPANDED_MON_BLOCK_NUM_3 - 1)))
		return einval;

	blkptr = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block4[blknum * MSGSIZE]);

//	statusValue = (usint)*blkptr;
	statusValue = RWORD_D2H(blkptr[0]);

	if ((statusValue & END_OF_MSG) == END_OF_MSG)
	{
		wordsptr = blkptr+3;

		// In Enhanced Monitor Mode, the hardware gives us the word count, and the firmware adds the END_OF_MSG bit into the message status word.
		// So, if we subtract the END_OF_MSG bit from the status word, we will get the word count of this message.
		numwords = statusValue - END_OF_MSG;

//		for (i=0; i< numwords; i++)
//			msgInfoPtr->words[i] = (usint)*wordsptr++;
		b2drmReadWordBufferFromDpr( (DWORD_PTR) wordsptr,
								(unsigned short *) msgInfoPtr->words,
								numwords,
								"Get_Message_Info_Px",
								B2DRMHANDLE );

		// blank out the array elements from the end of the message, words[numwords], to the end of the array words[35]
		for (i=numwords; i<36; i++)
			msgInfoPtr->words[i] = 0;

		msgInfoPtr->msgstatus = statusValue;
		
		//msgInfoPtr->msgCounter = (usint)*(blkptr+39);
		msgInfoPtr->msgCounter = RWORD_D2H(blkptr[39]);

		#ifdef MMSI
		/* if LINK mode, and MSW indicates wrong RT address, and there are no other error bits, then this is not an error; turn off ERR bit 00 */
		if (cardpx[handlepx].hubmode_MMSI == HUB_LINK)
		{
			if ((msgInfoPtr->msgstatus & BAD_RT_ADDR) == BAD_RT_ADDR)
			{
				if ((msgInfoPtr->msgstatus & ERROR_BITS_NO_MON_ADDR) == 0)
				{
					msgInfoPtr->msgstatus = msgInfoPtr->msgstatus & ~0x1; /* mask out Error bit */
				}
			}
		}
		#endif

		// Mark message as read
//		*blkptr = msgInfoPtr->msgstatus & (usint)(~END_OF_MSG);
		WWORD_H2D( blkptr[0], (msgInfoPtr->msgstatus & (usint)(~END_OF_MSG)) );

		return 0;
	}
	else
		return (enomsg);

	return 0;
}

int borland_dll Set_Enhanced_Mon_Px (int handlepx, int enableFlag)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) return (emode);
	if (cardpx[handlepx].board_config != MON_BLOCK) return (emode);

	if ((enableFlag != ENABLE) && (enableFlag != DISABLE)) return einval;

	if (!cardpx[handlepx].enhancedMONblockFW)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

	/* Enable Enhanced Monitor */
	if (enableFlag == ENABLE)
	{
//		cardpx[handlepx].exc_bd_monseq->moduleFunction |= (unsigned char) ENHANCED_MON_MODE;
		WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->moduleFunction,
			(RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->moduleFunction ) | (unsigned char) ENHANCED_MON_MODE) );

		cardpx[handlepx].isEnhancedMon = 1;
	}

	/* Disable Enhanced Monitor */
	else
	{
//		cardpx[handlepx].exc_bd_monseq->moduleFunction &= (unsigned char) (~ENHANCED_MON_MODE);
		WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->moduleFunction,
			(RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->moduleFunction ) & (unsigned char) ~ENHANCED_MON_MODE) );

		cardpx[handlepx].isEnhancedMon = 0;
	}


	return 0;
}

/*
Is_Enhanced_Mode_Supported_Px

Description: Given we are in MonSeq mode, does this module support Enhanced Monitor mode 
Output Parameter: 	isFeatureSupported	TRUE/FALSE
Return Values:	func_invalid, ebadhandle, emode, 0 (success)

NOTE: Usage: Call this function to determine if the feature is supported.
If TRUE, then you can call the function Set_Enhanced_Mon_Px .
*/

int borland_dll Is_Enhanced_Mon_Supported_Px(int handlepx, int *isFeatureSupported)
{
#ifdef MMSI
	return (func_invalid);
#else

	*isFeatureSupported = FALSE;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* Sequential Monitor Mode */
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK) 
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	*isFeatureSupported = cardpx[handlepx].enhancedMONblockFW;

	return 0;
#endif
}

/*
Enable_Mon_1553A_Support_Px

Description: Use this function to set up whether Mode Code messages for the given RT 
will be recognized as conforming with the 1553A or 1553B spec

Input Parameters:
	rtNum	the desired RT number
	enableflag	TRUE/FALSE

- if enableFlag is true, then for this rtNum:
(a) only messages with SA 0 will be recognized as mode codes, with all its ramifications in 1553A
(b) RT response of more than 7 usec dead time (which is 9 usec midbit to midbit) will be considered as NO RESPONSE

- if enableFlag is false, then for this rtNum:
(a) messages with SA 0 or SA 31 will be recognized as mode codes, with all its ramifications in 1553B
(b) RT response of more than 9 usec dead time (which is 12 usec midbit to midbit) will be considered as NO RESPONSE

Return Values:	func_invalid [MMSI], ebadhandle, einval, emode, efeature_not_supported_PX, efeature_not_supported_MMSI, 0 (success)

NOTES: Usage: Call this function to determine if the feature is supported.
- The default is to treat MC messages for any RT as conforming with the 1553B spec.
- If the user does not call this new function, the Monitor will behave as it has until today. 
- In addition, once one calls this new function after Init_Module_Px, with enableFlag==true,
the ability to set the allowed RT response time (by calling function Set_Mon_Response_Time_1553A_Px) will not be allowed.
- Calling Init_Module_Px again, will reset the feature to standard Monitor, as it was until today.

*/

int borland_dll Enable_Mon_1553A_Support (int rtNum, int enableflag)
{
	return Enable_Mon_1553A_Support_Px (g_handlepx,rtNum, enableflag);
}
int borland_dll Enable_Mon_1553A_Support_Px (int handlepx, int rtNum, int enableflag)
{
#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ((enableflag != ENABLE) && (enableflag != DISABLE))
		return einval;

	if ((rtNum < 0) && (rtNum > 31))
		return einval;

//	if (cardpx[handlepx].exc_bd_bc->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return emode;

	// Verify that the firmware supports the "Mode per RT" feature
	if (!cardpx[handlepx].isModePerRTSupported)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

//	cardpx[handlepx].exc_bd_monseq->moduleFunction |= MODE_PER_RT_MODE;
	WWORD_H2D( cardpx[handlepx].exc_bd_monseq->moduleFunction, (RWORD_D2H ( cardpx[handlepx].exc_bd_monseq->moduleFunction) | MODE_PER_RT_MODE ) );

	if (enableflag == ENABLE)
//		cardpx[handlepx].exc_bd_monseq->modePerRT[rtNum] = 0xA;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->modePerRT[rtNum], 0xA );
	else
//		cardpx[handlepx].exc_bd_monseq->modePerRT[rtNum] = 0xB;
		WWORD_H2D( cardpx[handlepx].exc_bd_monseq->modePerRT[rtNum], 0xB );

	return 0;
#endif
}

/* written Dec 2014

Get_Next_Mon_IRIG_Message_Px

Description: Reads the message block following the block read in the previous
call to Get_Next_Mon_IRIG_Message_Px; ttag is in BCD representation.

The first call to Get_Next_Mon_IRIG_Message_Px will return block 0.

Note:
(a) Currently, this function is supported in the PxIII (NiosII-based) modules.
(b) If the next block does not contain a new message an error will be returned.

Input parameters:
	handlepx		Module handle returned from the earlier call to Init_Module_Px

Output parameters:
	msgptr   Pointer to the structure defined below in which to return the message
		  struct MONIRIGMSG {
			unsigned short int msgstatus;
			unsigned short int timetagHiHi;
			unsigned short int timetagHiLo;
			unsigned short int timetagLoHi;
			unsigned short int timetagLoLo;
			unsigned short int words[36]; // for RT2RT 2 command words 2 status words + 32 data
			unsigned short int msgCounterHi;
			unsigned short int msgCounterLo;
		  }

	msg_counter		A 32-bit serial message count associated with this message.

timetag words are in BCD; see function Get_Next_Mon_IRIGttag_Message_Px for timetag words in decimal representation

Return values:
		block number	If successful
		emode			If board is not configured as Sequential Monitor mode
		enomsg		If there is no message to return
		ewrongprocessor	If not PxIII module
		ebadhandle		If handle parameter is out of range or not allocated
		eoverrunEOMflag	If message was overwritten in middle of being read here
		efeature_not_supported	If IrigB Timetag feature is not supported on this module
		eNoIrigModeSet	If IrigB Timetag feature is supported on this module, but was not invoked using function Set_IRIG_TimeTag_Mode_Px
*/

int borland_dll Get_Next_Mon_IRIG_Message_Px (int handlepx, struct MONIRIGMSG *msgptr, unsigned long *msg_counter)
{
// variables used for reading directly from card memory DPR
//	usint *blkptr;
//	usint msgstat;
//	int numwords,i;

// variables used for DMA
//	usint * pDmaCopy;
//	struct MONIRIGMSG * pMsgDmaCopy;
//	int		dmaCopyIsValid;

	int rt2rtFlag;
	struct MONIRIGMSG irigMsgCopy1;
	struct MONIRIGMSG irigMsgCopy2;
	usint * pDpram = NULL;
	int wordLoopCounter;

	int	status = Successful_Completion_Exc;


	if (cardpx[handlepx].processor != PROC_NIOS) return (ewrongprocessor);
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].irigTtagModeSupported == FALSE)
#ifdef MMSI
		return efeature_not_supported_MMSI;
#else
		return efeature_not_supported_PX;
#endif

	if (cardpx[handlepx].irigModeEnabled != TRUE)
#ifdef MMSI
		return eNoIrigModeSet_MMSI;
#else
		return eNoIrigModeSet_PX;
#endif

	// check if board is configured as an RT or BC or MON
//	if (cardpx[handlepx].exc_bd_monseq->board_config != MON_BLOCK)
	if (cardpx[handlepx].board_config != MON_BLOCK)
		return (emode);

	// The only mode this works in so far is MON_BLOCK, so don't need an if block to set up the dpram pointers
	// for each mode ... 	// from here on, board_config is inherently set to be MON_BLOCK

	// handle wraparound of buffer
	cardpx[handlepx].nextblock %= cardpx[handlepx].lastblock;
	if (cardpx[handlepx].nextblock < EXPANDED_IRIG_BLOCK_NUM_1) 
		pDpram = (usint *)&(cardpx[handlepx].exc_bd_monseq->msg_block[cardpx[handlepx].nextblock * MONSEQ_IRIG_BLOCKSIZE]);
	else if ((cardpx[handlepx].nextblock >= EXPANDED_IRIG_BLOCK_NUM_1) && (cardpx[handlepx].nextblock < EXPANDED_IRIG_BLOCK_NUM_2))
		pDpram = (usint *)&cardpx[handlepx].exc_bd_monseq->msg_block2[(cardpx[handlepx].nextblock - EXPANDED_IRIG_BLOCK_NUM_1) * MONSEQ_IRIG_BLOCKSIZE];
	else 
		pDpram = (usint *)&cardpx[handlepx].exc_bd_monseq->msg_block3[(cardpx[handlepx].nextblock - EXPANDED_IRIG_BLOCK_NUM_2) * MONSEQ_IRIG_BLOCKSIZE];

	rt2rtFlag = RT2RT_MSG;

/*
	// dma (and assume multi-message reads) doesn't currently work with IRIG
#ifdef DMA_SUPPORTED
	status = Get_DmaMonIRIGBlockCopy_Px(handlepx, pDpram, &pDmaCopy, &dmaCopyIsValid);
	if (dmaCopyIsValid == TRUE)
	{
		// explore pDmaCopy

		// sometime after reading from the DMA cache, set the cached EOM bit to 0
		*pDmaCopy = msgptr->msgstatus & (usint)(~END_OF_MSG);
	}
	else
	// Just get a single message directly from the module.

	{
#endif
*/
		// zero out the status word before beginning
		irigMsgCopy1.msgstatus = 0;

		b2drmReadWordBufferFromDpr( (DWORD_PTR) pDpram,
						(unsigned short *) &irigMsgCopy1,
						sizeof(struct MONIRIGMSG) / sizeof(unsigned short),
						"Get_Next_Mon_IRIG_Message_Px",
						B2DRMHANDLE );

		// help speed up message reads when there is nothing to read; if the EOM flag is not set, then there is no reason to re-read the message just yet
		if ((irigMsgCopy1.msgstatus & END_OF_MSG) != END_OF_MSG)
			return enomsg;

		//get a second copy of the header so we can check if any changes were made while we were in mid read
		b2drmReadWordBufferFromDpr( (DWORD_PTR) pDpram,
						(unsigned short *) &irigMsgCopy2,
						MONIRIGMSG_HEADER_SIZE / sizeof(unsigned short),
						"Get_Next_Mon_IRIG_Message_Px",
						B2DRMHANDLE );

		if (irigMsgCopy1.msgstatus != irigMsgCopy2.msgstatus)
			return eoverrunEOMflag;

		if (irigMsgCopy1.timetagLoLo != irigMsgCopy2.timetagLoLo)
			return eoverrunTtag;
		if (irigMsgCopy1.timetagLoHi != irigMsgCopy2.timetagLoHi)
			return eoverrunTtag;
		if (irigMsgCopy1.timetagHiLo != irigMsgCopy2.timetagHiLo)
			return eoverrunTtag;
		if (irigMsgCopy1.timetagHiHi != irigMsgCopy2.timetagHiHi)
			return eoverrunTtag;

		msgptr->msgstatus = irigMsgCopy1.msgstatus;
		msgptr->timetagHiHi = irigMsgCopy1.timetagHiHi;
		msgptr->timetagHiLo = irigMsgCopy1.timetagHiLo;
		msgptr->timetagLoHi = irigMsgCopy1.timetagLoHi;
		msgptr->timetagLoLo = irigMsgCopy1.timetagLoLo;

		//instead of 36 we could figure out the correct number based on the command word and Px status
		for (wordLoopCounter = 0; wordLoopCounter < 36; wordLoopCounter++) {
			msgptr->words[wordLoopCounter] = irigMsgCopy1.words[wordLoopCounter];
		}
		msgptr->spare = irigMsgCopy1.spare;
		msgptr->msgCounterLo = irigMsgCopy1.msgCounterLo;
		msgptr->msgCounterHi = irigMsgCopy1.msgCounterHi;

		// after reading, set the EOM bit (ON THE MODULE) to 0
//		*pDpram = msgptr->msgstatus & (usint)(~END_OF_MSG);
		WWORD_H2D( pDpram[0], (usint)(msgptr->msgstatus & (usint)(~END_OF_MSG)) );

/*
#ifdef DMA_SUPPORTED
	}	// End 'Just get a single message directly from the module.' in the case of "else NO-DMA"
#endif
*/

// (timetag wraparound is not an issue)

	// If we are waiting for a trigger, a valid message exists only if the
	// TRIGGER_FOUND bit is set in addition to the END_OF_MSG bit.

	if (cardpx[handlepx].wait_for_trigger)
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
	// if LINK mode, and MSW indicates wrong RT address, and there are no other error bits, then this is not an error; turn off ERR bit 00
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

	*msg_counter = ((unsigned int)(msgptr->msgCounterHi) << 16) | msgptr->msgCounterLo;
	cardpx[handlepx].nextblock++;
	return (cardpx[handlepx].nextblock - 1);
}

/* written Dec 2014, June 2015

Get_Next_Mon_IRIGttag_Message_Px

Description: Reads the message block following the block read in the previous
call to Get_Next_Mon_IRIGttag_Message_Px, with ttag in decimal representation.

The first call to Get_Next_Mon_IRIGttag_Message_Px will return block 0.

Note:
(a) Currently, this function is supported in the PxIII (NiosII-based) modules.
(b) If the next block does not contain a new message an error will be returned.

Input parameters:
	handlepx		Module handle returned from the earlier call to Init_Module_Px

Output parameters:
	msgptr   Pointer to the structure defined below in which to return the message
		  struct MONIRIGMSG_TTAG {
			unsigned short int msgstatus;
			t_IrigOnModuleTime IrigTime;  // ttag in decimal representation
			unsigned short int words[36]; // for RT2RT 2 command words 2 status words + 32 data
			unsigned short int msgCounterHi;
			unsigned short int msgCounterLo;
		  }

	msg_counter		A 32-bit serial message count associated with this message.

timetag words are in decimal; see function Get_Next_Mon_IRIG_Message_Px for timetag words in BCD representation

Return values:
		block number	If successful
		emode			If board is not configured as Sequential Monitor mode
		enomsg		If there is no message to return
		ewrongprocessor		If not PxIII module
		ebadhandle		If handle parameter is out of range or not allocated
		eoverrunEOMflag	If message was overwritten in middle of being read here
		efeature_not_supported_MMSI	If IrigB Timetag feature is not supported on this module
		eNoIrigModeSet_MMSI	If IrigB Timetag feature is supported on this module, but was not invoked using function Set_IRIG_TimeTag_Mode_Px
		efeature_not_supported_PX	If IrigB Timetag feature is not supported on this module
		eNoIrigModeSet_PX	If IrigB Timetag feature is supported on this module, but was not invoked using function Set_IRIG_TimeTag_Mode_Px
*/

int borland_dll Get_Next_Mon_IRIGttag_Message_Px (int handlepx, struct MONIRIGMSG_TTAG *msgptr, unsigned long *msg_counter)
{
	int status = Successful_Completion_Exc, i;
	struct MONIRIGMSG moduleMsgptr;
	unsigned short int moduleTime[4];
	const int MAX_MSG_SIZE = 36;

	status = Get_Next_Mon_IRIG_Message_Px (handlepx, &moduleMsgptr, msg_counter);
	if (status < Successful_Completion_Exc)
		return (status);

	// Convert the module Irig time from an array of 4 short ints to days; hours + mins; secs + usec; more usec
	moduleTime[3] = moduleMsgptr.timetagLoLo;
	moduleTime[2] = moduleMsgptr.timetagLoHi;
	moduleTime[1] = moduleMsgptr.timetagHiLo;
	moduleTime[0] = moduleMsgptr.timetagHiHi;

	Convert_IrigTimetag_BCD_to_Decimal_Px (moduleTime, &msgptr->IrigTime);


	for (i=0; i<MAX_MSG_SIZE; i++)
		msgptr->words[i] = moduleMsgptr.words[i];
	msgptr->msgstatus = moduleMsgptr.msgstatus;
	msgptr->msgCounterLo = moduleMsgptr.msgCounterLo;
	msgptr->msgCounterHi = moduleMsgptr.msgCounterHi;

	return (status);  // return the value from the call to Get_Next_Mon_IRIG_Message_Px (cardpx[handlepx].nextblock-1)
}

/*
This routine reads messages from the Sequential monitor. It will read up to  buffSize chars, 
Read_Next_Data_IRIG_Px calls this when it's own internal buffer is empty to refill its buffer.
This routine does a block DMA read which is much faster than reading one message at a time
while Read_Next_Data_IRIG_RTx is easier for the user to call.

The difficulties with this routine stem from the fact that the board is active and may be overwriting
messages that we are in middle of reading.

Since reading with DMA doesn't allow us the luxury of checking the buffer as we read it we have to
read everything and then check to see if some of it was corrupted by the board during the read.

*/

int  Read_Monitor_Block_Px(int handlepx, unsigned char **userbuffer, usint entrySize, uint *nummsgs,  int *overwritten, uint endLastBlockOffset) {

	usint status;
	uchar *rcv_start;	// beginning of area of dpram to read
	uchar *rcv_end;		// end of area of dpram to read
	uint maxMsgsInRcvBuffer;
	usint msgsToRead;
	usint msgsTillEndOfBuffer;
	usint ReadStart;	// byte offsets into the DPR
	usint ReadSizeinBytes;
	usint totalBytesRead;
	uint msgsRead;
	uint firstEntry;
	uint currentCount;
	uint currentCount1;
	int goodMsgs;
	uint lastCountRead;
	uint entrySizeInWords;
	unsigned char *buffer;
	int bytesInThisRead;
	int bytesInMaxRead;
	unsigned int numwords;
	usint words[2], wordsOne[2];
	usint *wordsptr;
	struct MONMSGPACKED *pMonMsg; //use to check msgCounter for data integrity
	usint firstMsgCounter;
	uint msgnum;
	buffer = *userbuffer;
	lastCountRead = cardpx[handlepx].lastCountRead;
	entrySizeInWords = entrySize / BYTES_IN_USINT;
	maxMsgsInRcvBuffer = cardpx[handlepx].lastblock;
	*overwritten = 0;


	// Determine the max size of a single read
	if (cardpx[handlepx].isUNet == FALSE)
		bytesInMaxRead = 0x1000;	//DMA max is 4kbytes
	else {
		if (b2drmIsUSB(B2DRMHANDLE))
			bytesInMaxRead = 0x8000;	// UNet connected by USB breaks up reads by itself into ~512 byte chunks
		else
			bytesInMaxRead = 1440;	// UNet connected by Ethernet
	}

	// Figure out how many message blocks we're going to read, hence how many bytes we're going to read.

	// .. read count twice in case we catch it between updates of the low and high half, if so do a third read
	// .. note: this is not necessary for 32 bit modules such as the UNET

	numwords = 2;
	wordsptr = (usint *)&(cardpx[handlepx].exc_bd_monseq->msgCount[0]);
	b2drmReadWordBufferFromDpr( (DWORD_PTR) wordsptr,
								(unsigned short *) words,
								numwords,
								"Read_Monitor_Block_Px",
								B2DRMHANDLE );
	b2drmReadWordBufferFromDpr( (DWORD_PTR) wordsptr,
								(unsigned short *) wordsOne,
								numwords,
								"Read_Monitor_Block_Px",
								B2DRMHANDLE );
	currentCount = (words[0] << 16) | words[1];
	currentCount1 = (wordsOne[0] << 16) | wordsOne[1];

	if (currentCount != currentCount1)
	{
		b2drmReadWordBufferFromDpr( (DWORD_PTR) wordsptr,
								(unsigned short *) words,
								numwords,
								"Read_Monitor_Block_Px",
								B2DRMHANDLE );
		currentCount = (words[0] << 16) | words[1];
	}

	// Figure out start and end points for the read

	if ((currentCount > lastCountRead) && ((currentCount - lastCountRead) < maxMsgsInRcvBuffer)) {
		*overwritten = NOT_OVERWRITTEN;
		msgsToRead = currentCount - lastCountRead;
	}

	//if counts are equal nothing came in (technically 64k labels may have come in that weren't read but in that case the user won't mind missing one more label)
	else if (currentCount == lastCountRead)
		return (0);
	else { //entire buffer is overwritten and we read from current+1 to current 
		*overwritten = DATA_OVERWRITTEN;
		msgsToRead = maxMsgsInRcvBuffer;
	}

	if (*overwritten == NOT_OVERWRITTEN) {
		firstEntry = lastCountRead % maxMsgsInRcvBuffer;
	}
	else { //overwritten - the whole buffer is new, oldest label is the one past the current pointer
		firstEntry = (currentCount + 1) % maxMsgsInRcvBuffer;
	}

	// We're going to read message blocks starting with firstEntry, and continuing up to the end of the
	// message-block area in which it's contained.

	// .. pointing at message blocks in the First Message block area
	if (firstEntry < (sizeof(cardpx[handlepx].exc_bd_monseq->msg_block) / entrySize )) {
		rcv_start = (uchar *)&(cardpx[handlepx].exc_bd_monseq->msg_block[firstEntry * entrySizeInWords]);
		rcv_end = (uchar *)(&cardpx[handlepx].exc_bd_monseq->msg_block) + sizeof(cardpx[handlepx].exc_bd_monseq->msg_block);
	}

	// .. pointing at message blocks in the Second Message block area
	else if ((firstEntry >= (sizeof(cardpx[handlepx].exc_bd_monseq->msg_block) / entrySize )) && (firstEntry < ((sizeof(cardpx[handlepx].exc_bd_monseq->msg_block) / entrySize ) + (sizeof(cardpx[handlepx].exc_bd_monseq->msg_block2) / entrySize )))) {
		rcv_start = (uchar *)&cardpx[handlepx].exc_bd_monseq->msg_block2[(firstEntry - (sizeof(cardpx[handlepx].exc_bd_monseq->msg_block) / entrySize )) * entrySizeInWords];
		rcv_end = (uchar *)(&cardpx[handlepx].exc_bd_monseq->msg_block2) + sizeof(cardpx[handlepx].exc_bd_monseq->msg_block2);
	}

	// .. pointing at message blocks in the Third (& Fourth) Message block area
	else {
		rcv_start = (uchar *)&cardpx[handlepx].exc_bd_monseq->msg_block3[(firstEntry - ((sizeof(cardpx[handlepx].exc_bd_monseq->msg_block2) + sizeof(cardpx[handlepx].exc_bd_monseq->msg_block)) / entrySize )) * entrySizeInWords];
		rcv_end = (uchar *)(&cardpx[handlepx].exc_bd_monseq->msg_block) + endLastBlockOffset;
	}

	// ReadStart is an offset; while rcv_start is a pointer. ReadStart will always be less than 64k

	ReadStart =  (usint)(rcv_start - (uchar *)(cardpx[handlepx].exc_bd_bc));
	msgsTillEndOfBuffer = (usint) ((rcv_end - rcv_start)) / entrySize;

	// Set the number of messages to read, to the lessor of the number of messages, or the number of 
	// messages remaining in the message-block-area.
	if ((msgsToRead * entrySize) > (rcv_end - rcv_start))
		ReadSizeinBytes = (usint) (rcv_end - rcv_start);
	else
		ReadSizeinBytes = (usint) (msgsToRead * entrySize);
	totalBytesRead = 0;

	// Handle reads from UNet devices
	if (cardpx[handlepx].isUNet)
	{
		//if we need to read more than can fit into a packet, use multiple reads

		while (ReadSizeinBytes > 0) {
			if (ReadSizeinBytes > bytesInMaxRead)
				bytesInThisRead = bytesInMaxRead;
			else
				bytesInThisRead = ReadSizeinBytes;

			b2drmReadWordBufferFromDpr ((DWORD_PTR)(cardpx[handlepx].exc_pc_dpr + ReadStart + totalBytesRead),
				(unsigned short *) (buffer + totalBytesRead),
				bytesInThisRead/2,
				"Read_Monitor_Block_Px",
				B2DRMHANDLE );
			totalBytesRead += bytesInThisRead;
			ReadSizeinBytes -= bytesInThisRead;
		} // end reading while

	} // end UNet

	else // handle DMA & RMA
	{
		if ((cardpx[handlepx].dmaAvailable == TRUE) && (cardpx[handlepx].useDmaIfAvailable == TRUE))
		{
			status = PerformLongDMARead	(cardpx[handlepx].card_addr, cardpx[handlepx].module,
										(void *) buffer, ReadSizeinBytes,
										(void *) ReadStart);
			if (status == 0) 
				totalBytesRead = ReadSizeinBytes;
		}

		//if DMA failed use RMA
		if (totalBytesRead == 0)
		{
			myMemCopy((void *)buffer, (void *)((char *)(cardpx[handlepx].exc_bd_bc) + ReadStart), ReadSizeinBytes, cardpx[handlepx].is32bitAccess); 
			totalBytesRead = ReadSizeinBytes;
		}
	} // end DMA/RMA

	// update totalBytesRead to reflect the msgs we just read
	msgsRead = totalBytesRead / entrySize;

// During this next phase we want to do an integrity check, to make sure that no more new messages came in and overwrote
// our data while we were in the middle of reading it.


	// .. look in the buffer for the first message < 200 greater than the first message we expected to read, that was not overwritten
	// .. we'll check the possibility of multiple reads having early reads ok and later ones corrupt
	if (*overwritten == NOT_OVERWRITTEN) {
		firstEntry = lastCountRead;
	}

	// .. overwritten - the whole buffer is new, oldest label is one passed the current pointer which is counter 200 less than current counter
	else {
		firstEntry = (currentCount - (maxMsgsInRcvBuffer - 1));
	}

	pMonMsg = (struct MONMSGPACKED *)*userbuffer;
	for (msgnum = 0; msgnum < msgsRead;  msgnum++) {
		//if we've been corrupted msgCounter will be greater than expected
		if (( pMonMsg->msgCounter < (firstEntry + maxMsgsInRcvBuffer)) && (pMonMsg->msgstatus != 0))
			break; //this is a good message
		pMonMsg++;
	}
	// reduce the number of messages returned by the number of corrupt messages
	*nummsgs = msgsRead - msgnum;
	// move the pointer of the buffer to skip over the corrupt labels
	*userbuffer = buffer + (msgnum * entrySize);
	if (msgnum > 0)
		*overwritten = DATA_OVERWRITTEN;

	//check rest of buffer for corruption
	firstMsgCounter = pMonMsg->msgCounter;
	pMonMsg++;
	goodMsgs = 0; //the previous loop got passed the bad messages, we'll now loop through good messages to see if we got overwritten in mid read
	//msgCounter should be monotonically increasing except for wraparound
	for (; msgnum < (msgsRead - 1);  msgnum++) {
		//msgCounter should be monotonically increasing except for wraparound, if status is zeromsg is about to be written by fw and is already corrupt
		if (((firstMsgCounter != 0xffff) && ((firstMsgCounter + 1) !=  pMonMsg->msgCounter)) || ((firstMsgCounter == 0xffff) && (pMonMsg->msgCounter != 0)) || (pMonMsg->msgstatus == 0)){
			*nummsgs = goodMsgs; //corrupt buffer, return only good messages
			break;
		}
		goodMsgs++;
		firstMsgCounter = pMonMsg->msgCounter;
		pMonMsg++;
	}
	return (0);
}

