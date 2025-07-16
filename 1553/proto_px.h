#ifndef __Px_PROTO_H
#define __Px_PROTO_H

#ifndef __EXC_DEF_PX_H
#include "excdef_px.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef EXC_LYNXOS
#include "exclinuxdef.h"
#endif

#ifdef __BORLANDC__
#include <windows.h>
#endif

#ifdef  _WIN32
#define borland_dll __declspec(dllexport)
#else
#define borland_dll 
#ifndef min
#define min(a, b)	((a) < (b)? (a): (b))
#endif
#ifndef __cplusplus
#ifndef max
#define max(a, b)	((a) > (b)? (a): (b))
#endif
#endif
#include "exclinuxdef.h"

#endif

#ifdef VXWORKSP
#include "exc4000vxp.h"
#include "excwindef.h"
#endif

typedef unsigned short int usint;
typedef unsigned int uint;
typedef unsigned char uchar;

#include "error_px.h"
#include "error_devio.h"
#include "flags_px.h"
#include "instance_px.h"
#include "pxincl.h"

#ifdef PXVISA
#define NIVISA_PXI  
#include "visa.h"
#endif
#include "bcrun.h"
#include "rtrun.h"
#include "monseq.h"
#include "dwnld.h"

#ifndef MMSI
#ifndef VXWORKSP
#include "univrun.h"
#endif
#endif

/* function prototypes */
#ifdef __cplusplus
extern "C" {
#endif

/* BCFRAME.C */
int borland_dll Create_Frame_Px (int handlepx, struct FRAME * framestruct);
int borland_dll Start_Frame_Px(int handlepx, int frameid, int nummsgs);
int borland_dll Select_Async_Frame_Px(int handlepx, int frameid, int nummsgs);
int borland_dll Send_Async_Frame_Px(int handlepx);
int borland_dll Alter_IMG_Px(int handlepx, int frameid, int msgentry, unsigned long img);
int borland_dll Alter_MsgSendTime_Px(int handlepx, int frameid, int msgentry, unsigned int mst);
int borland_dll Set_Interrupt_On_Msg_Px (int handlepx, int frameid, int msgentry, int enable);
int borland_dll Set_Interrupt_On_Msg_MMSI_Px (int handlepx, int id, int enable);
int borland_dll Enable_Checksum_Px (int handlepx, int frameid, int msgentry, int enable);
int borland_dll Enable_Checksum_Error_Px (int handlepx, int frameid, int msgentry, int enable);
int borland_dll Enable_Checksum_MMSI_Px(int handlepx, int id, int enable);
int borland_dll Enable_Checksum_Error_MMSI_Px(int handlepx, int id, int enable);

/* BCGET.C */
int borland_dll Get_BC_Status_Px (int handlepx);
int borland_dll Get_Msg_Status_Px (int handlepx, int id);
int borland_dll Get_Minor_Frame_Time_Px (int handlepx, long *frame_time);
int borland_dll Get_Msgentry_Status_Px (int handlepx, int frameid, int msgentry, usint *msgstatusword);
int borland_dll Get_Instruction_Counter_Px (int handlepx, usint *instructionCounter);

/* BCRTMSG.C */
int borland_dll BC_Check_Alter_Msgentry_Px (int handlepx, int frameid, int msgentry);
int borland_dll BC_Check_Alter_Msg_Px (int handlepx, int id);
int borland_dll Alter_Message_Px (int handlepx, int id, usint *data);
int borland_dll Alter_Cmd_Px (int handlepx, int id, usint rt, usint sa);
int borland_dll Read_Message_Px (int handlepx, int id, usint *data);
int borland_dll Create_1553_Message_Px (int handlepx, usint cmdtype, usint data[], short int *handle);
int borland_dll Get_BC_Msgentry_Px (int handlepx, int frameid, int msgentry, struct BCMSG *msgptr);
int borland_dll Get_BC_Message_Px (int handlepx, int id, struct BCMSG *msgptr);
int borland_dll Get_Next_Message_BCM_Px (int handlepx, struct MONMSG *msgptr, usint * msg_addr);
int borland_dll Re_Create_Message_Px(int handlepx, usint cmdtype, usint data[], short int id);
int borland_dll Command_Word_15EP(int rtnum, int type, int subaddr, int wordcount, usint *commandword);
int borland_dll Command_Word_Px(int rtnum, int type, int subaddr, int wordcount, usint *commandword);
int borland_dll Read_SRQ_Message_Px (int handlepx, usint *vector_status, usint *msg_status, usint *data, usint *srq_counter);

/* BCSET.C */
int borland_dll Insert_Msg_Err_Px (int handlepx, int id, int flags);
int borland_dll Reset_BC_Status_Px (int handlepx);
int borland_dll Run_BC_Px (int handlepx, int type);
int borland_dll Set_BC_Resp_Px (int handlepx, int time) ;
int borland_dll Set_Bit_Cnt_Px (int handlepx, int offset);
int borland_dll Set_Bus_Px (int handlepx, int id, int bus);
int borland_dll Set_Continue_Px (int handlepx, int id);
int borland_dll Set_Frame_Time_Px (int handlepx, long time);
int borland_dll Set_Halt_Px (int handlepx, int id);
int borland_dll Set_Interrupt_Px (int handlepx, int flag);
int borland_dll Set_Jump_Px (int handlepx, int id, int frameid, int intcnt);
int borland_dll Set_Retry_Px (int handlepx, int id, int num, int altchan);
int borland_dll Set_Stop_On_Error_Px (int handlepx, int id);
int borland_dll Set_Var_Amp_Px (int handlepx, int millivolts);
int borland_dll Set_Word_Cnt_Px (int handlepx, int offset);
int borland_dll Clear_Card_Px (int handlepx);
int borland_dll Clear_Frame_Px (int handlepx);
int borland_dll Set_Skip_Px (int handlepx, int id);
int borland_dll Set_Restore_Px (int handlepx, int id, int msgtype);
int borland_dll Send_Msg_Once_Px (int handlepx, int id, int msgtype);
int borland_dll Set_Minor_Frame_Time_Px (int handlepx, long micro_sec);
int borland_dll Set_Replay_Px(int handlepx, int flag);
int borland_dll Send_Timetag_Sync_Px(int handlepx, int flag);
int borland_dll Enable_SRQ_Support_Px (int handlepx, int enableflag);
int borland_dll Set_Frequency_Mode_Px (int handlepx, int enableflag);
int borland_dll Set_Sync_Pattern_Px (int handlepx, int pattern);
int borland_dll Set_Error_Location_Px (int handlepx, unsigned int errorWord, unsigned int zcerrorBit);
int borland_dll Set_Zero_Cross_Px (int handlepx, unsigned int zctype);
int borland_dll Set_IMG_On_Skip_Px (int handlepx, int enableflag);

/* ERROR.C */
int borland_dll Get_Error_String_Px(int errcode, char *errstring);
borland_dll char * Print_Error_Px (int errorcode);

/* GGET.C */
int borland_dll Get_Board_Status_Px (int handlepx) ;
int borland_dll Get_Id_Px (int handlepx);
int borland_dll Get_Mode_Px (int handlepx);
int borland_dll Get_RT_Info_Px (int handlepx, char *pRtNum, char *pRtLock);
int borland_dll Get_Rev_Level_Px (int handlepx);
int borland_dll Read_Start_Reg_Px (int handlepx, usint *startval);
int borland_dll Get_Card_Type_Px (int handlepx, usint *cardtype);
int borland_dll Get_Board_Options_Lo_Px (int handlepx, int *cardopt);
int borland_dll Get_Board_Options_Hi_Px (int handlepx, int *cardopt);
int borland_dll Get_Header_Exists_Px (int handlepx, int sa, int *enable);
int borland_dll Get_Header_Value_Px (int handlepx, int sa, int direction, usint *header_value);
int borland_dll Internal_Loopback_Px(int handlepx, struct i_loopback *ilbvals);
int borland_dll External_Loopback_Px(int handlepx, struct e_loopback *elbvals);
int borland_dll External_Loopback_MMSI_Px(int handlepx, struct e_loopback_MMSI *elbvals);
int borland_dll OnBoard_Loopback_Px(int handlepx, struct e_loopback *oblbvals);
int borland_dll Get_SerialNumber_Px (int handlepx, usint * serialNum);
int borland_dll Get_ModuleTime_Px (int handlepx, unsigned int * moduleTime);
void borland_dll Parse_CommandWord_Px (usint cmdWord, usint *RT, usint *SA, usint *direction, usint *wordCount, usint *RTid);
int borland_dll Get_Next_Mon_Message_Px (int handlepx, struct MONMSG *msgptr, unsigned long *msg_counter);
int borland_dll Get_MsgArrayPtr_Px(int handlepx, usint **msgarrayptr);
int borland_dll Get_UseDmaIfAvailable_Px(int handlepx, int * pUseDmaFlag);
int borland_dll Get_Message_Count_Px (int handlepx, unsigned int *msgCount);
int borland_dll Is_Expanded_Mode_Supported_Px(int handlepx, int *isFeatureSupported);
int borland_dll Is_IrigSignal_Present_Px (int handlepx, unsigned short *isIrigSignalPresent);
int borland_dll Get_ModuleIrigTime_BCD_Px (int handlepx, unsigned short *moduleTime);
int borland_dll Get_ModuleIrigTime_Px (int handlepx, t_IrigOnModuleTime *IrigTime);
int borland_dll Convert_IrigTimetag_BCD_to_Decimal_Px (unsigned short int *moduleTime, t_IrigOnModuleTime *IrigTime);
unsigned int BuildBCD_Px(unsigned int binaryValue, unsigned int bitOffset, unsigned int numDigits);
int borland_dll Preset_IrigTimeTag_Px (int handlepx, usint Day, usint Hour, usint Minute, usint Second);
int BCDtoBinary_Px(unsigned int BCDvalue);
void borland_dll PrintIRIGtime_BCD_Px(unsigned short int ttag1, unsigned short int ttag2, unsigned short int ttag3, unsigned short int ttag4, char * outString);
void borland_dll PrintIRIGtime_Px(t_IrigOnModuleTime *IrigTime, char *outString);
int borland_dll Get_Num_Ports_MMSI_Px (int handlepx, unsigned char *numPorts);

#ifdef DMA_SUPPORTED
/* not exported */
int Get_DmaMonBlockCopy_Px(int handlepx, usint * pDpram, usint ** ppDmaCopy, int * pDmaCopyIsValid);
int Get_DmaMonIRIGBlockCopy_Px(int handlepx, usint * pDpram, usint ** ppDmaCopy, int * pDmaCopyIsValid);
#endif

/* GSET.C */
int borland_dll Restart_Px (int handlepx);
int borland_dll Set_Mode_Px (int handlepx, int mode);
int borland_dll Stop_Px (int handlepx);
int borland_dll Set_Header_Exists_Px (int handlepx, int sa, int enable);
int borland_dll Set_Header_Value_Px (int handlepx, int sa, int direction, usint header_value);
int borland_dll Enable_1553A_Support_Px (int handlepx, int enableflag);
int borland_dll Set_Timetag_Res_Px(int handlepx, usint resolution);
int borland_dll Clear_Timetag_Sync_Px(int handlepx, int modecode, int flag);
int borland_dll Set_Hubmode_MMSI_Px(int handlepx, int hubmode);
int borland_dll Ignore_Timetag_Overrun_Px(int handlepx, int enableflag);
int borland_dll Set_ModuleTime_Px (int handlepx, unsigned int moduleTime);
int borland_dll Set_IRIG_TimeTag_Mode_Px (int handlepx, int flag);
int borland_dll Set_Expanded_Block_Mode_Px (int handlepx, int flag);
int borland_dll Set_UseDmaIfAvailable_Px(int handlepx, int useDmaFlag);

/* INITCARD.C */
#ifdef PXVISA
int borland_dll Init_Module_Px (char *rsrcname, usint modnum); 
#else 
int borland_dll Init_Module_Px (usint devnum, usint modnum);
int borland_dll Attach_Module_Px (usint devnum, usint modnum);
#endif

int borland_dll Release_Module_Px(int handlepx);

unsigned int borland_dll Get_Time_Tag_Px (int handlepx);
int borland_dll GetTimeTag_Px(int handlepx, unsigned int *Time_Tag);
int borland_dll Reset_Time_Tag_Px (int handlepx);
int borland_dll Set_RT_Broadcast_Px(int handlepx, int toggle);
int borland_dll DelayMicroSec_Px (int handlepx, unsigned int delayMicroSec);
int borland_dll Get_Instr_Px(int handlepx, unsigned long int *instr);
int borland_dll Get_DmaAvailable_Px(int handlepx, int * pDmaEnabled);

int borland_dll Get_PtrModuleInstance_Px(int handlepx, struct INSTANCE_PX ** ppModuleInstance);

/* for PCMCIA/Px only */
int borland_dll Host_Reset_PCMCIA_Px(int handlepx);
int borland_dll  Get_PCMCIA_HWInterface_Rev_Px(int handlertx, usint *hwInterfaceRev);
int borland_dll Global_Ttag_Reset_PCMCIA_Px(int handlepx);

/* for internal use only */
int Reset_Card_Px(int handlepx);
#ifdef _WIN32
int StartTimer_Px(LARGE_INTEGER *lStartTime);
int EndTimer_Px(LARGE_INTEGER lStartTime, unsigned int MsDelay);
#endif

/* blockRead.c */
/* for internal use only */
int Read_RT_Block_Px(int handle, unsigned char **userbuffer, usint entrySize, int bufferStart, int bufferEnd, int *nummsgs,  int *overwritten);
int myMemCopy( void *destPointer, void *srcPointer, int count, int is32BitAccess);
int PerformLongDMARead(int nDevice, int nModule, char *pBuffer, unsigned long dwLengthInBytes, char *pAddressOnCard);

/* MON.C */
int borland_dll Clear_Msg_Blks_Px (int handlepx);
int borland_dll Get_Message_Px (int handlepx, int blknum, struct MONMSG *msgptr);
int borland_dll Get_MON_Status_Px (int handlepx);
int borland_dll Run_MON_Px (int handlepx);
int borland_dll Set_Broad_Ctl_Px (int handlepx, int flag);
int borland_dll Set_MON_Concurrent_Px (int handlepx, int flag);
int borland_dll Run_LL_0018_Px(int handlepx);
int borland_dll Get_0018_Message_Px (int handlepx, struct MONMSG *msgptr, int *numwords);
int borland_dll Create_0018_Word_Px(int handlepx, usint cmdword, short int *id);
int borland_dll Set_Mon_Response_Time_Px(int handlepx, usint rtime);
int borland_dll Set_Mon_Response_Time_1553A_Px(int handlepx, usint rtime);
int  Read_Monitor_Block_Px(int handlepx, unsigned char **userbuffer, usint entrySize, uint *nummsgs,  int *overwritten, uint endLastBlockOffset);

/* MONLKUP.C */
int borland_dll Assign_Blk_Px (int handlepx, int rtid, int blknum);
int borland_dll Get_Last_Blknum_Px (int handlepx);
int borland_dll Enable_Lkup_Int_Px (int handlepx, int rtid, int toggle);

/* MONSEQ.C */
int borland_dll Get_Counter_Px (int handlepx);
int borland_dll Set_Cnt_Trig_Px (int handlepx, int blknum);
int borland_dll Set_Trigger1_Px (int handlepx, int trigger, int mask);
int borland_dll Set_Trigger2_Px (int handlepx, int trigger, int mask);
int borland_dll Set_Trigger_Mode_Px (int handlepx, int mode);
int borland_dll Get_Next_Message_New_Px (int handlepx, struct MONMSG *msgptr, int *overwritten);
int borland_dll Get_Next_Message_Old_Px (int handlepx, struct MONMSG *msgptr);
int borland_dll Get_Next_Message_Px (int handlepx, struct MONMSG *msgptr);
int borland_dll Get_Next_Message_32Bit_Px (int handlepx, struct MONMSG *msgptr);
int borland_dll Set_Message_Interval_Interrupt_Value_Px(int handlepx, usint interval);
int borland_dll Get_Message_Info_Px (int handlepx, int blknum, struct MONMSGINFO *msgInfoPtr);
int borland_dll Set_Enhanced_Mon_Px (int handlepx, int enableFlag);
int borland_dll Is_Enhanced_Mon_Supported_Px(int handlepx, int *isFeatureSupported);
int borland_dll Enable_Mon_1553A_Support_Px (int handlepx, int rtNum, int enableflag);
int borland_dll Get_Next_Mon_IRIG_Message_Px (int handlepx, struct MONIRIGMSG *msgptr, unsigned long *msg_counter);
int borland_dll Get_Next_Mon_IRIGttag_Message_Px (int handlepx, struct MONIRIGMSG_TTAG *msgptr, unsigned long *msg_counter);

/* PEEKNPOKE.C */
int borland_dll Exc_Peek_Px (int handlepx, usint offset, usint * data);
int borland_dll Exc_Poke_Px (int handlepx, usint offset, usint *data);
int borland_dll Exc_Peek_WORD_Px (int handlepx, usint offset, usint * data);
int borland_dll Exc_Poke_WORD_Px (int handlepx, usint offset, usint *data);
int borland_dll Exc_Peek_BYTE_Px (int handlepx, usint offset, BYTE *data);
int borland_dll Exc_Poke_BYTE_Px (int handlepx, usint offset, BYTE *data);
int borland_dll Exc_Peek_32Bit_Px (int handlepx, usint offset, unsigned int *data);
int borland_dll Exc_Poke_32Bit_Px (int handlepx, usint offset, unsigned int *data);
int borland_dll Exc_Peek_Word_Buffer_Px (int handlepx, usint offset, BYTE * buffer, usint size);
int borland_dll Exc_Poke_Word_Buffer_Px (int handlepx, usint offset, BYTE * buffer, usint size);

/* RTSET.C */
int borland_dll Get_RT_Stack_Entry_Count_Px (int handlepx, unsigned int *entryCount);
int borland_dll Assign_RT_Data_Px (int handlepx, int rtid, int blknum);
int borland_dll Load_Datablk_Px (int handlepx, int blknum, usint *words);
int borland_dll Load_Multiple_Datablk_Px (int handlepx, int blknum, int numOfBlocks, usint *words);
int borland_dll Read_Datablk_Px (int handlepx, int blknum, usint *words);
int borland_dll Read_RT_Status_Px (int handlepx);
int borland_dll Reset_RT_Interrupt_Px (int handlepx);
int borland_dll Run_RT_Px (int handlepx);
int borland_dll Set_Bit_Px (int handlepx, int rtnum, int bitvalue);
int borland_dll Set_Invalid_Data_Res_Px (int handlepx, int flag);
int borland_dll Set_Mode_Addr_Px (int handlepx, int flag);
int borland_dll Set_RT_Active_Px (int handlepx, int rtnum, int intrpt);
int borland_dll Set_Broad_Interrupt_Px (int handlepx, int intrpt);
int borland_dll Set_RT_Errors_Px (int handlepx, int errormask);
int borland_dll Set_RT_Interrupt_Px (int handlepx);
int borland_dll Set_RT_Nonactive_Px (int handlepx, int rtnum);
int borland_dll Set_RT_Resp_Time_Px (int handlepx, int nsecs);
int borland_dll Set_1553Status_Px (int handlepx, int rtnum, int statusvalue, int durationflag);
int borland_dll Set_Status_Px (int handlepx, int rtnum, int statusvalue);
int borland_dll Set_Vector_Px (int handlepx, int rtnum, int vecvalue);
int borland_dll Set_Wd_Cnt_Err_Px (int handlepx, int rtnum, int offset);
int borland_dll Get_RT_Message_Px (int handlepx, struct CMDENTRY *cmdstruct);
int borland_dll Get_Next_RT_Message_Px (int handlepx, struct CMDENTRYRT *cmdstruct);
int borland_dll Get_Next_RT_Message_With_Counter_Px (int handlepx, struct CMDENTRYRT *cmdstruct);
int borland_dll Get_Next_RT_Message_With_Status_Px (int handlepx, struct CMDENTRYRT *cmdstruct);
int borland_dll Set_Both_RT_Stacks_Px (int handlepx, int enableFlag);
int borland_dll Get_Blknum_Px (int handlepx, int rtid);
int borland_dll Get_Next_Message_RTM_Px (int handlepx, struct MONMSG *msgptr, usint * msg_addr);
int borland_dll RT_Id_Px(int rtnum, int type, int subaddr, int *rtid);
int borland_dll SA_Id_Px(int type, int subaddr, int wordcount, int *said);
int borland_dll Set_RT_Active_Bus_Px (int handlepx, int rtnum, usint bus);
int borland_dll Set_RTid_Interrupt_Px (int handlepx, int rtid, int enabled_flag, int int_type);
int borland_dll Set_RTid_Status_Px (int handlepx, int rtid, int enabled_flag, int status_type);
int borland_dll Set_SAid_Illegal_Px (int handlepx, int said, int enabled_flag, int broadcast_flag);
int borland_dll Assign_DB_Datablk_Px (int handlepx, int rtid, int enabled_flag, int blknum);
int borland_dll Read_RTid_Px (int handlepx, int rtid, usint *words);
int borland_dll Load_RTid_Px (int handlepx, int rtid, usint *words);
int borland_dll Get_RT_Sync_Info_Px(int handlepx, int rtnum, unsigned int *sync_timetag, usint *sync_data);
int borland_dll Get_RT_Sync_Entry_Px(int handlepx, int rtnum, usint *sync_timetag, usint *sync_data);
int borland_dll Clear_RT_Sync_Entry_Px(int handlepx, int rtnum);
int borland_dll RT_Links_Control_MMSI_Px(int handlepx, int enableflag, usint rtlinks);
int borland_dll Set_Checksum_Blocks_Px (int handlepx, int csum_blocks);
int borland_dll Get_Checksum_Blocks_Px (int handlepx);

int borland_dll Set_RTid_Multibuf_Px (int handlepx, int rtid, int numbufs);
int borland_dll Reset_RTid_Multibuf_Px (int handlepx, int rtid);
int borland_dll Get_Multibuf_Nextbuffer_Px (int handlepx, int rtid, int *nextbuf);

/* for internal use only */
int Validate_Block_Number(int handlepx, int blknum);
int Set_RT_In_Reg(int handlepx, int rtValue);
int Update_RT_From_Reg(int handlepx);

#ifndef VME4000
#ifndef EXC_LYNXOS
#ifndef VXWORKSP
/* DEVICEIO_EP.C */
int borland_dll Wait_For_Interrupt_15EP(int g_handlepx);
int borland_dll Wait_For_Interrupt_Px(int handlepx, unsigned int timeout);
int borland_dll Get_Interrupt_Count_Px(int handlepx, unsigned int*);
int borland_dll Wait_For_Multiple_Interrupts_Px(int numints, int *brd_array, unsigned int timeout, unsigned long *Interrupt_Bitfield);
int borland_dll InitializeInterrupt_Px(int handlepx);
int borland_dll Request_Interrupt_Notification_Px (int handlepx, HANDLE hEvent);
int borland_dll Cancel_Interrupt_Notification_Px(int handlepx);
/*For backward compatibility: */
int borland_dll Wait_For_Interrupt_Device_Px(int nCurrentDevice);
int borland_dll Get_Interrupt_Count_Device_Px(int nCurrentDevice, unsigned int *Sys_Interrupts_Ptr);
#endif /* not VXWORKSP */
#endif /* not EXC_LYNXOS */
#endif /* not vmepx */

#ifdef VXWORKSP
int borland_dll SetupInterruptHandler_Px(int handlepx, void * funcname);
int borland_dll ClearInterruptHandler_Px(int handlepx);
#endif

#if defined (__Lynx__) || defined (__linux__)
/* kbhit.c */
int borland_dll kbhit();
int borland_dll _kbhit();
#endif
#ifdef __cplusplus
}
#endif

#endif
