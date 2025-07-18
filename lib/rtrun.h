#ifndef __RTRUN_H
#define __RTRUN_H

#define MSGBLOCK_SIZE 0x1900 /* msgblock sizes in words */
#define MSGBLOCK2_SIZE 0x0700
#define MSGBLOCK3_SIZE 0x2000
#define BLOCKSIZE  32


#define ONLY_OLD_RT_STACK 0xa
#define BOTH_RT_STACKS	  0xb
#define ONLY_NEW_RT_STACK 0xc

#ifdef UNET_DEVICE
#pragma pack(2)
#endif

struct MON_STACK
{
	usint msgstatus;
	usint ttag_lo;
	usint ttag_hi;
	usint words[37]; /* for RT2RT 2 command words 2 status words + 32 data */
};

#ifdef UNET_DEVICE
#pragma pack()
#endif

struct exc_rt	    /* structure for exc board- for rt mode */
{
  usint data[MSGBLOCK_SIZE];		/* 0000h - stack + message block area	    */
  unsigned char active[32];			/* 3200h - rt lookup table		    */
  usint status[32];					/* 3220h - 1553 status words		    */
  unsigned char reserved_1[4];		/* 3260h */
  usint checksum_flag;				/* 3264h - 3265 the blocks less than this number  */
									/*	       will get check-summed	     */
  unsigned char mode_code;			/* 3266h - set 00000 and/or 11111 as mode   */
  unsigned char reserved_2[121];	/* 3267h - 32dfh */
  unsigned char word_count[32];		/* 32e0h - 32ffh - word count error table    */
  usint    msg_stack_old[126];		/* 3300h - 33fbh - fifo of last 42 cmds processed */
  unsigned char int_cond;			/* 33fch - interrupt condition registar     */
  unsigned char reserved_3[3];		/* 33fdh - 33ffh */
  usint    last_cmd[32];			/* 3400h - 343fh - last command word mode response  */
  usint    built_in_test[32];		/* 3440h - 347fh - bit mode command response  */
  usint    vector[32];				/* 3480h - 34bfh - send vector mode command response */
  usint alter_table[200];			/* 34c0h - 364fh: when we write to a block,  */
									/*                set the alter_table entry to be NO_ALTER, */
									/*                indicating to NOT alter this value now  */
  unsigned char reserved_4[0x830];	/* 3650h-3e7fh...uchar 2096(dec) == 0x830 */
  unsigned char revision;			/* 3e80h - new firmware revision register   */
  unsigned char reserved_50;		/* 3e81h */
  usint    board_type;				/* 3e82h - board type: 1553, 0018, 1760      */
  usint board_options;				/* 3e84h - type of board: 'P'==50 for PCM-Px, 'M'==4D for CONCM-PMx */
									/* 3e85h - board type:1=1553, 2=1760         */
  usint more_board_options;			/* 3e86h - 3e87h more options : bit0 - Monitor_Only_Mode	*/
  unsigned char reserved_51[8];		/* 3e88h - 3e8fh */
  usint volatile mon_ptr;			/* 3e90h - 3e91h - pointer to begin of next record -px only */
  usint DoubleBufferBadBlockNum;	/* 3e92 - illegal odd-numbered datablock that  															the user selected for use in
											  the user selected for use in double bufferring */
  usint DoubleBufferRTid;			/* 3e94 - associated RTid of the bad block number */
  
  usint countBusy;				/* 3E96h */
  usint msgCount[2];				/* 3E98h running count of messages sent (count retries seperately) */
  usint errCount[2];				/* 3E9ch running count of msg errors (count retries seperately) */

  usint	volatile serialNum;			/* 3ea0h - 3ea1h module serial number 	*/
  usint moduleTimeHi;				/* 3ea2h - 3ea3h module time (high bytes)	*/
  usint moduleTimeLo;	  			/* 3ea4h - 3ea5h module time (lo bytes)		*/
  unsigned char reserved_52a[0x16];	/* 3ea6h - 3ebbh			     */
  usint concMonWrapCount;			/* 3ebch - 3ebdh */
  usint msg_pointer;				/* 3ebeh - 3ebfh */
  usint header_exists[0x20];		/* 3ec0h - 3effh  32 words, for 1760         */
  usint header_val[0x20];			/* 3f00h - 3f3fh  32 words, for 1760, header val rcv */
  usint reserved_headervalxmt[0x20];/* not used in rt mode: 3f40 */
  unsigned char reserved_53[0x60];	/* 3f40h - 3fdfh			     */
  usint entryCountLo;				// 3fe0h count of RT Stack entries filled in - for RT to RT this will go up by 2 if both RTs are active
  usint entryCountHi;				// 3fe2h we'll put the low first in case we ever support 32 bit access
  usint notUsed[2];
  usint broadcast_flag;				/* 3fe8 */
  usint moduleFunction;				/* 3fea-b */
  unsigned char reserved_a[3];		/* 3fec-e */
  unsigned char rtProtocolOptions;	/* 3fefh - (0) send/suppress status (1) 1553a  */
  usint    msg_pointer_old; 		/* 3ff0h - 3ff1h - message stack pointer	    */
  unsigned char var_amplitude;		/* 3ff2h - variable amplitude (optional)    */
  unsigned char err_inject;			/* 3ff3h - global rt error injection	    */
  unsigned char rt_response;		/* 3ff4h - response time register	    */
  unsigned char reserved_6;			/* 3ff5h */
  unsigned char bit;				/* 3ff6h - for bit count error injection    */
  unsigned char time_res;			/* 3ff7h - time tag resolution register     */
  unsigned char hubmode_MMSI;		/* 3ff8h  */
  unsigned char reserved_55;		/* 3ff9h */
  unsigned char reserved_54;		/* 3ffah  */
  unsigned char msg_status;			/* 3ffbh - message complete indicator	    */
  unsigned char start;				/* 3ffch - start register		    */
  unsigned char board_status;		/* 3ffdh - board status register	    */
  unsigned char board_id;			/* 3ffeh - board id register		    */
  unsigned char board_config;		/* 3fffh - board configuration register     */
  unsigned char blk_lookup[0x800];	/* 4000h - 47ffh - data block lookup table  */
  usint rtidIP_cmonAP[0x1400];		/* 4800h - 57ffh: usint rtid ctrl (Intel proc) */
									/* 5800h - 5fffh: unsigned char rtid_brd_ctrl (PxS) */
									/* 6000h - 6fffh: msg_stack 512 cmds 32bit ttag (Intel)  starts at:rtidIP_cmonAP[0xc00] */
									/* 4800h - 6fffh: conc monitor table (AMD proc )*/
  unsigned char reset_hard;			/* 7000h reset hardware register     */
  unsigned char reset_interrupt;	/* 7001h reset interrupt register    */

  unsigned char reset_soft;			/* 7002h resets current bank only    */
  unsigned char options_select;		/* bit 0=1: RT 31 broadcast   */
  unsigned char reserved_8[3];
  unsigned char time_tag_reset_reg;	/* 7007  */
  usint reserved_9[0xc];			/* 7008h - 701fh */
  usint rtNumSingle;				/* 7020h - 7022h*/
  usint reserved_10[0xef];			/* 7022h - 71ffh*/
  usint data2[MSGBLOCK2_SIZE];		/* 7200h - 7fffh */
  usint cmonIP_rtidAP[0x2000];		/* 8000h-ffffh: conc monitor table (Intel proc)  */
									/* note that this is actually size 0x4000 */
									/* Only when using expanded_block_mode, this is size 0x2000, and data3 used */
									/* 8000h-8800h: rtid ctrl (AMD proc)	*/
  usint data3[MSGBLOCK3_SIZE];		/* c000- ffff  when expanded block mode is used */
};
struct RT_STACK_ENTRY
{
	usint command;     /* 1553 command word */
	usint timeTaghi;    
	usint timeTaglo;     
	usint msgStatus;      /* Excalibur status word for this command */
};

/*
note that time tag preset and time tag resolution registers are defined
as hi and lo char rather than as int because integers in C structures do
not begin at odd addresses
*/

struct CMDENTRY
{
	usint command;     /* 1553 command word */
	usint command2;    /* 1553 transmit command word for RT to RT messages */
	usint timetag;     /* time in microseconds between this message and the
					   last one. Not meaningful for the first command. */
	usint status;      /* Excalibur status word for this command */
};

struct CMDENTRYRT
{
	usint command;     /* 1553 command word */
	usint command2;    /* 1553 transmit command word for RT to RT messages */
	usint timetaghi;     /* time in microseconds between this message and the
						 last one. Not meaningful for the first command. */
	usint timetaglo;  
	usint status;      /* Excalibur status word for this command */
};

#endif
