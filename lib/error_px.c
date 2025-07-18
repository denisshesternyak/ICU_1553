#include "pxIncl.h"

#include "ExcUnetMs.h"

#ifndef NO_4000_DLL
#ifdef VXWORKSP
#include "exc4000vxp.h"
#else
#ifdef VXWORKS
#include "exc4000vx.h"
#else
#include "exc4000.h"
#endif /* VXWORKS */
#endif /* VXWORKSP */
#endif /* NO_4000_DLL */

#include <stdio.h> /* for sprintf */
#include <string.h> /* for strcpy */

char errStrPx[50];

/*
Get_Error_String_Px
               
Description: Accepts the error returns from other Galahad routines.
This routine returns the string containing a corresponding
error message.

Input parameters:  errorcode   The error code returned from a GALAHAD call

Output parameters: errorstring	An array of 255 characters, the message
				string that contains the corresponding error message.
				In case of bad input this routine contains a string denoting
                               	that.

Return values:  0		Always

Use this routine as per the following example:
   char ErrorStr[255];

   Get_Error_String_Px(errorcode, ErrorStr);
   printf("error is: %s", ErrorStr);

*/

int borland_dll Get_Error_String_Px(int errcode, char *errstring)
{
	char *localerr;
	int i, errlen=255;
	int unetErrorLen = 255;
	char unetError[255];

	localerr = Print_Error_Px(errcode);

	if (strncmp(localerr, "No such error", 13) == 0)
	{
		Get_Error_String_UNET(errcode, unetErrorLen, unetError);
		localerr = &unetError[0];
	}

#ifndef NO_4000_DLL
	/*  add change request to px and maybe other st dlls to consider dovi's idea -- this won't solve */
	if (strncmp(localerr, "No such error", 13) == 0)
	{
		Get_Error_String_4000(errcode, errlen, errstring);
	}
	else
#endif // NO_4000_DLL
	{
		/* do this so that there will be null termination */
		for (i=0; i<errlen; i++)
			errstring[i] = '\0';

		strncpy(errstring, localerr, errlen-1);
	}

	/* strcpy(errstring, localerr); */
	return 0;
}



/*
Print_Error     Coded 6-12-90 by D. Koppel

Obsolete. Retained for backward compatibility.

Description: Accepts the negative error returns from other Galahad routines.
This routine returns a char pointer to a string containing a corresponding
error message.

Input parameters:  errorcode   The error code returned from a GALAHAD call

Output parameters: none

Return values:  char pointer  To a message string that contains a
                              corresponding error message. In case of bad
                              input this routine returns a string denoting
                              that.

*/


/*  Print_Error - for backwards compatibility with PCI/Px and PCI/EP */
borland_dll char * Print_Error (int errorcode)
{
	return Print_Error_Px(errorcode);
}

borland_dll char * Print_Error_Px (int errorcode)
{
	char *perrStr = errStrPx;

	switch (errorcode)
	{
	case ebadid: return ("Undefined message id used as input [ebadid]\n");
	case einval: return ("Illegal value used as input [einval]\n");
	case emode:  return ("Mode specific command called when in the wrong mode [emode]\n");
	case ebadchan:return ("Tried to set bus to illegal value [ebadchan]\n");
	case esim_no_mem: return ("Not enough memory For Simulation [esim_no_mem]\n");
	case msg2big:return ("Attempted to create a message with too many words [msg2big]\n");
	case msgnospace: return ("Not enough space in message stack for this message [msgnospace]\n");
	case msgnospace2: return ("Not enough space in message stack for this message [msgnospace2]\n");
	case msg2many: return ("Exceeded maximum number of messages permitted (see MSGMAX value) [msg2many]\n");
	case frm_badid: return ("Attempted to place an undefined message into a message frame [frm_badid]\n");
	case frm_nostacksp:  return ("Not enough space in frame stack for this frame [frm_nostacksp]\n");
	case frm_erange:  return ("Frame id specified was not defined with Create_Frame[frm_erange]\n");
	case bcr_erange:  return ("Run_BC called with type > 255 [bcr_erange]\n");
	case frm_maxframe:  return ("Exceeded maximum number of frames permitted (see MAXFRAMES value) [frm_maxframe]\n");
	case frm_erangecnt:return ("Count greater than number of messages in the frame [frm_erangecnt]\n");
	case eintr: return ("Attempt to set an undefined interrupt [eintr]\n");
	case etiming: return ("Attempt to change a message while board is accessing that message [etiming]\n");
	case estackempty:   return ("Attempt to read command stack before any messages have been received [estackempty]\n");
	case enomsg:   return ("Attempt to read monitor message when no new messages have been received [enomsg]\n");
	case enoskip:   return ("Attempt to restore a message that wasn't skipped [enoskip]\n");
	case enoasync:  return ("The async frame contains fewer messages than the user wishes to send asynchronously [enoasync]\n");
	case etimeout: return ("Timed out waiting for BOARD_HALTED [etimeout]\n");
	case einvamp:  return ("Attempt to set an invalid amplitude [einvamp]\n");
	case eboardnotfound: return ("Attempt to initialize or switch to card invalid card address or socket [eboardnotfound]\n");
	case eboardnotalloc: return ("Attempt to access module which wasn't allocated by Init_Module [eboardnotalloc]\n");
	case enoid:  return ("Initialization could not identify device specified [enoid]\n");
	case etimeoutreset: return ("Timed out waiting for reset [etimeoutreset]\n");
#ifndef MMSI
	case ewrngmodule: return ("Module specified on EXC-4000 board is not Px Module [ewrngmodule]\n");
#else		
	case ewrngmodule: return ("Module specified on EXC-4000 board is not MMSI Module [ewrngmodule]\n");
#endif
	case enomodule: return ("No EXC-4000 module present at specified location [enomodule]\n");
	case enotallowedreset: return ("Hardware is not yet ready for software reset (timeout - more than 100 msec) [enotallowedreset]\n");
	case efrequencymodenotsupported: return("Firmware for this module does not support Frequency Mode [efrequencymodenotsupported]\n");
	case efrequencymodemustbeon: return("Frequency mode must be turned ON before calling this function [efrequencymodemustbeon]\n");
	case ebadhandle: return ("Invalid handle specified; should be value returned by Initialization function [ebadhandle]\n");
	case eboardtoomany: return ("Attempt to initialize too many boards; this board not initialized [eboardtoomany]\n");
	case frm_nostack:  return ("Start_Frame not yet called to set up the message stack [frm_nostack]\n");
	case func_invalid: return ("Function invalid for this card [func_invalid]\n");
	case ecallmewhenstopped: return ("The function can be called only when the module is stopped [ecallmewhenstopped]\n");

	case noirqset: return("No interrupt allocated [noirqset]\n");

	case ilbfailure: return("Internal loopback test failed [ilbfailure]\n");
	case elbfailure: return("External loopback test failed [elbfailure]\n");
	case rterror: return("Illegal Subaddress number selected [rterror]\n");
	case ebadcommandword:  return ("Bit 0x0400 in command word is either set or not set incorrectly [ebadcommandword]\n");
	case einit: return ("Card was not initialized [einit]\n");
	case enoalter: return ("This message is being transmitted; it cannot be altered now [enoalter]\n");
	case noconcurrmon: return("No concurrent monitor [noconcurrmon]\n");
	case eopenkernelpci: return ("Cannot open kernel device [eopenkernelpci]\n");
	case enobcmsg: return ("Attempt to read BC message when no new messages have been received [enobcmsg]\n");
	case einbcmsg: return ("Error in specified BC message [einbcmsg]\n");
	case ertbusy: return ("Current entry in the RT message stack is being written to. Try again [ertbusy]\n");
	case ewrongprocessor: return ("Attempted operation is not supported on the installed processor [ewrongprocessor]\n");
	//case eMsgNumberAvailable: return ("This error number is available for use. [eMsgNumberAvailable]\n");

	case econcurrmonmodule: return ("Module 1, 3 or 5 must first be selected [econcurrmonmodule]\n");
	case eoverrun: return ("Cannot read monitored message, messages were overrun [eoverrun]\n");
	case eoverrunTtag: return ("Cannot read monitored message, messages were timetag overrun [eoverrunTtag]\n");
	case eoverrunEOMflag: return ("Cannot read monitored message, messages were End Of Message flag overrun [eoverrunEOMflag]\n");

	case enoonboardloopback: return ("OnBoard Loopback feature is not available [enoonboardloopback]\n");

	case eminorframe: return ("Message is a MINOR_FRAME type message [eminorframe]\n");
	case edbnotset: return ("Double buffering was not set for this rtid [edbnotset]\n");
	case ebadblknum: return ("Illegal datablock specified when double-buffering used [ebadblknum]\n");
	case exreset: return ("Xilinx failed to reset [exreset]\n");
	case ercvfunc: return ("This function valid for Receive RTId only [ercvfunc]\n");
	case ebadttag: return ("Function invalid; timetag not working [ebadttag]\n");
	case ettagexternal: return ("External timetag clock not 4us [ettagexternal]\n");
	case enosrq: return ("SRQ currently disabled [enosrq]\n");
	case ejump: return ("This function not valid on JUMP message [ejump]\n");
	case uemode:return("Mode specific command called when in the wrong Universal mode[uemode] \n");
	case ueblkassigned:return("Datalk already assigned, cannot be assigned to this rtid in Universal mode[ueblkassigned] \n");
	case uetoomanymsgids:return("Attepted to map too many msgids to one datablk in Universal mode[uetoomanymsgids] \n");
	case edoublebuf: return ("multibuffering is incompatible with double buffering[edoublebuf]\n");
	case eboundary: return ("multibuffering cannot cross 256 or 512 block boundary\n");
	case esetmoduletime: return("Error attempting to set Module Time [esetmoduletime]\n");
	case ewintimerapifail: return("Error attempting to use the Windows high-resolution performance counter [ewintimerapifail]\n");
	case emodnum: return ("Invalid module number specified [emodnum]\n");
	case emonmode: return("Invalid Mode - the card/module only operates in Monitor Mode[emonmode]\n");
	case eclocksource: return ("Invalid clock source specified [eclocksource]\n");
	case ertvalue: return ("Invalid RT usage - Single Function module, either (a) does not match single function RT value, or (b) verify that you call function Set_RT_Active_Px before calling the current function [ertvalue]\n");
	case ebitlocked: return ("single function RT number register locked [ebitlocked]\n");
	case ebiterror: return ("single function RT number register error bit lit [ebiterror]\n");
	case esinglefunconlyerror: return ("this function is for single function only [esinglefunconlyerror]\n");
	case enotforsinglefuncerror: return ("this function is not for single function [enotforsinglefuncerror]\n");

	case eresptimeerror: return ("single function response time cannot be greater than 12000 [eresptimeerror]\n");
	case efeature_not_supported_PX: return ("Feature is not supported on this module [efeature_not_supported_PX]\n");
	case efeature_not_supported_MMSI: return ("Feature is not supported on this module [efeature_not_supported_MMSI]\n");
	case enotthreadsafe: return ("this function is not thread safe and is not supported [enotthreadsafe]\n");
	case einvalDurationflag: return ("Illegal durationflag value used as input to function Set_1553Status_Px[einvalDurationflag]\n");

	// added for IrigB timetag support functions
	case eNoIrigModeSet_PX: return ("Did not call function Set_IRIG_TimeTag_Mode_Px [eNoIrigModeSet_PX]\n");
	case eIrigTimetagSet_PX: return ("Called function Set_IRIG_TimeTag_Mode_Px; use Get_Next_Mon_IRIG_Message_Px [eIrigTimetagSet_PX]\n");
	case eNoIrigModeSet_MMSI: return ("Did not call function Set_IRIG_TimeTag_Mode_Px [eNoIrigModeSet_MMSI]\n");
	case eIrigTimetagSet_MMSI: return ("Called function Set_IRIG_TimeTag_Mode_Px; use Get_Next_Mon_IRIG_Message_Px [eIrigTimetagSet_MMSI]\n");

	/* these errors only apply to VME or LabVIEW realtime */
	case eviclosedev: return ("Error in ViClose device[eviclosedev]\n");
	case evicloserm: return ("Error in ViClose DefaultRM [evicloserm]\n");
	case eopendefaultrm: return ("Error in viOpenDefaultRM [eopendefaultrm]\n");
	case eviopen: return ("Error in viOpen [eviopen]\n");
	case evimapaddress: return ("Error in viMapAddress [evimapaddress]\n");
	case evicommand: return ("Error in VISA command [evicommand]\n");
	case einstallhandler: return ("Error in VISA install interrupt handler [einstallhandler]\n");
	case eenableevent: return ("Error in VISA enable event [eenableevent]\n");
	case euninstallhandler: return ("Error in VISA uninstall interrupt handler [euninstallhandler]\n");
	case edevnum: return ("Illegal device number (must be 0-255) [edevnum]\n");
	case einstr: return ("Bad ViSession value (instr)[einstr]\n");
	case evbadr: return ("VME-VXI base address specified out of bounds [evbadr]\n");
	case ebadrsrcnamepointer: return("Bad pointer passed in for resource name [ebadrsrcnamepointer]\n");

	case evmic_vmeInit: return ("Error in vmeInit [evmic_vmeInit]\n");
	case evmic_lock16window: return ("Error in vmeLockvmeWindow for A16 space [evmic_lock16window]\n");
	case evmic_get16windowadr: return ("Error in vmeGetVmeWindowAdr for A16 space [evmic_get16windowadr]\n");
	case evmic_lockmemwindow: return ("Error in vmeLockvmeWindow for A24 or A32 space [evmic_lockmemwindow]\n");
	case evmic_getmemwindowadr: return ("Error in vmeGetVmeWindowAdr for A24 or A32 space [evmic_getmemwindowadr]\n");
	case evmic_unlockwindow: return ("Error in vmeUnlockvmeWindow [evmic_unlockwindow]\n");
	case evmic_freewindowadr: return ("Error in vmeFreeVmeWindowAdr [evmic_freewindowadr]\n");

	case evmea16: return ("Error in mapping a16 space [evmea16]\n");
	case evmea32: return ("Error in mapping a32 space [evmea32]\n");
	case evmea24: return ("Error in mapping a24 space [evmea24]\n");

	case edipnum: return("Invalid DIP switch or jumper value specified for device number [edipnum]\n");
	case edevnotfound: return ("Device with specified device number not found [edevnotfound]\n");
	case ebyteswapping: return ("Master Byte Swapping must be enabled in BIOS/Universe for Excalibur board to run properly [ebyteswapping]\n");
	case eboardtypenotfound: return ("Board of this type not found in system [eboardtypenotfound]\n");
	case einterruptFunc: return ("Interrupt function already defined for module; call ClearInterruptHandler first [eInterruptFunc]\n");

	/* these errors do not apply to VME or LabVIEW realtime */
	case eopenkernel: return ("Error opening kernel device; check ExcConfig settings [eopenkernel]\n");
	case ekernelcantmap: return ("Error mapping memory [ekernelcantmap]\n");
	case ereleventhandle: return ("Error releasing the event handle [ereleventhandle]\n");
	case egetintcount: return ("Error getting interrupt count  [egetintcount]\n");
	case egetchintcount: return ("Error getting channel interrupt count  [egetchintcount]\n");
	case egetintchannels: return ("Error getting interrupt channels [egetintchannels]\n");
	case ewriteiobyte: return ("Error writing I/O memory [ewriteiobyte]\n");
	case ereadiobyte: return("Error reading I/O memory [ereadiobyte]\n");
	case egeteventhand1: return("Error getting event handle (in first stage) [egeteventhand1]\n");
	case egeteventhand2: return("Error getting event handle (in second stage) [egeteventhand2]\n");
	case eopenscmant: return("Error opening Service Control Manager (in startkerneldriver) [eopenscmant]\n");
	case eopenservicet: return("Error opening Service Control Manager (in stopkerneldriver)  [eopenservicet]\n");
	case estartservice: return("Error starting kernel service (in startkerneldriver)  [estartservice]\n");
	case eopenscmanp: return("Error opening Service Control Manager (in stopkerneldriver)  [eopenscmanp]\n");
	case eopenservicep: return("Error opening kernel service (in stopkerneldriver)  [eopenservicep]\n");
	case econtrolservice: return("Error in control service (in stopkerneldriver) [econtrolservice]\n");
	case eunmapmem: return("Error unmapping memory [eunmapmem]\n");
	case egetirq: return("Error getting IRQ number [egetirq]\n");
	case eallocresources: return("Error allocating resources; see readme.pdf for details on resource allocation problems [eallocresources]\n");
	case egetramsize: return ("Error getting RAM size [egetramsize]\n");
	case ekernelwriteattrib: return ("Error writing attribute memory [ekernelwriteattrib]\n");
	case ekernelreadattrib: return ("Error reading attribute memory [ekernelreadattrib]\n");
	case ekernelfrontdesk: return ("Error opening kernel device; check ExcConfig set up [ekernelfrontdesk]\n");
	case ekernelOscheck: return ("Error determining operating system [ekernelOscheck]\n");
	case ekernelfrontdeskload: return ("Error loading frontdesk.dll [ekernelfrontdeskload]\n");
	case ekerneliswin2000compatible: return ("Error determining Windows 2000 compatibility [ekerneliswin2000compatible]\n");
	case ekernelbankramsize: return("Error determining memory size [ekernelbankramsize]\n");
	case ekernelgetcardtype: return ("Error getting card type [ekernelgetcardtype]\n");
	case regnotset: return("Board not configured; reboot after ExcConfig is run and board is in slot [regnotset]\n");
	case ekernelbankphysaddr: return ("Error getting physical memory addressekernelbankphysaddr]\n");
	case ekernelclosedevice: return ("Error closing kernel device [ekernelclosedevice]\n");
	case ekerneldevicenotopen: return ("Error returned by kernel: device not open [ekerneldevicenotopen]\n");
	case ekernelinitmodule: return ("Error initializing kernel [ekernelinitmodule]\n");
	case ekernelbadparam: return ("Error returned by kernel: bad input parameter [ekernelbadparam]\n");
	case ekernelbadpointer: return ("Error returned by kernel: invalid pointer to output buffer [ekernelbadpointer]\n");
	case ekerneltimeout: return ("Timeout expired before interrupt occurred [ekerneltimeout]\n");
	case ekernelnotwin2000: return ("Error returned by kernel: operating system is not Windows 2000 compatible [ekernelnotwin2000]\n");
	case erequestnotification: return ("Error requesting interrupt notification [erequestnotification]\n");
	case ekernelnot4000card: return ("Error returned by kernel: designated board is not EXC-4000 [ekernelnot4000card]\n");
	case ekernelwaitinterrupt: return ("Error returned by kernel: Unable to wait on interrupt\n");
	case enotimplemented: return ("This function is not implemented\n");

	/* LabView dispatcher DLL init errors */
	case edispatchertargetDLLmissing: return ("Software tools DLL missing [edispatchertargetDLLmissing]\n");
	case edispatchertargetDLLincorrect: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect]\n");
	case edispatchertargetDLLincorrect0: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect0]\n");
	case edispatchertargetDLLincorrect1: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect1]\n");
	case edispatchertargetDLLincorrect2: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect2]\n");
	case edispatchertargetDLLincorrect3: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect3]\n");
	case edispatchertargetDLLincorrect4: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect4]\n");
	case edispatchertargetDLLincorrect5: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect5]\n");
	case edispatchertargetDLLincorrect6: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect6]\n");
	case edispatchertargetDLLincorrect7: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect7]\n");
	case edispatchertargetDLLincorrect8: return ("Incorrect software tools DLL [edispatchertargetDLLincorrect8]\n");
	case edispatcherbaddispatchhandle: return ("Bad LabVIEW dispatch handle [edispatcherbaddispatchhandle]\n");
	case edispatchercantfindfunction: return ("Can't find function in Software Tools DLL [edispatchercantfindfunction]\n");
	case edispatchbadDLLName: return ("Passed bad pointer for Software tools DLL name [edispatchbadDLLName]\n");

	/* Borland wrapper dispatcher DLL init errors */
	case eborlandwrappertargetDLLmissing: return ("Software tools DLL missing [eborlandwrappertargetDLLmissing]\n");
	case eborlandwrappertargetDLLincorrect: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect]\n");
	case eborlandwrappertargetDLLincorrect0: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect0]\n");
	case eborlandwrappertargetDLLincorrect1: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect1]\n");
	case eborlandwrappertargetDLLincorrect2: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect2]\n");
	case eborlandwrappertargetDLLincorrect3: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect3]\n");
	case eborlandwrappertargetDLLincorrect4: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect4]\n");
	case eborlandwrappertargetDLLincorrect5: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect5]\n");
	case eborlandwrappertargetDLLincorrect6: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect6]\n");
	case eborlandwrappertargetDLLincorrect7: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect7]\n");
	case eborlandwrappertargetDLLincorrect8: return ("Incorrect software tools DLL [eborlandwrappertargetDLLincorrect8]\n");
	case eborlandwrapperbaddispatchhandle: return ("Bad Borland wrapper dispatch handle [eborlandwrapperbaddispatchhandle]\n");
	case eborlandwrappercantfindfunction: return ("Can't find function in Software Tools DLL [eborlandwrappercantfindfunction]\n");
	case eborlandwrapperBadDLLName: return ("Passed bad pointer for Software tools DLL name [eborlandwrapperBadDLLName]\n");

	default: sprintf(errStrPx, "No such error %d\n", errorcode);
		return (perrStr);
	}
}

