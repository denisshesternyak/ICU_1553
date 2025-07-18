#include "pxIncl.h"
extern INSTANCE_PX cardpx[];

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)


/*
Assign_Blk     Coded 02.12.90 by D. Koppel

Description: Assigns a datablock to an RT subaddress.

Input parameters:  rtid    11 bit RT address
                   blknum  Block number 1-127, 0 deassigns block

Output parameters: none

Return values:  emode   If board is not in Monitor mode
                einval  If parameter is out of range
                0       If successful

Note: An RT identifier is composed of an RT address, subaddress and T/R 
indicater in the form:

            |============|=====|===============|
            | RT ADDRESS | T/R | RT SUBADDRESS |
            |============|=====|===============|

This block will then be used to store monitored data relating to this RT 
identifier.

*/

int borland_dll Assign_Blk (int rtid, int blknum)
{
	return Assign_Blk_Px (g_handlepx, rtid, blknum);
}

int borland_dll Assign_Blk_Px (int handlepx, int rtid, int blknum)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

       /* check if board is configured as a MONITOR in lookup mode	*/
//	if (cardpx[handlepx].exc_bd_monlkup->board_config != MON_LOOKUP)
	if (RBYTE_D2H( cardpx[handlepx].exc_bd_monseq->board_config ) != MON_LOOKUP)
		return (emode);

	if ((rtid < 0) || (rtid > 2047))
		return (einval);
	if ((blknum < 0) || (blknum > 127))
		return (einval);
#ifdef VME4000
	cardpx[handlepx].exc_bd_monlkup->lookup[rtid^1] = (unsigned char)blknum;
#else
//	cardpx[handlepx].exc_bd_monlkup->lookup[rtid] = (unsigned char)blknum;
	WBYTE_H2D( cardpx[handlepx].exc_bd_monlkup->lookup[rtid], (unsigned char)blknum );
#endif
	return(0);
}



/*
Get_Last_Blknum     Coded 01.12.90 by D. Koppel

Description: Returns the number of the last datablock written to.

Input parameters:  none

Output parameters: none

Return values:  emode       If board is not in Monitor mode
                otherwise   Number of the current data block in use, 1-127
                0           No block
*/

int borland_dll Get_Last_Blknum(void)
{
	return Get_Last_Blknum_Px(g_handlepx);
}


int borland_dll Get_Last_Blknum_Px(int handlepx)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	/* check if board is configured as a MONITOR in lookup mode	*/
//	if (cardpx[handlepx].exc_bd_monlkup->board_config != MON_LOOKUP)
	if (cardpx[handlepx].board_config != MON_LOOKUP)
		return (emode);

//	return(cardpx[handlepx].exc_bd_monlkup->last_blk);
	return( (int) RBYTE_D2H( cardpx[handlepx].exc_bd_monlkup->last_blk ) );
}



/*
Enable_Lkup_Int     Coded July 28, 97 by J Waxman

Description: Enables or disables interrupts for the specified rtid.

Input parameters:   rtid    11 bit RT address
                    toggle  0 to turn off, nonzero to turn on.

Output parameters:  none

Return values:  einval  If parameter is out of range
                0       If successful

*/

int borland_dll Enable_Lkup_Int (int rtid, int toggle)
{
	return Enable_Lkup_Int_Px (g_handlepx, rtid, toggle);
}

int borland_dll Enable_Lkup_Int_Px (int handlepx, int rtid, int toggle)
{
	unsigned char dBlockVal;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

	if ((rtid < 0) || (rtid > 2047))
		return einval;

	else
	{
#ifdef VME4000
		if (toggle)
			cardpx[handlepx].exc_bd_monlkup->lookup[rtid^1] |= 0x80;
		else
			cardpx[handlepx].exc_bd_monlkup->lookup[rtid^1] &= 0x7F;
#else
		dBlockVal = RBYTE_D2H( cardpx[handlepx].exc_bd_monlkup->lookup[rtid] );
		if (toggle)
//			cardpx[handlepx].exc_bd_monlkup->lookup[rtid] |= 0x80;
			WBYTE_H2D( cardpx[handlepx].exc_bd_monlkup->lookup[rtid], (dBlockVal | 0x80) );

		else
//			cardpx[handlepx].exc_bd_monlkup->lookup[rtid] &= 0x7F;
			WBYTE_H2D( cardpx[handlepx].exc_bd_monlkup->lookup[rtid], (dBlockVal & 0x7F) );
#endif
	}
	return 0;
}

