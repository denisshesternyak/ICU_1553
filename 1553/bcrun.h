#ifndef __BCRUN_H
#define __BCRUN_H

#define STACKSIZE_PX 0x1a00
#define STACKSIZE_MMSI 0x1a00
#define MSG_BLOCK1_SIZE 0x400

struct exc_bc                       				/* structure for exc board- for bc mode */
{
#ifdef MMSI
	usint stack[STACKSIZE_MMSI];         				/* 0000h - stack area */
#else
	usint stack[STACKSIZE_PX];         				/* 0000h - stack area */
#endif
	unsigned char active[32];						/* 3400h - 341fh: RT Active table                  */
	usint async_num_msgs;							/* 3420h - # of msgs in the async frame      */
	usint async_frame_ptr;							/* 3422h - pointer to begin of async frame   */
	usint async_start_flag;							/* 3424h - start sending the async frame     */
	unsigned char reserved_1[0xa5a];				/* 3426h-3e7fh */
	unsigned char revision_level;					/* 3e80h - new revision level of board      */
	unsigned char reserved_2;						/* 3e81h */
	usint board_type;								/* 3e82h - board type: 1553, 0018, 1760      */
	usint board_options;							/* 3e84h - type of board: 'P'==50 for PCM-Px, 'M'==4D for CONCM-PMx */
													/* 3e85h - board type:1=1553, 2=1760         */
	usint more_board_options;						/* 3e86h - 3e87h more options : bit0 - Monitor_Only_Mode */
	unsigned char ttagclrsync[2];					/* 3e88h - 3e89h  send ttag on mode code sync 1 and 17 */
	usint ttagsendsync;								/* 3e8ah - 3e8bh send ttag on mode code sync */
	usint bcProtocolOptions;						/* 3e8ch - protocol support: (0)1553a(non-MMSI), (1) srq disable */
	usint	expandedBlock;							/* 3e8e */
	usint	monPtr;									/* 3E90 */
	usint	reserved_42[2];							/* 3E92 */
	usint	countBusy;								/* 3E96 */
	usint	msgCount[2];							/* 3e98 running count of messages sent (count retries seperately) */
	usint	errCount[2];							/* 3e9c running count of msg errors (count retries seperately) */
	usint	volatile serialNum;						/* 3ea0h - 3ea1h module serial number 	*/
	usint moduleTimeHi;								/* 3ea2-3ea3 module time (high bytes)	*/
	usint moduleTimeLo;	  							/* 3ea4-3ea5 module time (lo bytes)		*/
	usint reserved_5[11];							/* 3ea6h - 3ebbh reserved			*/
	usint concMonWrapCount;							/* 3ebch - 3ebdh conc Monitor wrap counter */
	usint reserved_5_1;								/* 3ebeh - 3ebfh reserved			*/
	usint header_exists[0x20];						/* 3ec0h - 3effh 32 words, for 1760          */
	usint reserved_6[0x20];							/* 3f00h - 3f3fh reserved     */
	usint header_val[0x20];							/* 3f40h - 3f7fh 32 words, for 1760          */
	usint srqmessage1[4];							/* 3f80 vector mode code message for SRQ */
	usint srqmessage2[35];							/* 3f88 transmit message response for SRQ */
	usint srqmessagestatus[2];						/* 3fCE statuses for vector mode code, and xmt response */
	usint srqcounter;								/* 3fd2 */
	usint reserved_7[2];							/* 3fd4 */
	usint sync_pattern;								/* 3fd8  add production test error injection options */
	usint err_word_index;							/* 3fda */
	usint err_bit_index;							/* 3fdc */
	usint reserved_8[3];							/* 3fde */
	unsigned char	reserved_9;						/* 3FE4 */
	unsigned char replay;							/* 3fe5h enable replay mode */
	usint minor_frame_resolution;					/* 3fe6h - minor frame resolution	     */
	usint minor_frametime;							/* 3fe8h - minor frame time lo/hi           */
	usint instruction_counter; 						/* 3febh - instruction counter         */
	usint frametime_resolution;						/* 3fech - frame time resolution /frametime hi(MMSI)  */
	usint frametime;       							/* 3feeh - frame time /frametime lo (MMSI)  */
	usint pointer;									/* 3ff0h - stack pointer                    */
	unsigned char amplitude;						/* 3ff2h - variable amplitude control       */
	unsigned char resptime;							/* 3ff3h - BC response time register        */
	unsigned char word;								/* 3ff4h - word count register              */
	unsigned char bit;								/* 3ff5h - bit count register               */
	unsigned char loop_count;						/* 3ff6h - loop count register              */
	unsigned char reserved_10;						/* 3ff7h */
	unsigned char hubmode_MMSI;						/* 3ff8h */
	unsigned char rt_resptime;						/* 3ff9h - RT response time register        */
	unsigned char message_status;					/* 3ffah - message status register          */
	unsigned char interpt;							/* 3ffbh - interrupt condition register     */
	unsigned char start;							/* 3ffch - start register                   */
	unsigned char volatile board_status;			/* 3ffdh - board status register    */
	unsigned char volatile board_id;				/* 3ffeh - board id register        */
	unsigned char board_config;						/* 3fffh - board configuration register      */
	usint msg_block1[MSG_BLOCK1_SIZE];				/* 4000-47ff first msg_block area - both processors */
	usint msgblock2IP_cmonAP[MSG_BLOCK2_SIZE_IP];	/* 4800-6fffh: second msg_block area (Intel proc) */
													/* 4800-6fffh: conc monitor table (AMD proc )*/
	unsigned char reset_hard;						/* 7000h - reset current bank               */
	unsigned char reset_interrupt;					/* 7001h - reset interrupt register         */
#ifndef MMSI
	unsigned char reset_soft;						/* 7002h - resets both banks                */
#else
	unsigned char NumChannels;						/* 7002h - number of channels that exist on this module
															   for M8K: 2,4,6
															   for M4K: this is set to 0xFF which means 8 channels */
#endif
	unsigned char internal_mon_connect;				/* 7003h - connect modules internally to monitor */
	unsigned char bank_select;						/* 7004h - selects bank 00 or 1             */
	unsigned char concurrent_start;					/* 7005h - starts both banks running        */
	unsigned char int_level_select;					/* 7006h - software interrupt level select  */
													/*   (*) made char because of odd alignmnt */
	unsigned char time_tag_reset;					/* 7007h */
	usint time_tag[2];								/* lsb in time_tag[0], msb time_tag[3]*/
	unsigned char time_tag_opt;						/* 700ch-time tag counter clock source selector */
	unsigned char reserved_11;						/* 700dh */
	unsigned char xilinx_reset;						/* 700eh - write here 3 times followed by write to
															   reset register at 7000,7001 to reset Xilinx */

	unsigned char reserved_12;						/* 700fh */
	unsigned char volatile dualChannelReg;			/* 7010h 
														  BITS 	description 
														  0 		Only in PCMCIA/Px - reset allowed to channel 0 : 0 - not allowed; 1 - Allowed
														  1 		Only in PCMCIA/Px  - reset allowed to channel 1 : 0 - not allowed; 1 - Allowed
														  2		set to 0
														  3 		Only in PCMCIA/Px - Channel 0 present : 0 Not Present; 1 Present
														  4 		Only in PCMCIA/Px  - Channel 1 present : 0 Not Present; 1 Present */
	unsigned char volatile pcmciaEPorPx;			/* 7011h  EP/Px indicator : 'P' (50 Hex) - PCMCIA/EP (AMD or Intel), FF - PCMCIA/Px (Nios)*/
	unsigned char reserved_13[0xae];					/* 7012-70bf */
	usint	IRIGttag[4];								/* 70c0-70C7 running IRIG clock */
	unsigned char reserved_14[10];					/* 70c8-70d1 */

	usint IrigPreload_seconds;							//70d2h
	usint IrigPreload_hr_min;							//70d4h
	usint IrigPreload_days;							//70d6h
	usint IrigPreload_load;							//70d8h

	unsigned char reserved_15[0x26];					/* 70da-70ff */

	usint msgblock2AP[0x780];						/* 7100h-0x7fffh additional msg block area - room for 0x780 (1920 decimal) words */
	usint cmonIP[0x4000];							/* 8000h-0xffffh conc monitor table (Intel or Nios proc) */
};

struct instruction_stack                    /* structure of one stack entry */
{
	usint message_block_pointer;
	usint gap_time;
	usint img_mult;
	usint message_status_word;
};

// use this only when there is a specific problem -- in labwindows, and labview RT; ATEZ 64bit win8
#ifdef RTLABVIEW
#pragma pack(2)
#endif
struct FRAME
{
	short int id;
	long gaptime;
};
#ifdef RTLABVIEW
#pragma pack()
#endif
struct framearray         /* Data structure associating frame numbers */
                          /* with frame pointers and keeping track of the */
                          /* number of messages in a frame      */
{
	usint frameptr;
	usint msgcnt;
};

struct BCMSG  /* structure used in function Get_BC_Message */
{
	usint msgstatus;  /* status word containing the msg status flags */
	usint words[36];	   /* a pointer to an array of 1553 words */
};

struct i_loopback
{
	usint frame_val;
	usint frame_status;
	usint resp_status;
	usint early_val;
	usint receive_data1;
	usint status_1;
	usint receive_data2;
	usint status_2;
	usint mc_status;
	usint ttag_val_lo;
	usint ttag_val_hi;
	usint ttag_status;
};

struct e_loopback
{
	usint frame_val;
	usint frame_status;
	usint cmd_send[8];
	usint ttag_val_lo;
	usint ttag_val_hi;
	usint ttag_status;
};

struct e_loopback_MMSI
{
	usint frame_val;
	usint frame_status;
	usint early_val[9];
	usint receive_data1[9];
	usint status_1[9];
	usint receive_data2[9];
	usint status_2[9];
	usint ttag_val_lo;
	usint ttag_val_hi;
	usint ttag_status;
};


#endif
