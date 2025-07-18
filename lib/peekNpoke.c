#include <stdio.h>
#include "pxIncl.h"

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)


extern INSTANCE_PX cardpx[];

/* ----------   Peek and Poke routines     ---------- */

/*
Exc_Peek

  Description: Peek to read the value at some address in memory
  
  Syntax
	Exc_Peek (usint offset, usint *data)

  Input parameters:
	usint offset		the address offset in memory
  
  Output parameters:
	usint * data		the data found in memory at address offset

  Return values:
	ebadhandle		Invalid handle specified
	einit			the card was not initialized
	0
*/

/* 16-bit */
int borland_dll Exc_Peek (usint offset, usint * data)
{
	return Exc_Peek_Px (g_handlepx, offset, data);
}

int borland_dll Exc_Peek_WORD_Px (int handlepx, usint offset, usint * data)
{
	return Exc_Peek_Px (handlepx, offset, data);
}

int borland_dll Exc_Peek_Px (int handlepx, usint offset, usint * data)
{
	usint volatile *pDPRam;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_monseq == NULL)	return einit;
	if ( (cardpx[handlepx].isUNet == FALSE) && (cardpx[handlepx].exc_bd_monseq == NULL) ) return einit;

	/* pDPRam = cardpx[handlepx].exc_bd_bc->stack; */
	pDPRam = (usint *)(cardpx[handlepx].exc_bd_monseq);
//	data[0] = pDPRam[offset / sizeof(usint)];
	data[0] = RWORD_D2H( pDPRam[offset / sizeof(usint)] );

	return 0;
}

/*
Exc_Poke

  Description: Poke to write a value at some address in memory

  Syntax
	Exc_Poke (usint offset, usint *data)

  Input parameters:
	usint offset		the address offset in memory
	usint * data		the data to be written to memory at address offset
  
  Output parameters: none

  Return values:
	ebadhandle		Invalid handle specified
	einit			the card was not initialized
	0
*/

int borland_dll Exc_Poke (usint offset, usint *data)
{
	return Exc_Poke_Px (g_handlepx, offset, data);
}

int borland_dll Exc_Poke_WORD_Px (int handlepx, usint offset, usint * data)
{
	return Exc_Poke_Px (handlepx, offset, data);
}

int borland_dll Exc_Poke_Px (int handlepx, usint offset, usint *data)
{
	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_monseq == NULL)	return einit;
	if ( (cardpx[handlepx].isUNet == FALSE) && (cardpx[handlepx].exc_bd_monseq == NULL) ) return einit;

//	cardpx[handlepx].exc_bd_monseq->msg_block[offset / sizeof(usint)] = data[0];
	WWORD_H2D( cardpx[handlepx].exc_bd_monseq->msg_block[offset / sizeof(usint)], data[0]);

	return 0;
}

/* 8-bit */
int borland_dll Exc_Peek_BYTE(usint offset, BYTE *data)
{
	return Exc_Peek_BYTE_Px(g_handlepx, offset, data);
}

int borland_dll Exc_Peek_BYTE_Px (int handlepx, usint offset, BYTE *data)
{
	BYTE *bytepointer;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_monseq == NULL)	return einit;
	if ( (cardpx[handlepx].isUNet == FALSE) && (cardpx[handlepx].exc_bd_monseq == NULL) ) return einit;

	bytepointer = (BYTE *)cardpx[handlepx].exc_bd_monseq + offset;

#ifdef VME4000
	bytepointer = (BYTE *) ((unsigned int) bytepointer^1);
#endif

//	data[0] = *(bytepointer);
	data[0] = RBYTE_D2H( bytepointer[0] );

	return 0;
}

int borland_dll Exc_Poke_BYTE_Px (int handlepx, usint offset, BYTE *data)
{
	BYTE *bytepointer;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;
//	if (cardpx[handlepx].exc_bd_monseq == NULL)	return einit;
	if ( (cardpx[handlepx].isUNet == FALSE) && (cardpx[handlepx].exc_bd_monseq == NULL) ) return einit;

	bytepointer = (BYTE *)cardpx[handlepx].exc_bd_monseq + offset;

#ifdef VME4000
	bytepointer = (BYTE *) ((unsigned int) bytepointer^1);
#endif

//	*(bytepointer) = data[0];
	WBYTE_H2D( bytepointer[0], data[0]);

	return 0;
}

/* 32-bit */
/*
int borland_dll Exc_Peek_32Bit (usint offset, unsigned int * data)
{
	return Exc_Peek_32Bit_Px (g_handlepx, offset, data);
}
*/
int borland_dll Exc_Peek_32Bit_Px (int handlepx, usint offset, unsigned int * data)
{
	usint index;
	int sizeInt;
	usint *dataptr;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_monseq == NULL)	return einit;
	if ( (cardpx[handlepx].isUNet == FALSE) && (cardpx[handlepx].exc_bd_monseq == NULL) ) return einit;

	/* verify that the offset given is a 4-byte boundary, 0,4,8,12,16, etc; otherwise, illegal call 
	if ((offset %4) != 0)
		return eBadOffset; */
   
	sizeInt = sizeof(unsigned int);
	index = ((offset / 4 * 4) / sizeInt);

//	*data = *(unsigned int*)((DWORD_PTR)cardpx[handlepx].exc_bd_monseq + index*sizeInt);
	dataptr = (usint *) ((DWORD_PTR)cardpx[handlepx].exc_bd_monseq + index*sizeInt);

	b2drmReadWordBufferFromDpr( (DWORD_PTR) dataptr,
								(unsigned short *) data,
								sizeInt / sizeof(unsigned short),
								"Exc_Peek_32Bit_Px",
								B2DRMHANDLE );
	// FUTURE: Consider replacing b2drmReadWordBufferFromDpr with this!
	// *data = RDWORD_D2H( dataptr[0] );

	return 0;
}

/*
int borland_dll Exc_Poke_32Bit (usint offset, unsigned int *data)
{
	return Exc_Poke_32Bit_Px (g_handlepx, offset, data);
}
*/

int borland_dll Exc_Poke_32Bit_Px (int handlepx, usint offset, unsigned int *data)
{
	usint index;
	int sizeInt;
	unsigned short * dataptr;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_monseq == NULL)	return einit;
	if ( (cardpx[handlepx].isUNet == FALSE) && (cardpx[handlepx].exc_bd_monseq == NULL) ) return einit;

	/* verify that the offset given is a 4-byte boundary, 0,4,8,12,16, etc; otherwise, illegal call 
	if ((offset %4) != 0)
		return eBadOffset; */

	sizeInt = sizeof(unsigned int);
	index = ((offset / 4 * 4) / sizeInt);

	dataptr = (usint *) ((DWORD_PTR)cardpx[handlepx].exc_bd_monseq + index*sizeInt);

								
	b2drmWriteWordBufferToDpr ( (DWORD_PTR) dataptr,
									 (unsigned short *) data,
									 sizeInt / sizeof(unsigned short),
									 "Exc_Poke_32Bit_Px",
									 B2DRMHANDLE );

	// FUTURE: Consider replacing b2drmWriteWordBufferToDpr with this!
	// WDWORD_H2D( dataptr[0], data[0] );

	return 0;
}

/* ----------   Peek and Poke routines     ---------- */
#define MAX_BUFFER_SIZE 1024

// PLEASE NOTE THAT SIZE IS NUMBER OF BYTES!!! NOT NUMBER OF WORDS !!!!
int borland_dll Exc_Peek_Word_Buffer_Px (int handlepx, usint offset, BYTE * buffer, usint size)
{
	int i;
	BYTE volatile *pDPRam;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_monseq == NULL)	return einit;
	if ( (cardpx[handlepx].isUNet == FALSE) && (cardpx[handlepx].exc_bd_monseq == NULL) ) return einit;

	if (size > MAX_BUFFER_SIZE)
		return einit;

	pDPRam = (BYTE *)cardpx[handlepx].exc_bd_monseq + offset;

#ifdef VME4000
	pDPRam = (BYTE *) ((unsigned int) pDPRam^1) + offset;
#endif

	if (b2drmIsRemoteDevice(cardpx[handlepx].card_addr))
	{
		if (b2drmIsRemoteDeviceReachable(cardpx[handlepx].card_addr) == 0)
			b2drmReadWordBufferFromDpr ( (DWORD_PTR)pDPRam,
										(unsigned short*) buffer,
										(size/2),
										"Exc_Peek_Buffer_Px",
										B2DRMHANDLE );
		else return ebadhandle;
	}
	else
	{
		for (i = 0; i < size; i++)
			buffer[i] = pDPRam[i];
	}

	return 0;
}


// PLEASE NOTE THAT SIZE IS NUMBER OF BYTES!!! NOT NUMBER OF WORDS !!!!
int borland_dll Exc_Poke_Word_Buffer_Px (int handlepx, usint offset, BYTE * buffer, usint size)
{
	int i;
	BYTE volatile *pDPRam;

	if ((handlepx < 0) || (handlepx >= NUMCARDS)) return ebadhandle;
	if (cardpx[handlepx].allocated != 1) return ebadhandle;

//	if (cardpx[handlepx].exc_bd_monseq == NULL)	return einit;
	if ( (cardpx[handlepx].isUNet == FALSE) && (cardpx[handlepx].exc_bd_monseq == NULL) ) return einit;

	if (size > MAX_BUFFER_SIZE)
		return einit;
	
	pDPRam = (BYTE *)cardpx[handlepx].exc_bd_monseq + offset;
#ifdef VME4000
	pDPRam = (BYTE *) ((unsigned int) pDPRam^1)+ offset;
#endif

	if (b2drmIsRemoteDevice(cardpx[handlepx].card_addr))
	{
		if (b2drmIsRemoteDeviceReachable(cardpx[handlepx].card_addr) == 0)
			b2drmWriteWordBufferToDpr ( (DWORD_PTR)pDPRam,
										(unsigned short*) buffer,
										(size/2),
										"Exc_Peek_Buffer_Px",
										B2DRMHANDLE );
		else return ebadhandle;

	}
	else
	{
		for (i = 0; i < size; i++)
			pDPRam[i] = buffer[i];
	}
	
	return 0;
}

