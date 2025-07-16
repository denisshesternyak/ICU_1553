#ifndef __MONSEQ_H
#define __MONSEQ_H

// number of words in each message area
#define MONSEQ_MSGBLOCK_AREA_1 0x1f40
#define MONSEQ_MSGBLOCK_AREA_2 0x17e8
#define MONSEQ_MSGBLOCK_AREA_3 0x758 
#define MONSEQ_MSGBLOCK_AREA_4 0x4028
#define MAXMONBLOCKS_OLD			200
#define MAXMONBLOCKS_EXPANDED		800
#define MAXMONBLOCKS_ENHANCED		400

/* Enhanced Monitor */
#define ENHANCED_MON_MODE 2 
#define MODE_PER_RT_MODE 4

#define INFO_COMMAND	0xC000
#define INFO_DATA		0xD000
#define INFO_BUSA		0x0A00
#define INFO_BUSB		0x0B00
#define INFO_NOT_CONTIG	0x0010	
#define INFO_PARITY_ERR	0x0001
#define INFO_MANCH_ERR  0x0002


struct exc_mon_seq        /* structure for exc board - for MONITOR mode */
{
  usint volatile msg_block[MONSEQ_MSGBLOCK_AREA_1];	/* 0x0000h-3e7fh - First Message block area  (200 blocks) */
  unsigned char revision;							/* 3e80h - firmware revision register */
  unsigned char reserved_1;							/* 3e81h */
  usint board_type;			/* 3e82h - 3e83h  */  
  usint board_options;								/* 3e84h - type of board:
  														'P'==50 for PCM-Px,
  														'M'==4D for CONCM-PMx */
													/* 3e85h - board type:1=1553, 2=1760  */
  usint more_board_options;							/* 3e86h - 3e87h more options : bit0 - Monitor_Only_Mode, bit2=NIOS	*/
  unsigned char reserved_2[2];						/* 3e88h - 3e89h				*/
  usint moduleFunction;								/* 3e8ah - bit 0 - Expanded mode (0:Off 1:On) */
													/*		   bit 1 - Enhanced Mode (0:Off 1:On) - When Enhanced is On Expanded mode will not work */
  usint x1553a;										/* 3e8ch - 3e8dh: 1553a protocol support */
  usint response_time;								/* 3e8eh - 3e8fh                       */

  usint reserved_31[3];			/* 3E90 */
  usint countBusy;			/* 3E96 */
  usint msgCount[2];			/* 3E98 running count of messages sent (count retries separately) */
  usint errCount[2];			/* 3E9c running count of msg errors (count retries separately) */

  usint	volatile serialNum;							/* 3ea0h - 3ea1h module serial number 	*/
  usint moduleTimeHi;								/* 3ea2-3ea3 module time (high bytes)	*/
  usint moduleTimeLo;	  							/* 3ea4-3ea5 module time (lo bytes)		*/
  usint resptime_1553a;							    /* 3Ea6h */
  unsigned char reserved_4[8];						/* 3ea8h - 3eafh reserved			*/
  usint ll_pointer;									/* 3eb0h -  3eb1h                           */
  usint current_PS;									/* 3eb2h -  3eb3h                           */
  usint wrap_around;								/* 3eb4h -  3eb5h                           */
  usint overrun;									/* 3eb6h -  3eb7h                           */
  unsigned char reserved_5[4]; 						/* 3eb8h - 3ebbh			     */
  usint monWrapCount;								/* 3ebch - 3ebdh monitor Wrap counter */
  usint ExpandedCurrentBlock;						/* 3ebeh - 3ebfh			     */
  usint header_exists[0x20];						/* 3ec0h - 3effh  32 words, for 1760         */
  usint header_val[0x40];							/* 3f00h - 3f7fh  20=>rcv, 20=>xmt        */
  unsigned char reserved_6[0x68];					/* 3f80h - 3fe7h			     */
  usint broad_ctl;									/* 3fe8h - 3fe9: broadcast control; 1 for ON     */
  unsigned char mode_ctl;							/* 3feah - mode code = 00000 and/or 11111   */
  unsigned char trig_ctl;							/* 3febh - which triggers active            */
  usint trig_mask2;									/* 3fech - mask for trigger 2       */
  usint trigger2;									/* 3feeh - trigger 2 template       */
  usint trig_mask1;									/* 3ff0h - mask for trigger 1       */
  usint trigger1;									/* 3ff2h - trigger 1 template       */
  unsigned char cnt_trigger;						/* 3ff4h - counter trigger                  */
  unsigned char msg_counter;						/* 3ff5h - index of last monitored message  */
  unsigned char reserved_7;							/* 3ff6h */
  unsigned char time_res;							/* 3ff7h - time tag resolution register */
#ifdef MMSI
  unsigned char hubmode_MMSI;						/* 3ff8h  */
  unsigned char reserved_55;						/* 3ff9h */
#else
  usint interval_int;								/* 3ff8h - 3ff9h */
#endif
  unsigned char msg_status;							/* 3ffah - message complete indicator       */
  unsigned char int_cond;							/* 3ffbh - interrupt condition register     */
  unsigned char start;								/* 3ffch - start register                   */
  unsigned char volatile board_status;				/* 3ffdh - board status register            */
  unsigned char board_id;							/* 3ffeh - board id register                */
  unsigned char board_config;						/* 3fffh - board configuration register     */
  usint volatile msg_block2[MONSEQ_MSGBLOCK_AREA_2];			/* 4000h - 6fcfh Second message block (153 blocks) */
  unsigned char  modePerRT[32];						/* 6FD0h - 6FEFh */
  unsigned char  reserved_8[16];					/* 6FF0h - 6FFFh */
  unsigned char  HWhole[0x100];						/* 7000h - 70ffh */
  usint volatile msg_block3[MONSEQ_MSGBLOCK_AREA_3];/* 7100h - 7fafh Third message block (47 blocks) */
  usint volatile msg_block4[MONSEQ_MSGBLOCK_AREA_4];/* 7fb0h - 0xffff Fourth message block (400 blocks) */
													/*				 or word status table for Enhanced Monitor */
};

#pragma pack(2)

#include "pxincl.h"

struct MONIRIGMSG_TTAG
{
	unsigned short int msgstatus;
	t_IrigOnModuleTime IrigTime;
	unsigned short int words[36]; // for RT2RT 2 command words 2 status words + 32 data
	unsigned short int msgCounterLo;
	unsigned short int msgCounterHi;
};

struct MONIRIGMSG
{
	unsigned short int msgstatus;
	unsigned short int timetagLoLo;
	unsigned short int timetagLoHi;
	unsigned short int timetagHiLo;
	unsigned short int timetagHiHi;
	unsigned short int words[36]; // for RT2RT 2 command words 2 status words + 32 data
	unsigned short int spare;
	unsigned short int msgCounterLo;
	unsigned short int msgCounterHi;
};

struct MONMSGPACKED
{
	unsigned short int msgstatus;
	unsigned int elapsedtime;							/* to accomodate 32 bit ttag*/
	unsigned short int words[36];						/* for RT2RT 2 command words 2 status words + 32 data */
	unsigned short int msgCounter;
};
#pragma pack()
 
struct MONMSG
{
	unsigned short int msgstatus;
	unsigned int elapsedtime;							/* to accomodate 32 bit ttag*/
	unsigned short int words[36];						/* for RT2RT 2 command words 2 status words + 32 data */
	unsigned short int msgCounter;
};


struct MONMSGINFO  /* For Enhanced Monitor */
{
	usint msgstatus;
	usint reserved1;
	usint reserved2;
	usint words[36]; /* For each word: */
					 /* bit 00		Valid Parity */
					 /* bit 01		Valid Manchester */
					 /* bit 04		Non Contig */
					 /* bit 08-11	0xA ->Bus A, 0xB->Bus B */
					 /* bit 12-15	0xC ->Command Sync, 0xD->Data Sync */ 
	usint msgCounter;
};

/* For the host-memory buffer copy of DPRAM */
struct dmaShadow_Px
{
	usint	dmaCopyIsValid;
	DWORD_PTR	beginDpramOffset;
	DWORD_PTR	endDpramOffset;
	struct MONMSG msgBlockData[MAX_NUMBER_BLOCKS_IN_DMA_READ];
};

#endif
