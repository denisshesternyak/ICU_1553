#include "pxIncl.h"
#include "univrun.h"
extern INSTANCE_PX cardpx[];
t_univTable univTable[NUMCARDS];

#define BC_WCH_BIT 0x0040
#define BC_WCH_BIT_SHIFT 6
#define RT_WCE_BIT 0x0020
#define RT_WCE_BIT_SHIFT 5

/*
Note that  only Load_Datablk and the functions that readback data are processed
when card is running, in BC mode
*/

int borland_dll Set_Mode_UNIV_Px(int handlepx, int mode)
{
	int i, retval;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ((mode != UNIV_RT_MODE) && (mode != UNIV_BC_RT_MODE) && (mode != UNIV_MON_BLOCK))
		return einval;

	if ((mode == UNIV_RT_MODE) || (mode == UNIV_MON_BLOCK))
		retval =  Set_Mode_Px(handlepx, RT_MODE);
	else
	{
		/* zero out tables */
		for (i=0; i<NUM_RTIDS; i++)
			univTable[handlepx].rtidTable[i].blknum = 0;
		for (i=0; i<NUM_RTS; i++)
		{
			univTable[handlepx].rtTable[i].intrpt = 0;
			univTable[handlepx].rtTable[i].valsSet = 0;
			univTable[handlepx].rtTable[i].statusValue = i << 11;
		}
		univTable[handlepx].bitcountErrinj = FALSE;
		univTable[handlepx].parityErrinj = FALSE;
		univTable[handlepx].broadcastInterrupt = FALSE;
		univTable[handlepx].RTinterrupt = FALSE;
		univTable[handlepx].next_MON_entry = 0;
		univTable[handlepx].lastmsgttag = 0;
		for (i=0; i<MAX_READVALS; i++)
		{
			univTable[handlepx].readvalTable[i].messagenum = 0;
			univTable[handlepx].readvalTable[i].timetag = 0;
		}

		retval = Set_Mode_Px(handlepx, BC_RT_MODE);
	}
	cardpx[handlepx].univmode = mode;
	return retval;
}

int borland_dll Set_RT_Active_UNIV_Px (int handlepx, int rtnum, int intrpt)
{
	int retval;

	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Set_RT_Active_Px(handlepx, rtnum, intrpt);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	if ((intrpt != 0) && (intrpt != 1))
		return einval;

	retval = Set_RT_Active_Px(handlepx, rtnum, intrpt);
	if (retval == 0)
	{
		univTable[handlepx].rtTable[rtnum].intrpt = intrpt;
		return 0;
	}
	else
		return retval;
}

/* BLOCK RELATED */

/*
In BC_MODE this routine creates an entry in rtidTable
No error returned if rtid already assigned to a block;
function can be used to change, or cancel, assignment.
In order for RT mode and BC mode to be consistent,
in universal mode if a blknum is set to receive,
the same blknum cannot be used for any other rtid.

  At Run_UNIV the structure is completed, by associating blknums with
  BC messages containing command words that match the rtid.
*/
int borland_dll Assign_RT_Data_UNIV_Px (int handlepx, int rtid, int blknum)
{
	int i, table_blknum;

	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ((cardpx[handlepx].univmode != UNIV_RT_MODE)	&& (cardpx[handlepx].univmode != UNIV_BC_RT_MODE))
		return uemode;

	if ((rtid < 0) || (rtid >= NUM_RTIDS))
		return einval;
	if ((blknum < 0) || (blknum >= NUM_BLOCKS))
		return einval;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
	{
		/* first check that this block wasnt already assigned to another receive message */
		for (i=0; i<NUM_RTIDS; i++)
		{
			if (i == rtid)
				continue;
#ifdef VME4000
			table_blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[i^1];
#else
			table_blknum = cardpx[handlepx].exc_bd_rt->blk_lookup[i];
#endif
			table_blknum |= (cardpx[handlepx].exc_bd_rt->rtidIP_cmonAP[i] & RTID_EXPANDED_BLOCK) << RTID_EXPANDED_BLOCK_SHIFT;

			if (table_blknum == blknum)
			{
				if (TRTYPE(rtid) == RECEIVE)
					return ueblkassigned;
				if (TRTYPE(i) == RECEIVE)
					return ueblkassigned;			
			}
		}
		return Assign_RT_Data_Px(handlepx, rtid, blknum);
	}
	else /* BCRT MODE */
	{
		for (i=0; i<NUM_RTIDS; i++)
		{
			if (i == rtid)
				continue;
			if (univTable[handlepx].rtidTable[i].blknum == blknum)
			{
				if (TRTYPE(rtid) == RECEIVE)
					return ueblkassigned;
				if (TRTYPE(i) == RECEIVE)
					return ueblkassigned;			
			}
		}
		univTable[handlepx].rtidTable[rtid].blknum = blknum;
		return(0);
	}
}

/*
In BC_MODE this routine returns blocknum that was assigned to the rtid
*/
int borland_dll Get_Blknum_UNIV_Px (int handlepx, int rtid)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Get_Blknum_Px (handlepx, rtid);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	if ((rtid < 0) || (rtid >= NUM_RTIDS))
		return einval;

	return univTable[handlepx].rtidTable[rtid].blknum;
}

/*
In BC_MODE, if called in runtime, this routine calls Alter_Message_Px for all transmit messages
in frame that are assigned to this blknum.
*/
int borland_dll Load_Datablk_UNIV_Px (int handlepx, int blknum, usint *words)
{
	int i, d, msgnum, wdcnt, pos;
	usint data[36], msgid;

	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Load_Datablk_Px (handlepx, blknum, words);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	if ((blknum < 0) || (blknum >= NUM_BLOCKS))
		return einval;

	if (blknum == 0)
		return 0;

	/* Always move 32 words. Unused words don't matter. */
	for (i = 0; i < BLOCKSIZE; i++)
		univTable[handlepx].blockTable[blknum].data[i] = words[i];

	if ((cardpx[handlepx].exc_bd_bc->start & START_BIT) == START_BIT)
	{
		for (msgnum=1; msgnum<=univTable[handlepx].blockTable[blknum].numMsgs; msgnum++)
		{
			msgid =  univTable[handlepx].blockTable[blknum].msgTable[msgnum].msgid;
			wdcnt = univTable[handlepx].blockTable[blknum].msgTable[msgnum].wdcnt;
			pos = univTable[handlepx].blockTable[blknum].msgTable[msgnum].pos;
			/* values will only be filled in for RT active */
			Read_Message_Px(handlepx, msgid, data);
			for (d=0; d<wdcnt; d++)
				data[d+pos] = univTable[handlepx].blockTable[blknum].data[d];
			while(BC_Check_Alter_Msg_Px(handlepx, msgid)==DO_NOT_ALTER_MSG);
			Alter_Message_Px(handlepx, msgid, &data[pos-1]);
		}
	}
	return(0);
}

/* RTID RELATED */

/*
In BC_MODE this routine maintains an array of rtids needing interrupts.
At Run_RT, Set_Interrupt_On_Msg_Px is called for all messages with these rtids.
in universal mode, only interrupt on message is allowed (not on error)
*/
int borland_dll Set_RTid_Interrupt_UNIV_Px (int handlepx, int rtid, int enabled_flag)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Set_RTid_Interrupt_Px (handlepx, rtid, enabled_flag, INT_ON_ENDOFMSG);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	if ((rtid < 0) || (rtid >= NUM_RTIDS))
		return einval;

	if (enabled_flag == ENABLE)
		univTable[handlepx].rtidTable[rtid].intrpt = TRUE;
	else if (enabled_flag == DISABLE)
		univTable[handlepx].rtidTable[rtid].intrpt = FALSE;
	else
		return einval;

	return(0);
}


/* RT RELATED */

/*
In BC_MODE this routine maintains an array of Status values.
At Run_RT, for each msgid with this rtnum, Alter_Message_Px is called to put in the Status word.
*/
int borland_dll Set_Status_UNIV_Px (int handlepx, int rtnum, int statusvalue)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Set_Status_Px (handlepx, rtnum, statusvalue);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	if ((rtnum < 0) || (rtnum > 31))
		return (einval);

	univTable[handlepx].rtTable[rtnum].statusValue = (usint)statusvalue;
	return(0);
}

/*
In BC_MODE this routine maintains an array of vector data values.
At Run_RT, the message list is checked for 'send vector' mode codes
and Alter_Message_Px is called to put in the vector data.
*/
int borland_dll Set_Vector_UNIV_Px (int handlepx, int rtnum, int vecvalue)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Set_Vector_Px (handlepx, rtnum, vecvalue);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	if ((rtnum < 0) || (rtnum > 31))
		return (einval);

	univTable[handlepx].rtTable[rtnum].valsSet |= VECTOR_VAL_SET;
	univTable[handlepx].rtTable[rtnum].vectorValue = (usint)vecvalue;

	return(0);
}

/*
In BC_MODE this routine maintains an array of BIT data values.
At Run_RT, the message list is checked for BIT mode codes and
Alter_Message_Px is called to put in the BIT data.
*/
int borland_dll Set_Bit_UNIV_Px (int handlepx, int rtnum, int bitvalue)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Set_Bit_Px (handlepx, rtnum, bitvalue);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	if ((rtnum < 0) || (rtnum > 31))
		return (einval);

	univTable[handlepx].rtTable[rtnum].valsSet |= BIT_VAL_SET;
	univTable[handlepx].rtTable[rtnum].bitValue = (usint)bitvalue;

	return(0);
}

/* GENERAL FLAG */

/*
In BC_MODE this routine maintains a flag.
At Run_RT, Set_Interrupt_On_Msg_Px is called for all messages with these rts.
MSG_CMPLT or 0 are the allowed flags
*/
int borland_dll Set_Interrupt_UNIV_Px (int handlepx, int flag)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((flag != MSG_CMPLT) && (flag != 0))
		return einval;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Set_RT_Interrupt_Px (handlepx);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	if (flag == MSG_CMPLT)
		univTable[handlepx].RTinterrupt = TRUE;
	else
		univTable[handlepx].RTinterrupt = FALSE;
	return(0);
}


/*
In BC_MODE this routine maintains a flag. Parity and/or bitcount errors allowed, transmit commands only.
In order for rt to be consistent with bc mode, status, and data error in first data word, are set for each.
At Run_RT, the errors are injected into all transmit messages with active RTs.
*/

int borland_dll Set_RT_Errors_UNIV_Px (int handlepx, int errormask)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
	if ((cardpx[handlepx].univmode != UNIV_RT_MODE) && (cardpx[handlepx].univmode != UNIV_BC_RT_MODE))
		return uemode;

	if (((errormask & VALID_ERRINJ_MASK) != UNIV_BIT_CNT_ERRINJ) &&
		((errormask & VALID_ERRINJ_MASK) != UNIV_PARITY_ERRINJ) &&
		(errormask != VALID_ERRINJ_MASK) && (errormask != 0))
		return einval;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Set_RT_Errors_Px (handlepx, errormask);

	/* else univ-bc-rt-mode */

	if ((errormask & UNIV_BIT_CNT_ERRINJ) == UNIV_BIT_CNT_ERRINJ)
		univTable[handlepx].bitcountErrinj = TRUE;
	else
		univTable[handlepx].bitcountErrinj = FALSE;

	if ((errormask & UNIV_PARITY_ERRINJ) == UNIV_PARITY_ERRINJ)
		univTable[handlepx].parityErrinj = TRUE;
	else
		univTable[handlepx].parityErrinj = FALSE;

	return(0);
}

/*
In BC_MODE this routine maintains a flag.
At Run_RT, Set_Interrupt_On_Msg_Px is called for all Broadcast messages.
*/
int borland_dll Set_Broad_Interrupt_UNIV_Px (int handlepx, int intrpt)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Set_Broad_Interrupt_Px (handlepx, intrpt);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	if (intrpt == 1)
		univTable[handlepx].broadcastInterrupt = TRUE;
	else if (intrpt == 0)
		univTable[handlepx].broadcastInterrupt = FALSE;
	else
		return einval;

	return(0);
}

/* NOTHING */

/*
In BC_MODE this routine does nothing.
(Mode code is determined by Create_1553_Message_Px)
*/
int borland_dll Set_Mode_Addr_UNIV_Px (int handlepx, int flag)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
		return Set_Mode_Addr_Px (handlepx, flag);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	return(0);
}

/*
In BC_MODE this routine does nothing.
(Broadcast is determined by Create_1553_Message_Px)
*/
int borland_dll Set_RT_Broadcast_UNIV_Px(int handlepx, int toggle)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((cardpx[handlepx].univmode == UNIV_RT_MODE) || (cardpx[handlepx].univmode == UNIV_MON_BLOCK))
		return Set_RT_Broadcast_Px(handlepx, toggle);
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	return(0);
}

#define MSGENTRY_SIZE 4
int borland_dll Run_UNIV_Px (int handlepx, int bcrunflag)
{
	int msgentry, msgid, rtid, rtnum, found, d, wdcnt, i, rtnum2, rtid2, blknum, frameid, j, flags;
	usint *stackptr;
	usint *msgptr, command, data[36], msgtype, modecode, command2;

	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((cardpx[handlepx].univmode == UNIV_RT_MODE) || (cardpx[handlepx].univmode == UNIV_MON_BLOCK))
		return Run_RT_Px (handlepx);

	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	/* clear out msgid table, then will rebuild */
	for (i=0; i< NUM_BLOCKS; i++)
	{
		univTable[handlepx].blockTable[i].numMsgs = 0;
	}

	for (frameid = 0; frameid <  cardpx[handlepx].nextframe; frameid++)
	{
		stackptr = (usint *)(cardpx[handlepx].exc_bd_bc) + ((cardpx[handlepx].framealloc[frameid].frameptr) / sizeof(usint));

		for (msgentry=0; msgentry<cardpx[handlepx].framealloc[frameid].msgcnt; msgentry++, stackptr += (MSGENTRY_SIZE))
		{
			msgptr = (usint *)(cardpx[handlepx].exc_bd_bc) +(*stackptr / sizeof(usint));
			msgtype = (*msgptr) & 7;
			command = *(msgptr+1);
			rtid = (command & ~0x1f) >> 5;
			rtnum = (rtid & 0x7c0) >> 6;
			if ((msgtype == RT2RT) || (msgtype == BRD_RT2RT))
			{
				command2 = *(msgptr+2);              /*transmit side */
				rtid2 = (command2 & ~0x1f) >> 5;
				rtnum2 = (rtid2 & 0x7c0) >> 6;
			}
			else
				rtnum2 = NULL_RT;
			wdcnt = command & 0x1f;
			if (wdcnt == 0)
				wdcnt = 32;

#ifdef VME4000
			if ((cardpx[handlepx].exc_bd_bc->active[rtnum^1] & RT_ACTIVE_BIT) != RT_ACTIVE_BIT)
#else
			if ((cardpx[handlepx].exc_bd_bc->active[rtnum] & RT_ACTIVE_BIT) != RT_ACTIVE_BIT)
#endif
			{
				if (rtnum2 == NULL_RT)
					continue;
				else
					rtnum = NULL_RT;
			}

			if (rtnum2 != NULL_RT) /* use this temporarily to mean that its rt2rt message */
			{
#ifdef VME4000
				if ((cardpx[handlepx].exc_bd_bc->active[rtnum2^1] & RT_ACTIVE_BIT) != RT_ACTIVE_BIT)
#else
				if ((cardpx[handlepx].exc_bd_bc->active[rtnum2] & RT_ACTIVE_BIT) != RT_ACTIVE_BIT)
#endif
				{
					if (rtnum == NULL_RT)
						continue;
					rtnum2 = NULL_RT;
				}
			}

			/* get corresponding msgid */
			found = 0;
			for (msgid=1; msgid<cardpx[handlepx].nextid; msgid++)
				if (cardpx[handlepx].msgalloc[msgid] == msgptr)
				{
					found = 1;
					break;
				}
				if (found == 0)
				{
					return einval; /* this should never happen */
				}

				if (rtnum2 == NULL_RT)
					blknum = univTable[handlepx].rtidTable[rtid].blknum;
				else /* rt2rt */
					blknum = univTable[handlepx].rtidTable[rtid2].blknum;

				/* set interrupts, per msgentry in frameid */
				if (((msgtype==BRD_RCV)||(msgtype==BRD_RT2RT)||(msgtype==BRD_MODE))&& univTable[handlepx].broadcastInterrupt)
					Set_Interrupt_On_Msg_Px(handlepx, frameid, msgentry, 1);
				else if ( (rtnum != NULL_RT) && univTable[handlepx].RTinterrupt  &&
					((univTable[handlepx].rtTable[rtnum].intrpt == 1) || univTable[handlepx].rtidTable[rtid].intrpt) )
					Set_Interrupt_On_Msg_Px(handlepx, frameid, msgentry, 1);
				else if ( (rtnum2 != NULL_RT) && univTable[handlepx].RTinterrupt  &&
					((univTable[handlepx].rtTable[rtnum2].intrpt == 1) || univTable[handlepx].rtidTable[rtid2].intrpt) )
					Set_Interrupt_On_Msg_Px(handlepx, frameid, msgentry, 1);

				/* if this msgid is already in the frame don't deal with it (alter_message is on the msgid level)*/
				for (j=0; j<univTable[handlepx].blockTable[blknum].numMsgs; j++)
					if (univTable[handlepx].blockTable[blknum].msgTable[j].msgid == msgid)
						continue;

				if (blknum != 0)
				{
					/* update data words and other kinds of data, for transmit messages with rt active */
					if (msgtype==RT2BC)
					{
						for (d=0; d<wdcnt; d++)
							data[d+2] = univTable[handlepx].blockTable[univTable[handlepx].rtidTable[rtid].blknum].data[d];
						Alter_Message_Px(handlepx, msgid, &data[1]);
						j = ++(univTable[handlepx].blockTable[blknum].numMsgs);
						if (j > MAX_MSGS_PER_BLOCK)
							return uetoomanymsgids;
						univTable[handlepx].blockTable[blknum].msgTable[j].msgid = msgid;
						univTable[handlepx].blockTable[blknum].msgTable[j].pos = 2;
						univTable[handlepx].blockTable[blknum].msgTable[j].wdcnt = wdcnt;
					}
					else if  ((msgtype==RT2RT)||(msgtype == BRD_RT2RT))
					{
						if(rtnum2 != NULL_RT) /* rt2rt_xmt */
						{
							for (d=0; d<wdcnt; d++)
								data[d+3] = univTable[handlepx].blockTable[univTable[handlepx].rtidTable[rtid2].blknum].data[d];
							Alter_Message_Px(handlepx, msgid, &data[2]);
							j = ++(univTable[handlepx].blockTable[blknum].numMsgs);
							if (j > MAX_MSGS_PER_BLOCK)
								return uetoomanymsgids;
							univTable[handlepx].blockTable[blknum].msgTable[j].msgid = msgid;
							univTable[handlepx].blockTable[blknum].msgTable[j].pos = 3;
							univTable[handlepx].blockTable[blknum].msgTable[j].wdcnt = wdcnt;
						}
					}
				}

				/* set status, for all messages with active RT that have status val set */
				/* and set vector and bit values for corresponding mode codes, if nec */
				if (msgtype == BC2RT)
				{
					Read_Message_Px(handlepx, msgid, data);
					data[wdcnt+1] = univTable[handlepx].rtTable[rtnum].statusValue;
					Alter_Message_Px(handlepx, msgid, &data[1]);
				}
				else if (msgtype == RT2BC)
				{
					Read_Message_Px(handlepx, msgid, data);
					data[1] = univTable[handlepx].rtTable[rtnum].statusValue;
					Alter_Message_Px(handlepx, msgid, &data[1]);
				}
				else if (msgtype == RT2RT) /* at least one side is active */
				{
					Read_Message_Px(handlepx, msgid, data);
					if (rtnum2 != NULL_RT) /*rt2rt_xmt */
						data[2] = univTable[handlepx].rtTable[rtnum2].statusValue;
					if (rtnum != NULL_RT) /*rt2rt_rcv */
						data[wdcnt+3] = univTable[handlepx].rtTable[rtnum].statusValue;
					Alter_Message_Px(handlepx, msgid, &data[2]);
				}
				else if (msgtype == MODE)
				{
					modecode = (usint)(command & 0x1f);
					if (modecode == RECEIVE_MODECODE)
					{
						Read_Message_Px(handlepx, msgid, data);
						data[2] = univTable[handlepx].rtTable[rtnum].statusValue;
						Alter_Message_Px(handlepx, msgid, &data[1]);
					}
					else
					{
						Read_Message_Px(handlepx, msgid, data);
						data[1] = univTable[handlepx].rtTable[rtnum].statusValue;

						/* alter vector and bit value for those mode codes */
						if ((univTable[handlepx].rtTable[rtnum].valsSet & VECTOR_VAL_SET) &&
							(modecode == VECTOR_MODECODE))
							data[2] = univTable[handlepx].rtTable[rtnum].vectorValue;
						else if ((univTable[handlepx].rtTable[rtnum].valsSet & BIT_VAL_SET) &&
							(modecode == BIT_MODECODE))
							data[2]= univTable[handlepx].rtTable[rtnum].bitValue;

						Alter_Message_Px(handlepx, msgid, &data[1]);
					}
				}

				/* set error injection */
				if ((univTable[handlepx].bitcountErrinj) || (univTable[handlepx].parityErrinj))
				{
					Set_Error_Location_Px(handlepx, 0, 0);
					flags = 0;
					if (univTable[handlepx].parityErrinj) flags = PARITY_ERR_PX | DATA_ERR ;
					if (univTable[handlepx].bitcountErrinj) flags |= BIT_CNT_ERR;
					Insert_Msg_Err_Px(handlepx, msgid, flags);
				}
		}/* end 'for msgentry' */
	} /* end 'for frameid */
	return Run_BC_Px(handlepx, bcrunflag);
}

/* READING DATA */

#define RT_BITS_TO_ZERO_OUT 0x1c50
#define STAT_SAME_BITS 0xe1af
#define RT_TX_TIMEOUT_BIT 0x0400
#define BC_INVALID_MSG_BIT 0x0400
#define BC_INVALID_MSG_BIT_SHIFT 10
#define RT_INVALID_MSG_BIT 0x0100
#define RT_INVALID_MSG_BIT_SHIFT 8
#define BC_BITS_TO_ZERO_OUT 0x1E50 /* 4,6,9, 10-12 */
#define RT_ACTIVE_BIT 1
int borland_dll Get_Next_RT_Message_UNIV_Px (int handlepx, struct CMDENTRYRT *cmdstruct)
{
	int retval, i, init_nextMONentry;
	usint *blkptr, msgstat, command, rt2rtcommand = 0, rtnum, concmsgStatus;
	unsigned int timetag;
	int endofmsgset, rtactive;

	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (cardpx[handlepx].univmode == UNIV_RT_MODE)
	{
		retval = Get_Next_RT_Message_Px (handlepx, cmdstruct);
		if (retval >= 0)
			cmdstruct->status &= (WORD)(~RT_BITS_TO_ZERO_OUT);
		return retval;
	}

	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	/* handle wraparound of buffer */
	univTable[handlepx].next_MON_entry %= cardpx[handlepx].cmon_stack_size;
	init_nextMONentry = univTable[handlepx].next_MON_entry;

	while(1)
	{

		i  = univTable[handlepx].next_MON_entry;

		blkptr = (usint *) &(cardpx[handlepx].cmon_stack[i]);
		cmdstruct->timetaglo = *(blkptr+1);
		cmdstruct->timetaghi = *(blkptr+2);
		timetag = (unsigned int)((cmdstruct->timetaghi) << 16) | cmdstruct->timetaglo;

		if (univTable[handlepx].readvalTable[i].timetag == timetag)
			return enomsg;

		msgstat = *blkptr;
		endofmsgset = ((msgstat & END_OF_MSG) == END_OF_MSG);

		if (timetag < univTable[handlepx].lastmsgttag)
		{
			if (univTable[handlepx].lastmsgttag - timetag < 0x10000000)
				return eoverrunTtag; /* not that pointer is not updated */
			/* otherwise, it means that the timetag reached its maximum value and wrapped to 0*/
		}
		univTable[handlepx].lastmsgttag = timetag;

		command = *(blkptr+3);
		rtnum = (command & 0xf800)>>11;
		rtactive = TRUE;
#ifdef VME4000
		if ((cardpx[handlepx].exc_bd_bc->active[rtnum^1] & RT_ACTIVE_BIT) != RT_ACTIVE_BIT) 
#else
		if ((cardpx[handlepx].exc_bd_bc->active[rtnum] & RT_ACTIVE_BIT) != RT_ACTIVE_BIT) 
#endif
		{
			rtactive = FALSE;
		}
		/* try to see if rt2rt with other side active */
		if (msgstat & RT2RT_MSG_CONCM)
		{
			rt2rtcommand = *(blkptr+4);
			rtnum = (rt2rtcommand & 0xf800) >>11;
#ifdef VME4000
			if ((cardpx[handlepx].exc_bd_bc->active[rtnum^1] & RT_ACTIVE_BIT) == RT_ACTIVE_BIT)
#else
			if ((cardpx[handlepx].exc_bd_bc->active[rtnum] & RT_ACTIVE_BIT) == RT_ACTIVE_BIT)
#endif
				rtactive = TRUE;
		}
		if (rtactive)
			break;
		else
		{
			univTable[handlepx].next_MON_entry++;
			univTable[handlepx].next_MON_entry %= cardpx[handlepx].cmon_stack_size;
			if (univTable[handlepx].next_MON_entry == init_nextMONentry)
				return enomsg;
			else
			{
				/* mark it read */
				univTable[handlepx].readvalTable[univTable[handlepx].next_MON_entry].timetag = timetag;
				univTable[handlepx].next_MON_entry++;
				/* continue is implicit here */
			}
		}
	}/* end of while */
	cmdstruct->command = command;

	if (msgstat & RT2RT_MSG_CONCM)
		cmdstruct->command2 = rt2rtcommand; /* for RT to RT message add extra command */
	else
		cmdstruct->command2 = 0;

	cmdstruct->status = msgstat;
	/* added this, to check for overrun */
	if (endofmsgset && ((cmdstruct->status & END_OF_MSG) != END_OF_MSG))
		return eoverrunEOMflag;

	univTable[handlepx].readvalTable[univTable[handlepx].next_MON_entry].timetag = timetag;
	univTable[handlepx].next_MON_entry++;

	/* convert status word of bcrtconcmon to format of rt */
	concmsgStatus = (WORD)(cmdstruct->status & STAT_SAME_BITS); /* 0xe1af */
	/* OR wc hi 6 and wc lo 5 together as WC error 5 */
	concmsgStatus |=(WORD)((GETBIT(cmdstruct->status, BC_WCH_BIT_SHIFT))<< (RT_WCE_BIT_SHIFT));
	/* zero out 6 and 10 */
	concmsgStatus &= (WORD)(~BC_BITS_TO_ZERO_OUT); /* 0x0440 */

	cmdstruct->status = concmsgStatus;

	return (univTable[handlepx].next_MON_entry - 1);
}

/* MONITOR MODE */

/*
In BC_MODE this routine calls Get_Next_Message_BCM_Px. The status words are
adjusted so that a common status word is available in both BC and RT modes. (like rt mode, without tx timeout)
*/
#define CMON_SAME_BITS 0xFABF
#define RT_TX_TIMEOUT_BIT 0x0400
#define BC_INVALID_MSG_BIT 0x0400
#define RT_INVALID_MSG_BIT 0x0100
#define CMON_BITS_TO_ZERO_OUT 0x0440

int borland_dll Get_Next_Mon_Message_UNIV_Px (int handlepx, struct MONMSG *msgptr)
{
	int retval;
	WORD concmsgStatus, msg_addr;

	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((cardpx[handlepx].univmode == UNIV_RT_MODE) || (cardpx[handlepx].univmode == UNIV_MON_BLOCK))
	{
		retval = Get_Next_Message_RTM_Px (handlepx, msgptr, &msg_addr);
		if (retval < 0)
			return retval;
		msgptr->msgstatus &= (WORD)(~RT_TX_TIMEOUT_BIT); /* 0x0400 */
		return 0;
	}
	else if (cardpx[handlepx].univmode != UNIV_BC_RT_MODE)
		return (uemode);

	retval = Get_Next_Message_BCM_Px(handlepx, msgptr, &msg_addr);
	if (retval < 0)
		return retval;
	/* convert status word of bcrtconcmon to format of rtconcmon */
	concmsgStatus = (WORD)(msgptr->msgstatus & CMON_SAME_BITS); /* 0xfabf */
	/* OR wc hi and wc lo together as WC error  - zero out 6*/
	concmsgStatus |=(WORD)(GETBIT(msgptr->msgstatus, BC_WCH_BIT_SHIFT)<<RT_WCE_BIT_SHIFT);
	/* move bit 10 to 8, and zero out 10 */
	concmsgStatus |=(WORD)(GETBIT(msgptr->msgstatus, BC_INVALID_MSG_BIT_SHIFT)<<RT_INVALID_MSG_BIT_SHIFT);
	concmsgStatus &= (WORD)(~CMON_BITS_TO_ZERO_OUT);
	msgptr->msgstatus = concmsgStatus;
	return 0;
}

