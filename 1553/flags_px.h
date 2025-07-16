#ifndef __PX_FLAGS_H
#define __PX_FLAGS_H


                   /* Flags for use with Galahad routines */

	/* General Flags */
#define VERBOSE_DEBUGS 0
// To enable debug on UNET that failed initialization, uncomment the next line
// #define ENABLE_LOCAL_H2D_DEBUGS 1

        /* Init_Card */
#define SIMULATE        0xFFFF

#ifndef MAX_REGISTRY_DEVICES
	#define MAX_REGISTRY_DEVICES    16
#endif

#ifndef EXC_4000PCI
	#define EXC_4000PCI	MAX_REGISTRY_DEVICES+9
#endif

#define EXC_PCMCIA_MODULE	100
#define EXC_PCMCIA_MODULE_2ND	101

#define BOARD_PCMCIAEP		0x1000	/* Board_options bit indicating PCMCIA */
#define XP250_PROCESSOR 0x0400		/* flag for Module Options Register */
#define NIOSII_PROCESSOR 0x8000		/* flag for Module Options Register */
#define AMD_PROCESSOR 0x0800		/* flag indicating SPARTAN Xilinx chip, only used with AMD */
#define REPLAY_MODE 0x0800		/* flag indicating we are currently running in replay mode, for Intel & Nios processors only */
#define INVALID_DEV_MMSI 9999
#define INVALID_MOD_MMSI 9999
#define INVALID_DEV_PX 9999
#define INVALID_MOD_PX 9999

#define RTBROADCAST_FLAG 0x01

#define TRUE 1
#define FALSE 0

       /* Get_Board_Status flags */
#define BOARD_READY     0x0001
#define RAM_OK          0x0002
#define TIMERS_OK       0x0004
#define SELF_TEST_OK    0x0008
#define BOARD_HALTED    0x0010

        /* Set_Mode Options */
#define RT_MODE         0x02     /* Set up card as Remote Terminal(s)         */
#define BC_RT_MODE      0x04     /* Set up card as BC concurrent RT           */
#define MON_BLOCK       0x08     /* Set up as Monitor in Sequential Block mode*/
#define MON_LOOKUP      0x20     /* Set up as Monitor in Table Lookup mode    */
#define LL_0018_MODE    0x44     /* new 0018 mode linked list: LL18_EARLY_NEW */
#define DWNLD_MODE      0xdd     /* download mode */
#define BC_MODE         0x04     /*  obsolete - use BC_RT_MODE */

        /* Max message/frame sizes */
#define MAXFRAMES 20
#define MSGMAX   1660
#define MONSEQ_IRIG_BLOCKSIZE 44
#define MSGSIZE 40      /* number of words in a message */
#define MSGSIZEBYTES_MON_IRIG 88	/* number of bytes in a Monitor message block*/
#define MSGSIZEBYTES_MON 80	/* number of bytes in a Monitor message block*/
#define MSGSIZEBYTES_RT 64	/* number of bytes in a RT message block */
#define MONMSG_HEADER_SIZE 6 // Status + 32bit timetag
#define MONIRIGMSG_HEADER_SIZE 10 // Status + 64bit timetag

        /* Create_Message cmdtype values */
#define RT2BC           0x00     /* Send a Transmit message                   */
#define BC2RT           0x01     /* Send a Receive message                    */
#define RT2RT           0x02     /* Send an RT to RT Transfer message         */
#define MODE            0x03     /* Mode command                              */
#define BRD_RCV         0x04     /* Broadcast receive message                 */
#define BRD_RT2RT       0x05     /* Broadcast RT to RT message                */
#define BRD_MODE        0x06     /* Broadcast Mode Command                    */
#define MINOR_FRAME     0x0f     /* Minor Frame message - delay               */

        /* Alter_Cmd flags       */
#define SAME_RT 32               /* Don't change the RT of the command word   */
#define SAME_SA 32               /* Don't change the SA of the command word   */

        /* Get_BC_Status flags */
#define WAIT_FOR_CONTINUE  0x01/* Message named by Set_Halt was reached     */
#define MSG_CMPLT       0x02     /* Message sent                              */
#define END_OF_FRAME    0x04     /* Complete frame of messages sent           */
#define MSG_ERR         0x08     /* Some error occurred                       */

        /* Get_Msg_Status flags - for BC */
#define END_OF_MSG      0x8000   /* Message Complete */
#define BAD_CHECKSUM	0x4000	 /* last word is not 1760 checksum	      */
#define BC_BAD_CHECKSUM 0x4000	 /* Bad checksum on the BC (xmt) side (1760)  */
#define BAD_BUS         0x2000   /* RT response received on nonactive bus */
#define BAD_CHANNEL     0x2000   /* RT response received on nonactive bus */
#define ME_SET          0x1000   /* ME bit set in 1553 status word            */
#define BAD_STATUS      0x0800   /* Bit other than ME set in 1553 status word */
#define INVALID_MSG     0x0400   /* 1553 Message level error e.g. sync, wd cnt*/
#define LATE_RESP       0x0200   /* RT responded late   */
#define SELFTEST_ERR	0x0100	 /* Excalibur card failed internal self test: vmepx  */
#define BAD_HEADER_BC	0x0100	 /* bad 1760 header: m4k1553px pcmcia/epII  */
#define INVALID_WORD    0x0080   /* Bad bit count or Manchester or parity     */
#define WD_CNT_HI       0x0040   /* RT transmitted too many words             */
#define WD_CNT_LO       0x0020   /* RT transmitted too few words              */
#define COUNTER_WRAP	0x0010	/* when present without MSG_ERROR means counter wrapped around to 0 */
#define BAD_RT_ADDR     0x0010   /* 1553 status word contained wrong address  */
#define BAD_SYNC        0x0008   /* status or data sync was wrong             */
#define BAD_GAP         0x0004   /* invalid gap time occurred between words  */
#define MSG_ERROR       0x0001   /* error occured defined in other flags      */
#define STATUS_SET      0x0800   /* status bit was set */

#define ERROR_BITS_NO_RT_ADDR 0x5FEC /* for BC mode */
#define ERROR_BITS_NO_MON_ADDR 0x14EE /* for Monitor mode */

/* Concurrent Monitor Status */
#define RT2RT_MSG_CONCM 0x0002	 /* RT to RT message in BC/CMon and RT/CMON */
#define BAD_CHECKSUM_CONCM 0x2000 /* bad checksum in BC/cmon and rt/cmon */

#define BAD_HEADER_BCCM 0x0100 /* bad header in bc/cmon */
#define BAD_HEADER_RTCM 0x0400 /* bad header in rt/cmon */

#define BAD_HDEADER_RTCM 0x0400 /* bad header in rt/cmon */


        /*Insert_Msg_Err error types */
#define ERRFLAGS        0x5F00   /* error bits in control word                */

#define PARITY_ERR_MMSI   0x4000   /* selects even parity on 1553 words         */
#define PARITY_ERR_PX   0x4000   /* selects even parity on 1553 words         */

#define WD_CNT_ERR      0x1000   /* inserts bad word count based on Set_Wd_Cnt*/
#define BIT_CNT_ERR     0x0800   /* inserts bad bit count based on Set_Bit_Cnt*/
#define SYNC_ERR        0x0400   /* data type sync is inserted in command word*/
#define GAP_ERR         0x0300   /* invalid gap time between command and data */
#define DATA_ERR        0x0100   /* insert Parity, Sync or Bit count errors   */
                                 /* in data words instead of command word for */
                                 /* BC to RT messages                         */
#define NO_ERR_IN_MSG	0x0	/* to disable error injection in this message */

        /* Set_Bus input values */
#define XMT_BUS_A       0x0080
#define XMT_BUS_B       0x0000
/* retained for backward compatibility */
#define CHANNEL_A       0x0080
#define CHANNEL_B       0x0000

        /* Set_Halt Set_Continue Set_Jump Set_Stop_On_Error */
#define HALT_CONT       0x2000
#define JUMP            0x0008
#define STOP_ON_ERROR   0x8000

        /* Set_Skip   Set_Restore  */
#define COMMAND_CODE    0x000F
#define SKIP            7
#define CTL_SEND_ONCE	8	//only used in frequency mode where Jump isn't relevant

        /* Set Interrupt BC mode options */
#define MSG_CMPLT       0x02     /* Message sent                              */
#define END_OF_FRAME    0x04     /* Complete frame of messages sent           */
#define MSG_ERR         0x08     /* Some error occurred                       */
#define END_MINOR_FRAME 0x10     /* Minor Frame completed		      */
#define SRQ_MSG		0x20     /* interrupt when srq messages sent */
#define VALID_BC_RT_INT 0x3E     /* combination of above for BC/RT mode       */
#define BEGIN_MSG	0x01
#define VALID_RT_INT    0x03     /* combination of BEGIN_MSG & MSG_CMPLT      */

        /* Set_Retry options */
#define RETRY_SAME_BUS  0x0000   /* All retries on active bus                 */
#define RETRY_ALT_BUS   0x0040   /* Retries flip flop between buses           */
#define RETRY_MASK      0x0070   /* mask for retry number and type            */

        /* Get_RT_Message (Read_Cmd_Stack) status flags */
/*#define END_OF_MSG    0x8000   (* Complete 1553 message received            */
#define BUS_A           0x4000   /* Message was transferred on Bus A          */
#define RT_BAD_CHECKSUM 0x2000   /* Bad Checksum on the RT (receive) side     */
#define TX_TIME_OUT     0x0400   /* Transmitter status word not received      */
#define INVALID_WORD    0x0080   /* Bad bit count, manchester or parity       */
#define BAD_WD_CNT      0x0020   /* Wrong number of words received            */
#define BROADCAST       0x0010   /* Broadcast command received                */
#define BAD_SYNC        0x0008   /* Wrong sync for command or data detected   */
#define BAD_GAP         0x0004   /* Invalid Gapp time between 1553 words      */
#define RT2RTMSG        0x0002   /* RT to RT message received                 */
#define MSG_ERROR       0x0001   /* Error occured defined in other flags      */

        /* Set_RT_Errors options */
#define BIT_CNT         0x0002   /* Insert bad bit count based on SET_BIT_CNT */
#define BAD_GAP_TIME    0x0004   /* Insert bad gap between data words 1 and 2 */
#define STATUS_PARITY   0x0010   /* Send status word with even parity         */
#define STATUS_SYNC     0x0020   /* Send status word with data sync           */
#define DATA_PARITY     0x0040   /* Send data word with even parity           */
#define DATA_SYNC       0x0080   /* Send data word with command sync          */
#define RT_ERR_MASK     0x00f6   /* Mask of all valid error bits              */

        /* Set_Trigger_Mode options */
#define STORE_ONLY      8        /* Store only messages matching trigger(s)   */
#define STORE_AFTER     0x10     /* Store all messages once trigger is matched*/
#define STATUS_TRIGGER  0x80     /* trigger based on message status word      */


        /* Get_Message msgstatus flags - for Bus Monitor */
/*#define END_OF_MSG    0x8000   (* Message transfer has been completed       */
#define TRIGGER_FOUND   0x4000   /* Message matches requested trigger         */
#define RT2RT_MSG       0x2000   /* Message was RT to RT transfer             */
#define INVALID_MSG     0x0400   /* Word count or sync error occurred         */
#define BM_BAD_CHECKSUM 0x0200   /* Bad Checksum on the BM side               */
#define BUS_A_XFER      0x0100   /* Message was transferred on bus A          */
#define INVALID_WORD    0x0080   /* Bad bit count, manchester or parity       */
#define WD_CNT_HI       0x0040   /* RT transmitted too many words             */
#define WD_CNT_LO       0x0020   /* RT transmitted too few words              */
#define BAD_RT_ADDR     0x0010   /* 1553 status word contained wrong address  */
#define BAD_SYNC        0x0008   /* Status or data sync was wrong             */
#define BAD_GAP         0x0004   /* Invalid gap time occurred between words   */
#define MON_LATE_RESP   0x0002  /* Response time error occured within message */
#define MSG_ERROR       0x0001   /* error occured defined in other flags      */
#define BAD_HEADER_MON	0x0060   /* 1760 header error */

        /* Set_Interrupt for monitor mode and Get_Mon_Status flags */
#define TRIG_RCVD       0x01     /* Message received that matched trigger word*/
#define MSG_IN_PROGRESS 0x02     /* Message in Progress                       */
#define CNT_TRIG_MATCH  0x04     /* Set_Cnt_Trig block number message received*/
#define MSG_INTERVAL	0x08	 /* interrupt every time 'interval' num of msgs rcvd */
#define VALID_MON_INT   0x0F     /* mask of all valid Monitor interrupts      */

        /* Create_Frame, set_jump input */
#define FULLFRAME       0        /* set instruction counter to full frame     */


        /* Set_MON_Concurrent input */
	/* Also use for Set_Replay */
	/* Also use for Set_Both_RT_Stacks_Px */
#define ENABLE                1  /* Set current module concurrent to its pair */
				 /* Enable Replay mode */
#define DISABLE               0  /* Set current module not concurrent to its pair*/
				 /* Disable Replay mode */

	/* Set_Both_RT_Stacks_Px */
#define RT_USE_BOTH_STACKS_BIT 0x4

	/* Enable_1553A_Support_Px */
#define BC1553A_BIT 0x1
#define RT1553A_BIT 0x2

/* used in BC_Check_Alter_Msg & RT_Check_Alter_Msg */
#define NO_ALTER        0x7F00   /* do NOT alter this message NOW */
/* return from BC_Check_Alter_Msgentry */
#define ALTER_MSG				1
#define DO_NOT_ALTER_MSG	0

        /* 0018 message status flags */
#define LL0018_GET_BUS        0x0100   /* message transferred on Bus A */
#define LL0018_BAD_CHECKSUM   0x0200   /* bad checksum in 0018 message */
#define LL0018_NOT_FFFF       0x0400   /* invalid 1st 0018 word */
#define LL0018_INVALID_WORD   0x0800   /* invalid 0018 word */

/* for use in Command_Word utility function */
#define TRANSMIT 1  /* transmit command */
#define RECEIVE  0  /* receive command	  */

/* for Get_Next_Message_RTM  (in addition to other flags, see documentation) */
#define INVALID_MSG_RT2RT 0x0100
#define MSG_WD_CNT_ERR 0x0020

/* for Read_RT_Status */
#define RTSTAT_MSG_RECEIVED	0x1
#define RTSTAT_BAD_DBBLOCKNUM	0x2

/* for Set_RT_Active_Bus */
#define BUS_A_ONLY	2
#define BUS_B_ONLY	1
#define BUS_AB		0
#define BUS_NONE	3

/* for Set_RTid_Interrupt_Px  */
#define INT_ON_ENDOFMSG	0
#define INT_ON_ERR	1

/* for Set_RTid_Status_Px */
#define RTID_ILLEGAL	2
#define RTID_INACTIVE	3
#define RTID_BUSY		7

/* for set_header_exists */
#define HEADER_DISABLE		0
#define HEADER_ENABLE		1
#define HEADER_ENABLE_XMT	2
#define HEADER_ENABLE_RCV	3

/* for set_1553status */
#define DUR_ALWAYS	0
#define DUR_ONETIME	1


/* These flags are used internally for compilation of galahad dll */
#define NUMBOARDS 16  
#define NUMMODULES 16
#define NUMCARDS ((NUMBOARDS * NUMMODULES) + 1)


#define RESET_DELAY_PX 2500 /* initcard and gset */
#define RESET_DELAY_MMSI 2500 /* initcard and gset */
#define ALLOWED_TO_RESET_DELAY 1500 /* hardware is not yet ready for software reset (timeout - more than 100 msec)*/


/* used in get_next_message_RTM/BCM */
#define CMON_STACK_SIZE_AP 0x80
#define CMON_STACK_SIZE 0x199
#define CMON_STACK_SIZE_EBMODE 0xCC

#define MSG_BLOCK2_SIZE_AP 0x780
#define MSG_BLOCK2_SIZE_IP 0x1400
#define MSG_BLOCK3_SIZE_IP 0x0780
#define MSG_BLOCK4_SIZE_IP 0x4000
#define MSG_BLOCK3_SIZE_IP_AMD 0x4000
#define MSG_BLOCK2_SIZE 0x1400

/* used in get_card_type */
#define MOD_TYPE_1760 0x0200
#define MOD_TYPE_1553 0x0100
#define MOD_TYPE_MASK 0x0300

#define MOD_1760			0x1760
#define MOD_1760_MON_ONLY	0x4D17
#define MOD_1760_SF			0x1F17
#define MOD_1553			0x1553
#define MOD_1553_MON_ONLY	0x4D53
#define MOD_1553_SF			0x1F53
#define MOD_NOT_ACTIVE		0x0
/* flags for Read_Next_Data */
#define DATA_OVERWRITTEN -1
#define NOT_OVERWRITTEN   -2


/* more board options, used in Set_Mode_Px, External_Loopback_Px, OnBoard_Loopback_Px */
#define MONITOR_ONLY_MODE			0x0001
#define ON_BOARD_LOOPBACK			0x0002
#define EXPANDED_BC_MSG_BLOCK		0x0008
#define ENHANCED_MONITOR			0x0010
#define EXPANDED_MONSEQ_MSG_BLOCK	0x0020	
#define BUSY_BIT_RTID				0x0040
#define MODE_PER_RT_SUPPORTED		0x0080
#define IRIG_TIMETAG_SUPPORTED		0x0100
#define RT_ENTRY_COUNTER_SUPPORTED	0x0200
#define MMSI_TIMING					0x0400
#define FREQUENCY_MODE_SUPPORTED	0x0400

/* module more board options bits */
#define MONITOR_ONLY_MODE_MASK       0x01
#define RT_SINGLE_FUNCTION_MASK		0x04

/* used in Enable_SRQ_Support_Px and Read_SRQ_Message_Px */
#define NOSRQ_BIT 0x2

/* Set Zero Cross Errors */
#define ZC_NO_ERROR		0
#define	ZC_LATE100		1
#define	ZC_LATE150		2
#define	ZC_LATE200		3
#define	ZC_EARLY100		4
#define	ZC_EARLY150		5
#define	ZC_EARLY200		6
#define	ZC_STUCK_HI		8
#define	ZC_STUCK_LO		9
#define	ZC_STUCK_DEAD		10

/* Set_Hubmode_MMSI */
#define HUB_SWITCH	0
#define HUB_LINK	1
#define HUB_SPEC	2

/* RT_Links_Control_MMSI_Px */
#define RT_LINKS_ACTIVATE	0
#define RT_LINKS_SHUTDOWN	1
#define RT_LINK_0	0x1
#define RT_LINK_1	0x2
#define RT_LINK_2	0x4
#define RT_LINK_3	0x8
#define RT_LINK_4	0x10
#define RT_LINK_5	0x20
#define RT_LINK_6	0x40
#define RT_LINK_7	0x80
#define RT_LINKS_ALL	0xFF

/* Set_Msg_Interval_Interrupt_value_px */
#define NO_MSG_INTERVAL_INTERRUPT 0

/* mask for exc_bd_rt->moduleFunction and exc_bd_monseq->moduleFunction register */
#define EXPANDED_BLOCK_MODE				1  // 0x0, bit 00
#define MODE_PER_RT                     4  // 0x100, bit 02
#define IRIG_TIMETAG_MODE				8  // 0x1000, bit 03
#define MAX_IRIGMON_REGULAR_SIZE_BLOCKS		181  //non IRIG is 200, IRIG blocks are bigger
#define MAX_IRIGMON_BLOCKS				736  // non IRIG is 800, IRIG blocks are bigger (44 words per entry instead of 40)

#define RTID_EXPANDED_BLOCK		0x40
#define RTID_EXPANDED_BLOCK_SHIFT	2
#define EXPANDED_MON_BLOCK_NUM_1	200
#define EXPANDED_MON_BLOCK_NUM_2	353
#define EXPANDED_MON_BLOCK_NUM_3	400
#define EXPANDED_MON_BLOCK_NUM_4	800
#define EXPANDED_IRIG_BLOCK_NUM_1            181
#define EXPANDED_IRIG_BLOCK_NUM_2            320
#define EXPANDED_IRIG_BLOCK_NUM_3            360  // not used in the code
#define EXPANDED_IRIG_BLOCK_NUM_4            723  // not used in the code

/* used for DMA reads of up to 4k */
#define MAX_NUMBER_BLOCKS_IN_DMA_READ 51
#define MAX_NUMBER_BLOCKS_IN_USB_READ 6
#define MAX_NUMBER_BLOCKS_IN_ETH_READ 18
#define END_CMON_BANK_BCRT	0xFFCF
#define END_CMON_BANK_RT_EXP	0xBFBF
#define END_MON_BANK_1	0x3E7F
#define END_MON_BANK_2	0x6FCF
#define END_MON_BANK_3	0x7FAF
#define END_MON_BANK_4	0xFCAF

#define PROC_INTEL	0
#define PROC_AMD		1
#define PROC_NIOS		2

#define RT_SINGLE_FUNCTION_RTNUM_BITS 0x1F
#define RT_SINGLE_FUNCTION_PARITY_BIT 0x20
#define RT_SINGLE_FUNCTION_LOCK_BIT	0x40
#define RT_SINGLE_FUNCTION_ERROR_BIT 0x80

#define BC_PROT_IMG_250N_RES        0x08
#define BC_PROT_SKIP_WITH_IMG       0x10
#define BC_PROT_FREQUENCY 		0x20

/* used in timeout loops; using a #define string makes it easier to globally change this value */
#define TIMEOUT_VAL	50

#endif
