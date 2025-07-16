#ifndef __MONLKUP_H
#define __MONLKUP_H

struct exc_mon_lkup       /* structure for exc board - for MONITOR mode */
{
  usint msg_block[0x1a00];			/* 0x0000h-33ffh - Message block area      */
  unsigned char reserved_1[0xa80];	/* 3400h-3e7fh */
  unsigned char revision;			/* 3e80h - firmware revision register */
  unsigned char reserved_2;			/* 3e81h */
  usint board_type;					/* 3e82h - board type: 1553, 0018, 1760    */
  unsigned char reserved_3[2];		/* 3e84h-3e85h */
  usint more_board_options;			/* 3e86h - 3e87h more options : bit0 - Monitor_Only_Mode	*/
  unsigned char reserved_4[4];		/* 3e88h-3e8bh */
  usint x1553a;						/* 3e8ch - 1553a protocol support, at 3e8dh  */
  unsigned char reserved_5[0x12];	/* 3e8eh - 3e9fh                       */
  usint	volatile serialNum;		 	/* 3ea0h - 3ea1h module serial number 	*/
  usint moduleTimeHi;			 	/* 3ea2-3ea3 module time (high bytes)	*/
  usint moduleTimeLo;	  		 	/* 3ea4-3ea5 module time (lo bytes)		*/
  unsigned char reserved_6[0x142];	/* 3ea6h - 3fe7h reserved			*/
  usint broad_ctl;					/* 3fe8h - broadcast control; 1 for ON     */
  unsigned char mode_ctl;			/* 3feah - mode code = 00000 and/or 11111  */
  unsigned char reserved_7[7];		/* 3febh-3ff1h */
  unsigned char last_blk;			/* 3ff2h - last block number */
  unsigned char reserved_8[4];		/* 3ff3h-3ff6h */
  unsigned char time_res;			/* 3ff7h - time tag resolution register  */
  usint reserved_9;					/* 3ff8h - 3ff9h */
  unsigned char msg_status;			/* 3ffah - message complete indicator       */
  unsigned char int_cond;			/* 3ffbh - interrupt condition register     */
  unsigned char start;				/* 3ffch - start register                   */
  unsigned char board_status;		/* 3ffdh - board status register            */
  unsigned char board_id;			/* 3ffeh - board id register                */
  unsigned char board_config;		/* 3fffh - board configuration register     */
  unsigned char lookup[2048];		/* 4000h-47ffh - data block lookup table    */
  unsigned char reserved_10[10240];	/* 4800h-6fffh */
  unsigned char reset_hard;			/* 7000h - reset current bank               */
  unsigned char reset_interrupt;	/* 7001h - reset interrupt register         */
};

#endif
