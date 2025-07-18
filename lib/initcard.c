/* initcard for M4k1553PX */

#ifdef __linux__
//#include "exclinuxdef.h"
#include "WinTypes.h"
#include <unistd.h>
#define Sleep(x) (usleep(x*1000))
#include <string.h> 
#endif
#include <malloc.h>
#include "pxincl.h"
#include "excsysio.h"


#include "exc4000.h"

#define XILINX_RETRIES		3

#include "ExcUnetMs.h"
#include "UNetTiming.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)

INSTANCE_PX cardpx[NUMCARDS];
static int n_cards=0;			/* Flag indicating that this routine
								initialized the necessary cardpx array. */

int g_handlepx=0; /* index into the current board/module for backward compatibility*/

/* static LARGE_INTEGER *mFreq; */
/* static int mfFreqInitialized = 0; */
int Init_Attach_Px (usint device_num, usint module_num, BOOL resetFlag);
int InitPCMCIA(int handlepx, usint module_num, BOOL resetFlag);
int InitPCIe(int handlepx, BOOL resetFlag);


/* for old pcmcia/ep card */
#define CFG_REG_OFFSET 0x40000
#define SOFTWARE_RESET_BITVAL 0x80
#define BROADCAST_ENABLE_BITVAL 0x10
#define TTAG_RESET_BITVAL 0x20
extern int globalTemp;

#define RTBROADCAST_FLAG 0x01


/*
Init_Module    Coded 30.9.90 by D. Koppel

Description: This must be the first routine you call. Use this routine to
inform the drivers of the base address of the Excalibur card. The card is
initialized. If the SIMULATE argument is used,
a portion of memory equal to the size of the cards’ dual port RAM is set
aside. This area is then initialized with an id and version number for use
in testing programs when no card is available. Before exiting your program,
call Release_Card.

Input parameters:	SIMULATE indicating no actual board present.
			Any other input indicates M4K1553Px module on 4000PCI board.



Output parameters:	none

Return values:		emodnum			If invalid module number
					eboardtoomany	If no cardpx array elements left to
									support the newly desired module
					esim_no_mem		If init failed in Simulate mode due to
									malloc() call failure
					eopenkernel		If couldn't open kernel device; check ExcConfig settings
					enoid			If initialization could not identify the device specified
					exreset
					ekernelcantmap	If couldn't map memory
					etimeoutreset	If timed out waiting for reset
					handlepx		a number from 0-20, if successful

Change history:
	22-Nov-06(yyb):		Minor cleanup and comments.
	before 15-Nov-06:	Many changes.

9-Mar-2009	(YYB)
- changed the function-call to IsDMASupported() to call IsDMASupported_4000()
	instead.  IsDMASupported_4000() is a limiting wrapper function for
	IsDMASupported(); it returns false if the OS is Win9x, or if the card is
	a regular PCI 4000 card (as opposed to a PCI express card).
*/


int borland_dll Attach_Module_Px (usint device_num, usint module_num)
{
	BOOL resetFlag = FALSE;
	return Init_Attach_Px (device_num, module_num, resetFlag);
}

int borland_dll Init_Module_Px (usint device_num, usint module_num)
{
	BOOL resetFlag = TRUE;
	return Init_Attach_Px (device_num, module_num, resetFlag);
}

int Init_Attach_Px (usint device_num, usint module_num, BOOL resetFlag)
{
	int	i;
	int iError;						/* intermediate error routine passed from
									called module initialization routines. */
	int handlepx;

	struct exc_bc *allocBank = 0;

	int rtid;
	usint tempWord;

	// for checking if remote reset succeeded
	int tryxreset;  
	int efound;
	int xilinx_failed;
	LARGE_INTEGER startime;
	unsigned short int moreBoardOptions;

	/*	Check that the module number is a valid number; i.e., less than the count of modules
	if not a PCMCIA card -- PCMCIA cards are module number 100, 101.
	*/
	if ((module_num > NUMMODULES-1) && (module_num != EXC_PCMCIA_MODULE) && (module_num != EXC_PCMCIA_MODULE_2ND))
	{
		return emodnum;
	}

	/*	The first time init_module is called, initialize an array (named cardpx) of instance_px
	structures, with as many elements as we could ever need to track each module in the
	system. each instance_px structure stores module specific information, that is used by
	many functions in the DLL.
	*/
	if (n_cards == 0)
	{
		for (i=0; i<NUMCARDS; i++)
		{
			cardpx[i].nextframe = 0;
			cardpx[i].wait_for_trigger = 0;
			cardpx[i].triggers = 0;
			cardpx[i].trig_mode = STORE_AFTER;
			cardpx[i].allocated = 0;
			cardpx[i].intnum = 0;
			cardpx[i].isPCMCIA = 0;
			cardpx[i].isExCARD = 0;
			cardpx[i].isUNet = FALSE;
			cardpx[i].remoteDevHandle = -1;
			for (rtid=0; rtid < 2048; rtid++)
			{
				cardpx[i].NextBufToAccess[rtid] = 0;
			}
		}
		n_cards= 1;
	}

	/*	The first step to initialize a module is to allocate an instance_px structure, from the
	cardpx array to the module.

	Set the eboardtoomany flag (which translates to too many modules have had structs
	allocated and there are no more left in the cardpx array) just in case.  Next go look
	for either an cardpx element struct already allocated to the module, or an empty struct
	to allocate now.
	*/
	handlepx = eboardtoomany;

	/*	Check if the module was already allocated a cardpx element (and handle).
	If it was, just return the handle previously assigned.
	*/
	for (i=0; i<NUMCARDS; i++)
	{
		if ((cardpx[i].allocated == 1) &&
			(cardpx[i].card_addr != SIMULATE) &&
			(device_num == cardpx[i].card_addr) &&
			(module_num == cardpx[i].originalModuleNumber))
		{
			handlepx = i;
			g_handlepx = i;
			return handlepx;
		}
	}

	/*	If no cardpx element was allocated to the module, get an unallocated
		element and proceed to initialize it.
	*/
	for (i=0; i<NUMCARDS; i++)
	{
		if (cardpx[i].allocated == 0)
		{
			cardpx[i].card_addr = device_num;
			cardpx[i].module = module_num;  // this may later be modified; e.g. for PCMCIA cards or module numbers greater than 3
			cardpx[i].originalModuleNumber = module_num;  // keep track of the original module number here

			/* save original module number for use at top of Init_Module routine, in loop that determines if we have already initialized this module
			since the .module element of this structure will be changed if module_num>3 */
			
			handlepx = i;
			g_handlepx = i;
			break;
		}
	}

	/*	If no cardpx elements left, return the error.
	*/
	if (handlepx < 0)
	{
		return eboardtoomany;
	}

	// Let's get some board and module info.

	if (device_num != SIMULATE)
	{
		cardpx[handlepx].exc4000BrdType = 0;
		iError = Get_4000Board_Type(cardpx[handlepx].card_addr, &cardpx[handlepx].exc4000BrdType);
		if (iError <0)
			return iError; // bad module_num or could not open kernel device or could not map memory

 		if (module_num < EXC_PCMCIA_MODULE)
		{
			// if not PCMCIA card, then extract the module information using the 4000 level function
			// if PCMCIA card, we will set the appropriate structure element values in function InitPCMCIA
			iError = Get_4000Module_Info(cardpx[handlepx].card_addr, cardpx[handlepx].originalModuleNumber, &cardpx[handlepx].moduleInfo);
			if (iError <0)
				return iError;
		}
	}

	/*	Initialize module parameters for a simulated card (i.e. used for testing
	when no physical card is available.
	*/
	if (device_num == SIMULATE)
	{

		/* Allocate a block of memory (return an error if couldn't) to behave as the module's
		memory map, zero the memory and initialize some basic parameters.  (These parameters
		are usually initialized in the physical card, but for the simulated cards, they
		need initialization here.)
		*/
		allocBank = (struct exc_bc *) malloc ((size_t)0x7006);

		if (allocBank == 0x0)
		{
			return (esim_no_mem);
		}
		/* zero out the memory */
		for (i=0; i<0x7000; i++)
		{
			* ((char *) allocBank + i) = 0;
		}

		cardpx[handlepx].exc_bd_bc = allocBank;
		cardpx[handlepx].exc_bd_bc->board_id = 'E';			/* initialize board id */
		cardpx[handlepx].exc_bd_bc->board_status = 0x6f;	/* board ready all ok status */
		cardpx[handlepx].exc_bd_bc->revision_level = 0x00;	/* revision level 0.0 */
		cardpx[handlepx].intnum = 1;						/* interrupt 1 */

		cardpx[handlepx].processor = PROC_NIOS;

		cardpx[handlepx].isUNet = FALSE;
		cardpx[handlepx].remoteDevHandle = -1;

		cardpx[handlepx].dmaAvailable = FALSE;
		cardpx[handlepx].useDmaIfAvailable = FALSE;
		cardpx[handlepx].dmaBuffer.dmaCopyIsValid = FALSE;
		cardpx[handlepx].dmaBuffer.beginDpramOffset = 0;
		cardpx[handlepx].dmaBuffer.endDpramOffset = 0;
		cardpx[handlepx].isUnetMultiMsgReadSupported = FALSE;
		cardpx[handlepx].useUnetMultiMsgReadIfAvailable = FALSE;

		cardpx[handlepx].exc4000BrdType = 0;	// No such board type
#ifdef MMSI
		cardpx[handlepx].moduleInfo.moduleType = EXC4000_MODTYPE_MMSI;
#else
		cardpx[handlepx].moduleInfo.moduleType = EXC4000_MODTYPE_PX;
#endif
		cardpx[handlepx].moduleInfo.numOfBitsAccess = 16; // 16-bit access
		cardpx[handlepx].moduleInfo.moduleFamily = 4; // M4K Family
		strcpy(cardpx[handlepx].moduleInfo.moduleNameStr, "Simulated MMSI");

		cardpx[handlepx].irigTtagModeSupported = FALSE;
		cardpx[handlepx].irigModeEnabled = FALSE;

	}	// end of simulate

// ----- We have a live module; let's connect !!! -----

	// Next, check for UNet modules (which are easily identified by the registry 'IsRemote' value of 1).
	else if (b2drmIsRemoteDevice ((unsigned short) cardpx[handlepx].card_addr))
	{
		int sizeof_exc_bc;
		sizeof_exc_bc = sizeof(struct exc_bc);

		cardpx[handlepx].isUNet = TRUE;

		iError = b2drmInitRemoteDeviceModule (cardpx[handlepx].card_addr, cardpx[handlepx].module, &cardpx[handlepx].remoteDevHandle);
		if (iError < 0)
			return (iError);

		// Once we've made the connection with b2drmInitRemoteDeviceModule, we can turn on logging 
		// b2drmStartDebug(B2DRMHANDLE); 

		// Set a base address pointer to better access the module memory via struct pointers,
		// and tell the UNet that base address so it can calculate correct offsets.

// For some modules it is necessary to do a malloc()

		/*Allocate a block of memory to behave as the module's memory map and zero the memory */
		if (allocBank == 0)
			allocBank = (struct exc_bc *) malloc (sizeof_exc_bc);

		if (allocBank == 0)
			return (esim_no_mem);

		/* zero out the memory */
		for (i = 0; i < sizeof_exc_bc; i++)
			* ((char *) allocBank + i) = 0;
	
		// set the memory mapped base address of the device raw memory
		cardpx[handlepx].exc_bd_bc = allocBank;

		b2drmSetBaseAddress((DWORD_PTR) & (cardpx[handlepx].exc_bd_bc->stack[0]), B2DRMHANDLE );

		// Initialize the interesting instance variables
		cardpx[handlepx].isExCARD = 0;
		cardpx[handlepx].isPCMCIA = 0;
		cardpx[handlepx].processor = PROC_NIOS;
		cardpx[handlepx].intnum = 1;  // This is at least a flag that the intnum was set.
		cardpx[handlepx].dmaAvailable = FALSE;
		cardpx[handlepx].useDmaIfAvailable = FALSE;
		cardpx[handlepx].dmaBuffer.dmaCopyIsValid = FALSE;
		cardpx[handlepx].dmaBuffer.beginDpramOffset = 0;
		cardpx[handlepx].dmaBuffer.endDpramOffset = 0;
		cardpx[handlepx].isUnetMultiMsgReadSupported = b2drmIsCmdClearEOMStatusSupported((int) cardpx[handlepx].card_addr );
		cardpx[handlepx].useUnetMultiMsgReadIfAvailable = TRUE;
		cardpx[handlepx].irigTtagModeSupported = FALSE;
		cardpx[handlepx].irigModeEnabled = FALSE;
		cardpx[handlepx].lastCountRead = 0;


		if (resetFlag == TRUE)
		{
#ifndef _WIN32
			int d;
#endif

			//	Reseting the remote Px module and see that it starts up okay.
			WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_id,  ' ' );		/* blank the Excalibur 'E' */ 
			WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_status, 0x00 );	/* clear the flags that say the module is running healthily */

			//Reset_Card_Px(handlepx); /* reset the module	*/
			WBYTE_H2D( cardpx[handlepx].exc_bd_bc->reset_hard, 1 );

			// Wait a reasonable moment before checking too soon after the reset
#ifdef _WIN32
			StartTimer_Px(&startime);
			while(EndTimer_Px(startime, 50) != 1);
#else
			for(d=0;d<TIMEOUT_VAL;d++) {
				Sleep(1L);
			}
#endif

			// wait for reset

			tryxreset= 0;
			//	do
			//	{
				tryxreset++;
				xilinx_failed = 0;
				efound = 0;
#ifdef _WIN32
				StartTimer_Px(&startime);
				// RESET_DELAY_PX 25*100
//				while(EndTimer_Px(startime, RESET_DELAY_PX) != 1)
				while(EndTimer_Px(startime, 10000) != 1) // Acually wait longer, until USB_INLINE speeds up
				{

					if (RBYTE_D2H(cardpx[handlepx].exc_bd_bc->board_id) == 'E')	
					{
						efound = 1;
						break;
					}
				}
#else
			for(d=0;d<TIMEOUT_VAL;d++) {
				Sleep(200L);
				if (RBYTE_D2H(cardpx[handlepx].exc_bd_bc->board_id) == 'E')	
				{
					efound = 1;
					break;
				}
			}
#endif

			if (efound == 0) {
				b2drmReleaseRemoteAccessToDevice( cardpx[handlepx].card_addr, cardpx[handlepx].module, B2DRMHANDLE);
				return(etimeoutreset);
			}
		} // end ResetFlag == TRUE
		// At this stage the remote access to the PX  has been established

	} // end connect to UNet devices

	else // Connect to PCI[e] devices
	{
		// Make sure this set properly for all non-UNet devices
		cardpx[handlepx].isUNet = FALSE;
		cardpx[handlepx].remoteDevHandle = -1;

		if (cardpx[handlepx].exc4000BrdType == EXCARD_BRDTYPE_1553PX)
		{
			cardpx[handlepx].isExCARD = 1;

			if (module_num == EXC_PCMCIA_MODULE)
				module_num = 0;
			else if (module_num == EXC_PCMCIA_MODULE_2ND)
				module_num = 1;

			cardpx[handlepx].module = module_num;
		}
		else
			cardpx[handlepx].isExCARD = 0;

		//	Initialize (physical as opposed to simulated) PCMCIA cards.
 		if (module_num >= EXC_PCMCIA_MODULE)
		{
			iError = InitPCMCIA(handlepx, module_num, resetFlag);
		}

		//	Initialize (physical as opposed to simulated) M4K PX modules.
		else
		{
			iError = InitPCIe(handlepx, resetFlag);
		}

		/*	If physical cards/modules could not be initialized, bail out here, returning the error.
		If there was no error then get the interrupt number and set it in the cardpx element.
		(Setting the interrupt number is done here, before the general set cardpx parameters,
		because physical and simulated cards have their interrupts recorded differently.)
		*/
		if (iError < 0)
			return (iError);

		Get_IRQ_Number(cardpx[handlepx].card_addr, &cardpx[handlepx].intnum);

		/* Check for DMA support, and set the default to use it if available. */
		cardpx[handlepx].dmaAvailable = IsDMASupported_4000(device_num);
		cardpx[handlepx].useDmaIfAvailable = TRUE;
		cardpx[handlepx].dmaBuffer.dmaCopyIsValid = FALSE;
		cardpx[handlepx].dmaBuffer.beginDpramOffset = 0;
		cardpx[handlepx].dmaBuffer.endDpramOffset = 0;
		
		// Turn off UNet Multi-Msg-Read support
		cardpx[handlepx].isUnetMultiMsgReadSupported = FALSE;
		cardpx[handlepx].useUnetMultiMsgReadIfAvailable = FALSE;

		cardpx[handlepx].lastCountRead = 0;

	}	// end connect to PCI[e] (regular non-unet module) devices

	//  ====  ALL MODULES CONTINUE HERE ==== 


	/*	Populate the module's cardpx element.  This is done for all modules,
	both physical and simulated.
	*/
	cardpx[handlepx].allocated = 1;

	/*	Set the rest of the memory map pointers at the Px module, PCMCIA card, or the
	SIMULATED card's memory. (The .exc_bd_bc pointer was set by MapMemory calls
	during the (physical or simulated) card initialization routines.)
	*/
	cardpx[handlepx].exc_bd_dwnld = (struct exc_dwnld *)cardpx[handlepx].exc_bd_bc;
	cardpx[handlepx].exc_bd_monseq = (struct exc_mon_seq *)cardpx[handlepx].exc_bd_bc;
	cardpx[handlepx].exc_bd_monlkup = (struct exc_mon_lkup *)cardpx[handlepx].exc_bd_bc;
	cardpx[handlepx].exc_bd_rt = (struct exc_rt *)cardpx[handlepx].exc_bd_bc;
	cardpx[handlepx].exc_pc_dpr = (char *)cardpx[handlepx].exc_bd_bc;

	/* Set other miscellaneous pointers etc...  */

	/* Check if this is single function RT */
//	if ((cardpx[handlepx].exc_bd_bc->more_board_options & RT_SINGLE_FUNCTION_MASK) == RT_SINGLE_FUNCTION_MASK)
	moreBoardOptions = RWORD_D2H(cardpx[handlepx].exc_bd_bc->more_board_options );
	if (( moreBoardOptions & RT_SINGLE_FUNCTION_MASK) == RT_SINGLE_FUNCTION_MASK)
	{
		cardpx[handlepx].singleFunction = TRUE;

		/* get value of rt from register to put in rtNumber of INSTANCE_PX for single function*/
		Update_RT_From_Reg(handlepx);
	}
	else
		cardpx[handlepx].singleFunction = FALSE;
	
	if (( moreBoardOptions & FREQUENCY_MODE_SUPPORTED) == FREQUENCY_MODE_SUPPORTED)
		cardpx[handlepx].frequencyModeSupported = TRUE;
	else
		cardpx[handlepx].frequencyModeSupported = FALSE;		

	cardpx[handlepx].nextid = 1;
	cardpx[handlepx].nextframe = 0;

	cardpx[handlepx].nextmsg = cardpx[handlepx].exc_bd_bc->msg_block1;
	cardpx[handlepx].top_stack = &(cardpx[handlepx].exc_bd_bc->stack[0]);
	cardpx[handlepx].nextstack = cardpx[handlepx].top_stack;

	cardpx[handlepx].multibufUsed = 0;

	cardpx[handlepx].is32bitAccess = 0;
	if (cardpx[handlepx].moduleInfo.numOfBitsAccess == 32)
		cardpx[handlepx].is32bitAccess = 1;

	cardpx[handlepx].frequencyModeON = FALSE;

	for (i=0 ; i < MSGMAX; i++)
	{
		cardpx[handlepx].msgalloc[i] = 0;
	}

	// Load the board_config shadow register
	cardpx[handlepx].board_config = RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_config );
	cardpx[handlepx].mode_code = 0;

	/*	If processor is Intel then Set Expanded Block FW on modules with Firmware
	version 2.13 and later.  If the processor is AMD there is no Expanded Block FW.
	If the processor is NIOS then set Expanded Block FW flag.
	*/

	if (cardpx[handlepx].processor == PROC_INTEL)
	{
		if (cardpx[handlepx].exc_bd_bc->revision_level >= 0x2D)
			cardpx[handlepx].expandedRTblockFW = TRUE;
		else 
			cardpx[handlepx].expandedRTblockFW = FALSE;

		if (cardpx[handlepx].exc_bd_bc->revision_level >= 0x31)
		{
			cardpx[handlepx].expandedMONblockFW = TRUE;
			cardpx[handlepx].enhancedMONblockFW = TRUE;
		}
		else 
		{
			cardpx[handlepx].expandedMONblockFW = FALSE;
			cardpx[handlepx].enhancedMONblockFW = FALSE;
		}

		cardpx[handlepx].expandedBCblockFW = FALSE;

		if (cardpx[handlepx].exc_bd_bc->revision_level < 0x17)
			cardpx[handlepx].RTstacksStatus = ONLY_OLD_RT_STACK;
		else /* From Rev 1.7 PxII (Intel) supports both RT stacks */
			cardpx[handlepx].RTstacksStatus = BOTH_RT_STACKS;

		cardpx[handlepx].isIMG250ns_resolution = FALSE;  /* default for PxII is to use old method of calculating IMGs - as per 155 nanosec resolution */
		if (cardpx[handlepx].exc_bd_bc->revision_level >= 0x35)
		{
			cardpx[handlepx].isIMG250ns_resolution = TRUE;
		}

		cardpx[handlepx].irigTtagModeSupported = FALSE;
		cardpx[handlepx].irigModeEnabled = FALSE;
	}
	else
	{
		if (cardpx[handlepx].processor == PROC_NIOS)
		{
//			cardpx[handlepx].exc_bd_bc->bcProtocolOptions |= BC_PROT_IMG_250N_RES;
			tempWord = RWORD_D2H( cardpx[handlepx].exc_bd_bc->bcProtocolOptions );
			tempWord |= BC_PROT_IMG_250N_RES;
			WWORD_H2D( cardpx[handlepx].exc_bd_bc->bcProtocolOptions, tempWord );

			cardpx[handlepx].expandedRTblockFW = TRUE;
			
//			if ((cardpx[handlepx].exc_bd_bc->more_board_options & ENHANCED_MONITOR) == ENHANCED_MONITOR)
		    if (( moreBoardOptions & ENHANCED_MONITOR) == ENHANCED_MONITOR)
				cardpx[handlepx].enhancedMONblockFW = TRUE;	
			else
				cardpx[handlepx].enhancedMONblockFW = FALSE;
		
			
//			if ((cardpx[handlepx].exc_bd_bc->more_board_options & EXPANDED_MONSEQ_MSG_BLOCK) == EXPANDED_MONSEQ_MSG_BLOCK)
			if (( moreBoardOptions & EXPANDED_MONSEQ_MSG_BLOCK) == EXPANDED_MONSEQ_MSG_BLOCK)
				cardpx[handlepx].expandedMONblockFW = TRUE;
			else 
				cardpx[handlepx].expandedMONblockFW = FALSE;

//			if ((cardpx[handlepx].exc_bd_bc->more_board_options & EXPANDED_BC_MSG_BLOCK) == EXPANDED_BC_MSG_BLOCK)
			if (( moreBoardOptions & EXPANDED_BC_MSG_BLOCK) == EXPANDED_BC_MSG_BLOCK)
				cardpx[handlepx].expandedBCblockFW = TRUE;
			else 
				cardpx[handlepx].expandedBCblockFW = FALSE;

			if (( moreBoardOptions & RT_ENTRY_COUNTER_SUPPORTED) == RT_ENTRY_COUNTER_SUPPORTED)
				cardpx[handlepx].RTentryCounterFW = TRUE;
			else 
				cardpx[handlepx].RTentryCounterFW = FALSE;

//			if ((cardpx[handlepx].exc_bd_bc->more_board_options & IRIG_TIMETAG_SUPPORTED) == IRIG_TIMETAG_SUPPORTED)
			if (( moreBoardOptions & IRIG_TIMETAG_SUPPORTED) == IRIG_TIMETAG_SUPPORTED)
				cardpx[handlepx].irigTtagModeSupported = TRUE;
			else 
				cardpx[handlepx].irigTtagModeSupported = FALSE;
			cardpx[handlepx].irigModeEnabled = FALSE;


			/* By default: PxIII (Nios) supports only the new RT stack */ 
			cardpx[handlepx].RTstacksStatus = ONLY_NEW_RT_STACK;

//			if ((cardpx[handlepx].exc_bd_bc->more_board_options & MODE_PER_RT_SUPPORTED) == MODE_PER_RT_SUPPORTED)
			if ((moreBoardOptions & MODE_PER_RT_SUPPORTED) == MODE_PER_RT_SUPPORTED)
				cardpx[handlepx].isModePerRTSupported = TRUE;	
			else
				cardpx[handlepx].isModePerRTSupported = FALSE;
		}// if PROC_NIOS
		else  /* AMD */
		{
			cardpx[handlepx].expandedRTblockFW = FALSE;

			cardpx[handlepx].expandedMONblockFW = FALSE;
			cardpx[handlepx].enhancedMONblockFW = FALSE;

			cardpx[handlepx].expandedBCblockFW = FALSE;

			/* PxI (AMD) supports only the old RT stack */
			cardpx[handlepx].RTstacksStatus = ONLY_OLD_RT_STACK;

			cardpx[handlepx].irigTtagModeSupported = FALSE;
			cardpx[handlepx].irigModeEnabled = FALSE;

		}
	}

	/*	Initialization completed successfully.  Return the handle for the new module.
	*/
	return (handlepx);
}

int InitPCMCIA(int handlepx, usint module_num, BOOL resetFlag)
{
	int iError, efound, isPCMCIApx;
#ifdef _WIN32
	LARGE_INTEGER startime;
#endif
	int dprBank;  /* number to use for Dual-Port RAM memory bank */


/*	Call the system device driver and actually try to reach the PCMCIA card */
	iError = OpenKernelDevice(cardpx[handlepx].card_addr);
	if (iError)
	{
		return iError;	/* it returns eopenkernel */
	}

	/* Get a pointer to memory bank #0 on the PCMCIA card */
	dprBank = EXC_BANK_DUALPORTRAM;  /* default set to bank 0 */
	if (module_num == EXC_PCMCIA_MODULE_2ND) {
		dprBank = EXC_BANK_DUALPORTRAM2;  /* set to bank 3 */
	}

	iError = MapMemory(cardpx[handlepx].card_addr, (void **) &(cardpx[handlepx].exc_bd_bc),
		dprBank);
	if (iError)
	{
		CloseKernelDevice(cardpx[handlepx].card_addr);
		return iError;
	}

	/* ***	(9-jan-07) Removed the following section of code because it was deemed inappropriate by Excalibur engineers
	who were getting errors running PxTest after someone had intentionally zeroed out memory before
	doing a module reset.

	Double check to see that we can read the memory on the card; specifically look for the
	'E' that is characteristic of Excalibur Px (and PCMCIA) boards.

	if (cardpx[handlepx].exc_bd_bc->board_id != 'E')
	{
		CloseKernelDevice(cardpx[handlepx].card_addr);
		return enoid;
	}  *** */

/*	(31oct06) Identify processor. First check for AMD, since it may share the same bit used
	to detect XP250.  Use a different bit to detect AMD (0x0800) - Spartan Xilinx indicator.
	Note that this bit is used to indicate Replay mode in BC mode; but that happens only
	after a call to Init_Module_Px. So, we are safe using this processor detection methodology.
*/

	cardpx[handlepx].isPCMCIA = 1;
	if (cardpx[handlepx].exc_bd_bc->pcmciaEPorPx == 'P') /* NIOS PCMCIA-Px */
	{
		isPCMCIApx = 1;
		cardpx[handlepx].processor = PROC_NIOS;
		cardpx[handlepx].module = module_num - EXC_PCMCIA_MODULE;
		/* .module is needed for use in deviceio_px.c function calls;
		here we use numbers 0 & 1, since we operate on the module as a "regular" module */

		cardpx[handlepx].moduleInfo.moduleType = EXC_BRDTYPE_1553PCMCIAPX;
		cardpx[handlepx].moduleInfo.numOfBitsAccess = 16; // 16-bit access
		cardpx[handlepx].moduleInfo.moduleFamily = 4; // M4K Family
		strcpy(cardpx[handlepx].moduleInfo.moduleNameStr, "M4K1553Px");
	}
	else /* Intel & AMD */
	{
		isPCMCIApx = 0;

		cardpx[handlepx].moduleInfo.moduleType = EXC_PCMCIA_MODULE;
		cardpx[handlepx].moduleInfo.numOfBitsAccess = 16; // 16-bit access
		cardpx[handlepx].moduleInfo.moduleFamily = 2; // PCMCIA Family

		/* Intel PCMCIA-EPII */
		if ((cardpx[handlepx].exc_bd_bc->board_options & XP250_PROCESSOR) == XP250_PROCESSOR) 
		{
			strcpy(cardpx[handlepx].moduleInfo.moduleNameStr, "PCMCIA-EPII");
			cardpx[handlepx].processor = PROC_INTEL;
			cardpx[handlepx].module = SINGLE_MODULE_CARD;
			/* .module is needed for use in deviceio_px.c function calls; use special number */
		}
		else /* AMD PCMCIA-EP */
		{
			strcpy(cardpx[handlepx].moduleInfo.moduleNameStr, "PCMCIA-EP");
			cardpx[handlepx].processor = PROC_AMD;
			cardpx[handlepx].module = SINGLE_MODULE_CARD;
			/* .module is needed for use in deviceio_px.c function calls; use special number */

			/*	If the PCMCIA card has an AMD processor, get a pointer to third memory bank (bank #1),
			which is only in the AMD PCMCIA card.
			*/
			iError = MapMemory(cardpx[handlepx].card_addr,(void**)& (cardpx[handlepx].ctrl_reg), EXC_BANK_CONTROL);
			if (iError)
			{
				CloseKernelDevice(cardpx[handlepx].card_addr);
				return iError; /* it returns ekernelcantmap */
			}
		}
	}

	// Note that i must identify the processor type and set isPCMCIA & isExCARD flags
	// before calling Reset_Card_Px
	if (resetFlag == TRUE)
	{

		/*	Reset the PCMCIA card and see that it starts up okay */
		efound = 0;
		/* blank the Excalibur 'E'*/
		cardpx[handlepx].exc_bd_bc->board_id = ' ';


		cardpx[handlepx].exc_bd_bc->board_status = 0x00;	/* board the flags that say
														the card is running healthily	*/

		iError = Reset_Card_Px(handlepx);
		if (iError)
		{
			CloseKernelDevice(cardpx[handlepx].card_addr);
			return iError;
		}

#ifdef _WIN32
		/*  wait for reset to complete (RESET_DELAY_PX is 25*100)			*/
		StartTimer_Px(&startime);

#ifdef MMSI
		while (EndTimer_Px(startime,RESET_DELAY_MMSI) != 1)
#else
		while (EndTimer_Px(startime,RESET_DELAY_PX) != 1)
#endif
		{
			if (cardpx[handlepx].exc_bd_bc->board_id=='E')	/* look to see the 'E' is back		*/
			{
				efound = 1;
				break;
			}
		}
#else
		int d;
		/* wait for reset */
		efound = FALSE;
		for(d=0;d<TIMEOUT_VAL;d++) {
			Sleep(200L);
			if (cardpx[handlepx].exc_bd_bc->board_id=='E') {efound = TRUE; break;}
		}
#endif //_WIN32

		if (efound == 0)										/* if the 'E' is not back in time,
															return a timeout error			*/
		{
			CloseKernelDevice(cardpx[handlepx].card_addr);
			cardpx[handlepx].card_addr = 99;
			cardpx[handlepx].module = 99;
			return(etimeoutreset);
		}

		if ((cardpx[handlepx].exc_bd_bc->board_status & 0xf) != 0xf) 	/* or if the module is not
																	healthy in time, return a timeout error	*/
		{
			CloseKernelDevice(cardpx[handlepx].card_addr);
			return(etimeoutreset);
		}

	} // end of "if (resetFlag == TRUE)"

	/*	If made it this far the PCMCIA card is OK, thus return success! */
	return 0;
}

int InitPCIe(int handlepx, BOOL resetFlag)
/* Note that this function is dependent on the Windows API Timer (hence it uses the LARGE_INTEGER type).
	Since this function is dependent on a Windows API call, it is not portable to other operating systems as written.	*/
{
#ifdef _WIN32
	LARGE_INTEGER startime;
#endif
	int efound, iError, xilinx_failed, tryxreset;

	/* ****************************************************************************
	06nov07: Added fix to support newer boards with more than 4 modules; 
	eg, AFDX has 6 modules, EXC-4000P104plus board with 5 modules

	Adjust the value for module numbers 4-7 to be 5-8; module #4 is the Global Registers area 

	if (module_num > 3)
		module_num++
	card[handle].module = adjusted value

	The functions
	- MapMemory
	- Get_Channel_Interrupt_Count (which underlies Get_Interrput_Count_XXX)
	need the adjusted value as an argument to these functions.
	**************************************************************************** */

	if (cardpx[handlepx].module > 3) cardpx[handlepx].module += 1;

#ifdef MMSI
	// if(modtype != EXC4000_MODTYPE_MMSI)
	if (cardpx[handlepx].moduleInfo.moduleType != EXC4000_MODTYPE_MMSI)

#else
	// if(modtype != EXC4000_MODTYPE_PX)
	if (cardpx[handlepx].moduleInfo.moduleType != EXC4000_MODTYPE_PX)
#endif
	{
		if (cardpx[handlepx].moduleInfo.moduleType == EXC4000_MODTYPE_NONE)
		{
			return enomodule; /* make new message */
		}
		else
		{
			return ewrngmodule;
		}
	}


	/*	Call the windows/system device driver and actually try to reach the module.)
	*/
	iError = OpenKernelDevice(cardpx[handlepx].card_addr);
	if (iError)
	{
		return iError; /* it returns eopenkernel */
	}

	/*	Get a pointer to the memory on board the Px module.
	*/
	iError = MapMemory(cardpx[handlepx].card_addr, (void **) &(cardpx[handlepx].exc_bd_bc),
		cardpx[handlepx].module);
	if (iError)
	{
		CloseKernelDevice(cardpx[handlepx].card_addr);
		return iError;
	}

/*
// Update 11-Feb-2021 YYB.... B2DRMHANDLE (aka cardpx[handlepx].remoteDevHandle) is now -1 for non-UNet family devices
// so boards using the old b2drmInitRemoteAccessToDevice connections can still get the UNet debugging, but we would need
// to add a solution for boards using the new b2drmInitRemoteDeviceModule type connection.

// Also the calls to b2drmSetBaseAddress() and b2drmStartLocalAccessToPx() are now irrelevant.

// Uncomment the flags_px.h #define ENABLE_LOCAL_H2D_DEBUGS
// to enable Host2Dpr-like debugs on local modules

#ifdef ENABLE_LOCAL_H2D_DEBUGS
	b2drmStartDebug(B2DRMHANDLE);
#endif

	// set the memory mapped base address of the device raw memory
	b2drmSetBaseAddress((unsigned int) & (cardpx[handlepx].exc_bd_bc->stack[0]), B2DRMHANDLE );

	// we have a LOCAL access device, so we do the real init on the local Px
	b2drmStartLocalAccessToPx(B2DRMHANDLE);
*/

	/*	(9-jan-07) Removed the following section of code because it was deemed inapropriate by Excalibur enginneers
	that were getting errors running PX test after someone had intentionally zeroed out memory before
	doing a module reset.

	Double check to see that we can read the memory on the module; specifically look for the
	'E' that is characteristic of Excalibur Px (and PCMCIA) boards.

	if (cardpx[handlepx].exc_bd_bc->board_id != 'E')
	{
	CloseKernelDevice(cardpx[handlepx].card_addr);
	return enoid;
	}*/

	/*	(31oct06) Identify processor. First check for AMD, since it may share the same bit used
	to detect XP250.  Use a different bit to detect AMD (0x0800) - Spartan Xilinx indicator.
	Note that this bit is used to indicate Replay mode in BC mode; but that happens only
	after a call to Init_Module_Px. So, we are safe using this processor detection methodology.
	*/

//	if ((cardpx[handlepx].exc_bd_bc->board_options & AMD_PROCESSOR) == AMD_PROCESSOR)
	if ((RWORD_D2H(cardpx[handlepx].exc_bd_bc->board_options) & AMD_PROCESSOR) == AMD_PROCESSOR)	
	{
		cardpx[handlepx].processor = PROC_AMD;
	}
	else
	{
//		if ((cardpx[handlepx].exc_bd_bc->board_options & XP250_PROCESSOR) == XP250_PROCESSOR)
		if ((RWORD_D2H(cardpx[handlepx].exc_bd_bc->board_options) & XP250_PROCESSOR) == XP250_PROCESSOR)
		{
			cardpx[handlepx].processor = PROC_INTEL;
		}
		else
		{
//			if ((cardpx[handlepx].exc_bd_bc->board_options & NIOSII_PROCESSOR) == NIOSII_PROCESSOR)
			if ((RWORD_D2H(cardpx[handlepx].exc_bd_bc->board_options) & NIOSII_PROCESSOR) == NIOSII_PROCESSOR)
			{
				cardpx[handlepx].processor = PROC_NIOS;
			}
		}
	}

	// Note that i must identify the processor type and set isPCMCIA & isExCARD flags
	// before calling Reset_Card_Px
	if (resetFlag == TRUE)
	{

		/*	Reset the module and see that it starts up okay.
		*/
//		cardpx[handlepx].exc_bd_bc->board_id = ' ';  /* blank the Excalibur 'E'	*/
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_id,  ' ' );  /* blank the Excalibur 'E' */
//		cardpx[handlepx].exc_bd_bc->board_status = 0x00;  /* board the flags that say the module is running healthily	*/
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_status, 0x00 );

		cardpx[handlepx].isPCMCIA = 0;

		Reset_Card_Px(handlepx);  /* reset the module */

		/* wait for reset */

		tryxreset= 0;
		do
		{
			tryxreset++;
			xilinx_failed = 0;
			efound = 0;
#ifdef _WIN32
			StartTimer_Px(&startime);

#ifdef MMSI
			// RESET_DELAY_MMSI 25*100
			while(EndTimer_Px(startime, RESET_DELAY_MMSI) != 1)
#else
			// RESET_DELAY_PX 25*100
			while(EndTimer_Px(startime, RESET_DELAY_PX) != 1)
#endif
			{
//				if (cardpx[handlepx].exc_bd_bc->board_id=='E')
				if (RBYTE_D2H(cardpx[handlepx].exc_bd_bc->board_id) == 'E')
				{
					WBYTE_H2D( cardpx[handlepx].exc_bd_bc->errCount, 0xab );  // IZAK Remove 
					efound = 1;
					break;
				}
			}
#else
			int d;
			for(d=0;d<TIMEOUT_VAL;d++) {
				Sleep(200L);
//				if (cardpx[handlepx].exc_bd_bc->board_id=='E')
				if (RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_id ) == 'E')
					{efound = TRUE; break;}

			}
#endif // _WIN32

			if (efound == 0)
			{
				CloseKernelDevice(cardpx[handlepx].card_addr);
				return(etimeoutreset);
			}

			/* When the (Xilinx) hardware failed, the board status is set to 2 (RAM_OK). */
//			if ((cardpx[handlepx].exc_bd_bc->board_status & 0xf)==0x2)
			if ((RBYTE_D2H( cardpx[handlepx].exc_bd_bc->board_status) & 0xf)==0x2)
			{
				xilinx_failed = 1;
				/* xilinx did not load properly; reset it 3 times & then reset card */
				if (tryxreset == XILINX_RETRIES)
				{
					CloseKernelDevice(cardpx[handlepx].card_addr);
					return exreset;
				}

//				cardpx[handlepx].exc_bd_bc->xilinx_reset = 0x1;
//				cardpx[handlepx].exc_bd_bc->xilinx_reset = 0x1;
//				cardpx[handlepx].exc_bd_bc->xilinx_reset = 0x1;
				WBYTE_H2D( cardpx[handlepx].exc_bd_bc->xilinx_reset, 1 );
				WBYTE_H2D( cardpx[handlepx].exc_bd_bc->xilinx_reset, 1 );
				WBYTE_H2D( cardpx[handlepx].exc_bd_bc->xilinx_reset, 1 );

				/* board reset */
//				cardpx[handlepx].exc_bd_bc->board_id = ' ';  // initialize board to garbage
				WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_id, ' ' );  // initialize board to garbage

//				cardpx[handlepx].exc_bd_bc->board_status = 0x00; // board reset will fix it
				WBYTE_H2D( cardpx[handlepx].exc_bd_bc->board_status, 0x00 ); // board reset will fix it

				Reset_Card_Px(handlepx);
			}
		} while(xilinx_failed == 1); /* end of xilinx do while loop */

	} // end of "if (resetFlag == TRUE)"


	/*	If made it this far the Px module is OK, thus return success!
	*/
	return 0;
}

/*
Release_Module

Description: Before exiting a program, you must call this routine for each
module you initialized with Init_Module.

Input parameters:  handlde to card/module
Output parameters: none
Return values:  0 always
*/

int borland_dll Release_Module_Px(int handlepx)
{
	if ((handlepx <0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated == 1)
	{
		// Check if this is a remote device. Note: "card_addr" saves the Device number
		//if (b2drmIsRemoteDevice ((unsigned short) cardpx[handlepx].card_addr))
		if (cardpx[handlepx].isUNet == TRUE)
		{
			b2drmReleaseRemoteAccessToDevice( cardpx[handlepx].card_addr, cardpx[handlepx].module, B2DRMHANDLE);

			// We tried initiallizing the UNet code with and without a malloc(); this will cover us either way.
			if (cardpx[handlepx].exc_bd_bc != NULL)
			{
				free(cardpx[handlepx].exc_bd_bc);
				cardpx[handlepx].exc_bd_bc = NULL;
			}
		}
		else if (cardpx[handlepx].card_addr == SIMULATE)
			free(cardpx[handlepx].exc_bd_bc);
		else
		{
			if (cardpx[handlepx].module == SINGLE_MODULE_CARD)
				Release_Event_Handle(cardpx[handlepx].card_addr);
			else
				Release_Event_Handle_For_Module(cardpx[handlepx].card_addr, cardpx[handlepx].module);
			CloseKernelDevice(cardpx[handlepx].card_addr);
		}
		
		cardpx[handlepx].allocated = 0;
		cardpx[handlepx].card_addr = INVALID_DEV_PX;
		cardpx[handlepx].module = INVALID_DEV_PX;
		cardpx[handlepx].originalModuleNumber = INVALID_DEV_PX;
		cardpx[handlepx].intnum = 0;
		cardpx[handlepx].isUNet = FALSE;
		cardpx[handlepx].remoteDevHandle = -1;
		return 0;
	}
	else
		return ebadhandle;
}

/* for backward compatibility only */
int borland_dll Select_Module(int mod)
{
	/* here you select the module within the current board */

	int i;

	if (mod >= NUMMODULES)
		return einval;

	for (i=0; i<NUMCARDS; i++)
	{
		if ((cardpx[i].allocated == 1) &&
			(cardpx[i].module == mod))
		{
			g_handlepx = i;
			return g_handlepx;
		}
	}

	return eboardnotalloc;
}

int borland_dll Get_Module (void)
{
	return (cardpx[g_handlepx].module) ;
}

/*
Get_Time_Tag

Description: Returns the running time tag of the board.
modes.
Input parameters:  none
Output parameters: none
Return values:	32 bit Time_Tag
*/

unsigned int borland_dll Get_Time_Tag(void)
{
	return Get_Time_Tag_Px(g_handlepx);
}

int borland_dll GetTimeTag_Px(int handlepx, unsigned int *Time_Tag)
{
	*Time_Tag = Get_Time_Tag_Px(handlepx);
	return 0;
}

unsigned int borland_dll Get_Time_Tag_Px(int handlepx)
{
	usint tempword[2];
	unsigned int Time_Tag; /* 32 bit time tag value */

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1))
	{
		tempword[0] = *(cardpx[handlepx].ctrl_reg);
		tempword[1] = *(cardpx[handlepx].ctrl_reg+1);
	}
	else
	{
		// read ttagLo twice, to help verify that we freeze it for reading
//		tempword[0] = cardpx[handlepx].exc_bd_bc->time_tag[0];
//		tempword[0] = cardpx[handlepx].exc_bd_bc->time_tag[0];
		tempword[0] = RWORD_D2H( cardpx[handlepx].exc_bd_bc->time_tag[0] );
		tempword[0] = RWORD_D2H( cardpx[handlepx].exc_bd_bc->time_tag[0] );

//		tempword[1] = cardpx[handlepx].exc_bd_bc->time_tag[1];
		tempword[1] = RWORD_D2H( cardpx[handlepx].exc_bd_bc->time_tag[1] );
	}
	Time_Tag = ((unsigned int)tempword[1]<<16) + tempword[0];
	return Time_Tag;
}

/*
Reset_Time_Tag	   Coded 14-7-96 by David Gold
Description: Resets the current bank's time tag register.
Input parameters:  none
Output parameters: none
Return values:	0
*/

int borland_dll Reset_Time_Tag (void)
{
	return Reset_Time_Tag_Px(g_handlepx);
}
int borland_dll Reset_Time_Tag_Px (int handlepx)
{
	usint dwValue;
	int i;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* cardpx[handlepx].lastmsgttag = 0; */
	if ((cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1))
	{
		if (cardpx[handlepx].card_addr != SIMULATE)
		{
			ReadAttributeMemory(cardpx[handlepx].card_addr, CFG_REG_OFFSET, &dwValue);
			dwValue |= TTAG_RESET_BITVAL;   /* 0x20 */
			WriteAttributeMemory(cardpx[handlepx].card_addr, CFG_REG_OFFSET, dwValue);
			/* Sleep (1L); */
			for (i=0; i<10; i++)
				globalTemp = cardpx[handlepx].exc_bd_bc->revision_level;
			dwValue &= ~TTAG_RESET_BITVAL;  /* 0x20 */
			WriteAttributeMemory(cardpx[handlepx].card_addr, CFG_REG_OFFSET, dwValue);
		}
	}
	else
//		cardpx[handlepx].exc_bd_bc->time_tag_reset = 1;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->time_tag_reset, 1 );
	return(0);
}

int borland_dll Set_RT_Broadcast(int toggle)
{
	return Set_RT_Broadcast_Px(g_handlepx, toggle);
}
int borland_dll Set_RT_Broadcast_Px(int handlepx, int toggle)
{
	usint dwValue;
	usint tempBFlag;

#ifdef MMSI
	return (func_invalid);
#else

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_rt->board_config != RT_MODE)
	if (cardpx[handlepx].board_config != RT_MODE)
		return (emode);

	if ((cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1))
	{
		if (cardpx[handlepx].card_addr != SIMULATE)
		{
			ReadAttributeMemory(cardpx[handlepx].card_addr, CFG_REG_OFFSET, &dwValue);
			if (toggle)
				dwValue |= BROADCAST_ENABLE_BITVAL;
			else
				dwValue &= ~BROADCAST_ENABLE_BITVAL;
			WriteAttributeMemory(cardpx[handlepx].card_addr, CFG_REG_OFFSET, dwValue);
		}
	}
	else
	{
		if (toggle)
		{
//			cardpx[handlepx].exc_bd_rt->options_select = 1;
			WBYTE_H2D( cardpx[handlepx].exc_bd_rt->options_select, 1 );

//			cardpx[handlepx].exc_bd_rt->broadcast_flag |= RTBROADCAST_FLAG;
			tempBFlag = RWORD_D2H( cardpx[handlepx].exc_bd_rt->broadcast_flag );
			WWORD_H2D( cardpx[handlepx].exc_bd_rt->broadcast_flag, (tempBFlag | RTBROADCAST_FLAG) );
		}
		else
		{
//			cardpx[handlepx].exc_bd_rt->options_select = 0;
			WBYTE_H2D( cardpx[handlepx].exc_bd_rt->options_select, 0 );

//			cardpx[handlepx].exc_bd_rt->broadcast_flag &= ~RTBROADCAST_FLAG;
			tempBFlag = RWORD_D2H( cardpx[handlepx].exc_bd_rt->broadcast_flag );
			WWORD_H2D( cardpx[handlepx].exc_bd_rt->broadcast_flag, (tempBFlag & ~(usint)RTBROADCAST_FLAG) );
		}
	}
	return (0);
#endif
}

/* The following are for internal use only */

int Reset_Card_Px(int handlepx)
{
#ifdef __linux__
	int pollcount;
#endif
	usint dwValue;
	int i, allowedReset;
#ifdef _WIN32
	LARGE_INTEGER startime;
#endif

	if ((cardpx[handlepx].processor == PROC_AMD) && (cardpx[handlepx].isPCMCIA == 1))
	{
		if (cardpx[handlepx].card_addr != SIMULATE)
		{
			ReadAttributeMemory(cardpx[handlepx].card_addr, CFG_REG_OFFSET, &dwValue);
			dwValue |= SOFTWARE_RESET_BITVAL;  /*0x80; */
			WriteAttributeMemory(cardpx[handlepx].card_addr, CFG_REG_OFFSET, dwValue);
			/* Sleep(2L); */
			for (i=0; i<10; i++)
				globalTemp = cardpx[handlepx].exc_bd_bc->revision_level;
			ReadAttributeMemory(cardpx[handlepx].card_addr, CFG_REG_OFFSET, &dwValue);
			dwValue &= ~SOFTWARE_RESET_BITVAL;
			WriteAttributeMemory(cardpx[handlepx].card_addr, CFG_REG_OFFSET, dwValue);
		}
	}
	else
	{
		if ((cardpx[handlepx].processor == PROC_NIOS) && 
			((cardpx[handlepx].isPCMCIA == 1) || (cardpx[handlepx].isExCARD))) 
		{
			/* Before reset, check if the module is present and that we are allowed to do reset */

			allowedReset = 0;

			if (cardpx[handlepx].module == 0) /* channel 0 */
			{
//				if ((cardpx[handlepx].exc_bd_bc->dualChannelReg & 0x8) == 0x8) /* channel 0 is present on the card */
				if ((RBYTE_D2H(cardpx[handlepx].exc_bd_bc->dualChannelReg) & 0x8) == 0x8) /* channel 0 is present on the card */

				{
					/* Poll to verify that the hardware says that we are now allowed to reset the channel */
#ifdef _WIN32
					StartTimer_Px(&startime);
					while (EndTimer_Px(startime, ALLOWED_TO_RESET_DELAY) != 1)
					{
//						if ((cardpx[handlepx].exc_bd_bc->dualChannelReg & 0x1) == 0x1)
						if ((RBYTE_D2H(cardpx[handlepx].exc_bd_bc->dualChannelReg) & 0x1) == 0x1)
						{
							allowedReset = 1;
							break;
						}
					}
#else
					// we allow up to 1.5 seconds - thus we will loop up to 150 times, each time waiting
					// 10 milliseconds.
					for (pollcount = 0; pollcount < 15; pollcount++) {
						usleep(10000);  // specified in microseconds
						if ((cardpx[handlepx].exc_bd_bc->dualChannelReg & 0x1) == 0x1)
						{
							allowedReset = 1;
							break;
						}
					}
#endif // _WIN32
				}
				else return enomodule; /*  Channel 0 - Not present */
				
				
			}
			else if (cardpx[handlepx].module == 1) /* channel 1 */
			{
				
//				if ((cardpx[handlepx].exc_bd_bc->dualChannelReg & 0x10) == 0x10)/* channel 1 is present on the card */
				if (( RBYTE_D2H(cardpx[handlepx].exc_bd_bc->dualChannelReg) & 0x10) == 0x10)/* channel 1 is present on the card */
					
				{
					/* Poll to verify that the hardware says that we are now allowed to reset the channel */
#ifdef _WIN32
					StartTimer_Px(&startime);
					while (EndTimer_Px(startime, ALLOWED_TO_RESET_DELAY) != 1)
					{
//						if ((cardpx[handlepx].exc_bd_bc->dualChannelReg & 0x2) == 0x2)
						if ((RBYTE_D2H(cardpx[handlepx].exc_bd_bc->dualChannelReg) & 0x2) == 0x2)
						{
							allowedReset = 1;
							break;
						}
					}
#else
					// we allow up to 1.5 seconds - thus we will loop up to 150 times, each time waiting
					// 10 milliseconds.
					for (pollcount = 0; pollcount < 15; pollcount++) {
						usleep(10000);  // specified in microseconds
						if ((cardpx[handlepx].exc_bd_bc->dualChannelReg & 0x1) == 0x1)
						{
							allowedReset = 1;
							break;
						}
					}
#endif // _WIN32
				}
				else return enomodule; /*  Channel 1 - Not present */
			}
			
			if (!allowedReset) return enotallowedreset;
			
			
			
			
		}
		
		
//		cardpx[handlepx].exc_bd_bc->reset_hard = 1;
		WBYTE_H2D( cardpx[handlepx].exc_bd_bc->reset_hard, 1 );

		// Wait a reasonable moment before checking too soon after the reset
		Sleep (30);

	}

	return 0;
}

/*
DelayMicroSec_Px

Description: Replacement for Sleep function, to wait a specific amount of time.
We use the on-board time tag to determine how much time has passed, to implement
the time delay. Hence, it is more accurate than the Windows API Sleep function.

Input parameters:	handlepx
					delayMicroSec		the number of micro seconds to wait
Output parameters:	none
Return value:			0				OK
							ebadttag		problem with Get_Time_Tag
*/

int borland_dll DelayMicroSec_Px (int handlepx, unsigned int delayMicroSec)
{
	unsigned int timeElapsed, startTime, currentTime;
	unsigned int prevTimeElapsed;
	unsigned int wrapTime=0xffffffff;   /* value of timetag at roll-over back to 0 */
	int num_calls_to_ttag = 0;  /* to check for frozen ttag */
	int res, retval=0;
	usint source;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if (b2drmIsRemoteDevice ((unsigned short) cardpx[handlepx].card_addr))
	{
		Sleep(delayMicroSec/1000);
		return 0;
	}
	/* ??? what happens here for PCMCIA/PxIII */
	if (cardpx[handlepx].isPCMCIA != 1)
	{
		Get_Time_Tag_Source_4000 (cardpx[handlepx].card_addr, &source);
		if (source == EXC4000_EXTERNAL_CLOCK)
			retval = ettagexternal;
	}
	startTime = currentTime = Get_Time_Tag_Px(handlepx);
	prevTimeElapsed = timeElapsed = 0;

	// In BC mode, this register address 0x3FF7 is reserved. It seems to be always set to 0. So the time_res will be set to 1*4=4, which is correct.
//	res = (cardpx[handlepx].exc_bd_rt->time_res +1) * 4;  /* res is (4 * time_res) ms */
	res = (RBYTE_D2H( cardpx[handlepx].exc_bd_rt->time_res ) + 1) * 4;  /* res is (4 * time_res) ms */

	while (timeElapsed < delayMicroSec)
	{
		currentTime = Get_Time_Tag_Px(handlepx);
//		if ( (num_calls_to_ttag++ > 100) && (currentTime == startTime) )
//			return ebadttag;
		if ( (num_calls_to_ttag++ > 5) && (currentTime == startTime) )
		{
			retval = ebadttag;
			break;
		}

		if (currentTime < startTime)  /* wraparound check */
			timeElapsed = (wrapTime - startTime) + currentTime;
		else
			timeElapsed = (currentTime - startTime);

		timeElapsed *= res;
		if (timeElapsed == prevTimeElapsed)
		{
			retval = ebadttag;
			break;
		}
		else
			prevTimeElapsed = timeElapsed;
	}
	return retval;
}

#ifdef _WIN32
/*
StartTimer_Px

Description: StartTimer_Px and EndTimer_Px are used together to track time.  They are based
on the the high performance Windows Timer API, which is able to provide times that exceed 1
microsecond accuracy on most hardware. (The type LARGE_INTEGER is a union, and .Quadpart is
the piece of that union that is a signed 64-bit integer.  For details of the union see:
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winprog/winprog/large_integer_str.asp)

Note that since this function is dependent on the Windows API Timer, it is not portable to other
operating systems as written.

Input parameters:	none

Output parameters:	lStartTime	-	The number of ticks (that had occurred since the
									system boot) when the lStartTime variable was set.

Return value:		none

*/

int StartTimer_Px(LARGE_INTEGER *lStartTime)
{
	/*	Use the Windows API Timer to set lStartTime to the number of ticks since the system booted.*/
	
	QueryPerformanceCounter(lStartTime);

	if ((*lStartTime).QuadPart == 0)
	{
		return ewintimerapifail;
	}

	return 0;

}	/* end function StartTimer_Px */


/*

EndTimer_Px

Description: StartTimer_Px and EndTimer_Px are used together to track time.  They are based
on the the high performance Windows Timer API, which is able to provide times that exceed 1
microsecond accuracy on most hardware. (The type LARGE_INTEGER is a union, and .Quadpart is
the piece of that union that is a signed 64-bit integer.  For details of the union see:
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winprog/winprog/large_integer_str.asp)

Note that since this function is dependent on the Windows API Timer, it is not portable to other
operating systems as written.

Input parameters:	lStartTime	-	The number of ticks (that had occurred since the
									system boot) when the lStartTime variable was set.
					MsDelay		-	Minimum number of milliseconds to wait

Output parameters:	none

Return value:		ewintimerapifail -	failed attempt to use the Windows high-resolution
									performance counter/timer
					0			-	time is not up yet
					1			-	time is up

*/

int EndTimer_Px(LARGE_INTEGER lStartTime, unsigned int MsDelay)
{

	static LARGE_INTEGER lFreq;
	static int freqInitialized = 0;

	float time;
	LARGE_INTEGER lCurrentTime;
	LARGE_INTEGER lDelta;

	if (freqInitialized == 0)
	{
		QueryPerformanceFrequency(&lFreq);
		freqInitialized = 1;
	}

	if (lFreq.QuadPart == 0)
	{
		return ewintimerapifail;
	}

	/*	Get the number of ticks that have passed since the system booted, and calculate how many
	ticks (and then how much time) have passed since lStartTime.
	*/
	QueryPerformanceCounter(&lCurrentTime);
	lDelta.QuadPart = lCurrentTime.QuadPart - lStartTime.QuadPart;

	time = (float)(((double)lDelta.QuadPart*1000.)/(double)lFreq.QuadPart);

	/*	Return the "Is the time-up?" results.
	*/
	if ((time) > (float)MsDelay)
		return 1;
	else
		return 0;
}	/* end function EndTimer_Px */
#endif

/* ONLY for porting code written for pci/px or pci/ep card to M4K1553Px. NOT RECOMMENDED */
int borland_dll Init_Card(usint device_num)
{
	if (device_num == EXC_1553PCIPX)
		device_num = EXC_4000PCI;

	if (device_num == EXC_1553PCI)
		device_num = EXC_4000PCI;

	return Init_Module_Px (device_num, 0);
}


/* ONLY for porting code written for pci/px or pci/ep card to M4k1553Px. NOT RECOMMENDED */
int borland_dll Release_Card (void)
{
	return Release_Module_Px (0);
}


/* for PCMCIA/Px only */
int borland_dll Host_Reset_PCMCIA_Px(int handlepx)
{
	/* Perform Host Reset - to reload the firmware onto the modules of the card */
	int iError;
	int dprBank;  /* number to use for Dual-Port RAM memory bank */
	usint volatile *bank1;

	dprBank = 1;


	/*This function is only for PCMCIA/Px */
	if (cardpx[handlepx].exc_bd_bc->pcmciaEPorPx != 'P') return func_invalid;

	iError = MapMemory(cardpx[handlepx].card_addr, (void **) &(bank1),dprBank);
	if (iError)
	{
		CloseKernelDevice(cardpx[handlepx].card_addr);
		return iError;
	}

	*bank1 = 0x00C2;
	*bank1 = 0x0042;

	return 0;
}


/* for PCMCIA-Px and PCXMCIA/EP only */
int borland_dll  Get_PCMCIA_HWInterface_Rev_Px(int handlepx, usint *hwInterfaceRev)
{
	int iError;
 
	usint volatile *mem,rev;

	
	if ( (!cardpx[handlepx].isPCMCIA == 1) || (cardpx[handlepx].processor == PROC_AMD) )
		 return func_invalid;

	
	/* PCMCIA/Px */
	if (cardpx[handlepx].exc_bd_bc->pcmciaEPorPx == 'P')
	{
	
	
		iError = MapMemory(cardpx[handlepx].card_addr, (void **) &(mem),EXC_BANK_CONTROL);
		if (iError)
		{
			CloseKernelDevice(cardpx[handlepx].card_addr);
			return iError;
		}
		
		mem += 0x4;
		rev = (*mem & 0x00ff);
		*hwInterfaceRev = rev;

	}
	else /* PCMCIA/EPII */
	{

		iError = MapMemory(cardpx[handlepx].card_addr, (void **) &mem,EXC_BANK_DUALPORTRAM);
		if (iError)
		{
			CloseKernelDevice(cardpx[handlepx].card_addr);
			return iError;
		}

		mem += 0x8000;
		*mem = 0;
		rev =  *mem & 0x7;

		*hwInterfaceRev = rev;

	}

	return 0;
}


/* for PCMCIA/Px only */
int borland_dll Global_Ttag_Reset_PCMCIA_Px(int handlepx)
{
	/* Resets the timetags on module(s) of PCMCIA/Px
	if PCMCIA/P2, the ttags are now synchronized (call this funciton AFTER Init_Module_Px) */

	int iError;
	int dprBank;  /* number to use for Dual-Port RAM memory bank */
	unsigned char volatile *bank1;

	dprBank = 1;


	/*This function is only for PCMCIA/Px */
	if (cardpx[handlepx].exc_bd_bc->pcmciaEPorPx != 'P') return func_invalid;

	iError = MapMemory(cardpx[handlepx].card_addr, (void **) &(bank1), dprBank);
	if (iError)
	{
		CloseKernelDevice(cardpx[handlepx].card_addr);
		return iError;
	}

	*bank1 = 0x62;
	*bank1 = 0x42;

	return 0;
}


/** ------------------------------------------------------------------------------------
Get_DmaAvailable_Px()

Description: Returns an indicator as to the availablility of DMA for transfer data
between the host memory and the Px module memory.

Applies to Modes: Mode Independent

Input parameters:
	nodeHandle	the handle obtained from Init_Module_Px() when
				the node was initialized.

Output parameters:
	pDmaEnabled	(pointer to) a flag indicating if DMA memory
				access to DPRAM is available.
				Valid values are:
					ENABLE
					DISABLE

Return values:
	Successful_Completion_Exc
	ebadhandle

Design/Notes:

Coding history:
-----------------
1-Sep-2008	(YYB)
-	original coding

*/

int borland_dll Get_DmaAvailable_Px(int handlepx, int * pDmaEnabled)
{
	/* Check that the module handle is valid */
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* Set the output parameter. */
	if(cardpx[handlepx].dmaAvailable == TRUE)
		*pDmaEnabled = ENABLE;
	else
		*pDmaEnabled = DISABLE;

	return Successful_Completion_Exc;

}  /* -----  end of Get_DmaAvailable_Px()  ----- */


int borland_dll Get_PtrModuleInstance_Px(int handlepx, INSTANCE_PX ** ppModuleInstance)
{
	*ppModuleInstance = &cardpx[handlepx];
	return Successful_Completion_Exc;
}
