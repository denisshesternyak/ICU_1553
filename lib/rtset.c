#include "pxIncl.h"

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)

extern INSTANCE_PX cardpx[];
int Set_RT_Sync_Entry_Px(int handlepx, usint command, usint status, unsigned int timetaglo);
#define RT_MSG_ENTRIES_OLD 42
#define RT_MSG_ENTRIES 512
/* active table register bits */
#define RTACTIVE_INTERRUPT_BITS	0x3
#define RTACTIVE_BIT 0x1
#define RTINTERRUPT_BIT 0x2
#define MSGSTACK_BEGIN 0xc00
#define MSGSTACK_END 0x1400
#define MULTIBUFSHIFT 8
#define MAXBUFSMASK   0xf
#define ENTRYSIZE 4  /* number of words in an entry */

#define MIN_NUM_BLOCKS 1
#define MAX_NUM_BLOCKS 3

int globalTemp4;
/* for double buffering*/
#define DB_ENABLE_SET	0x10
#define DB_ACTIVE_SET 	0x20

typedef struct
{
	WORD commandWord;
	WORD timeTaghi;
	WORD timeTaglo;
	WORD msgStatus;
}t_msgStack;

/*
Get_RT_Stack_Entry_Count_Px

  Description: Returns the running message count
  Input parameters:  none
  Output parameters: entryCount - how many messages were transmitted or received
  Return values: 0
*/
int borland_dll Get_RT_Stack_Entry_Count_Px (int handlepx, unsigned int *entryCount)
{
	usint lclMsgCount[2];
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	*msgCount = ((unsigned int)(cardpx[handlepx].exc_bd_monseq->msgCount[0])<<16) + cardpx[handlepx].exc_bd_monseq->msgCount[1];
	b2drmReadWordBufferFromDpr( (DWORD_PTR) &cardpx[handlepx].exc_bd_rt->entryCountLo,
			(unsigned short *) &lclMsgCount,
			2,
			"Get_RT_Stack_Entry_Count_Px",
			B2DRMHANDLE );
	//firmware update the high before the low so moving from 1ffff to 20000 has an interim value of 2ffff for a few nanoseconds
	//doing a second read if the low is ffff takes care of his rare anamoly
	if (lclMsgCount[1] == 0xffff)
		b2drmReadWordBufferFromDpr( (DWORD_PTR) &cardpx[handlepx].exc_bd_rt->entryCountLo,
			(unsigned short *) &lclMsgCount,
			2,
			"Get_RT_Stack_Entry_Count_Px",
			B2DRMHANDLE );

	*entryCount = (lclMsgCount[1] << 16) + lclMsgCount[0];
	return 0;
}

/*
An RT identifier is composed of an RT address,
subaddress and T/R indicater in the form:

            |============|=====|===============|
            | RT ADDRESS | T/R | RT SUBADDRESS |
            |============|=====|===============|

*/
/*
Assign_RT_Data     Coded 17.11.90 by D. Koppel
Description: Associates a data block with a subaddress within the active RT.
Input parameters:   rtid     11 bit identifier including RT#, T/R bit and
                             subaddress.
                    blknum   Data block 0-199. 0 represents a default for
                             all unassigned RT's and is not recommended for
                             use by anyone interested in using the data.
Output parameters:  none
Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful

This block will then be used to store data transferred to this RT identifier.
This data can be retrieved by the user via the Read_Datablk routine.  This
block can also be filled with the Load_Datablk routine and the loaded data
will be transferred by the RT during a Transfer command from a BC.
This routine is used to assign one of the 200/256/512 available data blocks to a particular RT identifier.
*/

int borland_dll Assign_RT_Data (int rtid, int blknum)
{
	return Assign_RT_Data_Px (g_handlepx, rtid, blknum);
}
int borland_dll Assign_RT_Data_Px (int handlepx, int rtid, int blknum)
{
	usint curctrl;
	int maxbufs;
	int status;

	usint *dataptr;

	unsigned short tmp;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as an RT */
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	if ((rtid < 0) || (rtid > 2047))
		return (einval);

	if (cardpx[handlepx].singleFunction && (((rtid>>6) & cardpx[handlepx].rtNumber) != cardpx[handlepx].rtNumber))
		return ertvalue;

	status = Validate_Block_Number(handlepx, blknum);
	if (status < 0)
		return einval;

	/* if card is PCMCIA/EP (old), there is no RTid Control Table;
	skip the next few lines of code that depend on the existence of this table 	*/

	//if ( (cardpx[handlepx].processor != PROC_AMD) || (cardpx[handlepx].isPCMCIA != 1) ) {
	if (cardpx[handlepx].multibufUsed == 1)
	{
		// if using multi-bufferring, need to check that the request does not cross a block boundary
		// if not, then we save going out to the card for an unnecessary read (saves time on the UNET/UNET2)
	
		/*  multibuffering cannot cross 256 or 512 block boundary */
//		curctrl = cardpx[handlepx].rtid_ctrl[rtid];
		curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[rtid] );

		maxbufs = ((curctrl >> MULTIBUFSHIFT) & MAXBUFSMASK) + 1;

		/* The targeted blocks start at blknum and go for additional (maxbufs - 1) blocks.
		So (blknum + (maxbufs-1)) cannot be greater than 255/511; thus (blknum + maxbufs)
		cannot be greater than 256/512.
		*/
		if (((blknum < 256) && ((blknum + maxbufs) > 256)) || ((blknum + maxbufs) > 512))
			return eboundary;
	}

	/* blk_lookup is unsigned char, therefore the top bit is put somewhere else */

#ifdef VME4000
	cardpx[handlepx].exc_bd_rt->blk_lookup[rtid^1] = (unsigned char)blknum;
#else
//	cardpx[handlepx].exc_bd_rt->blk_lookup[rtid] = (unsigned char)blknum;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->blk_lookup[rtid], blknum );
#endif
	if (blknum > 255)
	{
//		cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid] |= RTID_EXPANDED_BLOCK;
		tmp = RWORD_D2H( cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid] );
		tmp |= RTID_EXPANDED_BLOCK;
		WWORD_H2D( cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid], tmp );
	}
	else
	{
//		cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid] &= ~RTID_EXPANDED_BLOCK;
		tmp = RWORD_D2H( cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid] );
		tmp &= ~RTID_EXPANDED_BLOCK;
		WWORD_H2D( cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid], tmp );
	}

	/* 	add the code from Load_Datablk to extract a pointer to the beginning of the data block area
		then set the blknum entry into the .msgalloc array to point to the beginning of this block
		then, the user can directly access the block */

	if (blknum < 200)
		dataptr = &(cardpx[handlepx].exc_bd_rt->data[blknum * BLOCKSIZE]);
	else if (blknum < 256)
		dataptr = &(cardpx[handlepx].exc_bd_rt->data2[(blknum-200) * BLOCKSIZE]);
	else
		dataptr = &(cardpx[handlepx].exc_bd_rt->data3[(blknum-256) * BLOCKSIZE]);

	cardpx[handlepx].msgalloc[blknum] = dataptr ;   /* address where the block sits */

	return(0);
}

/*
Load_Datablk     Coded 18.11.90 by D. Koppel

Description: Assigns data to a specified datablock. This data will be used
to respond to a Transmit command or a Transmit mode command.

Note: The same data block may be assigned to multiple RT's. Datablock 0 is
the default datablock used by any Active RT for which Assign_RT_Data has
not been called. Since this block is the default for Receive commands as
well, its data may change in an unpredictable way. It is not advisable to
use this block.

Input parameters:  blknum   Number of block (0-199) to assign data to.
                            0 represents a default for all unassigned RT's
                            and is not recommended for use by anyone
                            interested in using the data.
                   *words   Pointer to an array of up to 32 words of data
                            to be placed in the block

Output parameters: none
Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Load_Datablk (int blknum, usint *words)
{
	return Load_Datablk_Px (g_handlepx, blknum, words);
}
int borland_dll Load_Datablk_Px (int handlepx, int blknum, usint *words)
{
//	int i;
	int status;
	usint *dataptr;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	status = Validate_Block_Number(handlepx, blknum);
	if (status < 0)
		return einval;

	if (blknum < 200)
		dataptr = &(cardpx[handlepx].exc_bd_rt->data[blknum * BLOCKSIZE]);
	else if (blknum < 256)
		dataptr = &(cardpx[handlepx].exc_bd_rt->data2[(blknum-200) * BLOCKSIZE]);
	else
		dataptr = &(cardpx[handlepx].exc_bd_rt->data3[(blknum-256) * BLOCKSIZE]);

	/* Always move 32 words. Unused words don't matter. */
//	for (i = 0; i < BLOCKSIZE; i++)
//		*(dataptr+i) = words[i];

//	for (i = 0; i < BLOCKSIZE; i++)
//		WWORD_H2D( dataptr[i],  words[i] );
	b2drmWriteWordBufferToDpr ( (DWORD_PTR) dataptr,
								words,
								BLOCKSIZE,
								"Load_Datablk_Px",
								B2DRMHANDLE );

	return(0);
}

/*
Load_Multiple_Datablk_Px     Coded 12 Jul 21 by D. Koppel & A Meth

Description: Assigns data to a specified group of consecutive datablocks.
This data will be used to respond to Transmit commands or Transmit mode commands.

Note: The same data block may be assigned to multiple RT's. Datablock 0 is
the default datablock used by any Active RT for which Assign_RT_Data has
not been called. Since this block is the default for Receive commands as
well, its data may change in an unpredictable way. It is not advisable to
use this block.

Input parameters:  	blknum   
							Number of block (0-199) to assign data to.
                            0 represents a default for all unassigned RT's
                            and is not recommended for use by anyone
                            interested in using the data.
					numOfBlocks	
							how many blocks to write (currently allowed values are from 1-3)
					*words   Pointer to an array of up to 32 words of data
                            to be placed in the block

Output parameters: none
Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Load_Multiple_Datablk_Px (int handlepx, int blknum, int numOfBlocks, usint *words)
{
//	int i;
	int status;
	usint *dataptr;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	// verify that blknum is a legal value
	if ((numOfBlocks < MIN_NUM_BLOCKS) || (numOfBlocks > MAX_NUM_BLOCKS))
		return (einval);

	status = Validate_Block_Number(handlepx, blknum);
	if (status < 0)
		return einval;

	if (blknum+numOfBlocks < 200)
		dataptr = &(cardpx[handlepx].exc_bd_rt->data[blknum * BLOCKSIZE]);
	else if (blknum+numOfBlocks < 256)
		dataptr = &(cardpx[handlepx].exc_bd_rt->data2[(blknum+numOfBlocks-200) * BLOCKSIZE]);
	else
		dataptr = &(cardpx[handlepx].exc_bd_rt->data3[(blknum+numOfBlocks-256) * BLOCKSIZE]);

	// Always move blocks of size 32 (BLOCKSIZE) words. Unused words don't matter.
	// Move the numOfBlocks number of blocks, to save individual writes to the memory of UNET.
//	for (i = 0; i < BLOCKSIZE; i++)
//		*(dataptr+i) = words[i];

//	for (i = 0; i < BLOCKSIZE; i++)
//		WWORD_H2D( dataptr[i],  words[i] );

	b2drmWriteWordBufferToDpr((DWORD_PTR)dataptr, words, BLOCKSIZE*numOfBlocks, "Load_Datablk_Px", handlepx);

	return(0);
}


/*
Read_Datablk     Coded 19.11.90 by D. Koppel

Description: Returns the number of words requested from a particular block.
The most common use of this routine is to retrieve data from a block
assigned to a receive RT. To do this, you will set up the block and check
Read_RT_Status (or wait for an interrupt). When a message is received, call
Read_Cmd_Stack to find out which RT received the message. Lastly, call this
routine to retrieve the data.

Input parameters:   blknum   Number of block (0-199) to which to assign data
                             Note: Data block 0, while technically a legal
                             block, should be avoided. It is the default
                             block for all RTs and all active RTs with no
                             block assigned will end up here.
                    *words   Pointer to an array of 32 words. Although the
                             message may contain fewer than 32 words, 32
                             words will be transferred

Output parameters:  none

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful

Change history:
-----------------
08-sep-2008	(YYB)
- added DMA support

10-Dec-2008	(YYB)
- corrected the size of the block-to-copy in the PerformDMARead() calls.

*/

int borland_dll Read_Datablk (int blknum, usint *words)
{
	return Read_Datablk_Px (g_handlepx, blknum, words);
}


int borland_dll Read_Datablk_Px (int handlepx, int blknum, usint *words)
{
	int i;
	int	status = Successful_Completion_Exc;
	usint *dataptr;
	void * datablockOffset;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	status = Validate_Block_Number(handlepx, blknum);
	if (status < 0)
		return einval;

	if (blknum < 200)
		dataptr = &(cardpx[handlepx].exc_bd_rt->data[blknum * BLOCKSIZE]);
	else if (blknum < 256)
		dataptr = &(cardpx[handlepx].exc_bd_rt->data2[((blknum-200) * BLOCKSIZE)]);
	else
		dataptr = &(cardpx[handlepx].exc_bd_rt->data3[((blknum-256) * BLOCKSIZE)]);

	if ((cardpx[handlepx].dmaAvailable == TRUE) && (cardpx[handlepx].useDmaIfAvailable == TRUE))
	{
		datablockOffset = (void *) ((DWORD_PTR) dataptr - (DWORD_PTR) (cardpx[handlepx].exc_bd_rt));
#ifdef DMA_SUPPORTED
		status = PerformDMARead(cardpx[handlepx].card_addr, cardpx[handlepx].module,
			&(words[0]), MSGSIZEBYTES_RT, datablockOffset);
		if (status < 0)
#endif
		{
			/* Always move 32 words. Unused words don't matter. */
			for (i = 0; i < BLOCKSIZE; i++)
				words[i] = *(dataptr+i);
		}
	}

	else
	{
		/* Always move 32 words. Unused words don't matter. */
//		for (i = 0; i < BLOCKSIZE; i++)
//			words[i] = *(dataptr+i);

		// the UNet Device does not have DMA so do regular read of the WORDS buffer
		b2drmReadWordBufferFromDpr( (DWORD_PTR) dataptr,
									(unsigned short *) words,
									BLOCKSIZE, // BLOCKSIZE = number of WORDs
									"Read RT Datablk",
									B2DRMHANDLE );

	}

	return(0);
}

/*
Read_RT_Status     Coded 19.11.90 by D. Koppel

Description: Returns 1 if a 1553 Message has been received since the last
time Read_RT_Status was called.

Input parameters:  none
Output parameters: none
Return values:   emode   If board is not in RT mode
                 status  1 = message received
                        0 = no message received
*/

int borland_dll Read_RT_Status (void)
{
	return Read_RT_Status_Px (g_handlepx);
}

int borland_dll Read_RT_Status_Px (int handlepx)
{
	int i;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

//	i = cardpx[handlepx].exc_bd_rt->msg_status;
	i = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->msg_status );
	if (i == 1)
//		cardpx[handlepx].exc_bd_rt->msg_status = 0;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->msg_status, 0 );
	return (i);
}

/*
Reset_RT_Interrupt     Coded 19.11.90 by D. Koppel

Description: Turns off request for Message Complete interrupt. No Message
Complete interrupt will be generated following this call until
Set_RT_Interrupt is called.

Input parameters:  none

Output parameters: none

Return values:  emode  If board is not in RT mode
                0      If successful
*/

int borland_dll Reset_RT_Interrupt(void)
{
	return Reset_RT_Interrupt_Px(g_handlepx);
}

int borland_dll Reset_RT_Interrupt_Px(int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
//	cardpx[handlepx].exc_bd_rt->int_cond = 0;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->int_cond ,0 );
	return (0);
}

/*
Run_RT     Coded 19.11.90 by D. Koppel

Description: Starts RT simulation for all RTs that have been activated with
Set_Rt_Active.

Input parameters:  none
Output parameters: none
Return values:  emode	If board is not in RT mode
		etimeout     If timed out waiting for BOARD_HALTED
                0      If successful


--------------------------
Note to the user (24nov04):

The latency of this function has been increased from 1 microsec to
47 microsec (as observed on a 2.4 GHz Pentium machine). This is because
we added code to reset the status words in the Message Stacks before
actually starting up the module. This is used to ensure that no old messages
are left in the stack if the user calls Stop, Run_RT. Hence, we prevent
timetags from being read out of order.

--------------------------

*/

int borland_dll Run_RT(void)
{
	return Run_RT_Px(g_handlepx);
}

int borland_dll Run_RT_Px(int handlepx)
{
	int i;
	int timeout=0;
	usint *blkptr;

// Loop of single writes replaced by block write
//	t_msgStack *msgStack;

	BYTE volatile *pDPRam;
	int size;
	usint *pZeroedDataBuffer;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as an RT */
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	while (timeout < 10000)
	{
		if ( Get_Board_Status_Px(handlepx) & BOARD_HALTED )
			break;

		/* access DPR, to give the f/w time to write the HALTED bit
		reading from DPR takes about 150 nanosec, on any processor */
		for (i=0; i<3; i++)
//			globalTemp4 = cardpx[handlepx].exc_bd_bc->revision_level;
			globalTemp4 = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->revision_level );

		timeout++;
	}
	if (timeout == 10000)
		return (etimeout);

	/* check the error bit for single function */
//	if (cardpx[handlepx].singleFunction && ((cardpx[handlepx].exc_bd_rt->rtNumSingle & RT_SINGLE_FUNCTION_ERROR_BIT) == RT_SINGLE_FUNCTION_ERROR_BIT))
	if (cardpx[handlepx].singleFunction && ((RWORD_D2H( cardpx[handlepx].exc_bd_rt->rtNumSingle ) & RT_SINGLE_FUNCTION_ERROR_BIT) == RT_SINGLE_FUNCTION_ERROR_BIT))
		return (ebiterror);

	/* get value of rt from register to put in rtNumber of INSTANCE_PX for single function */
	if (cardpx[handlepx].singleFunction)
		Update_RT_From_Reg(handlepx);

	cardpx[handlepx].next_MON_entry = 0;
	cardpx[handlepx].lastmsgttag = 0;

	/* moved from Stop, to allow user to flush stack after a Stop */
	cardpx[handlepx].next_rt_entry = 0;
	cardpx[handlepx].next_rt_entry_old = 0;
	cardpx[handlepx].numMsgsInLclRTmessageBuffer = 0;
	cardpx[handlepx].lastRTstackCountRead = 0;
	/* Reset the status words in the message stacks so no old messages are left in the stack */
	/* This new feature may add a bit of time to the start process */
	if (cardpx[handlepx].processor != PROC_AMD)

	{
/*
// Replaced the loop of single writes with a bulk write.
// (The UNET could not keep up with the number of single writes.)
// The number of writes is double because we now initialize not-just-the-status-words

		msgStack = (t_msgStack *)&(cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[MSGSTACK_BEGIN]);
		for (i=0; i<RT_MSG_ENTRIES; i++)
//			msgStack[i].msgStatus = 0;
			WWORD_H2D( msgStack[i].msgStatus ,  0 );
*/

		//Erase msg stack
		pDPRam = (BYTE *)(&cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[MSGSTACK_BEGIN]);
		size = (RT_MSG_ENTRIES * ENTRYSIZE) / sizeof(usint);
		pZeroedDataBuffer = (usint *) malloc(size*sizeof(usint));
		for (i=0; i<size; i++)
			pZeroedDataBuffer[i] = 0;
		b2drmWriteWordBufferToDpr ( (DWORD_PTR)pDPRam,
									pZeroedDataBuffer,
									size,
									"Run_RT_Px",
									B2DRMHANDLE );
		free(pZeroedDataBuffer);
	}
	/* update old msgstack used by amd processor firmware */
	blkptr = (usint *)&(cardpx[handlepx].exc_bd_rt->msg_stack_old[0]);

/*
// Again we replace the for loop with a block write:

	for (i=0; i<RT_MSG_ENTRIES_OLD; i++)
	{
//		*(blkptr+2) = 0;
		WWORD_H2D( blkptr[2], 0 );
		//b2drmWriteWordToDpr( (DWORD_PTR) (blkptr+2) , (unsigned short) 0 , "Run_RT_Px: update old amd msgstack" );

		blkptr += 3;
	}
*/

	//Erase old msg stack
	pDPRam = (BYTE *)(&cardpx[handlepx].exc_bd_rt->msg_stack_old[0]);
	size = sizeof(cardpx[handlepx].exc_bd_rt->msg_stack_old) / sizeof(usint);
	pZeroedDataBuffer = (usint *) malloc(size*sizeof(usint));
	for (i=0; i<size; i++)
		pZeroedDataBuffer[i] = 0;
	b2drmWriteWordBufferToDpr ( (DWORD_PTR) pDPRam,
									pZeroedDataBuffer,
									size,
									"Run_RT_Px",
									B2DRMHANDLE );
	free(pZeroedDataBuffer);
	pZeroedDataBuffer = NULL;

	/* end new feature */

//	cardpx[handlepx].exc_bd_rt->start = 1;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->start, 1 );
	return (0);
}



/*
Set_Bit     Coded 19.11.90 by D. Koppel

Description: Provides a value to be returned in response to the Transmit
BIT (Built in Test) Word mode command for the given RT.

Input parameters:  rtnum     Address of the RT (0-31) to return this value
                   bitvalue  BIT response value - the value to be set

Output parameters: none

Return values  emode   If board is not in RT mode
               einval  If parameter is out of range
               0       If successful
*/

int borland_dll Set_Bit (int rtnum, int bitvalue)
{
	return Set_Bit_Px (g_handlepx, rtnum, bitvalue);
}

int borland_dll Set_Bit_Px (int handlepx, int rtnum, int bitvalue)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);

	if (cardpx[handlepx].singleFunction)
	{
		if (rtnum != cardpx[handlepx].rtNumber)
			return ertvalue;

//		cardpx[handlepx].exc_bd_rt->built_in_test[0] = (usint)bitvalue;
		WWORD_H2D( cardpx[handlepx].exc_bd_rt->built_in_test[0],(usint)bitvalue );
	}
	else
//		cardpx[handlepx].exc_bd_rt->built_in_test[rtnum] = (usint)bitvalue;
		WWORD_H2D( cardpx[handlepx].exc_bd_rt->built_in_test[rtnum] ,(usint)bitvalue );
	return (0);
}



/* Set_Bit_Cnt  is defined among the Bus Controller routines */



/*
Set_Invalid_Data_Res     Coded 19.11.90 by D. Koppel

Description: Determines whether RTs are to respond to messages containing
invalid data words.

Input parameters:  flag   1 = respond
                          0 = do not respond

Output parameters: none

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_Invalid_Data_Res (int flag)
{
	return Set_Invalid_Data_Res_Px (g_handlepx, flag);
}

int borland_dll Set_Invalid_Data_Res_Px (int handlepx, int flag)
{
	unsigned char tmpRtProtocolOptions;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	if (flag == 0){
//		cardpx[handlepx].exc_bd_rt->rtProtocolOptions &= (unsigned char)~flag;
		tmpRtProtocolOptions = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->rtProtocolOptions );
		tmpRtProtocolOptions &= (unsigned char)~flag;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->rtProtocolOptions, tmpRtProtocolOptions );
	}
	else if (flag == 1){
//		cardpx[handlepx].exc_bd_rt->rtProtocolOptions |= (unsigned char)flag;
		tmpRtProtocolOptions = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->rtProtocolOptions );
		tmpRtProtocolOptions |= (unsigned char)flag;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->rtProtocolOptions, tmpRtProtocolOptions );
	}
	else
		return (einval);
	return (0);
}



/*
Set_Mode_Addr     Coded 19.11.90 by D. Koppel

Description: Defines subaddresses to be used for mode commands. The last 5
bits of a MODE command word are interpreted differently than for a normal
command. The Remote Terminal must be able to distinguish between them.

Input parameters:  flag   0 = 11111 and 00000 are mode commands
                          1 = 00000 only is a mode command
                          2 = 11111 only is a mode command

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_Mode_Addr (int flag)
{
	return Set_Mode_Addr_Px (g_handlepx, flag);
}

int borland_dll Set_Mode_Addr_Px (int handlepx, int flag)
{
#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config == RT_MODE)
	if (cardpx[handlepx].board_config == RT_MODE)
	{
		if ((flag < 0) || (flag > 2))
			return (einval);
//		cardpx[handlepx].exc_bd_rt->mode_code = (unsigned char)flag;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->mode_code , flag );
		cardpx[handlepx].mode_code = flag;
		return 0;
	}
//	if ((cardpx[handlepx].exc_bd_rt->board_config == MON_BLOCK) ||
//		(cardpx[handlepx].exc_bd_rt->board_config == MON_LOOKUP))
	if ((cardpx[handlepx].board_config == MON_BLOCK) ||
		(cardpx[handlepx].board_config == MON_LOOKUP))
	{
		if ((flag < 0) || (flag > 2))
			return (einval);
//		cardpx[handlepx].exc_bd_monseq->mode_ctl = (unsigned char)flag;
		WBYTE_H2D( cardpx[handlepx].exc_bd_monseq->mode_ctl , flag );
	}
	else  /* neither RT nor MONITOR mode */
		return(emode);
	return (0);
#endif
}

/*
Set_RT_Active     Coded 19.11.90 by D. Koppel

Description:  Causes a particular RT to be simulated. The board will get
1553 data words from, and store 1553 data words in, its assigned block or
block 0 if no block is assigned. ‘intrpt’ lets you request an interrupt for
every rtid associated with this RT. This routine may be called in BC mode to
turn on concurrent RT simulation.

Input parameters:  rtnum   RT Address 0-31
                   intrpt  1 = generate message complete interrupt when a
                               message is processed by this RT
                           0 = no interrupt

Return values:  emode   If board is not in RT nor BC mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_RT_Active (int rtnum, int intrpt)
{
	return Set_RT_Active_Px (g_handlepx, rtnum, intrpt);
}


int borland_dll Set_RT_Active_Px (int handlepx, int rtnum, int intrpt)
{
	int status;
	unsigned char tmpActiveRT;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_rt->board_config != RT_MODE) &&
//		(cardpx[handlepx].exc_bd_rt->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != RT_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);

	
//	if (cardpx[handlepx].exc_bd_rt->board_config == RT_MODE)
	if (cardpx[handlepx].board_config == RT_MODE)
	{
		/* for single function we have special place in register */
		if (cardpx[handlepx].singleFunction)
		{
			status = Set_RT_In_Reg(handlepx,rtnum);
			if (status == 0)
			{
#ifdef VME4000
				cardpx[handlepx].exc_bd_rt->active[1] &= ~RTACTIVE_INTERRUPT_BITS;
				if (intrpt == 1)
					cardpx[handlepx].exc_bd_rt->active[1] |= RTACTIVE_INTERRUPT_BITS;
				else
					cardpx[handlepx].exc_bd_rt->active[1] |= RTACTIVE_BIT;
#else
//				cardpx[handlepx].exc_bd_rt->active[0] &= ~RTACTIVE_INTERRUPT_BITS;
				tmpActiveRT = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[0]);
				tmpActiveRT &= ~RTACTIVE_INTERRUPT_BITS;
				WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[0], tmpActiveRT);

//				if (intrpt == 1)
//					cardpx[handlepx].exc_bd_rt->active[0] |= RTACTIVE_INTERRUPT_BITS;
//				else
//					cardpx[handlepx].exc_bd_rt->active[0] |= RTACTIVE_BIT;
				tmpActiveRT = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[0]);
				if (intrpt == 1)
					tmpActiveRT |= RTACTIVE_INTERRUPT_BITS;
				else
					tmpActiveRT |= RTACTIVE_BIT;
				WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[0], tmpActiveRT);
#endif

			}
			return status;
		}

#ifdef VME4000
		cardpx[handlepx].exc_bd_rt->active[rtnum^1] &= ~RTACTIVE_INTERRUPT_BITS;
		if (intrpt == 1)
			cardpx[handlepx].exc_bd_rt->active[rtnum^1] |= RTACTIVE_INTERRUPT_BITS;
		else
			cardpx[handlepx].exc_bd_rt->active[rtnum^1] |= RTACTIVE_BIT;
	}
	else
	{
		if (cardpx[handlepx].singleFunction)
			return (enotforsinglefuncerror);

		cardpx[handlepx].exc_bd_bc->active[rtnum^1] |= RTACTIVE_BIT;
	}
#else
//		cardpx[handlepx].exc_bd_rt->active[rtnum] &= ~RTACTIVE_INTERRUPT_BITS;
		tmpActiveRT = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[rtnum] );
		tmpActiveRT &= ~RTACTIVE_INTERRUPT_BITS;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[rtnum] , tmpActiveRT) ; 

		if (intrpt == 1){
//			cardpx[handlepx].exc_bd_rt->active[rtnum] |= RTACTIVE_INTERRUPT_BITS;
			tmpActiveRT = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[rtnum] );
			tmpActiveRT |= RTACTIVE_INTERRUPT_BITS;
			WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[rtnum], tmpActiveRT );
		}
		else{
//			cardpx[handlepx].exc_bd_rt->active[rtnum] |= RTACTIVE_BIT;
			tmpActiveRT = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[rtnum] );
			tmpActiveRT |= RTACTIVE_BIT;
			WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[rtnum], tmpActiveRT );
		}

	}
	else
	{
		if (cardpx[handlepx].singleFunction)
			return (enotforsinglefuncerror);

//		cardpx[handlepx].exc_bd_bc->active[rtnum] |= RTACTIVE_BIT;
		tmpActiveRT = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->active[rtnum] );
		tmpActiveRT |= RTACTIVE_BIT;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->active[rtnum], tmpActiveRT );

	}
#endif
	
	return (0);
}


/*
Set_Broad_Interrupt     Coded 04 May 98 by D. Koppel & A. Meth

Description:  If the input argument is set to 1, which means ON, then,
for broadcast messages received, an interrupt will be generated.

Input parameters:  intrpt  1 = turn the interrupt bit on
                           0 = no interrupt

Output parameters: none

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful

This feature does not apply to the following boards:
    1553VME/E, 1553VME/ZC
*/

int borland_dll Set_Broad_Interrupt (int intrpt)
{
	return Set_Broad_Interrupt_Px (g_handlepx, intrpt);
}

int borland_dll Set_Broad_Interrupt_Px (int handlepx, int intrpt)
{
	unsigned char tmpActive;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	if (intrpt == 1)

#ifdef VME4000
		cardpx[handlepx].exc_bd_rt->active[31^1] |= RTINTERRUPT_BIT;
	else if (intrpt == 0)
		cardpx[handlepx].exc_bd_rt->active[31^1] &= ~RTINTERRUPT_BIT;

#else

	{
//		cardpx[handlepx].exc_bd_rt->active[31] |= RTINTERRUPT_BIT;
		tmpActive = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[31]);
		tmpActive |= RTINTERRUPT_BIT;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[31], tmpActive );
	}
	else if (intrpt == 0)
	{
//		cardpx[handlepx].exc_bd_rt->active[31] &= ~RTINTERRUPT_BIT;
		tmpActive = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[31] ); 
		tmpActive &= ~RTINTERRUPT_BIT;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[31] , tmpActive);
	}

#endif


	else
		return einval;

	return (0);
}

/*
Set_RT_Errors     Coded 19.11.90 by D. Koppel

Description: Sets up which errors are to be inserted by all RTs. All
requested errors may be ORed together.

Input parameters:  errormask   The following errors are defined:
                               BIT_CNT
                               BAD_GAP_TIME
                               STATUS_SYNC
                               STATUS_PARITY
                               DATA_SYNC
                               DATA_PARITY

Output parameters: none

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful

Note: BIT_CNT should only be set if Set_Bit_Cnt has been called previously.

*/

int borland_dll Set_RT_Errors (int errormask)
{
	return Set_RT_Errors_Px (g_handlepx, errormask);
}

int borland_dll Set_RT_Errors_Px (int handlepx, int errormask)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].singleFunction)
		return(enotforsinglefuncerror);

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if (errormask & ~RT_ERR_MASK)
		return (einval);
//	cardpx[handlepx].exc_bd_rt->err_inject = (unsigned char)errormask;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->err_inject ,(unsigned char)errormask );
	return (0);
}

/*
Set_RT_Interrupt     Coded 19.11.90 by D. Koppel

Description: Requests that a Message Complete interrupt be generated when
an active RT finishes sending or receiving a 1553 message. This applies to
messages for which Set_RT_Active was called with intrpt = 1.

Input parameters:  none

Output parameters: none

Return values:  emode  If board is not in RT mode
                0      If successful

Note: In RT mode, interrupts can also be generated using Set_Interrupt.

*/

int borland_dll Set_RT_Interrupt(void)
{
	return Set_RT_Interrupt_Px(g_handlepx);
}

int borland_dll Set_RT_Interrupt_Px(int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if(cardpx[handlepx].intnum == 0)
		return(noirqset);

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
//	cardpx[handlepx].exc_bd_rt->int_cond = 2;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->int_cond , 2 );
	return (0);
}

/*
Set_RT_Nonactive     Coded 19.11.90 by D. Koppel

Description: Sets a particular RT address nonactive, thereby turning off
simulation for the specified RT. The board will not respond to messages
sent to nonactive RTs nor will it store messages to/from nonactive RTs.
This routine may be called in BC mode to turn off concurrent RT simulation.

Input parameters:   rtnum   Address of RT to be set nonactive (0-31)

Output parameters:  none

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful

*/

int borland_dll Set_RT_Nonactive (int rtnum)
{
	return Set_RT_Nonactive_Px (g_handlepx, rtnum);
}

int borland_dll Set_RT_Nonactive_Px (int handlepx, int rtnum)
{
	unsigned char tmpActiveRT;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_rt->board_config != RT_MODE) &&
//		(cardpx[handlepx].exc_bd_rt->board_config != BC_RT_MODE))
	if ((cardpx[handlepx].board_config != RT_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);

	if (cardpx[handlepx].singleFunction)
		return(enotforsinglefuncerror);
	
#ifdef VME4000
	if (cardpx[handlepx].board_config == RT_MODE)
		cardpx[handlepx].exc_bd_rt->active[rtnum^1] &= ~RTACTIVE_INTERRUPT_BITS;
	else
		cardpx[handlepx].exc_bd_bc->active[rtnum^1] &= ~RTACTIVE_BIT;
#else
//	if (cardpx[handlepx].exc_bd_rt->board_config == RT_MODE)
	if (cardpx[handlepx].board_config == RT_MODE)
	{
//		cardpx[handlepx].exc_bd_rt->active[rtnum] &= ~RTACTIVE_INTERRUPT_BITS;
		tmpActiveRT = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[rtnum] );
		tmpActiveRT &= ~RTACTIVE_INTERRUPT_BITS;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[rtnum] , tmpActiveRT);
	}
	else
	{
//		cardpx[handlepx].exc_bd_bc->active[rtnum] &= ~RTACTIVE_BIT;
		tmpActiveRT = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->active[rtnum] );
		tmpActiveRT &= ~RTACTIVE_BIT;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->active[rtnum] , tmpActiveRT );
	}
#endif
	return (0);
}

/*
Set_RT_Resp_Time     Coded 19.11.90 by D. Koppel

Description:Sets response time for RTs. This is the time between receipt of
the command (and data for BC à RT messages) and the transmission of the 1553
status word by the RT. This routine may be called in BC mode to adjust the
timing of simulated concurrent RTs.

Input parameters:  nsecs   Time in nanoseconds; minimum value is 4000,
                           maximum is 42000 (MMSI: 12000)
Output parameters: none

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_RT_Resp_Time (int nsecs)
{
	return Set_RT_Resp_Time_Px (g_handlepx, nsecs);
}

int borland_dll Set_RT_Resp_Time_Px (int handlepx, int nsecs)
{

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

#ifdef MMSI
//	if (cardpx[handlepx].exc_bd_rt->board_config == RT_MODE)
	if (cardpx[handlepx].board_config == RT_MODE)
//		cardpx[handlepx].exc_bd_rt->rt_response = (unsigned char)(unsigned char)(nsecs/50);
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->rt_response, (unsigned char)(nsecs/50) );

//	else if (cardpx[handlepx].exc_bd_rt->board_config == BC_RT_MODE)
	else if (cardpx[handlepx].board_config == BC_RT_MODE)
//		cardpx[handlepx].exc_bd_bc->rt_resptime = (unsigned char)(nsecs/50);
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->rt_resptime, (unsigned char)(nsecs/50) );
#else

//	if (cardpx[handlepx].exc_bd_rt->board_config == RT_MODE)
	if (cardpx[handlepx].board_config == RT_MODE)
	{
		if ((cardpx[handlepx].singleFunction) && (nsecs > 12000))
			return eresptimeerror;

//		cardpx[handlepx].exc_bd_rt->rt_response = (unsigned char)((nsecs - 4000) / 155);
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->rt_response , (unsigned char)((nsecs - 4000) / 155) );
	}
//	else if (cardpx[handlepx].exc_bd_rt->board_config == BC_RT_MODE)
	else if (cardpx[handlepx].board_config == BC_RT_MODE)
	{
		if (cardpx[handlepx].singleFunction)
			return (enotforsinglefuncerror);

//		cardpx[handlepx].exc_bd_bc->rt_resptime = (unsigned char)((nsecs - 4000) / 155);
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->rt_resptime , (unsigned char)((nsecs - 4000) / 155) );
	}
#endif

	else
		return (emode);
	return 0;
}

/*
Set_1553Status     Coded 24.4.03 Nomi

Description: Provides a value to be transmitted as the 1553 status word for
the specified RT, with a flag specifying duration.

Input parameters:  rtnum        Address of the RT to return this value(0-31)
                   statusvalue  1553 status word value to be transmitted
			 duration     DUR_ALWAYS or DUR_ONETIME

Output parameters: none

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_1553Status (int rtnum, int statusvalue, int durationflag)
{
	return Set_1553Status_Px (g_handlepx, rtnum, statusvalue, durationflag);
}
#define IMMED_CLEAR_BITVAL 0x10
int borland_dll Set_1553Status_Px (int handlepx, int rtnum, int statusvalue, int durationflag)
{
	unsigned char *pActive;
	usint *pStatus;

	unsigned char tmpActive;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);
	if ((durationflag != DUR_ONETIME) && (durationflag != DUR_ALWAYS))
		return einvalDurationflag;

	/* set up pointers to the active RT element and to the RT status word - for each case */
	if (cardpx[handlepx].singleFunction)
	{
		if (rtnum != cardpx[handlepx].rtNumber)
			return ertvalue;
#ifdef VME4000
		pActive = &cardpx[handlepx].exc_bd_rt->active[1];
		pStatus = &cardpx[handlepx].exc_bd_rt->status[1];
#else
		pActive = &cardpx[handlepx].exc_bd_rt->active[0];
		pStatus = &cardpx[handlepx].exc_bd_rt->status[0];
#endif
	}
	else  /* multi-function module */
	{
#ifdef VME4000
		pActive = &cardpx[handlepx].exc_bd_rt->active[rtnum^1];
		pStatus = &cardpx[handlepx].exc_bd_rt->status[rtnum^1];
#else
		pActive = &cardpx[handlepx].exc_bd_rt->active[rtnum];
		pStatus = &cardpx[handlepx].exc_bd_rt->status[rtnum];
#endif
	}
	
	/* set the RT status word */
//	*pStatus = (usint)statusvalue;
	WWORD_H2D( pStatus[0], (usint)statusvalue );

	/* set the duration setting */
//	if (durationflag == DUR_ONETIME)
//		*pActive |= IMMED_CLEAR_BITVAL;
//	else if (durationflag == DUR_ALWAYS)
//		*pActive &= ~IMMED_CLEAR_BITVAL;
	tmpActive = RBYTE_D2H( pActive[0] );
	if (durationflag == DUR_ONETIME)
		tmpActive |= IMMED_CLEAR_BITVAL;
	else if (durationflag == DUR_ALWAYS)
		tmpActive &= ~IMMED_CLEAR_BITVAL;
	WBYTE_H2D( pActive[0], tmpActive );

	return (0);
}

int borland_dll Set_Status (int rtnum, int statusvalue)
{
	return Set_Status_Px (g_handlepx, rtnum, statusvalue);
}

int borland_dll Set_Status_Px (int handlepx, int rtnum, int statusvalue)
{
	return Set_1553Status_Px(handlepx, rtnum, statusvalue, DUR_ALWAYS);
}

/* Set_Var_Amp is defined among the Bus Controller routines */



/*
Set_Vector     Coded 19.11.90 by D. Koppel

Description: Provides a value to be transmitted in response to the Transmit
Vector Word mode command for the specified RT.

Input parameters:  rtnum     Address of the RT to return this value (0-31)
                   vecvalue  Service Request vector response value to be set

Output parameters: none

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_Vector (int rtnum, int vecvalue)
{
	return Set_Vector_Px (g_handlepx, rtnum, vecvalue);
}

int borland_dll Set_Vector_Px (int handlepx, int rtnum, int vecvalue)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);
	if (cardpx[handlepx].singleFunction)
	{
		if (rtnum != cardpx[handlepx].rtNumber)
			return ertvalue;

//		cardpx[handlepx].exc_bd_rt->vector[0] = (usint)vecvalue;
		WWORD_H2D( cardpx[handlepx].exc_bd_rt->vector[0] ,(usint)vecvalue );
	}
	else
//		cardpx[handlepx].exc_bd_rt->vector[rtnum] = (usint)vecvalue;
		WWORD_H2D( cardpx[handlepx].exc_bd_rt->vector[rtnum] ,(usint)vecvalue );

	return (0);
}



/*
Set_Wd_Cnt_Err     Coded 19.11.90 by D. Koppel

Description: Requests the selected RT to insert a Word Count Error in its
transmissions. Each individual RT can be set to send up to 3 words more
than the word count or up to three words less than the word count.

Input parameters:  rtnum   Address of the RT to return this value (0-31)
                   offset  Valid values are -3 to +3 to add to correct
                           word count

Output parameters: none

Return values: emode   If board is not in RT mode
               einval  If parameter is out of range
               0       If successful
*/

int borland_dll Set_Wd_Cnt_Err (int rtnum, int offset)
{
	return Set_Wd_Cnt_Err_Px (g_handlepx, rtnum, offset);
}

int borland_dll Set_Wd_Cnt_Err_Px (int handlepx, int rtnum, int offset)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);
	if ((offset < -3) || (offset > 3))
		return (einval);
	if (cardpx[handlepx].singleFunction)
		return(enotforsinglefuncerror);
#ifdef VME4000
	cardpx[handlepx].exc_bd_rt->word_count[rtnum^1] = (unsigned char)offset;
#else
//	cardpx[handlepx].exc_bd_rt->word_count[rtnum] = (unsigned char)offset;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->word_count[rtnum] ,(unsigned char)offset );
#endif

	return (0);
}


/*
Get_RT_Message     Coded Nov 28, 93 by D. Koppel

Description: Reads the next available entry from the message stack; the
entry immediately following the previous entry returned via a previous call
to this routine or to Read_Cmd_Stack. If the next block does not contain a
new entry an error will be returned.  Note that if you fall  RT_MSG_ENTRIES_OLD entries
behind the board, it will be impossible to know if the message returned is
newer or older than the previous message returned.

Input parameters:  none

Output parameters:  A pointer to a structure of type CMDENTRY containing
                    information regarding the next available entry from the
                    message stack.
                    struct CMDENTRY *cmdstruct
                    struct CMDENTRY {
                      int command;   1553 command word
                      int command2;  1553 transmit command word for RT to RT
                      int timetag;   timetag of message
                      int status;    status containing the following flags:
                                     END_OF_MSG      BUS_A
                                     TX_TIME_OUT     INVALID_WORD
                                     BAD_WD_CNT,     BROADCAST
                                     BAD_SYNC        BAD_GAP
                                     RT2RTMSG        ERROR
                                     In addition bit 6 will always be set to 1.
                    }

Return values:	emode		If board is not in RT mode
			enomsg	If no unread messages are available to return
			ertbusy	If current message in stack is being written to.
			entry number	If successful. Valid values are 0- RT_MSG_ENTRIES_OLD-1.
			func_invalid	If the module is a PxIII in default mode using the new RT message stack
					and therefore won't find any messages in the old RT message stack.
*/

#define ALREADY_READ   0x040
#define TRANSMIT_FLAG  0x400
#define ENTRYSIZE_OLD 3  /* number of words in an entry */
int borland_dll Get_RT_Message (struct CMDENTRY *cmdstruct)
{
	return Get_RT_Message_Px (g_handlepx, cmdstruct);
}
int borland_dll Get_RT_Message_Px (int handlepx, struct CMDENTRY *cmdstruct)
{
	usint *blkptr, *blkptr2, msgstatus, wordCount, command2;
	unsigned int micro_sec;

	unsigned char tmpRtProtocolOptions;

#ifdef MMSI
	return (func_invalid);
#else
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as an RT */
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	if (cardpx[handlepx].RTstacksStatus == ONLY_NEW_RT_STACK)
	{
		/* The module is a Nios-PxIII, which uses ONLY the new larger RT 
		Message Stack exclusively by default*/

		/* Set the value for the next time so that we do not bother doing it again*/
		cardpx[handlepx].RTstacksStatus = BOTH_RT_STACKS;

		/* If the RT is Running: First time called, set RT Protocol Options bit 02 */
//		if ((cardpx[handlepx].exc_bd_rt->start == 1))
		if ((RBYTE_D2H( cardpx[handlepx].exc_bd_rt->start) == 1))
		{
			/* Stop the RT  */
//			cardpx[handlepx].exc_bd_rt->start = 0;
			WBYTE_H2D( cardpx[handlepx].exc_bd_rt->start , 0 );

			/* Set the RT stack to be "BOTH STACKS"*/
//			cardpx[handlepx].exc_bd_rt->rtProtocolOptions |= RT_USE_BOTH_STACKS_BIT;
			tmpRtProtocolOptions = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->rtProtocolOptions );
			tmpRtProtocolOptions |= RT_USE_BOTH_STACKS_BIT;
			WBYTE_H2D( cardpx[handlepx].exc_bd_rt->rtProtocolOptions, tmpRtProtocolOptions);

			/* add DelayMicroSec_Px between STOP, set register, and START again
			on some machines, the stop + change register did not catch */
			DelayMicroSec_Px(handlepx, 100);

			/* ReStart the RT */
//			cardpx[handlepx].exc_bd_rt->start = 1;
			WBYTE_H2D( cardpx[handlepx].exc_bd_rt->start ,1 );

		}
		else /* If the RT is stopped, set the bit for next time */
		{
			/* Set the RT stack to be "BOTH STACKS"*/
//			cardpx[handlepx].exc_bd_rt->rtProtocolOptions |= RT_USE_BOTH_STACKS_BIT;
			tmpRtProtocolOptions = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->rtProtocolOptions );
			tmpRtProtocolOptions |= RT_USE_BOTH_STACKS_BIT;
			WBYTE_H2D( cardpx[handlepx].exc_bd_rt->rtProtocolOptions, tmpRtProtocolOptions);
		}
	}

	/* handle wraparound of buffer */
	cardpx[handlepx].next_rt_entry_old = (usint)(cardpx[handlepx].next_rt_entry_old % RT_MSG_ENTRIES_OLD);
	blkptr = (usint *)&(cardpx[handlepx].exc_bd_rt->msg_stack_old[cardpx[handlepx].next_rt_entry_old * ENTRYSIZE_OLD]);
//	msgstatus = *(blkptr+2);
	msgstatus = RWORD_D2H( blkptr[2] );
	if ((msgstatus & (END_OF_MSG | ALREADY_READ)) == END_OF_MSG)
	{
		cardpx[handlepx].next_rt_entry_old ++;
		msgstatus |= ALREADY_READ;
//		*(blkptr+2) = msgstatus;
		WWORD_H2D( blkptr[2], msgstatus );
//		cmdstruct->command = *(blkptr);
		cmdstruct->command = RWORD_D2H( blkptr[0] );

		if ((msgstatus & RT2RTMSG) && (!(cmdstruct->command & TRANSMIT_FLAG)) )
			/* RT2RT, RCV part (1st cmd) - RCV-RT is active */
		{
			blkptr2 = (usint *) &(cardpx[handlepx].exc_bd_rt->msg_stack_old[(cardpx[handlepx].next_rt_entry_old %  RT_MSG_ENTRIES_OLD) * ENTRYSIZE_OLD]);
//			msgstatus = *(blkptr2+2);
			msgstatus = RWORD_D2H( blkptr2[2] );
			if (!( (msgstatus & (END_OF_MSG | ALREADY_READ)) == END_OF_MSG ))

				/* In AMD processor firmware, there is a significant delay between
				writing the first and second parts of the rt2rt msg in msg_stack
				Instead of returning enomsg, in this case, try and wait for it */

			{
				if (cardpx[handlepx].processor != PROC_AMD)
				{
					wordCount = cmdstruct->command & 0x1f;
					if (wordCount == 0) wordCount = 32;
					micro_sec = wordCount*20 + 100;
				}
				else /* new processor */
					micro_sec = 3;
				DelayMicroSec_Px(handlepx, micro_sec);
			}

			/* We waited enough. Now check if transmit side of RT2RT msg came in.
			If so, it is the other side of the above RT2RT message. Fill in cmd2 values and return.

			If nothing new came in -- or something other than an RT2RT transmit,
			conclude that it was an rt2rt msg with rcv rt active, transmit rt not active.
			Nothing else to fill in, just slip back into regular message processing */

//			msgstatus = *(blkptr2+2);
			msgstatus = RWORD_D2H( blkptr2[2] );
//			command2 = *blkptr2;
			command2 = RWORD_D2H( blkptr2[0] );
			if ( ((msgstatus & (END_OF_MSG | ALREADY_READ)) == END_OF_MSG) &&
				((msgstatus & RT2RTMSG) && (command2 & TRANSMIT_FLAG)) )
			{
				msgstatus |= ALREADY_READ;
//				*(blkptr2+2) = msgstatus;
				WWORD_H2D( blkptr2[2],  msgstatus );
				cmdstruct->status = msgstatus;
//				cmdstruct->timetag = *(blkptr2+1);
				cmdstruct->timetag = RWORD_D2H( blkptr2[1] );
				cmdstruct->command2 = command2;
				cardpx[handlepx].next_rt_entry_old = (usint)(cardpx[handlepx].next_rt_entry_old % RT_MSG_ENTRIES_OLD);
				cardpx[handlepx].next_rt_entry_old++;
//				msgstatus = *(blkptr+2);
				msgstatus = RWORD_D2H( blkptr[2] );
				if((msgstatus & (END_OF_MSG|ALREADY_READ))!= (END_OF_MSG|ALREADY_READ))
					return ertbusy;
				return (cardpx[handlepx].next_rt_entry_old - 1) ;
			}
		}  /* RT to RT */

		cmdstruct->command2 = 0;/* correction */
//		cmdstruct->timetag = *(blkptr+1);
		cmdstruct->timetag = RWORD_D2H( blkptr[1] );
//		msgstatus = *(blkptr+2);
		msgstatus = RWORD_D2H( blkptr[2] );
		if ((msgstatus & (END_OF_MSG | ALREADY_READ )) != (END_OF_MSG | ALREADY_READ))
			return ertbusy;

		cmdstruct->status = msgstatus;
		Set_RT_Sync_Entry_Px(handlepx, cmdstruct->command, msgstatus, (unsigned int)(cmdstruct->timetag));

		return (cardpx[handlepx].next_rt_entry_old - 1);
	} /* message found */
	else {
		return (enomsg);
	}
#endif
}

/*
Get_Next_RT_Message     Coded Nov 28, 93 by D. Koppel

Description: Reads the next available entry from the message stack; the
entry immediately following the previous entry returned via a previous call
to this routine or to Read_Cmd_Stack. If the next block does not contain a
new entry an error will be returned.  Note that if you fall  RT_MSG_ENTRIES entries
behind the board, it will be impossible to know if the message returned is
newer or older than the previous message returned.

Input parameters:  none

Output parameters:  A pointer to a structure of type CMDENTRYRT containing
                    information regarding the next available entry from the
                    message stack.
                    struct CMDENTRYRT *cmdstruct
                    struct CMDENTRYRT {
                      usint command;   1553 command word
                      usint command2;  1553 transmit command word for RT to RT
                      usint timetaghi;   timetag of message
                      usint timetaglo;   timetag of message
                      usint status;    status containing the following flags:
                                     END_OF_MSG      BUS_A
                                     TX_TIME_OUT     INVALID_WORD
                                     BAD_WD_CNT,     BROADCAST
                                     BAD_SYNC        BAD_GAP
                                     RT2RTMSG        ERROR
                                     In addition bit 6 will always be set to 1.
                    }

Return values:  emode         If board is not in RT mode
                enomsg        If no unread messages are available to return
                ertbusy			If current message in stack is being written to.
                entry number  If successful. Valid values are 0- RT_MSG_ENTRIES-1.
		    func_invalid  If not Intel processor firmware revision 1.7 or greater.

*/

#define ALREADY_READ   0x040
#define TRANSMIT_FLAG  0x400
int borland_dll Get_Next_RT_Message (struct CMDENTRYRT *cmdstruct)
{
	return Get_Next_RT_Message_Px (g_handlepx, cmdstruct);
}
int borland_dll Get_Next_RT_Message_Px (int handlepx, struct CMDENTRYRT *cmdstruct)
{
// Future enhancment; last tested on 3-sep-2017; not ready for distribution yet.
//	if (cardpx[handlepx].RTentryCounterFW)
//		return Get_Next_RT_Message_With_Counter_Px (handlepx, cmdstruct);
//	else
		return Get_Next_RT_Message_With_Status_Px (handlepx, cmdstruct);
}

int borland_dll Get_Next_RT_Message_With_Counter_Px (int handlepx, struct CMDENTRYRT *cmdstruct)
{
	usint msgstatus, msgstatus2, command2, ttaglo, ttaghi;
	unsigned int ttag = 0, ttag2 = 0;
	struct RT_STACK_ENTRY RTstackEntry, RTstackEntry2;
	int retStatus, overwritten;
	int bufferStart, bufferEnd;
	// Isaac: for reading the full message from the stack 
//	t_msgStack *orgMsgStack, *orgMsgStack2;
//	t_msgStack localMsgStack, localMsgStack2, msgStackCheck;


	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as an RT */
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	/* check if the card supports the new rt msgstack */
	if (cardpx[handlepx].RTstacksStatus == ONLY_OLD_RT_STACK)
		return func_invalid;

	cardpx[handlepx].next_rt_entry = (usint)(cardpx[handlepx].next_rt_entry % RT_MSG_ENTRIES);

	overwritten = NOT_OVERWRITTEN;
	bufferStart = (int)((char *)&cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[MSGSTACK_BEGIN] - (char *)cardpx[handlepx].exc_bd_rt);
	bufferEnd = (int)((char *)&cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[MSGSTACK_END] - (char *)cardpx[handlepx].exc_bd_rt);
	if (cardpx[handlepx].numMsgsInLclRTmessageBuffer == 0) {
		cardpx[handlepx].pNextMsgInLclRTmessageBuffer = (struct RT_STACK_ENTRY *)cardpx[handlepx].LclRTmessageBuffer;
		retStatus = Read_RT_Block_Px(handlepx, (unsigned char **)&cardpx[handlepx].pNextMsgInLclRTmessageBuffer,  sizeof( struct RT_STACK_ENTRY), bufferStart,bufferEnd,&cardpx[handlepx].numMsgsInLclRTmessageBuffer, &overwritten);
		if (cardpx[handlepx].numMsgsInLclRTmessageBuffer == 0)
			return enomsg;
	}
	
	myMemCopy((void *)&RTstackEntry, (void *)cardpx[handlepx].pNextMsgInLclRTmessageBuffer, sizeof( struct RT_STACK_ENTRY), cardpx[handlepx].is32bitAccess);
	cardpx[handlepx].pNextMsgInLclRTmessageBuffer++;
	cardpx[handlepx].numMsgsInLclRTmessageBuffer--;
	if (overwritten == NOT_OVERWRITTEN)
		cardpx[handlepx].next_rt_entry++;
	else
		cardpx[handlepx].next_rt_entry = overwritten+1;//overwritten contains first entry number in buffer

	// Starting at this point msgStack points to our local message stack instead of dpram

	msgstatus = RTstackEntry.msgStatus;
	cmdstruct->command = RTstackEntry.command;
	ttaglo = RTstackEntry.timeTaglo;
	ttaghi = RTstackEntry.timeTaghi;

	ttag = (unsigned int)(ttaghi << 16) + ttaglo;
		
	if ((msgstatus & RT2RTMSG) && (!(cmdstruct->command & TRANSMIT_FLAG)) ) {
		/* RT2RT, RCV part (1st cmd) - RCV-RT is active */

		//if there is no next msg, the 2nd rt isn't being simulated, the fw updates entryCount by 2 for RT2RT both active messages
		if (cardpx[handlepx].numMsgsInLclRTmessageBuffer > 0) {

			myMemCopy((void *)&RTstackEntry2, (void *)cardpx[handlepx].pNextMsgInLclRTmessageBuffer, sizeof( struct RT_STACK_ENTRY), cardpx[handlepx].is32bitAccess);

			msgstatus2 = RTstackEntry2.msgStatus;
			command2 = RTstackEntry2.command;

			if ( ((msgstatus2 & RT2RTMSG) && (command2 & TRANSMIT_FLAG))) 	{
				ttag = (unsigned int)(ttaghi << 16) + ttaglo;
				ttag2 = (unsigned int)(RTstackEntry2.timeTaghi << 16) + RTstackEntry2.timeTaglo;
				if (ttag == ttag2) {
					cardpx[handlepx].pNextMsgInLclRTmessageBuffer++;
					cardpx[handlepx].numMsgsInLclRTmessageBuffer--;

					// These three lines are NEEDED because the RT2RT messages return
					// at the end of this if rather than at the regular exit
					cmdstruct->status = msgstatus2;
					cmdstruct->timetaghi = RTstackEntry2.timeTaghi;
					cmdstruct->timetaglo = RTstackEntry2.timeTaglo;
					cmdstruct->command2 = command2;

					// Keep this mod because we want to be ready to return the correct
					// value to the user below
					cardpx[handlepx].next_rt_entry = (usint)(cardpx[handlepx].next_rt_entry % RT_MSG_ENTRIES);
					cardpx[handlepx].next_rt_entry++;
					return (cardpx[handlepx].next_rt_entry - 1) ;
				}  // RT to RT both RT's simulated by us
			}	//possible RT to RT or two independent RT to RT messages
		}	//there is another message in local memory
	}	//first message is RT to RT receive
	cmdstruct->command2 = 0;/* correction */
	cmdstruct->timetaghi = ttaghi;
	cmdstruct->timetaglo = ttaglo;
	cmdstruct->status = msgstatus;
	//Set_RT_Sync_Entry_Px(handlepx, cmdstruct->command, msgstatus, ttag2);
	Set_RT_Sync_Entry_Px(handlepx, cmdstruct->command, msgstatus, ttag);

	return (cardpx[handlepx].next_rt_entry - 1);
}

int borland_dll Get_Next_RT_Message_With_Status_Px (int handlepx, struct CMDENTRYRT *cmdstruct)
{
	usint msgstatus, msgstatus2, command2, ttaglo, ttaghi;
	int msgstack_index;
	t_msgStack *msgStack, *msgStack2;
	unsigned int ttag = 0, ttag2 = 0;

	// Isaac: for reading the full message from the stack 
	t_msgStack *orgMsgStack, *orgMsgStack2;
	t_msgStack localMsgStack, localMsgStack2, msgStackCheck;


	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as an RT */
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	/* check if the card supports the new rt msgstack */
	if (cardpx[handlepx].RTstacksStatus == ONLY_OLD_RT_STACK)
		return func_invalid;

	/* handle wraparound of buffer */
	cardpx[handlepx].next_rt_entry = (usint)(cardpx[handlepx].next_rt_entry % RT_MSG_ENTRIES);
	msgstack_index = MSGSTACK_BEGIN + (cardpx[handlepx].next_rt_entry * ENTRYSIZE);
	msgStack = (t_msgStack *)&(cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[msgstack_index]);

	// Isaac: In order for the code to be readable and as much as possible to be as the original code, 
	//        we first read the full message to a local struct and all the checking will be on the local 
	//        and only the status modification will be in the DPR in the real message values
	orgMsgStack = msgStack;
	b2drmReadWordBufferFromDpr( (DWORD_PTR) msgStack,
								(unsigned short *) &localMsgStack,
								(unsigned int) (sizeof( localMsgStack ) / sizeof(unsigned short)),
								 " RT msgStack ",
								 B2DRMHANDLE );

	// help speed up RT mode when there is nothing to read; if the EOM flag is not set, then there is no reason to re-read the message just yet
	if ((localMsgStack.msgStatus & END_OF_MSG) != END_OF_MSG)
		return enomsg;

	//get a second copy so we can check if any changes were made while we were in mid read
	b2drmReadWordBufferFromDpr( (DWORD_PTR) msgStack,
								(unsigned short *) &msgStackCheck,
								(unsigned int) (sizeof( msgStackCheck ) / sizeof(unsigned short)),
								 " RT msgStackCheck ",
								 B2DRMHANDLE );

	// if nothing changed between the two reads, and EOM is set, then we have a good full message in hand - ready to process
	if ((localMsgStack.commandWord != msgStackCheck.commandWord) ||
		(localMsgStack.msgStatus != msgStackCheck.msgStatus) ||
		(localMsgStack.timeTaghi != msgStackCheck.timeTaghi) ||
		(localMsgStack.timeTaglo != msgStackCheck.timeTaglo))
		return ertbusy;


	// Starting at this point msgStack points to our local message stack instead of dpram

	msgStack = &localMsgStack;

	msgstatus = msgStack->msgStatus;

	if ((msgstatus & (END_OF_MSG | ALREADY_READ)) == END_OF_MSG)
	{
		cardpx[handlepx].next_rt_entry++;
		msgstatus |= ALREADY_READ;

	//	Update status both locally and on the board
		msgStack->msgStatus = msgstatus;
		WWORD_H2D( orgMsgStack->msgStatus , msgstatus); // actual writing to DPR

		cmdstruct->command = msgStack->commandWord;
		ttaglo = msgStack->timeTaglo;
		ttaghi = msgStack->timeTaghi;

		ttag = (unsigned int)(ttaghi << 16) + ttaglo;
		
		if ((msgstatus & RT2RTMSG) && (!(cmdstruct->command & TRANSMIT_FLAG)) )
			/* RT2RT, RCV part (1st cmd) - RCV-RT is active */
		{
			msgstack_index = MSGSTACK_BEGIN + ((cardpx[handlepx].next_rt_entry % RT_MSG_ENTRIES) * ENTRYSIZE);
			msgStack2 = (t_msgStack *) &(cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[msgstack_index]);

			orgMsgStack2 = msgStack2;
			b2drmReadWordBufferFromDpr( (DWORD_PTR) msgStack2,
										(unsigned short *) &localMsgStack2,
										(unsigned int) (sizeof( localMsgStack2 ) / sizeof(unsigned short)),
										" RT2RT msgStack2 ",
										B2DRMHANDLE );

			//get a second copy so we can check if any changes were made while we were in mid read
			b2drmReadWordBufferFromDpr( (DWORD_PTR) msgStack2,
										(unsigned short *) &msgStackCheck,
										(unsigned int) (sizeof( msgStackCheck ) / sizeof(unsigned short)),
										" RT2RT msgStackCheck ",
										B2DRMHANDLE );
			if ((localMsgStack2.commandWord != msgStackCheck.commandWord) ||
				(localMsgStack2.msgStatus != msgStackCheck.msgStatus) ||
				(localMsgStack2.timeTaghi != msgStackCheck.timeTaghi) ||
				(localMsgStack2.timeTaglo != msgStackCheck.timeTaglo))
				return ertbusy;

			// Starting at this point msgStack2 points to our local message stack instead of dpram
			msgStack2 = &localMsgStack2;


			//	Update status both locally and on the board
			msgstatus2 = msgStack2->msgStatus;

			command2 = msgStack2->commandWord;

			if ( ((msgstatus2 & (END_OF_MSG | ALREADY_READ)) == END_OF_MSG) &&
				((msgstatus2 & RT2RTMSG) && (command2 & TRANSMIT_FLAG)))
			{
				ttag = (unsigned int)(ttaghi << 16) + ttaglo;
				ttag2 = (unsigned int)(msgStack2->timeTaghi << 16) + msgStack2->timeTaglo;
				if (ttag == ttag2)
				{
					msgstatus2 |= ALREADY_READ;

					//	Update status both locally and on the board
					msgStack2->msgStatus = msgstatus2;
					WWORD_H2D( orgMsgStack2->msgStatus , msgstatus2); // actual writing to DPR


					// These three lines are NEEDED because the RT2RT messages return
					// at the end of this if rather than at the regular exit
					cmdstruct->status = msgstatus2;
					cmdstruct->timetaghi = msgStack2->timeTaghi;
					cmdstruct->timetaglo = msgStack2->timeTaglo;

					cmdstruct->command2 = command2;

					// Keep this mod because we want to be ready to return the correct
					// value to the user below
					cardpx[handlepx].next_rt_entry = (usint)(cardpx[handlepx].next_rt_entry % RT_MSG_ENTRIES);
					cardpx[handlepx].next_rt_entry++;


					// These next (three or) four lines are no longer needed since we
					// now maintain integrity by working with a copy

//					msgstatus = msgStack->msgStatus;
//					msgstatus = RWORD_D2H( orgMsgStack->msgStatus ); 

//					if ((msgstatus & (END_OF_MSG|ALREADY_READ))!= (END_OF_MSG|ALREADY_READ))
//						return ertbusy;
					return (cardpx[handlepx].next_rt_entry - 1) ;
				}

			}
		}  /* RT to RT */

		cmdstruct->command2 = 0;/* correction */
		cmdstruct->timetaghi = ttaghi;
		cmdstruct->timetaglo = ttaglo;

// The next couple of lines are no longer needed because we are working with an intact entire message.
//		msgstatus = msgStack->msgStatus;
//		if ((msgstatus & (END_OF_MSG | ALREADY_READ )) != (END_OF_MSG | ALREADY_READ))
//			return ertbusy;

		cmdstruct->status = msgstatus;
		//Set_RT_Sync_Entry_Px(handlepx, cmdstruct->command, msgstatus, ttag2);
		Set_RT_Sync_Entry_Px(handlepx, cmdstruct->command, msgstatus, ttag);

		return (cardpx[handlepx].next_rt_entry - 1);
	} /* message found */
	else
		return (enomsg);
}

/*	Set_Both_RT_Stacks_Px ()

     Coded 2008 by D. Shachor

	There are two message stacks that can be used on current Px modules.
	On the original (AMD-based) modules there was a single message stack, and
	a second message stack area was added to later revisions of the Intel-based
	modules.  By default, the AMD-based modules and the early Intel-based
	modules use the original (old) stack area, while the later revisions of the
	Intel-based boards use both.  The (NIOS II-based) PxIII modules only
	use the newer stack area by default, but they can be set to use both by
	setting the rtProtocolOptions-RT_USE_BOTH_STACKS_BIT.

Input parameters:	handlepx
			enableFlag	ENABLE	Enable the old RT message stack
							area and use both stacks.
					DISABLE	Disable the old RT message stack
							area, and only use the new area.

Output parameters:  none

Return values:	ebadhandle	If the handle to the PxModule is not a valid value
			einval		If the enableFlag parameter is out of range
			emode		If not in RT mode
			0		If successful
*/

int borland_dll Set_Both_RT_Stacks_Px (int handlepx, int enableFlag)
{
	unsigned char tmpRtProtocolOptions;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE) return emode;
	if (cardpx[handlepx].board_config != RT_MODE) return emode;
	if ((enableFlag != ENABLE) && (enableFlag != DISABLE)) return einval;

	/* This function is effective only for the Nios module (PxIII) */
	if (cardpx[handlepx].processor != PROC_NIOS) return(0);

	/* this function is effective only when the module is stopped */
//	if (cardpx[handlepx].exc_bd_rt->start == 1) return ecallmewhenstopped;
	if (RBYTE_D2H( cardpx[handlepx].exc_bd_rt->start ) == 1) return ecallmewhenstopped;

	/* Enable the old RT message stack area, and use both RT message stack areas. */
	if (enableFlag == ENABLE)
	{
//		cardpx[handlepx].exc_bd_rt->rtProtocolOptions |= (unsigned char) RT_USE_BOTH_STACKS_BIT;
		tmpRtProtocolOptions = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->rtProtocolOptions );
		tmpRtProtocolOptions |= (unsigned char) RT_USE_BOTH_STACKS_BIT;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->rtProtocolOptions, tmpRtProtocolOptions);

		cardpx[handlepx].RTstacksStatus = BOTH_RT_STACKS;
		/* changing the value of this indicator is relevant ONLY for Nios, and should not be touched for other processors */
	}
	/* Disable the old RT message stack area, and only use the new message stack area. */
	else
	{
//		cardpx[handlepx].exc_bd_rt->rtProtocolOptions &= (unsigned char) ~RT_USE_BOTH_STACKS_BIT;
		tmpRtProtocolOptions = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->rtProtocolOptions );
		tmpRtProtocolOptions &= (unsigned char) ~RT_USE_BOTH_STACKS_BIT;
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->rtProtocolOptions, tmpRtProtocolOptions);

		cardpx[handlepx].RTstacksStatus = ONLY_NEW_RT_STACK;
		/* changing the value of this indicator is relevant ONLY for Nios, and should not be touched for other processors */
	}
	
	return (0);
}

/*
Get_Blknum     Coded 16.06.97 by Reuben Kantor

Description: Retrieves data block number which was assigned to specified
RTid.
Input parameters:   rtid   11 bit identifier including RT#, T/R bit and
                           subaddress.
Output parameters:  none
Return values:  emode         If board is not in RT mode
                einval        If parameter is out of range
                block number  If successful. Valid values are 0-199/255/511.
*/

int borland_dll Get_Blknum (int rtid)
{
	return Get_Blknum_Px (g_handlepx, rtid);
}

int borland_dll Get_Blknum_Px (int handlepx, int rtid)
{
	int blknum;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return emode;

	if ((rtid < 0) || (rtid > 2047))
		return einval;

	if (cardpx[handlepx].singleFunction && (((rtid>>6) & cardpx[handlepx].rtNumber) != cardpx[handlepx].rtNumber))
		return ertvalue;

#ifdef VME4000
	blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[rtid^1];
#else
//	blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[rtid];
	blknum = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->blk_lookup[rtid] );
#endif

//	blknum |= (cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid] & RTID_EXPANDED_BLOCK) << RTID_EXPANDED_BLOCK_SHIFT;
	blknum |= ( RWORD_D2H(cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid]) & RTID_EXPANDED_BLOCK) << RTID_EXPANDED_BLOCK_SHIFT;
	return blknum;
}

/*
Get_Next_Message_RTM	Coded Nov 28, 93 by D. Koppel
			Updated for Concurrent Monitor, Apr 98, by A. Meth

Note: This routine is like Get_Next_Message, which gets the next Bus Monitor
message block. However, it is used *only* for a RT/Concurrent Bus Monitor
(hence the appended "_RTM" in the function name).

Description: Reads the message block following the block read in the
previous call to Get_Next_Message_RTM.

The first call to Get_Next_Message_RTM will return block 0.

Note: If the next block does not contain a new message an error will be
returned. Note that if you fall 128 blocks behind the board it will be
impossible to know if the message returned is newer or older than the
previous message returned unless you check the timetag. The routine
therefore returns the timetag itself in the elapsed time field rather
than a delta value.

Input parameters: msgptr   Pointer to the structure defined below in which
			   to return the message
		  struct MONMSG {
		  int msgstatus; Status word containing the following flags:
				 END_OF_MSG	 RT2RT_MSG
				 TRIGGER_FOUND	 INVALID_MSG
				 BUS_A	 INVALID_WORD
				 WD_CNT_HI	 WD_CNT_LO
				 BAD_RT_ADDR	 BAD_SYNC
				 BAD_GAP	 MON_LATE_RESP
				 MSG_ERROR
		  unsigned long elapsedtime; The timetag associated with the
					     message.
		  unsigned int words [36];   A pointer to an array of 1553
					     words in the sequence they
					     were received over the bus
		  }

Output parameters: none

Return values:	block number  If successful. Valid values are 0-0x80 (0-128).
		emode	      If board is not in RT mode
		enomsg	      If there is no message to return


Change history:
-----------------
8-Sep-2008	(YYB)
-	added DMA support

10-Dec-2008	(YYB)
- corrected a pointer assignment when DMA is not available
	(the sense of an "if" test was backwards)

*/

int borland_dll Get_Next_Message_RTM (struct MONMSG *msgptr, usint * msg_addr)
{
	return Get_Next_Message_RTM_Px (g_handlepx, msgptr, msg_addr);
}

int borland_dll Get_Next_Message_RTM_Px (int handlepx, struct MONMSG *msgptr, usint *msg_addr)
{
	struct MONMSGPACKED msgCopy1packed;
	struct MONMSGPACKED msgCopy2packed;

	usint * pDpram = NULL;
	int wordLoopCounter;

	usint * pDmaCopy;
	struct MONMSGPACKED  * pMsgDmaCopyPacked;
	int dmaCopyIsValid;

	int	status = Successful_Completion_Exc;

#ifdef MMSI
	return (func_invalid);
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as an RT */
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	/* handle wraparound of buffer */
	cardpx[handlepx].next_MON_entry %= cardpx[handlepx].cmon_stack_size;

	pDpram = (usint *) &(cardpx[handlepx].cmon_stack[cardpx[handlepx].next_MON_entry]);

	// UNet takes advantage of this function too. Though initialy written for DMA, it was
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
										"Get_Next_Message_RTM_Px",
										B2DRMHANDLE);

		// help speed up message reads when there is nothing to read; if the EOM flag is not set, then there is no reason to re-read the message just yet
		if ((msgCopy1packed.msgstatus & END_OF_MSG) != END_OF_MSG)
			return enomsg;

		//get a second copy of the header so we can check if any changes were made while we were in mid read
		b2drmReadWordBufferFromDpr( (DWORD_PTR) pDpram,
									(unsigned short *) &msgCopy2packed,
									MONMSG_HEADER_SIZE / sizeof(unsigned short),
									"Get_Next_Message_RTM_Px",
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


	*msg_addr = msgptr->msgCounter;
	cardpx[handlepx].next_MON_entry++;
	return (cardpx[handlepx].next_MON_entry - 1);
}

int borland_dll RT_Id_Px(int rtnum, int type, int subaddr, int *rtid)
{
	if ((rtnum <0) || (rtnum > 31))
		return einval;
	if ((type != TRANSMIT) && (type != RECEIVE))
		return einval;
	if ((subaddr <0 ) || (subaddr > 31))
		return einval;

	*rtid = (rtnum << 6) | (type <<5) | subaddr;
	return 0;
}

int borland_dll SA_Id_Px(int type, int subaddr, int wordcount, int *said)
{
	if ((type != TRANSMIT) && (type != RECEIVE))
		return einval;
	if ((subaddr <0 ) || (subaddr > 31))
		return einval;
	if ((wordcount <0) || (wordcount > 31))
		return einval;

	*said = (type << 10) | (subaddr <<5) | wordcount;
	return 0;
}

/*
Set_RT_Active_Bus

Description:  Causes a particular RT to respond to specified bus/es.

Input parameters:  rtnum   RT Address 0-31
                   bus     BUS_A_ONLY, BUS_B_ONLY, BUS_AB, BUS_NONE

Return values:  emode   If board is not in RT mode
                einval  If parameter is out of range
                0       If successful
*/

int borland_dll Set_RT_Active_Bus (int rtnum, usint bus)
{
	return Set_RT_Active_Bus_Px (g_handlepx, rtnum, bus);
}
#define INACTIVEBUS_BITS 0xC
/* CR1 */
int borland_dll Set_RT_Active_Bus_Px (int handlepx, int rtnum, usint bus)
{
	usint curval;

#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);
	if (bus > 3)
		return einval;


	if (cardpx[handlepx].singleFunction)
	{
#ifdef VME4000
		curval = cardpx[handlepx].exc_bd_rt->active[0^1] & ~INACTIVEBUS_BITS;
		cardpx[handlepx].exc_bd_rt->active[0^1] = curval | (bus<<2);
#else
//		curval = cardpx[handlepx].exc_bd_rt->active[0] & ~INACTIVEBUS_BITS;
		curval = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[0] ) & ~INACTIVEBUS_BITS;
//		cardpx[handlepx].exc_bd_rt->active[0] = curval | (bus<<2);
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[0], (curval | (bus<<2)) );
#endif
	}
	else
	{

#ifdef VME4000
		curval = cardpx[handlepx].exc_bd_rt->active[rtnum^1] & ~INACTIVEBUS_BITS;
		cardpx[handlepx].exc_bd_rt->active[rtnum^1] = curval | (bus<<2);
#else
//		curval = cardpx[handlepx].exc_bd_rt->active[rtnum] & ~INACTIVEBUS_BITS;
		curval = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->active[rtnum] ) & ~INACTIVEBUS_BITS;
//		cardpx[handlepx].exc_bd_rt->active[rtnum] = curval | (bus<<2);
		WBYTE_H2D( cardpx[handlepx].exc_bd_rt->active[rtnum], (curval | (bus<<2)) );
#endif
	}

	return (0);
#endif
}
/*
Set_RTID_Interrupt

Description:
Input parameters:   rtid   11 bit identifier including RT#, T/R bit and
                           subaddress.
Output parameters:  none
Return values:  emode         If board is not in RT mode
                einval        If parameter is out of range
                0  		If successful.
*/

int borland_dll Set_RTid_Interrupt_Px (int handlepx, int rtid, int enabled_flag, int int_type)
{
	usint curctrl;

	/* if the card is an old PCMCIA/EP (AMD), then this functionality does not exist  */
	if ( (cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1) ) {
		return(func_invalid);
	}

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return emode;

	if ((rtid < 0) || (rtid > 2047))
		return einval;

	if (cardpx[handlepx].singleFunction && (((rtid>>6) & cardpx[handlepx].rtNumber) != cardpx[handlepx].rtNumber))
		return ertvalue;

	if ((enabled_flag != ENABLE) && (enabled_flag != DISABLE))
		return einval;
	if ((int_type != INT_ON_ERR) && (int_type !=INT_ON_ENDOFMSG))
		return einval;

//	curctrl = cardpx[handlepx].rtid_ctrl[rtid];
	curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[rtid] );

	curctrl &= (usint)~(1 << int_type); /* mask out bit 0 or 1, according to int_type */
	curctrl |= (usint)(enabled_flag << int_type);

//	cardpx[handlepx].rtid_ctrl[rtid] = curctrl;
	WWORD_H2D( cardpx[handlepx].rtid_ctrl[rtid], curctrl );

	return 0;
}

int borland_dll Set_RTid_Status_Px (int handlepx, int rtid, int enabled_flag, int status_type)
{
	usint curctrl;
	
#ifdef MMSI
	if (status_type == RTID_BUSY)
		return einval;
#endif

	/* if the card is an old PCMCIA/EP (AMD), then this functionality does not exist  */
	if ( (cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1) ) {
		return(func_invalid);
	}

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return emode;
	if ((rtid < 0) || (rtid > 2047))
		return einval;
	if ((enabled_flag != ENABLE) && (enabled_flag != DISABLE))
		return einval;
	if ((status_type != RTID_ILLEGAL) && (status_type != RTID_INACTIVE) && (status_type != RTID_BUSY))
		return einval;

	/* Verify that the FW supports the feature */
	if (status_type == RTID_BUSY)
	{
//		if ((cardpx[handlepx].exc_bd_rt->more_board_options & BUSY_BIT_RTID) != BUSY_BIT_RTID)
		if ((RWORD_D2H( cardpx[handlepx].exc_bd_rt->more_board_options) & BUSY_BIT_RTID) != BUSY_BIT_RTID)
			return efeature_not_supported_PX;
	}

//	curctrl = cardpx[handlepx].rtid_ctrl[rtid];
	curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[rtid] );

	curctrl &= (usint)~(1 << status_type); /* mask out appropriate bit */
	curctrl |= (usint)(enabled_flag << status_type);

//	cardpx[handlepx].rtid_ctrl[rtid] = curctrl;
	WWORD_H2D( cardpx[handlepx].rtid_ctrl[rtid], curctrl );

	return 0;
}



int borland_dll Set_SAid_Illegal_Px (int handlepx, int said, int enabled_flag, int broadcast_flag)
{
	usint curctrl;

	if (!cardpx[handlepx].singleFunction)
		return func_invalid;


	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return emode;
	if ((said < 0) || (said > 0x7ff))
		return einval;
	if ((enabled_flag != ENABLE) && (enabled_flag != DISABLE))
		return einval;
	if ((broadcast_flag != ENABLE) && (broadcast_flag != DISABLE))
		return einval;

	if (broadcast_flag == DISABLE)
	{
//		curctrl = cardpx[handlepx].rtid_ctrl[said];
		curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[said] );

		curctrl &= (usint)~(1 << RTID_ILLEGAL); /* mask out appropriate bit */
		curctrl |= (usint)(enabled_flag << RTID_ILLEGAL);

//		cardpx[handlepx].rtid_ctrl[said] = curctrl;
		WWORD_H2D( cardpx[handlepx].rtid_ctrl[said], curctrl );
	}
	else /*Broadcast*/
	{
//		curctrl = cardpx[handlepx].rtid_brd_ctrl[said];
		curctrl = (usint) RBYTE_D2H( cardpx[handlepx].rtid_brd_ctrl[said] );

		curctrl &= (usint)~(1 << RTID_ILLEGAL); /* mask out appropriate bit */
		curctrl |= (usint)(enabled_flag << RTID_ILLEGAL);

//		cardpx[handlepx].rtid_brd_ctrl[said] = (unsigned char)curctrl;
		WBYTE_H2D( cardpx[handlepx].rtid_brd_ctrl[said], (unsigned char)curctrl );
	}

	return 0;
}


int borland_dll Assign_DB_Datablk(int rtid, int enabled_flag, int blknum)
{
	return Assign_DB_Datablk_Px(g_handlepx, rtid, enabled_flag, blknum);
}

int borland_dll Assign_DB_Datablk_Px (int handlepx, int rtid, int enabled_flag, int blknum)
{
	usint curctrl;
	int status;

	usint tmpRtidIP_cmonAP;

	/* if the card is an old PCMCIA/EP (AMD), then this functionality does not exist  */
	if ( (cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1) ) {
		return(func_invalid);
	}

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as an RT */
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	if ((rtid < 0) || (rtid > 2047))
		return (einval);

	if (cardpx[handlepx].singleFunction && (((rtid>>6) & cardpx[handlepx].rtNumber) != cardpx[handlepx].rtNumber))
		return ertvalue;

	if ((enabled_flag != ENABLE) && (enabled_flag != DISABLE))
		return einval;
	if (((rtid>>5) & 0x1) == TRANSMIT)
		return ercvfunc;

//	curctrl = cardpx[handlepx].rtid_ctrl[rtid];
	curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[rtid] );

	if (enabled_flag == DISABLE)
	{
		curctrl &= ~(DB_ENABLE_SET); /* mask out appropriate bit */
		curctrl &= ~(DB_ACTIVE_SET); /* mask out active bit in case alternate one was being used */

//		cardpx[handlepx].rtid_ctrl[rtid] = curctrl;
		WWORD_H2D( cardpx[handlepx].rtid_ctrl[rtid], curctrl );

		return 0;
	}

	status = Validate_Block_Number(handlepx, blknum);
	if (status < 0)
		return einval;

	if (blknum%2 != 0)
		return ebadblknum;

	curctrl |= DB_ENABLE_SET; /* set the db bit */

//	cardpx[handlepx].rtid_ctrl[rtid] = curctrl;
	WWORD_H2D( cardpx[handlepx].rtid_ctrl[rtid], curctrl );

#ifdef VME4000
	cardpx[handlepx].exc_bd_rt->blk_lookup[rtid^1] = (unsigned char)blknum;
#else
//	cardpx[handlepx].exc_bd_rt->blk_lookup[rtid] = (unsigned char)blknum;
	WBYTE_H2D( cardpx[handlepx].exc_bd_rt->blk_lookup[rtid],(unsigned char)blknum );
#endif

//	if (blknum > 255)
//		cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid] |= RTID_EXPANDED_BLOCK;
//	else
//		cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid] &= ~RTID_EXPANDED_BLOCK;
	tmpRtidIP_cmonAP = RWORD_D2H( cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid] );
	if (blknum > 255)
		tmpRtidIP_cmonAP |= RTID_EXPANDED_BLOCK;
	else
		tmpRtidIP_cmonAP &= ~RTID_EXPANDED_BLOCK;
	WWORD_H2D( cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[rtid], tmpRtidIP_cmonAP);

	return(0);
}


/*
Read_RTid_Px

Description:		Used to read data based on rtid, this is necessary for rtid's using double buffering or
						multibuffering where the user doesn't know the block number to be read
Input parameters:   rtid     	11 bit identifier including RT#, T/R bit and
                             	subaddress.
Output parameters:  	words		a pointer to an array of 32 words in which to copy the data
Return values:  		emode    If board is not in RT mode
                		einval   If parameter is out of range
                		0  		If successful.
Change History
--------------
8-Sep-2008	(YYB)
-	added DMA support

10-Dec-2008	(YYB)
- corrected the size of the block-to-copy in the PerformDMARead() calls.

*/

/* 25oct06  NOTE: What about MMSI support ? new Multi-Buffer feature not yet supported */

int borland_dll Read_RTid (int rtid, usint *words)
{
	return Read_RTid_Px(g_handlepx, rtid, words);
}

int borland_dll Read_RTid_Px (int handlepx, int rtid, usint *words)
{
	usint curctrl, blkoffset, *dataptr;
	int i, blknum, maxbufs;
	void * datablockOffset;
	int	status = Successful_Completion_Exc;

	/* if the card is an old PCMCIA/EP (AMD), then this functionality does not exist  */
	if ( (cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1) ) {
		return(func_invalid);
	}

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtid < 0) || (rtid > 2047))
		return (einval);

	if (cardpx[handlepx].singleFunction && (((rtid>>6) & cardpx[handlepx].rtNumber) != cardpx[handlepx].rtNumber))
		return ertvalue;

//	curctrl = cardpx[handlepx].rtid_ctrl[rtid];
	curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[rtid] );

	/* This function can now be used to read the data block/s of any rtid, whether or not double-buffered, transmit or receive
	if ((curctrl & DB_ENABLE_SET) != DB_ENABLE_SET)
	return edbnotset;
	*/

#ifdef VME4000
	blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[rtid^1];
#else
//	blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[rtid];
	blknum = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->blk_lookup[rtid] );
#endif
	blkoffset = (curctrl & DB_ACTIVE_SET) == DB_ACTIVE_SET;
	/* if multibuffering is in use add the offset of the next buffer to access */
	blknum += cardpx[handlepx].NextBufToAccess[rtid];
	maxbufs = ((curctrl >> MULTIBUFSHIFT) & MAXBUFSMASK) + 1;

	cardpx[handlepx].NextBufToAccess[rtid] = (cardpx[handlepx].NextBufToAccess[rtid]+1) % maxbufs;

	/* handle extended and expanded buffers */
	if (curctrl & RTID_EXPANDED_BLOCK) {
		dataptr = &(cardpx[handlepx].exc_bd_rt->data3[((blknum+blkoffset) * BLOCKSIZE)]);
	}
	else if (blknum < 200) {
		dataptr = &(cardpx[handlepx].exc_bd_rt->data[(blknum+blkoffset) * BLOCKSIZE]);
	}
	else {
		dataptr = &(cardpx[handlepx].exc_bd_rt->data2[((blknum+blkoffset-200) * BLOCKSIZE)]);
	}

	if ((cardpx[handlepx].dmaAvailable == TRUE) && (cardpx[handlepx].useDmaIfAvailable == TRUE))
	{
		datablockOffset = (void *) ((DWORD_PTR) dataptr - (DWORD_PTR) (cardpx[handlepx].exc_bd_rt));
#ifdef DMA_SUPPORTED
		status = PerformDMARead(cardpx[handlepx].card_addr, cardpx[handlepx].module,
			&(words[0]), MSGSIZEBYTES_RT, datablockOffset);
		if (status < 0)
#endif
		{
			/* Always move 32 words. Unused words don't matter. */
			for (i = 0; i < BLOCKSIZE; i++)
				words[i] = *(dataptr+i);
		}
	}

	else
	{
		/* Always move 32 words. Unused words don't matter. */
//		for (i = 0; i < BLOCKSIZE; i++)
//			words[i] = *(dataptr+i);

		b2drmReadWordBufferFromDpr( (DWORD_PTR) dataptr,
									(unsigned short *) words,
									BLOCKSIZE, // BLOCKSIZE = number of WORDs
									"Read RTid block of words",
									B2DRMHANDLE );

	}

	return(0);
}

/*
Load_RTid_Px

Description:		Used to write data based on rtid, this is necessary for rtid's using
						multibuffering since the user doesn't know the block number to be written
Input parameters:   rtid     	11 bit identifier including RT#, T/R bit and
                             	subaddress.
					words		a pointer to an array of 32 words from which to copy the data
Output parameters:  	none
Return values:  		emode    If board is not in RT mode
                		einval   If parameter is out of range
                		0  		If successful.
*/

// 25oct06  NOTE: What about MMSI support? new Multi-Buffer feature not yet supported

int borland_dll Load_RTid_Px (int handlepx, int rtid, usint *words)
{
	usint curctrl, *dataptr;
	int blknum, maxbufs;
//	int i;  // loop replaced with b2drmWriteWordBufferToDpr

	/* if the card is an old PCMCIA/EP (AMD), then this functionality does not exist  */
	if ( (cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1) ) {
		return(func_invalid);
	}

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtid < 0) || (rtid > 2047))
		return (einval);

	if (cardpx[handlepx].singleFunction && (((rtid>>6) & cardpx[handlepx].rtNumber) != cardpx[handlepx].rtNumber))
		return ertvalue;

#ifdef VME4000
	blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[rtid^1];
#else
//	blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[rtid];
	blknum = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->blk_lookup[rtid] );
#endif


//	curctrl = cardpx[handlepx].rtid_ctrl[rtid];
	curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[rtid] );

	/* In case multibuffering is in use add the offset of the next buffer to access */
	blknum += cardpx[handlepx].NextBufToAccess[rtid];
	maxbufs = ((curctrl >> MULTIBUFSHIFT) & MAXBUFSMASK) + 1;

	cardpx[handlepx].NextBufToAccess[rtid] = (cardpx[handlepx].NextBufToAccess[rtid]+1) % maxbufs;

	/* handle extended and expanded buffers */
	if (curctrl & RTID_EXPANDED_BLOCK) {
		dataptr = &(cardpx[handlepx].exc_bd_rt->data3[((blknum) * BLOCKSIZE)]);
	}
	else if (blknum < 200) {
		dataptr = &(cardpx[handlepx].exc_bd_rt->data[blknum * BLOCKSIZE]);
	}
	else {
		dataptr = &(cardpx[handlepx].exc_bd_rt->data2[(blknum-200) * BLOCKSIZE]);
	}

	/* Always move 32 words. Unused words don't matter. */
//	for (i = 0; i < BLOCKSIZE; i++)
////		*(dataptr+i) = words[i];
//		 WWORD_H2D( dataptr[i], words[i] );

	b2drmWriteWordBufferToDpr ( (DWORD_PTR) dataptr,
									 (unsigned short *) words,
									 BLOCKSIZE, // BLOCKSIZE = number of WORDs
									 "Load_RTid_Px",
									 B2DRMHANDLE );

	return(0);
}

   /* check whether command is sync mode code. get the RT number
      save current ttag, and data if sync w/data, for this RT */

int Set_RT_Sync_Entry_Px(int handlepx, usint command, usint status, unsigned int timetag)
{
	unsigned char flag;
	int mc=0, mctype, rtnum, rtid, blknum, numfirst, numlast;
	usint words[32];

	/* The flag is the mode code address. flag values:  0:31/0 1:0 2:31 */
//	flag = cardpx[handlepx].exc_bd_rt->mode_code;
//	flag = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->mode_code );
	flag = cardpx[handlepx].mode_code;
	if ((flag == 0) || (flag == 1)) /* address 0 */
	{
		if ((command & 0x3E0)==(0<<5))
			mc = 1;
	}
	if ((flag == 0) || (flag == 2)) /* address 31 */
		if ((command & 0x3E0)==(31<<5))
			mc = 1;

	if (mc == 0)
		return 0;

	mctype = command & 0x1f;
	if ((mctype !=1) && (mctype != 17))
		return 0;

	if (status & BROADCAST)
	{
		numfirst = 0;
		numlast = 30;
	}
	else
	{
		numfirst = (command & 0xf800)>>11;
		numlast = numfirst;
	}

	rtid = command >> 5;
	blknum = Get_Blknum_Px(handlepx, rtid);
	Read_Datablk_Px (handlepx, blknum, words);

	if (cardpx[handlepx].singleFunction)
	{
		numfirst = 0;
		numlast = 0;
	}

	for (rtnum = numfirst; rtnum <= numlast; rtnum++)
	{
		cardpx[handlepx].rt_sync_timetag[rtnum] = timetag;
		if (mctype == 17)  /* 17: with data */
			cardpx[handlepx].rt_sync_data[rtnum] = words[0];
		else /* 1: w/o data */
			cardpx[handlepx].rt_sync_data[rtnum] = 0;
	}
	return 0;
}
/*
Get_RT_Sync_Info_Px

int borland_dll Get_RT_Sync_Info_Px(int handlepx, int rtnum, unsigned int
*sync_timetag, usint *sync_data);

Description:    Extract the (internally stored) timetag and data word (if
any) of the most recent occurrence            of a SYNC mode code message
(MC-1 or MC-17) for this RT.

Input parameters:       rtnum           The number of the RT for which we
want to get the SYNC information.

Output parameters:      sync_timetag    The 32-bits of the timetag
reading when the SYNC message
                                         (MC-1 or MC-17) arrived.
                         sync_data       The data word of the SYNC message
(for sync with data, MC-17, only).

Return values:  ebadhandle      If invalid handle specified
                 emode           If board is not in RT mode
                einval           If parameter is out of range
                0                If successful

*/
int borland_dll Get_RT_Sync_Info_Px(int handlepx, int rtnum, unsigned int *sync_timetag, usint *sync_data)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);

	if (cardpx[handlepx].singleFunction)
	{
		if (rtnum != cardpx[handlepx].rtNumber)
			return ertvalue;

		*sync_timetag = cardpx[handlepx].rt_sync_timetag[0];
		*sync_data = cardpx[handlepx].rt_sync_data[0];
	}
	else
	{
		*sync_timetag = cardpx[handlepx].rt_sync_timetag[rtnum];
		*sync_data = cardpx[handlepx].rt_sync_data[rtnum];
	}

	return 0;
}

/* for backward compatibility only */
int borland_dll Get_RT_Sync_Entry_Px(int handlepx, int rtnum, usint *sync_timetag, usint *sync_data)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);

	if (cardpx[handlepx].singleFunction)
	{
		if (rtnum != cardpx[handlepx].rtNumber)
			return ertvalue;

		*sync_timetag = (usint)(cardpx[handlepx].rt_sync_timetag[0]);
		*sync_data = cardpx[handlepx].rt_sync_data[0];
	}
	else
	{
		*sync_timetag = (usint)(cardpx[handlepx].rt_sync_timetag[rtnum]);
		*sync_data = cardpx[handlepx].rt_sync_data[rtnum];
	}



	return 0;
}
/*
Clear_RT_Sync_Entry_Px

int borland_dll Clear_RT_Sync_Entry_Px(int handlepx, int rtnum);

Description:    Clear (reset to 0) the internally stored timetag and data
word (if any) relating to the
                 occurrence of a SYNC mode code message (MC-1 or MC-17) for
this RT.

Input parameters:       rtnum           The number of the RT for which we
want to clear the SYNC information.

Output parameters:      none

Return values:  ebadhandle      If invalid handle specified
                 emode           If board is not in RT mode
                einval           If parameter is out of range
                0                If successful

*/
int borland_dll Clear_RT_Sync_Entry_Px(int handlepx, int rtnum)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);
	if ((rtnum < 0) || (rtnum > 31))
		return (einval);

	if (cardpx[handlepx].singleFunction)
	{
		if (rtnum != cardpx[handlepx].rtNumber)
			return ertvalue;

		cardpx[handlepx].rt_sync_timetag[0] = 0;
		cardpx[handlepx].rt_sync_data[0] = 0;
	}
	else
	{
		cardpx[handlepx].rt_sync_timetag[rtnum] = 0;
		cardpx[handlepx].rt_sync_data[rtnum] = 0;
	}

	return 0;
}

int borland_dll RT_Links_Control_MMSI_Px(int handlepx, int enableflag, usint rtlinks)
{
	usint tempWord;
	
#ifndef MMSI
	return (func_invalid);
#else
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	if (rtlinks > RT_LINKS_ALL)
		return einval;

	if (enableflag == RT_LINKS_ACTIVATE)
	{
//		cardpx[handlepx].exc_bd_rt->options_select |= rtlinks;
		tempWord = RWORD_D2H( cardpx[handlepx].exc_bd_rt->options_select) | rtlinks;
		WWORD_H2D( cardpx[handlepx].exc_bd_rt->options_select, tempWord );
	}

	else if (enableflag == RT_LINKS_SHUTDOWN)
	{
//		cardpx[handlepx].exc_bd_rt->options_select &= ~rtlinks;
		tempWord = RWORD_D2H( cardpx[handlepx].exc_bd_rt->options_select) & ~rtlinks;
		WWORD_H2D( cardpx[handlepx].exc_bd_rt->options_select, tempWord );
	}
	else
		return einval;

	return 0;

#endif
}

/*
* Set_Checksum_Blocks - Coded 20 July 97 by A. Meth
*
* Description - This routine sets which block numbers (associated with
*               RT-subaddress) are to get a checksum.
*
* Input:        csum_blocks  the checksum block number upper limit;
*                            e.g., a value of 5 means that data block numbers
*                            0-4 will be associated with checksum, and block
*                            numbers 5-199/255/512 will not be associated with
*                            checksum
*
* Output:       0 for success  <0 for failure
*/

int borland_dll Set_Checksum_Blocks (int csum_blocks)
{
	return Set_Checksum_Blocks_Px(g_handlepx, csum_blocks);
}


int borland_dll Set_Checksum_Blocks_Px (int handlepx, int csum_blocks)
{
	int status;
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	status = Validate_Block_Number(handlepx, csum_blocks);
	if (status < 0)
		return einval;

//	return(cardpx[handlepx].exc_bd_rt->checksum_flag = (usint)csum_blocks);
	WWORD_H2D( cardpx[handlepx].exc_bd_rt->checksum_flag, (usint)csum_blocks );
	return(0); // for success
}


/*
* Get_Checksum_Blocks - Coded 20 July 97 by A. Meth
*
* Description - This routine returns the number of blocks (associated with
*               RT-subaddress) that are allowed to get a checksum.
*
* Output:       int <= 199/255/512 depending if expanded block mode used
*               0 if not set
*/

int borland_dll Get_Checksum_Blocks (void)
{
	return Get_Checksum_Blocks_Px(g_handlepx);
}

int borland_dll Get_Checksum_Blocks_Px (int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

//	return(cardpx[handlepx].exc_bd_rt->checksum_flag);
	return( RWORD_D2H(cardpx[handlepx].exc_bd_rt->checksum_flag) );
}

/*
Set_RTid_Multibuf_Px

Description:						sets up an rtid to use multiple buffers
Input parameters:   rtid     	11 bit identifier including RT#, T/R bit and
                             	subaddress.
                    numbufs	the number of buffers to use for this rtid - up to 16 is permitted
Output parameters:  none
Return values:  emode         If board is not in RT mode
                einval        If parameter is out of range
                edoublebuf	  If an attempt is made to set multibuffers on an rtid already using double buffering
				func_invalid  If the firmware doesn't support this feature
                0  		If successful.
*/

int borland_dll Set_RTid_Multibuf (int rtid, int numbufs)
{
	return Set_RTid_Multibuf_Px (g_handlepx, rtid, numbufs);
}

int borland_dll Set_RTid_Multibuf_Px (int handlepx, int rtid, int numbufs)
{
	usint curctrl, blknum;
#define MULTIBUFMASK 0x00FF

#ifdef MMSI
	return (func_invalid);
#else

	/* if the card is an old PCMCIA/EP (AMD), then this functionality does not exist  */
	if ( (cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1) ) {
		return(func_invalid);
	}

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return emode;

	if (cardpx[handlepx].singleFunction && (((rtid>>6) & cardpx[handlepx].rtNumber) != cardpx[handlepx].rtNumber))
		return ertvalue;

//	if ((cardpx[handlepx].exc_bd_bc->revision_level < 0x30) && (cardpx[handlepx].processor != PROC_NIOS))
	if ((RBYTE_D2H( cardpx[handlepx].exc_bd_bc->revision_level) < 0x30) && (cardpx[handlepx].processor != PROC_NIOS))
		return func_invalid;

	if ((rtid < 0) || (rtid > 2047))
		return einval;
	if (numbufs > 16)
		return einval;
#ifdef VME4000
	blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[rtid^1];
#else
//	blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[rtid];
	blknum = RBYTE_D2H( cardpx[handlepx].exc_bd_rt->blk_lookup[rtid] );
#endif

//	curctrl = cardpx[handlepx].rtid_ctrl[rtid];
	curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[rtid] );

	/* multibuffering cannot cross 256 or 512 block boundary */

	/* The targeted blocks start at blknum and go for additional (maxbufs - 1) blocks.
	So (blknum + (maxbufs-1)) cannot be greater than 255/511; thus (blknum + maxbufs)
	cannot be greater than 256/512.
	*/
	if ((blknum < 256) && ((blknum + numbufs) > 256))
		return eboundary;

	/* don't allow both double buffering and multibuffering, it confuses the firmware */
	if ((numbufs > 0) && (curctrl & DB_ENABLE_SET))
		return edoublebuf;

	curctrl &= MULTIBUFMASK;     /* clear the number of buffer field and the current buffer field */
	if (numbufs > 0)			 /* 0 and 1 both imply no multibuffering, both write 0 to the board */
		curctrl |= (usint)((numbufs - 1) << MULTIBUFSHIFT);

//	cardpx[handlepx].rtid_ctrl[rtid] = curctrl;
	WWORD_H2D( cardpx[handlepx].rtid_ctrl[rtid], curctrl );

	cardpx[handlepx].NextBufToAccess[rtid] = 0;
	
	if (numbufs > 1)
		cardpx[handlepx].multibufUsed = 1;

	return 0;
#endif
}

/*
Reset_RTid_Multibuf_Px

Description:						forces next read/write to use first buffer on both firmware and driver sides
Input parameters:   rtid     	11 bit identifier including RT#, T/R bit and
                             	subaddress.
Output parameters:  none
Return values:  emode         If board is not in RT mode
                einval        If parameter is out of range
                edoublebuf		If an attempt is made to set multibuffers on an rtid already using double buffering
                0  		If successful.
*/

int borland_dll Reset_RTid_Multibuf_Px (int handlepx, int rtid)
{
	usint curctrl;
#define NEXTBUFMASK 0x0FFF

#ifdef MMSI
	return (func_invalid);
#else

	/* if the card is an old PCMCIA/EP (AMD), then this functionality does not exist  */
	if ( (cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1) ) {
		return(func_invalid);
	}

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return emode;

	if ((rtid < 0) || (rtid > 2047))
		return einval;

	if (cardpx[handlepx].singleFunction && (((rtid>>6) & cardpx[handlepx].rtNumber) != cardpx[handlepx].rtNumber))
		return ertvalue;

//	curctrl = cardpx[handlepx].rtid_ctrl[rtid];
	curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[rtid] );

	curctrl &= NEXTBUFMASK;  /* clear the current buffer field */

//	cardpx[handlepx].rtid_ctrl[rtid] = curctrl;
	WWORD_H2D( cardpx[handlepx].rtid_ctrl[rtid], curctrl );

	cardpx[handlepx].NextBufToAccess[rtid] = 0;
	return 0;
#endif
}


/*
Get_Multibuf_Nextbuf_Px

Description:						returns the next buffer to be accessed by the board. When the board is in middle of
										accessing a buffer, it will return the number of the following buffer
Input parameters:   rtid     	11 bit identifier including RT#, T/R bit and
                             	subaddress.
Output parameters:  nextbuf	the next buffer to be accessed by the board
Return values:  emode         If board is not in RT mode
                einval        If parameter is out of range
                edoublebuf		If an attempt is made to set multibuffers on an rtid already using double buffering
                0  		If successful.
*/

int borland_dll Get_Multibuf_Nextbuffer_Px (int handlepx, int rtid, int *nextbuf)
{
	usint curctrl;
#define NEXTBUFSHIFT 12

#ifdef MMSI
	return (func_invalid);
#else

	/* if the card is an old PCMCIA/EP (AMD), then this functionality does not exist  */
	if ( (cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1) ) {
		return(func_invalid);
	}

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return emode;

	if ((rtid < 0) || (rtid > 2047))
		return einval;

	if (cardpx[handlepx].singleFunction && (((rtid>>6) & cardpx[handlepx].rtNumber) != cardpx[handlepx].rtNumber))
		return ertvalue;

//	curctrl = cardpx[handlepx].rtid_ctrl[rtid];
	curctrl = RWORD_D2H( cardpx[handlepx].rtid_ctrl[rtid] );

	*nextbuf = curctrl >> NEXTBUFSHIFT;
	return 0;
#endif
}

/*
Validate_Block_Number

Description:

Input Parameter:	blknum		[Checksum] Block Number for the RTid
Output Parameter:	none
Return Values:		einval		the block number input is out of valid range
					0			success
*/
int Validate_Block_Number(int handlepx, int blknum)
{
	/* verify that blknum is a legal value */
	if ((blknum < 0) || (blknum > 511))
		return (einval);

	/* if old firmware (pre-2.13), verify that blknum is a legal value, < 200 */
	if (!cardpx[handlepx].expandedRTblockFW)
	{
		if (blknum > 199)
			return einval;
	}

	/* if no Expanded Blocks, then verify that blknum is a legal value, < 256 */
//	if (!(cardpx[handlepx].exc_bd_rt->moduleFunction & EXPANDED_BLOCK_MODE))
//	if (!( RWORD_D2H(cardpx[handlepx].exc_bd_rt->moduleFunction) & EXPANDED_BLOCK_MODE))
	if (!( cardpx[handlepx].userRequestedRTexpanded ))
	{
		if (blknum > 255)
			return einval;
	}

	return 0;
}

/* Internal function to change the RT number in single function mode */
int Set_RT_In_Reg(int handlepx, int rtValue)
{
	usint rtRegister;
	BOOL parity;
	int counter;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (!cardpx[handlepx].singleFunction)
		return esinglefunconlyerror;

	if ((rtValue < 0) || (rtValue > 31))
		return (einval);

	/* if lock bit is 1 - return error */
//	if ((cardpx[handlepx].exc_bd_rt->rtNumSingle & RT_SINGLE_FUNCTION_LOCK_BIT) == RT_SINGLE_FUNCTION_LOCK_BIT)
	if (( RWORD_D2H(cardpx[handlepx].exc_bd_rt->rtNumSingle) & RT_SINGLE_FUNCTION_LOCK_BIT) == RT_SINGLE_FUNCTION_LOCK_BIT)
		return (ebitlocked);

	rtRegister = 0;

	/* insert value from param for Set_RT_Active function */
	parity = TRUE;
	for (counter = 0; counter < 5; counter++)
	{
		if ((rtValue & (1 << counter)) > 0)
			parity = !parity;
	}

	rtRegister |= (rtValue & RT_SINGLE_FUNCTION_RTNUM_BITS);
	if (parity)
		rtRegister |= (usint)RT_SINGLE_FUNCTION_PARITY_BIT;
	else
		rtRegister &= (usint)(~RT_SINGLE_FUNCTION_PARITY_BIT);

	/* shove the rt value and parity into the register */
//	cardpx[handlepx].exc_bd_rt->rtNumSingle = rtRegister;
	WWORD_H2D( cardpx[handlepx].exc_bd_rt->rtNumSingle, rtRegister );

	DelayMicroSec_Px(handlepx, 100000);

	/* check the error bit */
//	if ((cardpx[handlepx].exc_bd_rt->rtNumSingle & RT_SINGLE_FUNCTION_ERROR_BIT) == RT_SINGLE_FUNCTION_ERROR_BIT)
	if ((RWORD_D2H(cardpx[handlepx].exc_bd_rt->rtNumSingle) & RT_SINGLE_FUNCTION_ERROR_BIT) == RT_SINGLE_FUNCTION_ERROR_BIT)
		return (ebiterror);

	cardpx[handlepx].rtNumber = rtValue;

//	cardpx[handlepx].exc_bd_rt->status[0] = (usint)(rtValue << 11);
	WWORD_H2D( cardpx[handlepx].exc_bd_rt->status[0] , (rtValue << 11) );

	return 0;
}

/* Internal function to get the value from the register for cardpx[handlepx].rtNumber */
int Update_RT_From_Reg(int handlepx)
{
	usint rtRegister;
	usint origRTRegister;
	BOOL parity;
	int counter;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (!cardpx[handlepx].singleFunction)
		return esinglefunconlyerror;

//	rtRegister = cardpx[handlepx].exc_bd_rt->rtNumSingle;
//	origRTRegister = cardpx[handlepx].exc_bd_rt->rtNumSingle;
	rtRegister = origRTRegister = RWORD_D2H( cardpx[handlepx].exc_bd_rt->rtNumSingle );

	rtRegister &=  RT_SINGLE_FUNCTION_RTNUM_BITS;

	/* set the value of rtNumber */
	cardpx[handlepx].rtNumber = rtRegister;

//	cardpx[handlepx].exc_bd_rt->status[0] = (usint)(rtRegister << 11);
	WWORD_H2D( cardpx[handlepx].exc_bd_rt->status[0] ,(rtRegister << 11) );

	/* we must now update the parity bit into register as we can't be sure that the parity bit is correct.
	- A jumper will change the rt number but not the parity bit.  */

	/* calculate the parity bit */
	parity = TRUE;
	for (counter = 0; counter < 5; counter++)
	{
		if ((rtRegister & (1 << counter)) > 0)
			parity = !parity;
	}

	if (parity)
		origRTRegister |= (usint)RT_SINGLE_FUNCTION_PARITY_BIT;
	else
		origRTRegister &= (usint)(~RT_SINGLE_FUNCTION_PARITY_BIT);

	/* shove the updated parity into the register */
//	cardpx[handlepx].exc_bd_rt->rtNumSingle = origRTRegister;
	WWORD_H2D( cardpx[handlepx].exc_bd_rt->rtNumSingle,  origRTRegister );

	DelayMicroSec_Px(handlepx, 100000);

	/* we check the error bit */
//	if ((cardpx[handlepx].exc_bd_rt->rtNumSingle & RT_SINGLE_FUNCTION_ERROR_BIT) == RT_SINGLE_FUNCTION_ERROR_BIT)
	if (( RWORD_D2H(cardpx[handlepx].exc_bd_rt->rtNumSingle) & RT_SINGLE_FUNCTION_ERROR_BIT) == RT_SINGLE_FUNCTION_ERROR_BIT)
		return (ebiterror);

	return 0;
}
