#ifndef __DWNLD_H
#define __DWNLD_H

struct exc_dwnld				/* structure for Px module - for download mode	*/
{
	unsigned char reserved_1[0x3e80];	/* 0000h - 3e7fh : "Mode Specific Registers"	*/
	unsigned char revision_level;		/* 3e80h -       : new revision level of board	*/
	unsigned char reserved_2[0x05];	/* 3e81h - 3e85h : Reserved						*/
	usint more_board_options;			/* 3e86h - 3e87h more options : bit0 - Monitor_Only_Mode	*/
	unsigned char reserved_3[0x18];	/* 3e88h - 3e9fh : Reserved						*/
	usint	volatile serialNum;			/* 3ea0h - 3ea1h : module serial number			*/
	usint moduleTimeHi;				/* 3ea2h - 3ea3h : module time (high bytes)		*/
	usint moduleTimeLo;				/* 3ea4h - 3ea5h : module time (lo bytes)		*/
	unsigned char reserved_4[0x156];	/* 3ea6h - 3ffbh : 								*/
	unsigned char start;				/* 3ffch         : start						*/
	unsigned char board_status;		/* 3ffdh         : module status				*/
	unsigned char board_id;			/* 3ffeh         : module id					*/
	unsigned char board_config;		/* 3fffh         : module mode-configuration	*/
	unsigned char opSucceeded;  		/* 4000h         : download operation success	*/
	/*  unsigned char reserved_5[];	   4001h - ffffh : rest of module's memory		*/
};

#endif
