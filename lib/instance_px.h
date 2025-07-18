#ifndef __PX_INSTANCE_H
#define __PX_INSTANCE_H

#ifdef _WIN32
#include <windows.h>
#else
#include "UNetTiming.h"
#endif // _WIN32
#include <stdio.h>

#ifndef __EXC4000_H_
#include "exc4000.h"
#endif

#ifndef __PX_FLAGS_H
#include "flags_px.h"
#endif

#ifndef __BCRUN_H
#include "bcrun.h"
#endif

#ifndef __MONSEQ_H
#include "monseq.h"
#endif
#define MAXBUFFERSIZE	0x10000	//max buffer size is size of dpram, 
								//it really only needs to be the size of the largest monitor contiguous buffer

#ifdef MMSI
struct s_RcvBufManagement_MMSI {
#else
struct s_RcvBufManagement_PX {
#endif
	struct MONIRIGMSG *pNextIRIGMsgInLclBuffer;
	struct MONMSGPACKED *pNextMsgInLclBuffer;
	unsigned int numMsgsInLclBuffer;
	unsigned char buffer[MAXBUFFERSIZE]; // note:  This must be 32-bit aligned or the memcpy version WILL NOT WORK
};

extern int g_handlepx;
typedef struct INSTANCE_PX
{
	usint					allocated;				/* flag to mark that the current instance of this structure
													   is actively being used to track a module */
	usint					card_addr;				/* for PCI and VME only: the device number of the (carrier) board (eg 25 or 80h) */
	char					devname[50];			/* for visa and rt*/
	int						intnum;					/* interrupt number (for the entire board? - not only this specific module) */
	unsigned long int		instr;					/* vme - this is a handle given to us by the VISA device driver
													   that it created when it opened the session to our VME 4000 card. */
	void					*card_address;			/* vme - the memory address (of the extended user address space for VME )
													   to the base of the card, obtained from the kernel driver (or equiv) */
	WORD					exc4000BrdType;
	t_ModuleInfo			moduleInfo;					// used by Get_Module_Info()
	int						isPCMCIA;				/* flag to indicate that this device/module is a PCMCIA/EP card or not */
	int						isExCARD;
	BOOL					isUNet;						// flag to indicate that the device is a UNet device
	int						remoteDevHandle;			// A UNet-DLL assigned handle
	BOOL					isUnetMultiMsgReadSupported;		/* flag to indicate that the UNet device can read multiple messages in a single read-request */
	BOOL					useUnetMultiMsgReadIfAvailable;		/* flag (also) set in initcard indicating that UNet Multi-Msg-read should be used if available */
	int						isIMG250ns_resolution;		/* flag to indicate for PxII (Intel processor) that we are using 250 nanosec resolution on IMG */
	usint					dmaAvailable;			/* flag set in initcard indicating that DMA access to memory is available */
	usint					useDmaIfAvailable;		/* flag (also) set in initcard indicating that DMA should be used if available */
	struct dmaShadow_Px		dmaBuffer;				/* dma buffer of almost 4k (which holds a max of 51 message blocks cloned from DPRAM) */
	usint					*msgalloc [MSGMAX];		/* array of pointers to the messages in memory, e.g., 0x4000, 4014, 404A, ... */
	usint					commandtype[MSGMAX];
	int						nextid;					/* id of the next message to be created, starts from 1, output parameter of Create_1553_Message  */
	usint					*nextmsg;				/* pointer to location in memory where the next message will be place, base address of module + e.g., 0x4000-4014-... */
	usint					*nextstack;				/* pointer to the next available position on the instruction stack, base address of module + 0x0-8-10-18... */
	struct instruction_stack		*curstack;
	struct framearray		framealloc [MAXFRAMES];
	int						nextframe;				/* next available frame number */
	struct exc_bc			*exc_bd_bc;				/* pointer to the base address of the module from the BC's perspective */
	struct exc_rt			*exc_bd_rt;				/* pointer to the base address of the module from the RT's perspective */
	struct exc_mon_seq		*exc_bd_monseq;			/* pointer to the base address of the module from the Sequential Monitor's perspective */
	struct exc_mon_lkup		*exc_bd_monlkup;		/* pointer to the base address of the module from the Lookup Mode Monitor's perspective */
	struct exc_dwnld		*exc_bd_dwnld;			/* pointer to the base address of the module from the Download Mode's perspective */
	char					*exc_pc_dpr;			// character pointer to module
	usint					*top_stack;
	usint					nextblock;
	usint					wait_for_trigger;
	usint					triggers;
	usint					trig_mode;
	int						next_rt_entry;
	int						next_rt_entry_old;
	int						next_MON_entry;
	char					msg_block;				/* which of the 2 msg block areas is in use */
	usint					module;
	usint					originalModuleNumber;
	unsigned int			lastmsgttag;
	unsigned int			rt_sync_timetag[32];
	usint					rt_sync_data[32];
	int						processor;				/* 0=Intel, 1=AMD (old), 2=NIOSII */
	usint					*rtid_ctrl;
	unsigned char 			*rtid_brd_ctrl;			/* For single function RT - Broadcast saId control table */
	struct MON_STACK		*cmon_stack;
	int 					cmon_stack_size;
	usint					*msg_block2;
	int						msg_block2_size;
	usint					*msg_block3;
	int						msg_block3_size;
	usint					*msg_block4;
	int						msg_block4_size;
	usint					*ctrl_reg;				/* for old pcmcia/ep */
	int						ignoreoverrun;			/* for sequential monitor mode */
	int						expandedBCblockFW;
	int						expandedRTblockFW;
	int						expandedMONblockFW;		/* for sequential monitor mode */
	int						enhancedMONblockFW;		/* for BC monitor mode */
	int						RTentryCounterFW;		/* for RT mode			*/
	int						univmode;				/* for universal mode */
	BOOL					singleFunction;
	usint					rtNumber;
	char					NextBufToAccess[2048];
	int						lastblock;				/*for sequential monitor mode */
	int						isEnhancedMon;
	int						userRequestedRTexpanded;
	int						isExpandedMon;
	BOOL					frequencyModeSupported;		/* to indicate that Frequency Mode is supported */
	BOOL					frequencyModeON;		/* to indicate that Frequency Mode is ON */
	int						RTstacksStatus;

	/* in initcard.c we set this RTstacksStatus indicator as follows: 
		AMD:Old stack only
		Intel:Both
		Nios:Default is only the new stack, and you can change it to use both stacks
		
		NOTE: Changing the value of this indicator is relevant ONLY for Nios, 
		and should not be touched for other processors
	*/

	unsigned char			board_config;
	unsigned char			mode_code;
	usint					monWrapCount;			// used by Get_Next_Mon_Message_Px to keep track of the hi part of the message counter
	usint					prevMonMsgCount;		// used by Get_Next_Mon_Message_Px to identify when to refresh monWrapCount
	usint					hiMsgCount;				// for the high counter for non Irig messages that have only 16 bit counters
	struct RT_STACK_ENTRY 	*pNextMsgInLclRTmessageBuffer;
	unsigned int  			numMsgsInLclRTmessageBuffer;
	unsigned char 			LclRTmessageBuffer[0x1000]; // note:  This must be 32-bit aligned or the memcpy version WILL NOT WORK
	usint					prevCounterInBuffer;  //MsgCounter of last message in the last block read by Read_Monitor_Block_Px
	usint					prevCounterInMsg; //MsgCounter of last message returned by Get_Next_Message_New_Px
	unsigned int			lastRTstackCountRead;
	unsigned int			is32bitAccess;
	unsigned int 			lastCountRead; //MsgCounter of last message in the last block read by Read_Monitor_Block_Px with high 16 bit included
	unsigned int 			multibufUsed;  // indicator if the user is using multibuffer feature in RT mode; by calling function Set_RTid_Multibuf_Px 

	unsigned char hubmode_MMSI;
	/* to store hubmode for MMSI-MONITOR, since DPR register at 0x3FF8 is not available */
	BOOL 				isModePerRTSupported;  // new Monitor feature, set RTs to be 1553A or 1553B
	int					irigTtagModeSupported;		// new 48bit IRIGb timetag mode is supported in this firmware
	int					irigModeEnabled;  // new Monitor feature, timetag is IrigB

#ifdef MMSI
	struct s_RcvBufManagement_MMSI mBufManagement; 
#else
	struct s_RcvBufManagement_PX mBufManagement; 
#endif

	unsigned int timetagBefore, timetagAfter, timetagDelta;
	FILE *DEBUGoutFile;
	LARGE_INTEGER lDebugTime[10];
} INSTANCE_PX;

#endif
