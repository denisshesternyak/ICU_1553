#ifndef __PX_ERROR_H
#define __PX_ERROR_H


/* *****************************************************************
   NOTE: Whenever you add a new error, it must be added in the
         following places:
         (a) in the Print_Error routine in the file ERROR.C 
        
   ***************************************************************** */


/* These error codes are also used by other products */

#define Successful_Completion_Exc 0 
#define ebadid				-1  /* undefined message id used as input */
#define einval				-2  /* illegal value used as input */
#define emode				-3  /* Mode specific command called when in the wrong mode */
#define ebadchan			-4  /* tried to set channel to illegal value */
#define esim_no_mem			-5  /* Simulate mode failed due to Malloc call failure */
#define msg2big				-6  /* attempted to create a message with too many words */
#define msgnospace			-7  /* not enough space in message stack for this message */
#define msgnospace2			-107  /* not enough space in message stack for this message, second place in same function */
#define msg2many			-8  /* exceeded maximum number of messages permitted ( 250 ) */
#define frm_badid			-9  /* attempted to place an undefined message into a message frame */
#define frm_nostacksp		-10 /* not enough space in frame stack for this frame */
#define frm_erange			-11 /* call to Start_Frame with an undefined frame  */
#define bcr_erange          -12 /* Run_BC called with type > 255  */
								/* call to Start_Frame with count greater than the number of messages in the frame  */
#define frm_maxframe		-13 /* exceeded maximum number of frames permitted ( 20 ) */
#define frm_erangecnt		-14
#define eintr               -15 /* attempt to set an undefined interrupt */
#define etiming             -16 /* attempt to change a message while board is accessing that message */
#define estackempty         -17 /* attempt to read command stack before any messages have been recd */
#define enomsg				-18 /* attempt to read messages when no new msgs available */
#define enoskip             -19 /* attempt to restore a message that wasn't skipped */
#define enoasync            -20 /* the async frame contains fewer messages 
								   than the user wishes to send asynchronously */
#define etimeout			-21 /* timed out waiting for BOARD_HALTED */
#define einvamp             -22 /* attempt to set invalid amplitude */
#define eboardnotfound      -23 /* too many boards initialized */
#define eboardnotalloc      -24 /* attempt to switch to board which was not allocated in init_card */
#define enoid				-25 /* could not find a card at given device number */
#define etimeoutreset		-26
#define ewrngmodule			-27
#define enomodule			-28
#define enotallowedreset	-29 /* PCMCIA/Px NiosII-based: hardware is not yet ready for software reset (timeout - more than 100 msec) */
#define efrequencymodenotsupported	-30  /* Firmware for this module does not support Frequency Mode */
#define efrequencymodemustbeon 		-31  /* Frequency mode must be turned ON before calling this function */
#define ebadhandle			-33
#define eboardtoomany		-36 /* too many boards initialized */
#define frm_nostack			-37
#define eBadOffset			-38 /* bad offset as parameter to Exc_Peek_32Bit or Exc_Poke_32Bit; 
								   offset must be a multiple of 4 */
#define func_invalid        -49
#define ecallmewhenstopped  -50 /* the function can be called only when the module is stopped */
#define noirqset			-53 
#define ilbfailure			-57
#define elbfailure			-58
#define rterror             -59 
#define ebadcommandword     -60 /*bit 0x0400 in command word is */
#define einit               -61 /* the card was not initialized */
#define enoalter			-62 /* this message is being transmitted*/
#define noconcurrmon   	    -63	
#define eopenkernelpci      -64 
#define enobcmsg	        -65
#define einbcmsg            -66
#define ertbusy				-67
#define ewrongprocessor		-68 /* attempted an operation not supported on installed processor */
//#define eMsgNumberAvailable		-69 /* this error number is available for use in the future in new funcitons */
#define esetmoduletime		-70	/* error attempting to set Module Time */
#define ewintimerapifail	-71	/* error attempting to use Windows high-resolution performance Timer/Counter */
#define eoverrunTtag		-72
#define eoverrunEOMflag		-73
#define econcurrmonmodule   -81
#define eoverrun			-82
#define enoonboardloopback	-83
#define eminorframe			-84
#define edbnotset			-85
#define ebadblknum			-86
#define exreset				-87
#define ercvfunc			-88
#define ebadttag			-89
#define ettagexternal		-90
#define enosrq				-91
#define ejump				-92
#define uemode				-93
#define ueblkassigned		-94
#define uetoomanymsgids		-95
#define edoublebuf			-96
#define eboundary			-97
#define emonmode			-98
#define ertvalue		-99   /* PxS: illegal rt value used in single function rt; OR use of RT before calling function Set_RT_Active_Px */
#define ebitlocked		-100  /* rt single function register locked */
#define ebiterror		-101  /* rt single function register error bit lit */
#define esinglefunconlyerror -102  /* this function is for single function only */
#define enotforsinglefuncerror	-103  /* this function is not for single function */
#define eresptimeerror	-104  /* single function response time cannot be greater than 12000 */
#define efeature_not_supported_PX	-105
#define efeature_not_supported_MMSI	-114
/* error number -107 is taken by msgnospace2, see next to error number -7 */
/* #define msgnospace2			-107  */
#define enotthreadsafe		-108
#define einvalDurationflag	-109

#define eNoIrigModeSet_PX	-110  //Did not call function Set_IRIG_TimeTag_Mode_Px
#define eIrigTimetagSet_PX	-111  //Called function Set_IRIG_TimeTag_Mode_Px; use Get_Next_Mon_IRIG_Message_Px
#define eNoIrigModeSet_MMSI	-112  //Did not call function Set_IRIG_TimeTag_Mode_Px
#define eIrigTimetagSet_MMSI	-113  //Called function Set_IRIG_TimeTag_Mode_Px; use Get_Next_Mon_IRIG_Message_Px

// See above: #define efeature_not_supported_MMSI	-114

#endif
