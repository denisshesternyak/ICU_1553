#ifndef __UNIVRUN_H
#define __UNIVRUN_H

#define MAX_MSGS_PER_BLOCK 20

#define NUM_BLOCKS 512
#define NUM_RTIDS 2048
#define NUM_RTS 32
#define MAX_READVALS 409

#define UNIV_RT_MODE   1
#define UNIV_BC_RT_MODE 2
#define UNIV_MON_BLOCK  4

#define UNIV_BIT_CNT_ERRINJ   0x0002
#define UNIV_PARITY_ERRINJ    0X0050
#define VALID_ERRINJ_MASK (UNIV_BIT_CNT_ERRINJ | UNIV_PARITY_ERRINJ)

#define GETBIT(x,y)	(((x) & (1<<(y))) != 0)
#define TRTYPE(x)		((x & 0x20)>>5)

#define BIT_MODECODE 19
#define VECTOR_MODECODE 16
#define RECEIVE_MODECODE 17
#define RT_ACTIVE_BIT 1

#define VECTOR_VAL_SET 1
#define BIT_VAL_SET 2

#define NULL_RT 33
#define START_BIT 1

typedef struct
{
	usint msgid;
	int wdcnt;
	int pos;
}t_msgTable; /*indexed by msgid */

typedef struct
{
	usint numMsgs; 	/* contains number of msgs associated with this block */
	t_msgTable msgTable[MAX_MSGS_PER_BLOCK]; /* msgid of messages with cmd == rtid */
	usint data[32]; /* results of load_datablk stored here */
} t_blockTable; /* indexed by blknum */

typedef struct
{
	int blknum;
	int intrpt;

}t_rtidTable; /* indexed by rtid */

typedef struct
{
	int intrpt;
	int valsSet;    /* 1=bitval 2=vectorval */
	usint bitValue;
	usint vectorValue;
	usint statusValue;
}t_rtTable; /* indexed by rt */

typedef struct
{
	int messagenum;
	unsigned int timetag;

}t_readvalTable; /* indexed by place in cmon stack */

typedef struct
{
	t_blockTable blockTable[NUM_BLOCKS];
	t_rtidTable rtidTable[NUM_RTIDS];
	t_rtTable rtTable[NUM_RTS];
	int bitcountErrinj;
	int parityErrinj;
	int broadcastInterrupt;
	int RTinterrupt;
	int next_MON_entry;
	unsigned int lastmsgttag;
	t_readvalTable readvalTable[MAX_READVALS];
}t_univTable; /* one per device, like instance */

/* function prototypes */
#ifdef __cplusplus
extern "C" {
#endif

	int borland_dll Set_Mode_UNIV_Px(int handlepx, int mode);
	int borland_dll Set_RT_Active_UNIV_Px (int handlepx, int rtnum, int intrpt);
	int borland_dll Assign_RT_Data_UNIV_Px (int handlepx, int rtid, int blknum);
	int borland_dll Get_Blknum_UNIV_Px (int handlepx, int rtid);
	int borland_dll Load_Datablk_UNIV_Px (int handlepx, int blknum, usint *words);
	int borland_dll Set_RTid_Interrupt_UNIV_Px (int handlepx, int rtid, int enabled_flag);
	int borland_dll Set_Status_UNIV_Px (int handlepx, int rtnum, int statusvalue);
	int borland_dll Set_Vector_UNIV_Px (int handlepx, int rtnum, int vecvalue);
	int borland_dll Set_Bit_UNIV_Px (int handlepx, int rtnum, int bitvalue);
	int borland_dll Set_Interrupt_UNIV_Px (int handlepx, int flag);
	int borland_dll Set_RT_Errors_UNIV_Px (int handlepx, int errormask);
	int borland_dll Set_Broad_Interrupt_UNIV_Px (int handlepx, int intrpt);
	int borland_dll Set_Mode_Addr_UNIV_Px (int handlepx, int flag);
	int borland_dll Set_RT_Broadcast_UNIV_Px(int handlepx, int toggle);
	int borland_dll Run_UNIV_Px (int handlepx, int bcrunflag);
	int borland_dll Get_Next_RT_Message_UNIV_Px (int handlepx, struct CMDENTRYRT *cmdstruct);
	int borland_dll Get_Next_Mon_Message_UNIV_Px (int handlepx, struct MONMSG *msgptr);
#ifdef __cplusplus
}
#endif


#endif
