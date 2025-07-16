#include "pxIncl.h"
extern INSTANCE_PX cardpx[];

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)


/*
BC_Check_Alter_Msgentry

Description: Check the msg_status word to see if this message is available to be altered.

Before a call to Alter_Message, the user must check that this
message is available to be altered - i.e. it is not in the process of being
transmitted. (If we do NOT check for this, then we may alter a message in the middle of its
transmission - ie, a lack of data integrity.)

Input parameters:  frameid	  Frame identifier returned from a prior call to Create_Frame
            		msgentry 	Index of message in the specified frame

Output parameters: none

Return values:
		success:
			ALTER_MSG				If msg is available now to be altered
			DO_NOT_ALTER_MSG	If msg is NOT available to be altered
      failure:
			emode	 If not bc or bcrt mode
			frm_erange frame id specified was not created using create_frame
			frm_erangecnt msg entry specified is greater than num msgs in frame
*/


int borland_dll BC_Check_Alter_Msgentry (int frameid, int msgentry)
{
	return BC_Check_Alter_Msgentry_Px(g_handlepx, frameid, msgentry);
}

int borland_dll BC_Check_Alter_Msgentry_Px (int handlepx, int frameid, int msgentry)
{
	int status;
	usint statusword;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	status = Get_Msgentry_Status_Px(handlepx, frameid, msgentry, &statusword);
	if (status <0)
		return status;  /* includes checking frameid and msgentry */

	if (statusword == NO_ALTER)  /* check for NO_ALTER status */
		return (DO_NOT_ALTER_MSG);
	else
		return (ALTER_MSG);

}  /* end function BC_Check_Alter_Msg_Entry */


/*
BC_Check_Alter_Msg - Coded 29 Dec 97 by A. Meth

Description: Before a call to Alter_Message, the user must check that this
message is not in the process of being transmitted. If we do NOT check for
this, then we may alter a message in the middle of its transmission - ie, a
lack of data integrity. Check the msg_status word to see if this message is
not in the NO_ALTER state.

Input parameters:  id	   Message identifier returned from a prior call to
			   Create_Message

Output parameters: none

Return values:	  enoalter	If msg not available now to be altered
		  ebadid	If message id is not a valid message id
		  frm_nostack	if Start_Frame was not yet called to set up the message stack
		  0	   If successful

*/


int borland_dll BC_Check_Alter_Msg (int id)
{
	return BC_Check_Alter_Msg_Px(g_handlepx, id);
}

int borland_dll BC_Check_Alter_Msg_Px (int handlepx, int id)
{
	int status = Get_Msg_Status_Px(handlepx, id);

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (status == NO_ALTER)  /* check for NO_ALTER status */
		return (enoalter);
	else if (status < 0)     /* bad id */
		return (status);
	else
		return (0);

}  /* end function BC_Check_Alter_Msg */

/*
Alter_Message	  Coded 03.10.90 by D. Koppel
		  revised 14/10/93 not to overwrite command word
		  skip status word in Alter_Data    ecp e2 Mar 16 95

Description: Changes the data of a previously defined message. It receives
as input a pointer to an array of data words and copies these words into
the original message. The command word cannot be changed and therefore the
number of data words cannot be changed. Only the values of the data words
(and status word(s) for BC/RT when we simulate the RT as well) can be
changed.

Input parameters:  id	   Message identifier returned from a prior call to
			   Create_Message
		   words   Pointer to an array of up to 34 words of new data
			   (and status for BC/RT mode) for message id

Output parameters: none

Return values:	   emode   If not in BC mode
		   ebadid  If message id is not a valid message id
   		   eminorframe If message is minor frame message
		   0	   If successful

*/

int borland_dll Alter_Message (int id, usint *data)
{
	return Alter_Message_Px(g_handlepx, id, data);
}

int borland_dll Alter_Message_Px (int handlepx, int id, usint *data)
{
	usint * msgptr;
	int wdcnt, msgtype;
	unsigned short lclData[2];
//	int i; // loop replaced with b2drmWriteWordBufferToDpr

	/* verify value handle is valid */
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;

	/* verify handle is allocated/initialized */
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* verify that we're in the proper mode to run the operation */
//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE) )
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	/* verify input value(s) is valid */
	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);

	msgptr = cardpx[handlepx].msgalloc[id];

//	msgtype = *msgptr++ & 7; /* get message type and advance to command word */
//	msgtype = RWORD_D2H( msgptr[0] ) & 7; /* get message type and advance to command word */
	//read both control and command word in a single read, more efficient for UNET
	b2drmReadWordBufferFromDpr( (DWORD_PTR) msgptr,
								(unsigned short *) lclData,
								2,
								"Alter_Message_Px",
								B2DRMHANDLE );
	msgtype = lclData[0] & 7;
	msgptr++;

	if (msgtype == SKIP)
		msgtype = cardpx[handlepx].commandtype[id];

	if (msgtype == JUMP)
		return ejump;

//	wdcnt = *msgptr++ & 0x1f;	/* get word count of message and advance to
//								data word   ++ added 14/10/93 */
//	wdcnt = RWORD_D2H( msgptr[0] ) & 0x1f;
	wdcnt = lclData[1] & 0x1f;

	msgptr++;

	if (msgtype == MINOR_FRAME)
		return eminorframe;

	if (wdcnt == 0)
		wdcnt = 32;
	if (msgtype != BRD_RCV) /* Dec 7, 2000: added this exception -- no status word here */
		wdcnt++;	/* May 30, 96 adjust wordcount to include status word */
	if ((msgtype == RT2RT) || (msgtype == BRD_RT2RT)) /* if RT to RT command*/
	{
		msgptr++; /* skip second command word to get to data word */
		if (msgtype == RT2RT)
			wdcnt++;  /* May 30, 96 adjust wordcount to include 2nd status word */
	}
	/* if message is a mode code, ignore word count - transfer 1 word */
	if ((msgtype == MODE) || (msgtype == BRD_MODE))
	{
		/*Changed from wdcnt = 1;  for MODE messages with 1 data word.
		In Create_1553_Messages, there is enough space allocated for this data word,
		so here we don't have to worry that we may be overwriting the next message*/
		wdcnt = 2;
	}

//	for (i=0; i < wdcnt; i++)
//		*msgptr++ = data[i];  /* move new data into message */
	b2drmWriteWordBufferToDpr ( (DWORD_PTR) msgptr,
									 (unsigned short *) data,
									 wdcnt,
									 "Alter_Message_Px",
									 B2DRMHANDLE );

	return(0);
}  /* end function Alter_Message */


/*
Alter_Cmd	  Coded 02.17.98 by D. Koppel

Description: Changes the command word of a previously defined message.
It receives as input an RT address and subaddress copies these into the
original command word. The number of data words cannot be changed.

Input parameters:  id	   Message identifier returned from a prior call to
			   Create_Message
		   RT	   The new RT number or SAME_RT to retain the old value
		   SA	   The new subaddress or SAME_SA to retain the old value

Output parameters: none

Return values:	   emode   If not in BC mode
		   ebadid  If message id is not a valid message id
		   0	   If successful

*/

int borland_dll Alter_Cmd (int id, usint rt, usint sa)
{
	return Alter_Cmd_Px(g_handlepx, id, rt, sa);
}


int borland_dll Alter_Cmd_Px (int handlepx, int id, usint rt, usint sa)
{
	usint * msgptr;
	int command;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);
	if (rt > SAME_RT)
		return einval;
	if (sa > SAME_SA)
		return einval;
	msgptr = cardpx[handlepx].msgalloc[id];
	msgptr++;		      /* advance to command word */

//	command = *msgptr;       /* get command word */
	command = RWORD_D2H( msgptr[0] );       /* get command word */

	if (rt != SAME_RT)
		command = (command & 0x07ff) | (rt << 11);
	if (sa != SAME_SA)
		command = (command & 0xfc1f) | (int)(sa << 5);

//	*msgptr = (usint)command;       /* write new command word to the board */
	WWORD_H2D( msgptr[0], (usint)command );   /* write new command word to the board */

	return(0);
}  /* end function Alter_Cmd */


/*
Read_Message	 Coded 03.10.90 by D. Koppel

Description: Allows you to read back data associated with a given message.
This is used for reading data returned from a Transmit command or for
reading the 1553 status word returned by an RT for all message types. The
output consists of the entire message in the same sequence as it appeared
on the bus including command words, status words and data.

Input parameters:  id	  Message identifier returned from a prior call to
			  Create_Message

Output parameters: words  Pointer to an array of up to 36 words for storing
			  command words, status words and data words
			  associated with the message id

Return values:	emode	If not in BC mode
		ebadid	If message id is not a valid message id
        	eminorframe If message is minor frame message
		0	If successful

*/

int borland_dll Read_Message (int id, usint *data)
{
	return Read_Message_Px(g_handlepx, id, data);
}

int borland_dll Read_Message_Px (int handlepx, int id, usint *data)
{
	usint * msgptr;
	int wdcnt, msgtype;
//	int i; // loop replaced with b2drmReadWordBufferFromDpr

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

//	msgtype = *msgptr & 7;      /* get message type */
	msgtype = RWORD_D2H( msgptr[0] ) & 7;

	if (msgtype == SKIP)
		msgtype = cardpx[handlepx].commandtype[id];
	if (msgtype == MINOR_FRAME)
		return eminorframe;
	if (msgtype == JUMP)
		wdcnt = 2;
	else
	{
//		wdcnt = *(++msgptr) & 0x1f; /* get word count of message */
		++msgptr;
		wdcnt = RWORD_D2H( msgptr[0] ) & 0x1f; /* get word count of message */

		if (wdcnt == 0)
			wdcnt = 32;

		/*   wdcnt += 2; */ /* add 1 word for status and 1 word for command */
		wdcnt++;
		if (msgtype != BRD_RCV) /* has no status word */
			wdcnt++;
		if (msgtype == RT2RT)  /* if RT to RT command */
			wdcnt += 2; /* for RT to RT add a second command and status word */
		if (msgtype == BRD_RT2RT) /* add 1 word for the extra command word */
			wdcnt++;
		if (msgtype == MODE)
			wdcnt = 3; /* 1 for command 1 for status 1 for data (assume data) */
		if (msgtype == BRD_MODE)
			wdcnt = 2; /* 1 for command 1 for data (assume data) */
	}
//	for (i=0; i < wdcnt; i++)
//		data[i] = *msgptr++;    /* move message into user array */
	b2drmReadWordBufferFromDpr( (DWORD_PTR) msgptr,
								(unsigned short *) data,
								wdcnt,
								"Read_Message_Px",
								B2DRMHANDLE );

	return(0);
}  /* end function Read_Message */


/*
Create_1553_Message	Coded Aug 10, 97 by J. Waxman, A. Meth

Description: Creates the basic building block of the 1553 protocol - the
message. A message contains a minimum of one command word and a maximum of
two command words and 32 data words. Each message created with this routine
returns a unique id which is used to set or read characteristics of the
message or to build a frame (i.e., a collection of messages) for subsequent
transmission. The message must define the data and status words to be sent
by the RT which is being simulated.

Note: This function is the same as Create_Message. It was modified to make
its parameters compatible with Visual Basic and Labview. All new code should
use this routine instead of Create_Message.

Input parameters:   cmdtype   A number representing the command type
		    data[],   A pointer to an array of command + data

Output parameters:  *handle   On success, returns a message id identifying
			      the message just created, for use in other
			      function calls.

Return values:	 emode		   If not in BC mode
		 ebadcommandword   If bit 0x0400 in the command word is
				   either set or not set incorrectly
		 einval 	   If parameter is out of range
		 msgnospace	   If there is not enough space in message
				   stack for this message
		 msg2many	   If exceeded maximum number of messages
				   permitted (250)
		 0		   If successful


Note: This routine allows the user to create a Message template to be used
in a call to Create_Message whereas before you had to say:

      msgstruct[0].cmdtype = BC2RT;
      msgstruct[0].size = 8;
      msgstruct[0].data = (usint *)msgdata1;
      handle = Create_Message(msgstruct[0]);

   Now you would say ...
      Create_1553_Message(BC2RT, msgdata1, &handle);

Note: There is now no need for MSG or MSG_DK structures, just pass the
parameters directly into Create_1553_Message.

Legal cmdtype's are:
   BC2RT, RT2BC, RT2RT, BRD_RCV, MODE, BRD_MODE, BRD_RT2RT, MINOR_FRAME

*/

int borland_dll Create_1553_Message(usint cmdtype, usint data[], short int *handle)
{
	return Create_1553_Message_Px(g_handlepx, cmdtype, data, handle);
}

int borland_dll Create_1553_Message_Px(int handlepx, usint cmdtype, usint data[], short int *handle)
{
	int size;
//	int i;  // replaced loop with b2drmWriteWordBufferToDpr
//	usint *pNextmsg;
	usint controlWord;
	unsigned char mode;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
	mode = cardpx[handlepx].board_config;
	if ((mode != BC_MODE) && (mode != BC_RT_MODE))
		return (emode);

	switch (cmdtype)
	{
	case MINOR_FRAME:
	case MODE:
	case BRD_MODE: break;

		/* if it is set, return an error */
	case BRD_RCV:
	case BC2RT: if (data[0] & 0x0400) return ebadcommandword; break;

		/* if it is not set, return an error */
	case RT2BC: if (!(data[0] & 0x0400)) return ebadcommandword; break;

		/* if first word is set or second word is not set, return an error */
	case BRD_RT2RT:
	case RT2RT: if (data[0] & 0x0400) return ebadcommandword;
		if (!(data[1] & 0x0400)) return ebadcommandword;
		break;

	default:	  return einval;
	}
	if (cmdtype == MINOR_FRAME)
		size  = 0;

	else
	{
		size = data[0] & 0x1f;

		if (size == 0)
			size = 32;

		/* add 1 for command word and 1 for status word */
		size += 2;
		if (cmdtype == RT2RT)
			size += 2;
		if (cmdtype == BRD_RT2RT) /*  only need command word */
			size += 1;
		if (cmdtype == BRD_RCV)
			size -= 1;

		if (cmdtype == BRD_MODE || cmdtype == MODE)
			size = 3;
	}
	/*
      In BC/CONCURRENT RT mode the user may put in an RT to RT message
      with 32 data words, 2 command words and two status words for 2
      simulated RTs.
	*/


	if (cardpx[handlepx].nextid == MSGMAX)
		return (msg2many);

/* if we are in msg_block1, check that there is room for the msg.
	If there isn't, set flag to msg_block2, and point to msg_block2.
	If we are in msg_block2, check that there is room for the msg.
	If there isn't, set flag to msg_block3, and point to msg_block3.
	If we are in msg_block3, check that there is room for the msg.
	*/

	if (cardpx[handlepx].msg_block == 1)
	{
		if ((cardpx[handlepx].nextmsg + size + 1) >
			(cardpx[handlepx].exc_bd_bc->msg_block1 + MSG_BLOCK1_SIZE)) /* out of msg space */
		{
			cardpx[handlepx].msg_block = 2;
			cardpx[handlepx].nextmsg = cardpx[handlepx].msg_block2;
		}
	}

	if (cardpx[handlepx].msg_block == 2)
	{
		if ((cardpx[handlepx].nextmsg + size + 1) >
			(cardpx[handlepx].msg_block2 + cardpx[handlepx].msg_block2_size)) /* out of msg space */
		{
			/* AMD on PCMCIA/EP memory goes until 0x8000 - i.e., there is no third message area */
			if ((cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1))
				return (msgnospace);

			cardpx[handlepx].msg_block = 3;
			cardpx[handlepx].nextmsg = cardpx[handlepx].msg_block3;
		}
	}

	if (cardpx[handlepx].msg_block == 3)
	{
		if ((cardpx[handlepx].nextmsg + size + 1) >
			(cardpx[handlepx].msg_block3 + cardpx[handlepx].msg_block3_size)) /* out of msg space */
		{
//			if ((cardpx[handlepx].exc_bd_bc->expandedBlock & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE)
			if ((RWORD_D2H( cardpx[handlepx].exc_bd_bc->expandedBlock ) & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE)
			{
				cardpx[handlepx].msg_block = 4;
				cardpx[handlepx].nextmsg = cardpx[handlepx].msg_block4;
			}
			else return (msgnospace);
		}
	}
	if (cardpx[handlepx].msg_block == 4)
	{
		if ((cardpx[handlepx].nextmsg + size + 1) >
      	(cardpx[handlepx].msg_block4 + cardpx[handlepx].msg_block4_size)) /* out of msg space */
			return (msgnospace);
	}

	cardpx[handlepx].msgalloc[cardpx[handlepx].nextid] = cardpx[handlepx].nextmsg;
	cardpx[handlepx].commandtype[cardpx[handlepx].nextid] = cmdtype;

	/* set control word  */
#ifdef MMSI
//	*cardpx[handlepx].nextmsg = (usint)(cmdtype);
	controlWord = (usint)(cmdtype);
#else
//	*cardpx[handlepx].nextmsg = (usint)(0x80 + cmdtype); /* 0x80 is bus A */
	controlWord = (0x80 + cmdtype);
#endif

	WWORD_H2D(cardpx[handlepx].nextmsg[0] , controlWord );

	/* copy message into dual port RAM */
	cardpx[handlepx].nextmsg++;

	/* ignore user's size parameter if it's too small for the message */
//	for (i = 0 ; i < size; i++)
//		cardpx[handlepx].nextmsg[i] = data[i];
	if (size > 0)  //for Minor frame or other special msg there is no data
		b2drmWriteWordBufferToDpr ( (DWORD_PTR) cardpx[handlepx].nextmsg,
									 (unsigned short *) data,
									 size, // size = number of WORDs
									 "Create_1553_Message_Px",
									 B2DRMHANDLE );

	/* leave an additional word for the status */
	cardpx[handlepx].nextmsg += size; /*  + 1;  don' want an extra word for good measure */
	*handle = cardpx[handlepx].nextid++;

	return 0;
}

/*
Get_BC_Message

Description: This function combines the status & contents of a message
	     in one structure. This can be used in place of
	     separately calling the function Get_Msgentry_Status for the status
	     word, and calling the function Read_Message which returns just
	     the contents of the message.

Input parameters: int frameid   Frame identifier returned from a prior call to Create_frame
						int msgentry  Index of message in frame (First msg in frame is msgentry 0)
Output parameter: struct BCMSG *msgptr
			       Pointer to the structure defined below in
			       which to return the data. Space should
			       always be allocated for 36 words of data to
			       accommodate the maximum case of RT2RT
			       transmission of 32 data words + 2 status
			       words + 2 command words.
		  typedef struct {
		  int msgstatus; Status word containing the following flags:
				 END_OF_MSG    INVALID_WORD
				 BAD_BUS       WD_CNT_HI
				 ME_SET        WD_CNT_LO
				 BAD_STATUS    BAD_SYNC
				 INVALID_MSG   BAD_RT_ADDR
				 LATE_RESP     MSG_ERROR
				 SELFTEST_ERR  BAD_GAP
		  int *words;	     A pointer to an array of 1553 words.
		  }

Return values:
			      emode	 If not bc or bcrt mode
               frm_erange frame id specified was not created using create_frame
               frm_erangecnt msg entry specified is greater than num msgs in frame
*/
#define MSGENTRY_SIZE 4
int borland_dll Get_BC_Msgentry (int frameid, int msgentry, struct BCMSG *msgptr)
{
	return Get_BC_Msgentry_Px (g_handlepx, frameid, msgentry, msgptr);
}

int borland_dll Get_BC_Msgentry_Px (int handlepx, int frameid, int msgentry, struct BCMSG *msgptr)
{
	int status, wdcnt, i, msgtype, id = 0, found;
	usint *msgstackptr, *msgentryptr;
	usint msgstatusword;

	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;

	status = Get_Msgentry_Status_Px(handlepx, frameid, msgentry, &msgstatusword);
	if (status <0)
		return status; /* emode, frm_erange and frm_erangecnt */

	msgptr->msgstatus = msgstatusword;

	for (i=0; i<36; i++)
		msgptr->words[i] = 0;

	msgstackptr = (usint *)(cardpx[handlepx].exc_bd_bc) + ((cardpx[handlepx].framealloc[frameid].frameptr) / sizeof(usint));
	msgstackptr += (msgentry * MSGENTRY_SIZE);

//	msgentryptr = (usint *)(cardpx[handlepx].exc_bd_bc) + (*msgstackptr / sizeof(usint));
	msgentryptr = (usint *)(cardpx[handlepx].exc_bd_bc) + (RWORD_D2H( msgstackptr[0] ) / sizeof(usint));

//	msgtype = *msgentryptr & 7;      /* get message type */
	msgtype = RWORD_D2H( msgentryptr[0] ) & 7;      /* get message type */

	if (msgtype == SKIP) /* get message id, and then get commandtype[id] */
	{
		found = 0;
		for (i=0; i < cardpx[handlepx].nextid; i++)
			if (cardpx[handlepx].msgalloc[i] == msgentryptr)
			{
				id = i;
				found = 1;
				break;
			}
			if (found == 0)
				return einval; /* this should never happen */

			msgtype = cardpx[handlepx].commandtype[id];
	}
	if (msgtype == MINOR_FRAME)
		return eminorframe;
	if (msgtype == JUMP)
		wdcnt = 2;
	else
	{
//		wdcnt = *(++msgentryptr) & 0x1f; /* get word count of message */
		++msgentryptr;
		wdcnt = RWORD_D2H( msgentryptr[0] ) & 0x1f; /* get word count of message */

		if (wdcnt == 0)
			wdcnt = 32;

		wdcnt++;
		if (msgtype != BRD_RCV) /* has no status word */
			wdcnt++;
		if (msgtype == RT2RT)  /* if RT to RT command */
			wdcnt += 2; /* for RT to RT add a second command and status word */
		if (msgtype == BRD_RT2RT) /* add 1 word for the extra command word */
			wdcnt++;
		if (msgtype == MODE)
			wdcnt = 3; /* 1 for command 1 for status 1 for data (assume data) */
		if (msgtype == BRD_MODE)
			wdcnt = 2; /* 1 for command 1 for data (assume data) */
	}

//	for (i=0; i < wdcnt; i++)
//		msgptr->words[i] = *msgentryptr++;    /* move message into user array */
	b2drmReadWordBufferFromDpr( (DWORD_PTR) msgentryptr,
								(unsigned short *) msgptr->words,
								wdcnt,
								"Get_BC_Msgentry_Px",
								B2DRMHANDLE );

	return(0);
}

/*
Get_BC_Message

Description: This function combines the status & contents of a message
	     in one structure, similar to the function Get_RT_Message (RT)
	     and Get_Next_Message (BM). This can be used in place of
	     separately calling the function Get_Msg_Status for the status
	     word, and calling the function Read_Message which returns just
	     the contents of the message.

Input parameters: int id    Message identifier returned from a prior call to
			    Create_1553_Message

Output parameter: struct BCMSG *msgptr
			       Pointer to the structure defined below in
			       which to return the data. Space should
			       always be allocated for 36 words of data to
			       accommodate the maximum case of RT2RT
			       transmission of 32 data words + 2 status
			       words + 2 command words.
		  typedef struct {
		  int msgstatus; Status word containing the following flags:
				 END_OF_MSG    INVALID_WORD
				 BAD_BUS       WD_CNT_HI
				 ME_SET        WD_CNT_LO
				 BAD_STATUS    BAD_SYNC
				 INVALID_MSG   BAD_RT_ADDR
				 LATE_RESP     MSG_ERROR
				 SELFTEST_ERR  BAD_GAP
		  int *words;	     A pointer to an array of 1553 words.
		  }

Return values:	On success, returns 0 or more of the following flags:
			      ebadid	 If message id is not a valid message id
			      frm_nostack   if Start_Frame was not yet called to set up the message stack
*/

int borland_dll Get_BC_Message (int id, struct BCMSG *msgptr)
{
	return Get_BC_Message_Px (g_handlepx, id, msgptr);
}

int borland_dll Get_BC_Message_Px (int handlepx, int id, struct BCMSG *msgptr)
{
	int status, i;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	status = Get_Msg_Status_Px(handlepx, id);
	if (status <0)
		return status; /* emode or ebadid */
	if (status == 0)
		return (enobcmsg);
	msgptr->msgstatus = (usint)status;
	if ((status & END_OF_MSG) != END_OF_MSG)
		return(einbcmsg);

	for (i=0; i<36; i++) msgptr->words[i] = 0;
	status = Read_Message_Px (handlepx, id, msgptr->words);
	return(status); /* actually always 0 */
}

/*
Get_Next_Message_BCM	Coded Nov 28, 93 by D. Koppel
			Updated for Concurrent Monitor, Apr 98, by A. Meth

Note: This routine is like Get_Next_Message, which gets the next Bus Monitor
message block. However, it is used *only* for a BC/Concurrent Bus Monitor
(hence the appended "_BCM" in the function name).

Description: Reads the message block following the block read in the
previous call to Get_Next_Message_BCM.

The first call to Get_Next_Message_BCM will return block 0.

Note: If the next block does not contain a new message an error will be
returned. Note that if you fall 128 blocks behind the board it will be
impossible to know if the message returned is newer or older than the
previous message returned unless you check the timetag. The routine
therefore returns the timetag itself in the elapsed time field rather
than a delta value.

Input parameters: none

Ouput parameters: msgptr   Pointer to the structure defined below in which
			   to return the message
		  struct MONMSG {
		  int msgstatus; Status word containing the following flags:
				 END_OF_MSG	 RT2RT_MSG
				 TRIGGER_FOUND	 INVALID_MSG
				 BUS_A		 INVALID_WORD
				 WD_CNT_HI	 WD_CNT_LO
				 BAD_RT_ADDR	 BAD_SYNC
				 BAD_GAP	 MON_LATE_RESP
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

        msg_addr	Pointer to the word[39] which contains the message serial number.

Return values:	block number  If successful. Valid values are 0-0x80 (0-128).
		emode	      If board is not in BC/Concurrent RT mode
		enomsg	      If there is no message to return

Change history:
-----------------
8-Sep-2008	(YYB)
-	added DMA support

10-Dec-2008	(YYB)
- corrected a pointer assignment when DMA is not available
	(the sense of an "if" test was backwards)
*/


int borland_dll Get_Next_Message_BCM (struct MONMSG *msgptr, usint * msg_addr)
{
	return Get_Next_Message_BCM_Px (g_handlepx, msgptr, msg_addr);
}

int borland_dll Get_Next_Message_BCM_Px (int handlepx, struct MONMSG *msgptr, usint * msg_addr)
{
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

	/* check if board is configured as a BC/RT */
//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return (emode);

	/* check if user set BC to be in expanded mode */
	if ((cardpx[handlepx].expandedBCblockFW) && 
//		((cardpx[handlepx].exc_bd_bc->expandedBlock & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE))
		(( RWORD_D2H( cardpx[handlepx].exc_bd_bc->expandedBlock ) & EXPANDED_BLOCK_MODE) == EXPANDED_BLOCK_MODE))
		return (func_invalid); /* when BC is in expanded mode there is no concurrent monitor */

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
										"Get_Next_Message_BCM_Px",
										B2DRMHANDLE);

		// help speed up message reads when there is nothing to read; if the EOM flag is not set, then there is no reason to re-read the message just yet
		if ((msgCopy1packed.msgstatus & END_OF_MSG) != END_OF_MSG)
			return enomsg;

		//get a second copy of the header so we can check if any changes were made while we were in mid read
		b2drmReadWordBufferFromDpr( (DWORD_PTR) pDpram,
									(unsigned short *) &msgCopy2packed,
									MONMSG_HEADER_SIZE / sizeof(unsigned short),
									"Get_Next_Message_BCM_Px",
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

#ifdef MMSI
	/* if LINK mode, and MSW indicates wrong RT address, and there are no other error bits, then this is not an error; turn off ERR bit 00 */
	if (cardpx[handlepx].exc_bd_bc->hubmode_MMSI == HUB_LINK)
	{
		if ((msgptr->msgstatus & BAD_RT_ADDR) == BAD_RT_ADDR)
		{
			if ((msgptr->msgstatus & ERROR_BITS_NO_RT_ADDR) == 0)
			{
				msgptr->msgstatus = msgptr->msgstatus & ~0x1; /* mask out Error bit */
			}
		}
	}
#endif


	*msg_addr = msgptr->msgCounter;
	cardpx[handlepx].next_MON_entry++;
	return (cardpx[handlepx].next_MON_entry - 1);
}

int borland_dll Command_Word_15EP(int rtnum, int type, int subaddr, int wordcount, usint *commandword)
{
	return Command_Word_Px(rtnum, type, subaddr, wordcount, commandword);
}

int borland_dll Command_Word_Px(int rtnum, int type, int subaddr, int wordcount, usint *commandword)
{
	if ((wordcount <0) || (wordcount >32))
		return einval;

	if (wordcount == 32)
		wordcount = 0;
	
	*commandword = (rtnum << 11) | (type << 10) | (subaddr << 5) | wordcount;

	return 0;
}

int borland_dll Re_Create_Message(usint cmdtype, usint data[], short int id)
{
	return Re_Create_Message_Px(g_handlepx, cmdtype, data, id);
}

int borland_dll Re_Create_Message_Px(int handlepx, usint cmdtype, usint data[], short int id)
{
	int size, totsize;
//	int i; // loop replaced with b2drmWriteWordBufferToDpr

	usint *msgptr, *nextptr, ustmp;
	// usint msgptrContents;  // this is used only in a section ofc code that was removed (commented out)

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if ((cardpx[handlepx].exc_bd_bc->board_config != BC_MODE) &&
//		(cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE))
	if ((cardpx[handlepx].board_config != BC_MODE) &&
		(cardpx[handlepx].board_config != BC_RT_MODE))
		return (emode);

	if ((id < 1) || (id >= cardpx[handlepx].nextid))
		return (ebadid);

	/* Checking that size of the incoming message is not larger than the size allocated to this id */
	/* To do this: first figure out size of incoming message */

// BUG FIX 24-Aug-2020: Ensure that we only look at the command code, which is the lowest 4 bits of cmdtype
//	switch (cmdtype)
	switch (cmdtype & COMMAND_CODE)
	{
	case MINOR_FRAME:
	case MODE:
	case BRD_MODE: break;

		/* if it is set, return an error */
	case BRD_RCV:
	case BC2RT: if (data[0] & 0x0400) return ebadcommandword; break;

		/* if it is not set, return an error */
	case RT2BC: if (!(data[0] & 0x0400)) return ebadcommandword; break;

		/* if first word is set or second word is not set, return an error */
	case BRD_RT2RT:
	case RT2RT: if (data[0] & 0x0400) return ebadcommandword;
		if (!(data[1] & 0x0400)) return ebadcommandword;
		break;

	default: return einval;
	}

	size = data[0] & 0x1f;

	if (cmdtype != MINOR_FRAME)
	{
		if (size == 0)
			size = 32;
	}

	/* add 1 for command word and 1 for status word */
	size += 2;
	if (cmdtype == RT2RT)
		size += 2;
	if (cmdtype == BRD_RT2RT) /*  only need command word */
		size += 1;
	if (cmdtype == BRD_RCV)
		size -= 1;

	if (cmdtype == BRD_MODE || cmdtype == MODE)
		size = 3;
	if (cmdtype == MINOR_FRAME)
		size  = 0;

	if (cardpx[handlepx].nextid == id+1) /* this msg is the last one allocated so far */
		nextptr = cardpx[handlepx].nextmsg;
	else
		nextptr = cardpx[handlepx].msgalloc[id+1];

	totsize = size +1; /* includes control word */
	if (cardpx[handlepx].msgalloc[id] + totsize > nextptr)
		return (msg2big);

	/* size is OK. now overwrite values */
	cardpx[handlepx].commandtype[id] = cmdtype;
	msgptr = cardpx[handlepx].msgalloc[id];

// BUG FIX 24-Aug-2020: Removed the next block of code that reads msgptr (into ustmp), and replace it with
// the assignment to cmdtype below (immediately after the call to b2drmWriteWordBufferToDpr).

/*
	// change only command code in control word; if SKIP, leave the cmdtype as is, so that it can be RESTOREd later
//	if ((*msgptr & 0xF) != SKIP)
	msgptrContents = RWORD_D2H( msgptr[0] );
	if ((msgptrContents & COMMAND_CODE) != SKIP)
	{
//		ustmp = (*msgptr & (usint)0xfff0) | cmdtype;
		ustmp = (msgptrContents & (usint)XMT_BUS_A) | cmdtype; //We now clear the HALT flag since the firmware now sets it in replay mode
	}
	else
	{
//		ustmp = *msgptr;
		ustmp = msgptrContents;
	}
*/

//	*msgptr++ = ustmp;

	/* copy message into dual port RAM */
//	for (i = 0 ; i < size; i++)
//		*msgptr++ = data[i];
	b2drmWriteWordBufferToDpr ( (DWORD_PTR) (msgptr + 1), //Start from the command word
									 (unsigned short *) data,
									 size, // size = number of WORDs
									 "Re_Create_Message_Px",
									 B2DRMHANDLE );

	ustmp = cmdtype;

	WWORD_H2D( msgptr[0], ustmp);

	return 0;
}

/*
Read_SRQ_Message

Description: Allows you to read back the command and data associated with the last
SRQ message.

Input parameters:  handle

Output parameters: vector status
			 message status
			 data  Pointer to an array of up to 36 words for storing
			  command words, status words and data words
			  associated with the SRQ message

Return values:	emode		If not in BC mode
        		enosrq	If SRQ was disabled by user
			0		If successful

*/
int borland_dll Read_SRQ_Message_Px (int handlepx, usint *vector_status, usint *msg_status, usint *data, usint *srq_counter)
{
	int wdcnt;
//	int i; // loop replaced with b2drmReadWordBufferFromDpr

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_bc->board_config != BC_RT_MODE)
	if (cardpx[handlepx].board_config != BC_RT_MODE)
		return (emode);

//	if (cardpx[handlepx].exc_bd_bc->bcProtocolOptions & NOSRQ_BIT)
	if ( (RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions )) & NOSRQ_BIT)
		return enosrq;

//	*vector_status = cardpx[handlepx].exc_bd_bc->srqmessagestatus[0];
	*vector_status = RWORD_D2H( cardpx[handlepx].exc_bd_bc->srqmessagestatus[0] );

//	*msg_status = cardpx[handlepx].exc_bd_bc->srqmessagestatus[1];
	*msg_status = RWORD_D2H( cardpx[handlepx].exc_bd_bc->srqmessagestatus[1] );

	/* zero out to show these were read */
//	cardpx[handlepx].exc_bd_bc->srqmessagestatus[0] = 0;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->srqmessagestatus[0], 0);

//	cardpx[handlepx].exc_bd_bc->srqmessagestatus[1] = 0;
	WWORD_H2D( cardpx[handlepx].exc_bd_bc->srqmessagestatus[1], 0);

//	*srq_counter = cardpx[handlepx].exc_bd_bc->srqcounter;
	*srq_counter = RWORD_D2H( cardpx[handlepx].exc_bd_bc->srqcounter );

//	wdcnt = cardpx[handlepx].exc_bd_bc->srqmessage2[1] & 0x1f; /* get word count of message */
	wdcnt = RWORD_D2H( cardpx[handlepx].exc_bd_bc->srqmessage2[1] ) & 0x1f; /* get word count of message */

	if (wdcnt == 0)
		wdcnt = 32;

	wdcnt += 2; /* status and command */

	/* move message into user array */
//	for (i=0; i < wdcnt; i++)
//		data[i] = cardpx[handlepx].exc_bd_bc->srqmessage2[i+1];

	b2drmReadWordBufferFromDpr( (DWORD_PTR) &(cardpx[handlepx].exc_bd_bc->srqmessage2[1]),
								(unsigned short *) data,
								wdcnt,
								"Read_SRQ_Message_Px",
								B2DRMHANDLE );

	return(0);
}  /* end function Read_SRQ_Message */

