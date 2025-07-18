#ifndef __Px_PROTO_BACK_H
#define __Px_PROTO_BACK_H

/* We need these for places where this is included before these structs are defined: */
struct MONMSG;
struct CMDENTRYRT;
struct CMDENTRY;
/* function prototypes */
#ifdef __cplusplus
extern "C" {
#endif

	/* BCFRAME.C */
	int borland_dll Create_Frame(struct FRAME * framestruct);
	int borland_dll Start_Frame(int frameid, int nummsgs);
	int borland_dll Select_Async_Frame(int frameid, int nummsgs);
	int borland_dll Send_Async_Frame(void);
	int borland_dll Alter_IMG(int frameid, int msgentry, unsigned long img);
	int borland_dll Alter_MsgSendTime(int frameid, int msgentry, unsigned int mst);
	int borland_dll Set_Interrupt_On_Msg (int frameid, int msgentry, int enable);
	int borland_dll Enable_Checksum(int frameid, int msgentry, int enable);
	int borland_dll Enable_Checksum_Error(int frameid, int msgentry, int enable);
	int borland_dll Set_Checksum_Blocks(int csum_blocks);
	int borland_dll Get_Checksum_Blocks(void);

	/* BCGET.C */
	int borland_dll Get_BC_Status (void);
	int borland_dll Get_Msg_Status (int id);
	int borland_dll Get_Minor_Frame_Time (long *frame_time);
	int borland_dll Get_Msgentry_Status (int frameid, int msgentry, usint *msgstatusword);

	/* BCRTMSG.C */
	int borland_dll BC_Check_Alter_Msgentry (int frameid, int msgentry);
	int borland_dll BC_Check_Alter_Msg (int id);
	int borland_dll Alter_Message (int id, usint *data);
	int borland_dll Alter_Cmd (int id, usint rt, usint sa);
	int borland_dll Read_Message (int id, usint *data);
	int borland_dll Create_1553_Message (usint cmdtype, usint data[], short int *handle);
	int borland_dll Get_BC_Msgentry (int frameid, int msgentry, struct BCMSG *msgptr);
	int borland_dll Get_BC_Message (int id, struct BCMSG *msgptr);
	int borland_dll Get_Next_Message_BCM (struct MONMSG *msgptr, usint * msg_addr);
	int borland_dll Re_Create_Message (usint cmdtype, usint data[], short int id);

	/* BCSET.C */
	int borland_dll Insert_Msg_Err (int id, int flags);
	int borland_dll Reset_BC_Status (void);
	int borland_dll Run_BC (int type);
	int borland_dll Set_BC_Resp (int time) ;
	int borland_dll Set_Bit_Cnt (int offset);
	int borland_dll Set_Channel (int id, int bus);
	int borland_dll Set_Bus (int id, int bus);
	int borland_dll Set_Continue (int id);
	int borland_dll Set_Frame_Time (long time);
	int borland_dll Set_Halt (int id);
	int borland_dll Set_Interrupt (int flag);
	int borland_dll Set_Jump (int id, int frameid, int intcnt);
	int borland_dll Set_Retry (int id, int num, int altchan);
	int borland_dll Set_Stop_On_Error (int id);
	int borland_dll Set_Var_Amp (int millivolts);
	int borland_dll Set_Word_Cnt (int offset);
	int borland_dll Clear_Card (void);
	int borland_dll Clear_Frame (void);
	int borland_dll Set_Skip (int id);
	int borland_dll Set_Restore (int id, int msgtype);
	int borland_dll Set_Minor_Frame_Time (long micro_sec);
	int borland_dll Set_Replay(int flag);
	int borland_dll Clear_Timetag_Sync(int modecode, int flag);
	int borland_dll Send_Timetag_Sync(int flag);

	/* ERROR.C */
	borland_dll char * Print_Error (int errorcode);

	/* GGET.C */
	int borland_dll Get_Board_Status (void) ;
	int borland_dll Get_Id (void);
	int borland_dll Get_Mode (void);
	int borland_dll Get_RT_Info (char *pRtNum, char *pRtLock);
	int borland_dll Get_Rev_Level (void);
	int borland_dll Read_Start_Reg(usint *startval);
	int borland_dll Get_Card_Type (usint *cardtype);
	int borland_dll Get_Board_Options_Lo (int *cardopt);
	int borland_dll Get_Board_Options_Hi (int *cardopt);
	int borland_dll Get_Header_Exists (int sa, int *enable);
	int borland_dll Get_Header_Value (int sa, int direction, usint *header_value);
	int borland_dll Internal_Loopback(struct i_loopback *ilbvals);
	int borland_dll External_Loopback(struct e_loopback *elbvals);

	/* GSET.C */
	int borland_dll Restart (void);
	int borland_dll Set_Mode (int mode);
	int borland_dll Stop (void);
	int borland_dll Set_Header_Exists (int sa, int enable);
	int borland_dll Set_Header_Value (int sa, int direction, usint header_value);
	int borland_dll Enable_1553A_Support (int enableflag);
	int borland_dll Set_Timetag_Res(usint resolution);

	/* INITCARD.C */
	/*These are on 4000PCI only, for backward compatiblity */
	int borland_dll Init_Card(usint device_num);
	int borland_dll Release_Card (void);
	int borland_dll Select_Module(int mod);
	int borland_dll Get_Module (void);

	unsigned int borland_dll Get_Time_Tag (void);
	int borland_dll Reset_Time_Tag (void);
	int borland_dll Set_RT_Broadcast(int toggle);

	int borland_dll Exc_Peek_BYTE(usint offset, BYTE *data);

	/* MON.C */
	int borland_dll Clear_Msg_Blks (void);
	int borland_dll Get_Message (int blknum, struct MONMSG *msgptr);
	int borland_dll Get_MON_Status (void);
	int borland_dll Run_MON (void);
	int borland_dll Set_Broad_Ctl (int flag);
	int borland_dll Set_MON_Concurrent(int flag);
	int borland_dll Run_LL_0018(void);
	int borland_dll Get_0018_Message (struct MONMSG *msgptr, int *numwords);
	int borland_dll Create_0018_Word(usint cmdword, short int *handle);
	int borland_dll Set_Mon_Response_Time(usint rtime);

	/* MONLKUP.C */
	int borland_dll Assign_Blk (int rtid, int blknum);
	int borland_dll Get_Last_Blknum(void);
	int borland_dll Enable_Lkup_Int (int rtid, int toggle);

	/* MONSEQ.C */
	int borland_dll Get_Counter (void);
	int borland_dll Set_Cnt_Trig (int blknum);
	int borland_dll Set_Trigger1 (int trigger, int mask);
	int borland_dll Set_Trigger2 (int trigger, int mask);
	int borland_dll Set_Trigger_Mode (int mode);
	int borland_dll Get_Next_Message (struct MONMSG *msgptr);

	/* PEEKNPOKE.C */
	int borland_dll Exc_Peek (usint offset, usint * data);
	int borland_dll Exc_Poke (usint offset, usint *data);

	/* RTSET.C */
	int borland_dll Assign_RT_Data (int rtid, int blknum);
	int borland_dll Load_Datablk (int blknum, usint *words);
	int borland_dll Read_Datablk (int blknum, usint *words);
	int borland_dll Read_RT_Status (void);
	int borland_dll Reset_RT_Interrupt (void);
	int borland_dll Run_RT (void);
	int borland_dll Set_Bit (int rtnum, int bitvalue);
	int borland_dll Set_Invalid_Data_Res (int flag);
	int borland_dll Set_Mode_Addr (int flag);
	int borland_dll Set_RT_Active (int rtnum, int intrpt);
	int borland_dll Set_Broad_Interrupt (int intrpt);
	int borland_dll Set_RT_Errors (int errormask);
	int borland_dll Set_RT_Interrupt (void);
	int borland_dll Set_RT_Nonactive (int rtnum);
	int borland_dll Set_RT_Resp_Time (int nsecs);
	int borland_dll Set_1553Status (int rtnum, int statusvalue, int durationflag);
	int borland_dll Set_Status (int rtnum, int statusvalue);
	int borland_dll Set_Vector (int rtnum, int vecvalue);
	int borland_dll Set_Wd_Cnt_Err (int rtnum, int offset);
	int borland_dll Get_RT_Message (struct CMDENTRY *cmdstruct);
	int borland_dll Get_Next_RT_Message (struct CMDENTRYRT *cmdstruct);
	int borland_dll Get_Blknum (int rtid);
	int borland_dll Get_Next_Message_RTM (struct MONMSG *msgptr, usint * msg_addr);
	int borland_dll Set_RT_Active_Bus (int rtnum, usint bus);
	int borland_dll Assign_DB_Datablk(int rtid, int enabled_flag, int blknum);
	int borland_dll Read_RTid (int rtid, usint *words);
	int borland_dll Set_RTid_Multibuf (int rtid, int numbufs);


#ifdef __cplusplus
}
#endif

#endif
