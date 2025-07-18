#include "pxIncl.h"
extern INSTANCE_PX cardpx[];

#define MSGENTRY_SIZE 4

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)

/*
Create_Frame     Coded 07.10.90 by D. Koppel

  Description:  Creates message frames composed of previously defined messages.
  Up to 20 frames may be defined. The frame to be executed upon a call to
  Run_BC is identified with the Start_Frame command. An entry with id = 0
  indicates the end of the frame.
  
	Input parameters:  FRAME framestruct [];
	struct FRAME {
	int id;         Message identifier returned by
	Create_1553_Message or
	0 for last message
	int gaptime;    Gap Time between this message and
	following message in microseconds
	};
	
	  Output parameters: frameid     Frame identifier of frame just created (<20),
	  for use in calling many of the routines in
	  BC mode.
	  Return values:   frm_badid     If tried to place an undefined message into a
	  message frame
	  frm_nostacksp   If there is not enough space in frame stack for this frame
	  frm_maxframe  If exceeded maximum number of frames permitted (20)
	  frameid       If successful
	  
*/

int borland_dll Create_Frame(struct FRAME * framestruct)
{
	return Create_Frame_Px(g_handlepx, framestruct);
}

int borland_dll Create_Frame_Px(int handlepx, struct FRAME * framestruct)
{
	struct FRAME *curframe;
	struct instruction_stack *pistack;

	long temp, gapTime;
	usint tempWord;
	usint multiplyer;
	unsigned char mode;
	int isIMG_250ns;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;


//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	mode = cardpx[handlepx].board_config;
	if ((mode != BC_MODE) && (mode != BC_RT_MODE))
		return (emode);

	pistack = (struct instruction_stack *) cardpx[handlepx].nextstack;

	// for 250 nano resolution (Nios only, as per user selection)
//		isIMG_250ns = (cardpx[handlepx].exc_bd_bc->bcProtocolOptions & BC_PROT_IMG_250N_RES) &&
//			(cardpx[handlepx].processor == PROC_NIOS)
	isIMG_250ns = (RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions ) & BC_PROT_IMG_250N_RES) &&
			(cardpx[handlepx].processor == PROC_NIOS);

	for (curframe = framestruct; curframe->id > 0; curframe++)
	{
		if (curframe->id >= cardpx[handlepx].nextid)
			return (frm_badid);
		else if ((pistack + 1) > (struct instruction_stack *)
			((char *)(cardpx[handlepx].exc_bd_bc->stack) + sizeof(cardpx[handlepx].exc_bd_bc->stack))) /* out of stack */
			return (frm_nostacksp);
		else if (cardpx[handlepx].nextframe >= MAXFRAMES)
			return (frm_maxframe);

		/* set up instruction frame in dual port ram */
//		pistack->message_block_pointer = (usint)((char*)cardpx[handlepx].msgalloc[curframe->id]-(char*)cardpx[handlepx].exc_bd_bc);
		WWORD_H2D( pistack->message_block_pointer , (usint)((char*)cardpx[handlepx].msgalloc[curframe->id]-(char*)cardpx[handlepx].exc_bd_bc) );

#ifdef MMSI
//		pistack->gap_time = (usint)curframe->gaptime;
		WWORD_H2D( pistack->gap_time, (usint)curframe->gaptime );

		// This is for all MMSI (AMD?, INTEL & NIOS); so we won't use 'isIMG_250ns' defined above.
//		if (cardpx[handlepx].exc_bd_bc->bcProtocolOptions & BC_PROT_IMG_250N_RES)
		tempWord = RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions );
		if (tempWord & BC_PROT_IMG_250N_RES)
		{
			temp = curframe->gaptime * 4;

//			pistack->gap_time = (usint)temp;
			WWORD_H2D( pistack->gap_time, (usint)temp);

//			pistack->img_mult = temp >> 16;
			WWORD_H2D( pistack->img_mult, (temp >> 16) );
		}
#else
		// for 250 nano resolution (Nios only, as per user selection)
		if (isIMG_250ns)
		{
			/* the input unit is nano Second */
			gapTime = curframe->gaptime;

//			/* calibration for the minimum IMG  */
			if (gapTime >= 30000) 
				gapTime -= 4000;

			gapTime = gapTime * 4;  // the user writes microsec, and internally we are running at resolution of 250 nanosec

//			pistack->gap_time = (usint)gapTime;
			WWORD_H2D( pistack->gap_time , (usint)gapTime );
//			pistack->img_mult = (usint)(gapTime >> 16);
			WWORD_H2D( pistack->img_mult, (usint)(gapTime >> 16) );
		}

		/* Intel only, for f/w rev >= 3.5, we force using resolution of 250 nanosec, and write it to
		bcProtocolOptions register for use by firmware */
		else if ((cardpx[handlepx].isIMG250ns_resolution == TRUE) &&
			(cardpx[handlepx].processor == PROC_INTEL))
		{
			// this code only applies to Intel and won't be in a UNet
			cardpx[handlepx].exc_bd_bc->bcProtocolOptions = cardpx[handlepx].exc_bd_bc->bcProtocolOptions | BC_PROT_IMG_250N_RES;

			/* the input unit is nano Second */
			gapTime = curframe->gaptime;
			gapTime *= 4;

			pistack->gap_time = (usint)gapTime;
			pistack->img_mult = (usint)(gapTime >> 16);

		}

		else /* For 155 nano resolution */
		{

			/* the input unit is micro Second */
			multiplyer = (usint)(curframe->gaptime / 10158);
			temp = ((curframe->gaptime / (multiplyer+ 1)) * 1000) / 155;

//			pistack->gap_time = (usint)temp;
			WWORD_H2D( pistack->gap_time, (usint)temp );
//			pistack->img_mult = multiplyer;
			WWORD_H2D( pistack->img_mult, multiplyer );

		}
#endif

//		pistack->message_status_word = 0;        /* set return status to 0 */
		WWORD_H2D( pistack->message_status_word, 0 );        /* set return status to 0 */

		pistack++;

	} /* end of the for loop */


	/*
	frameptr is the offset into the board of the instruction block
	and as such is only 15 bits long. The 16th bit may be used for
	boards in segment X8000
	*/
	cardpx[handlepx].framealloc[cardpx[handlepx].nextframe].frameptr =
		(usint)((char *)cardpx[handlepx].nextstack - (char *)cardpx[handlepx].exc_bd_bc);
	cardpx[handlepx].framealloc[cardpx[handlepx].nextframe].msgcnt = (usint)(curframe - framestruct);
	cardpx[handlepx].nextstack = (unsigned short *)pistack;

	return (cardpx[handlepx].nextframe++);
}   /* end of function Create_Frame */


/*
	Start_Frame     Coded 07.10.90 by D. Koppel
	
	  Description: With multiple calls to Create_Frame you can create multiple
	  frames. Start_Frame selects which frame to run when Run_BC is called.
	  
		Input parameters:  frameid  Frame identifier returned from a prior call to
		Create_Frame
		intcnt   Number of messages within the frame to execute;
		FULLFRAME to execute the entire frame.
		
		  Output parameters: none
		  
			Return values  frm_erange      If called to Start_Frame with an undefined
			frame
			frm_erangecnt   If called to Start_Frame with count greater
			than the number of messages in the frame
			0               If successful
			
			  Note: This routine must be called prior to each call on Run_BC. If the card
			  is stopped, either by calling Stop or because all requested messages have been
			  transmitted, the board may be restarted by calling Start_Frame and then
			  Run_BC.
			  
*/

int borland_dll Start_Frame(int frameid, int nummsgs)
{
	return Start_Frame_Px(g_handlepx, frameid, nummsgs);
}

int borland_dll Start_Frame_Px(int handlepx, int frameid, int nummsgs)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		( cardpx[handlepx].board_config != BC_RT_MODE) )
		return (emode);

	/* range check frame id */
	if ((frameid < 0) || (frameid >= cardpx[handlepx].nextframe))
		return (frm_erange);
	/* range check instruction count */
	else if (nummsgs > cardpx[handlepx].framealloc[frameid].msgcnt)
		return (frm_erangecnt);

	/* set up frame pointer and instruction count */
//	cardpx[handlepx].exc_bd_bc->pointer = cardpx[handlepx].framealloc[frameid].frameptr;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->pointer, cardpx[handlepx].framealloc[frameid].frameptr );

	/*
	* We want curstack to get the same segment as exc_bd_bc
	* but the offset of exc_bd_bc->pointer
	*/

//	cardpx[handlepx].curstack = (struct instruction_stack *) ((char *)(cardpx[handlepx].exc_bd_bc)
//		+ (int) cardpx[handlepx].exc_bd_bc->pointer);
	cardpx[handlepx].curstack = (struct instruction_stack *) ((char *)(cardpx[handlepx].exc_bd_bc)
		+ (int) RWORD_D2H( cardpx[handlepx].exc_bd_bc->pointer ) );

#ifdef MMSI
	if ((nummsgs == FULLFRAME)|| (cardpx[handlepx].framealloc[frameid].msgcnt < nummsgs))
//		cardpx[handlepx].exc_bd_bc->instruction_counter = cardpx[handlepx].framealloc[frameid].msgcnt;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->instruction_counter, cardpx[handlepx].framealloc[frameid].msgcnt );


	else
//		cardpx[handlepx].exc_bd_bc->instruction_counter = nummsgs;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->instruction_counter, nummsgs );
#else
	if ((nummsgs == FULLFRAME)|| (cardpx[handlepx].framealloc[frameid].msgcnt < nummsgs))
	{
//		cardpx[handlepx].exc_bd_bc->instruction_counter =
//			( (cardpx[handlepx].framealloc[frameid].msgcnt & 0xff00) >> 8)
//			| ( (cardpx[handlepx].framealloc[frameid].msgcnt & 0x00ff) << 8);
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->instruction_counter ,
			( (cardpx[handlepx].framealloc[frameid].msgcnt & 0xff00) >> 8)
			| ( (cardpx[handlepx].framealloc[frameid].msgcnt & 0x00ff) << 8));
	}
	else
//		cardpx[handlepx].exc_bd_bc->instruction_counter =
//		( (nummsgs & 0xff00) >> 8) | ( (nummsgs & 0x00ff) << 8) ;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->instruction_counter,
		( (nummsgs & 0xff00) >> 8) | ( (nummsgs & 0x00ff) << 8) );

#endif

	return (0);
}   /* end of function Start_Frame */


/*
	Select_Async_Frame        - Coded 01 Mar 98 by A. Meth
	
	  Description: With multiple calls to Create_Frame you can create multiple
	  frames. Select_Async_Frame selects a frame to be set up for asynchronous
	  transmission when the current message is finished being sent, by calling
	  Send_Async_Frame.
	  
		Input parameters:  frameid  Frame identifier returned from a prior call to
		Create_Frame
		nummsgs  Number of messages within the frame to execute;
		0 (or FULLFRAME) to execute the entire frame.
		
		  Output parameters: none
		  
			Return values:
			frm_erange      If called Select_Async_Frame with an undefined frame
			frm_erangecnt   If called to Select_Async_Frame with count greater than
			the number of messages in the frame
			0               If successful
			
			  Notes:
			  1. This routine is valid in (a) BC and (b) BC/Concurrent RT modes only.
			  2. This routine must be called prior to a call to Send_Async_Frame.
*/

int borland_dll Select_Async_Frame (int frameid, int nummsgs)
{
	return Select_Async_Frame_Px (g_handlepx, frameid, nummsgs);
}

int borland_dll Select_Async_Frame_Px (int handlepx, int frameid, int nummsgs)
{
	/* modeled on Start_Frame */
#ifdef MMSI
	return func_invalid;
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* range check frame id */
	if ((frameid < 0) || (frameid >= cardpx[handlepx].nextframe))
		return (frm_erange);

	/* range check instruction count */
	else if (nummsgs > cardpx[handlepx].framealloc[frameid].msgcnt)
		return (frm_erangecnt);

	/* set up frame pointer and instruction count */
//	cardpx[handlepx].exc_bd_bc->async_frame_ptr = cardpx[handlepx].framealloc[frameid].frameptr;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->async_frame_ptr, cardpx[handlepx].framealloc[frameid].frameptr );

	if ( (nummsgs == FULLFRAME) ||
		(cardpx[handlepx].framealloc[frameid].msgcnt < nummsgs) )
	{
//		cardpx[handlepx].exc_bd_bc->async_num_msgs
//			= cardpx[handlepx].framealloc[frameid].msgcnt;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->async_num_msgs,
			cardpx[handlepx].framealloc[frameid].msgcnt );
	}
	else
//		cardpx[handlepx].exc_bd_bc->async_num_msgs      = (usint)nummsgs;
		WWORD_H2D( cardpx[handlepx].exc_bd_bc->async_num_msgs, (usint) nummsgs );

	return (0);
}


/*
Send_Async_Frame        - Coded 09.08.96 by D. Koppel

  Description - This routine will cause the asynchronous frame set up by
  Select_Async_Frame to be sent when the current message
  is finished being sent.
  
	Input: none
	
	  Output: 0 for success, < 0 for failure
	  
		Note: This routine is valid in (a) BC and (b) BC/Concurrent RT modes only.
*/

int borland_dll Send_Async_Frame (void)
{
	return Send_Async_Frame_Px(g_handlepx);
}

int borland_dll Send_Async_Frame_Px (int handlepx)
{
#ifdef MMSI
	return func_invalid;
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_bc->async_num_msgs < 1)
	if (RWORD_D2H( cardpx[handlepx].exc_bd_bc->async_num_msgs ) < 1)
		return enoasync;  /* not enough messages in async frame */

//	cardpx[handlepx].exc_bd_bc->async_start_flag = 1;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->async_start_flag, 1 );

	return 0;

}


/*
Alter_IMG     Coded 22.02.96 by D. Koppel

  Description: Changes the intermessage gap time of a previously defined
  message.
  
	Input parameters:  frameid      Frame identifier returned from a prior call
	to Create_Frame
	msgentry     Entry within the frame, i.e., 0 for the
	first message in the frame, 1 for the second
	message etc.
	img          New intermessage gap time in microseconds

	  Output parameters: none
	  
		Return values:    frm_erange     If frameid is not a valid frame id
		frm_erangecnt  If msgentry is out of range
		0              If successful
		
*/

#ifdef MMSI
#define MAX_IMG 65535 
#else
#define MAX_IMG 83204178 
#define MSG_FUNC_BITS 0xE000
#endif
int borland_dll Alter_IMG(int frameid, int msgentry, unsigned long img)
{
	return Alter_IMG_Px(g_handlepx, frameid, msgentry, img);
}

int borland_dll Alter_IMG_Px(int handlepx, int frameid, int msgentry, unsigned long img)
{
	usint bits;
	struct instruction_stack *pistack;
	unsigned long temp,gapTime;
	int multiplyer;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE) )
		return (emode);

	/* range check frame id */
	if ((frameid < 0) || (frameid >= cardpx[handlepx].nextframe))
		return (frm_erange);

	/* range check msgentry */
	if (msgentry >= cardpx[handlepx].framealloc[frameid].msgcnt)
		return (frm_erangecnt);

	if (img > MAX_IMG) 
		return einval;

	/* get pointer to beginning of frame */
	pistack = (struct instruction_stack *)(cardpx[handlepx].exc_bd_bc) +
		((cardpx[handlepx].framealloc[frameid].frameptr) / sizeof(usint));

	/* point to message within frame */
	pistack += (msgentry);

	/* point to IMG field within message */
#ifdef MMSI
//	pistack->gap_time = (usint)img;
	WWORD_H2D( pistack->gap_time, (usint)img);
#else

//	bits = pistack->img_mult & MSG_FUNC_BITS; /* save values of bits 13, 14, 15 */ 
	bits = RWORD_D2H(pistack->img_mult) & MSG_FUNC_BITS; /* save values of bits 13, 14, 15 */ 

	/* for 250 nano resolution (Nios only - as per user selection) */
//	if ((cardpx[handlepx].exc_bd_bc->bcProtocolOptions & BC_PROT_IMG_250N_RES) &&
//		(cardpx[handlepx].processor == PROC_NIOS))
	if ((RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions ) & BC_PROT_IMG_250N_RES) &&
		(cardpx[handlepx].processor == PROC_NIOS))
	{
		/* the input unit is nano Second */
		gapTime = img;

		/* calibration for the minimum IMG  */
		if (gapTime >= 30000) 
			gapTime -= 4000;

		gapTime = gapTime * 4;  // the user writes microsec, and internally we are running at resolution of 250 nanosec

//		pistack->gap_time = (usint)gapTime;
		WWORD_H2D( pistack->gap_time , (usint)gapTime );

//		pistack->img_mult = (((usint)(gapTime >> 16)) | bits);
		WWORD_H2D( pistack->img_mult, ((usint)(gapTime >> 16)) | bits );
	}

	/* Intel only, for f/w rev >= 3.5, we force using resolution of 250 nanosec, and write it to
	bcProtocolOptions register for use by firmware */

	// 22jul20: since this code is for Intel only, it does not need to be UNET-able

	else if ((cardpx[handlepx].isIMG250ns_resolution == TRUE) &&
		(cardpx[handlepx].processor == PROC_INTEL))
	{
		cardpx[handlepx].exc_bd_bc->bcProtocolOptions = cardpx[handlepx].exc_bd_bc->bcProtocolOptions | BC_PROT_IMG_250N_RES;

		/* the input unit is nano Second */
		gapTime = img;
		gapTime *= 4;

		pistack->gap_time = (usint)gapTime;
		pistack->img_mult = (usint)(gapTime >> 16);

	}

	else /* For 155 nano resolution */
	{

		/* the input unit is micro Second */
		multiplyer = (int)(img / 10158);
		temp = ((img / (multiplyer+ 1)) * 1000) / 155;

//		pistack->gap_time = (usint)temp;
		WWORD_H2D( pistack->gap_time, (usint)temp );
//		pistack->img_mult = (usint)multiplyer | bits;
		WWORD_H2D( pistack->img_mult, ((usint)multiplyer | bits) );
	}

#endif

	return 0;
}   /* end of function Alter_IMG */


int borland_dll Alter_MsgSendTime(int frameid, int msgentry, unsigned int mst)
{
	return Alter_MsgSendTime_Px(g_handlepx, frameid, msgentry, mst);
}

#define INT_MASK  0xE000
int borland_dll Alter_MsgSendTime_Px(int handlepx, int frameid, int msgentry, unsigned int mst)
{
	struct instruction_stack *pistack;
	usint intval, hi_mstval;

#ifdef MMSI
	return func_invalid;
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_MODE))
	if ((cardpx[handlepx].board_config != BC_RT_MODE) &&
		(cardpx[handlepx].board_config != BC_MODE) )
		return (emode);

	/* range check frame id */
	if ((frameid < 0) || (frameid >= cardpx[handlepx].nextframe))
		return (frm_erange);

	/* range check msgentry */
	if (msgentry >= cardpx[handlepx].framealloc[frameid].msgcnt)
		return (frm_erangecnt);


	/* get pointer to beginning of frame */
	pistack = (struct instruction_stack *)(cardpx[handlepx].exc_bd_bc) + ((cardpx[handlepx].framealloc[frameid].frameptr) / sizeof(usint));
	/* point to message within frame */
	pistack += (msgentry);

	/* point to IMG field within message */
//	pistack->gap_time = (usint)mst;
	WWORD_H2D( pistack->gap_time, (usint)mst );

	/* save bits 13, 14 and 15 */
//	intval = (usint)(pistack->img_mult & INT_MASK);
	intval = (usint) (RWORD_D2H( pistack->img_mult ) & INT_MASK);

	hi_mstval = (usint)((mst >>16) & (~INT_MASK));

//	pistack->img_mult = intval | hi_mstval;
	WWORD_H2D( pistack->img_mult, (intval | hi_mstval) );

	return 0;
}   /* end of function Alter_MsgSendTime */


/*
	* Set_Interrupt_On_Msg     Coded 06 July 98 by D. Koppel & A. Meth
	*
	* Description - This routine causes an interrupt to be generated when this
	*               message is completed. For Px modules only.
	*
	*
	* Input:        frameid    a frame id returned from a call to Create_Frame
	*               msgentry   the message number within the frame
	*               enable     1 for enable, 0 for disable
	*
	* Output:       0 for success  <0 for failure
*/

int borland_dll Set_Interrupt_On_Msg (int frameid, int msgentry, int enable)
{
	return Set_Interrupt_On_Msg_Px(g_handlepx, frameid, msgentry, enable);
}

#define INTERRUPT_ON_MSG_BIT 0x2000
int borland_dll Set_Interrupt_On_Msg_Px (int handlepx, int frameid, int msgentry, int enable)
{
	struct instruction_stack *pistack;
	usint tempWord;

#ifdef MMSI
	return func_invalid;
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE) 
		return (emode);

	/* range check frame id */
	if ((frameid < 0) || (frameid >= cardpx[handlepx].nextframe))
		return (frm_erange);

	/* range check msgentry */
	if (msgentry >= cardpx[handlepx].framealloc[frameid].msgcnt)
		return (frm_erangecnt);

	/* get pointer to beginning of frame */
	pistack = (struct instruction_stack *)(cardpx[handlepx].exc_bd_bc) + ((cardpx[handlepx].framealloc[frameid].frameptr) / sizeof(usint));
	/* point to message within frame */
	pistack += (msgentry);

//	tempWord = pistack->img_mult;
	tempWord = RWORD_D2H( pistack->img_mult );

	if (enable)
		tempWord |= INTERRUPT_ON_MSG_BIT; 
	else
		tempWord &= ~INTERRUPT_ON_MSG_BIT;

//	pistack->img_mult = tempWord;
	WWORD_H2D( pistack->img_mult, tempWord );

	return 0;
}


/*
	* Set_Interrupt_On_Msg_MMSI_Px     Coded 11 Mar 19 by D. Koppel & A. Meth
	*
	* Description - This routine causes an interrupt to be generated when this
	*               message is completed. For MMSI modules only.
	*
	*
	* Input:        id		a message id returned from a call to Create_1553_Message_Px
	*               enable	1 for enable, 0 for disable
	*
	* Output:       0 for success  <0 for failure
*/

#define CTL_INT_ON_MSG 0x0200
int borland_dll Set_Interrupt_On_Msg_MMSI_Px (int handlepx, int id, int enable)
{
	usint * msgptr;
	usint tempWord;

#ifndef MMSI
	return func_invalid;
#endif

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
//	tempWord = *msgptr;
	tempWord = RWORD_D2H( msgptr[0] );

	if (enable)
		tempWord |= CTL_INT_ON_MSG; 
	else
		tempWord &= ~CTL_INT_ON_MSG;

	// *msgptr = tempWord;
		WWORD_H2D( msgptr[0], tempWord );

	return(0);

}

/*
* Enable_Checksum     Coded 05.01.97 by D. Koppel
*
* Description - This routine selects whether the last word of a message
*               should be a checksum word or a regular data word
*
*
* Input:        frameid    a frame id returned from a call to Create_Frame
*               msgentry   the message number within the frame
*               enable     1 for checksumming, 0 for regular data
*
* Output:       0 for success  <0 for failure
*/

int borland_dll Enable_Checksum(int frameid, int msgentry, int enable)
{
	return Enable_Checksum_Px(g_handlepx, frameid, msgentry, enable);
}

#define ENABLE_CHECKSUM_BIT 0x8000
int borland_dll Enable_Checksum_Px(int handlepx, int frameid, int msgentry, int enable)
{
	struct instruction_stack *pistack;
	usint tempWord;

#ifdef MMSI
	return func_invalid;
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return (emode);

	/* range check frame id */
	if ((frameid < 0) || (frameid >= cardpx[handlepx].nextframe))
		return (frm_erange);

	/* range check msgentry */
	if (msgentry >= cardpx[handlepx].framealloc[frameid].msgcnt)
		return (frm_erangecnt);

	/* get pointer to beginning of frame */
	pistack = (struct instruction_stack *)(cardpx[handlepx].exc_bd_bc) + ((cardpx[handlepx].framealloc[frameid].frameptr) / sizeof(usint));
	/* point to message within frame */
	pistack += (msgentry);

//	tempWord = pistack->img_mult;
	tempWord = RWORD_D2H( pistack->img_mult );

	if (enable)
		tempWord |= ENABLE_CHECKSUM_BIT;
	else
		tempWord &= ~ENABLE_CHECKSUM_BIT;

//	pistack->img_mult = tempWord;
	WWORD_H2D( pistack->img_mult, tempWord );

	return 0;
}


/*
* Enable_Checksum_Error - Coded 18 Aug 97 by D. Zharnest & A. Meth
*
* Description - This routine sets whether we will do checksum error injection
*               NOTE: This only works for BC2RT and RT2RT messages (the RCV
*               part thereof).
*
*
* Input:        frameid    a frame id returned from a call to Create_Frame
*               msgentry   the message number within the frame
*               enable     1 for checksum error injection
*                          0 for normal checksum
*
* Output:       0 for success  <0 for failure
*/

int borland_dll Enable_Checksum_Error(int frameid, int msgentry, int enable)
{
	return Enable_Checksum_Error_Px(g_handlepx, frameid, msgentry, enable);
}
#define ENABLE_CHECKSUM_ERROR_BIT 0x4000
int borland_dll Enable_Checksum_Error_Px(int handlepx, int frameid, int msgentry, int enable)
{
	struct instruction_stack *pistack;
	usint tempWord;

#ifdef MMSI
	return func_invalid;
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE) 
		return (emode);

	if (cardpx[handlepx].singleFunction)
		return (enotforsinglefuncerror);

	/* range check frame id */
	if ((frameid < 0) || (frameid >= cardpx[handlepx].nextframe))
		return (frm_erange);

	/* range check msgentry */
	if (msgentry >= cardpx[handlepx].framealloc[frameid].msgcnt)
		return (frm_erangecnt);

	/* get pointer to beginning of frame */
	pistack = (struct instruction_stack *)(cardpx[handlepx].exc_bd_bc) + ((cardpx[handlepx].framealloc[frameid].frameptr) / sizeof(usint));
	/* point to message within frame */
	pistack += (msgentry);

//	tempWord = pistack->img_mult;
	tempWord = RWORD_D2H( pistack->img_mult );

	if (enable)
		tempWord |= ENABLE_CHECKSUM_ERROR_BIT;
	else
		tempWord &= ~ENABLE_CHECKSUM_ERROR_BIT;

//	pistack->img_mult = tempWord;
	WWORD_H2D( pistack->img_mult, tempWord );

	return 0;
}


#define MMSI_ENABLE_CHECKSUM_BIT 0x0040
int borland_dll Enable_Checksum_MMSI_Px(int handlepx, int id, int enable)
{
	usint tempWord;

#ifndef MMSI
	return func_invalid;
#else

	usint *msgptr;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);

	msgptr = cardpx[handlepx].msgalloc[id];

	/* turn on bit in control word */
//	tempWord = *msgptr;
	tempWord = RWORD_D2H( msgptr[0] );
	if (enable)
		tempWord |= MMSI_ENABLE_CHECKSUM_BIT;
	else
		tempWord &= ~MMSI_ENABLE_CHECKSUM_BIT;

//	*msgptr = tempWord;
	WWORD_H2D( msgptr[0], tempWord );
	return(0);
#endif
}

#define MMSI_ENABLE_CHECKSUM_ERROR_BIT 0x0080
int borland_dll Enable_Checksum_Error_MMSI_Px(int handlepx, int id, int enable)
{
	usint *msgptr;
	usint tempWord;

#ifndef MMSI
	return func_invalid;
#endif

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;


//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);

	msgptr = cardpx[handlepx].msgalloc[id];

	/* turn on bit in control word */
//	tempWord = *msgptr;
	tempWord = RWORD_D2H( msgptr[0] );
	if (enable)
		tempWord |= MMSI_ENABLE_CHECKSUM_ERROR_BIT;
	else
		tempWord &= ~MMSI_ENABLE_CHECKSUM_ERROR_BIT;

//	*msgptr = tempWord;
	WWORD_H2D( msgptr[0], tempWord );
	return(0);
}

