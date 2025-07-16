#include "pxIncl.h"

#include "ExcUnetMs.h"
#define B2DRMHANDLE (cardpx[handlepx].remoteDevHandle)

extern INSTANCE_PX cardpx[];

/*
This routine reads messages from a circular buffer structure, Inputs include the start and end of the structure and the size of an entry.
This makes it generic to any circular structure.  The routine uses the message counter
to determine how many messages came in since the previous call and if an overrun has occurred. An overrun occurs if more messages came in
than the buffer has room for.
*/

int  Read_RT_Block_Px(int handlepx, unsigned char **userbuffer, usint entrySize, int bufferStart, int bufferEnd, int *nummsgs,  int *overwritten) {

	usint	status;
	int lastCountRead, entriesInRcvBuffer, currentCount, entriesToRead, entriesTillEndOfBuffer;
	usint firstReadStart, secondReadStart; // byte offsets into the DPR
	usint firstReadSizeinBytes, secondReadSizeInBytes, totalBytesRead;
	int 	endCount, countsDuringRead, entriesRead, corruptentries;
	unsigned char *buffer;
	int bytesInMaxRead, bytesInThisRead, readSoFar;


	buffer = *userbuffer;
	lastCountRead = cardpx[handlepx].lastRTstackCountRead;
	entriesInRcvBuffer = (bufferEnd - bufferStart) / entrySize;
	Get_RT_Stack_Entry_Count_Px(handlepx, &currentCount);
	if ((currentCount > lastCountRead) && ((currentCount - lastCountRead) < entriesInRcvBuffer)) {
		*overwritten = NOT_OVERWRITTEN;
		entriesToRead = currentCount - lastCountRead;
	}
	else if (currentCount == lastCountRead) {
		*nummsgs = 0;
		return (0);
	}
	else { //entire buffer is overwritten and we read from current+1 to current 
		*overwritten = currentCount % entriesInRcvBuffer;
		entriesToRead = entriesInRcvBuffer;
	}
	if (cardpx[handlepx].isUNet == FALSE)
		bytesInMaxRead = 0x1000;	//DMA max is 4kbytes
	else {
		if (b2drmIsUSB(B2DRMHANDLE))
			bytesInMaxRead = 504;
		else
			bytesInMaxRead = 1440;
	}
	if (*overwritten == NOT_OVERWRITTEN) {
		firstReadStart =  bufferStart + ((lastCountRead % entriesInRcvBuffer) * entrySize);
		entriesTillEndOfBuffer = ((bufferEnd - firstReadStart)+1) / entrySize;
		//the data wraps we need to read from current till the end and from the beginning till the newest data
		if (entriesToRead > entriesTillEndOfBuffer) {
			firstReadSizeinBytes = (bufferEnd - firstReadStart);
			secondReadStart =  bufferStart;
			secondReadSizeInBytes = (entriesToRead - entriesTillEndOfBuffer) * entrySize;
		}
		//no wrap just read from current till the number of new entries in the buffer
		else {
			firstReadSizeinBytes = entriesToRead * entrySize;
			secondReadStart = secondReadSizeInBytes = 0;
		}
	}
	else { //overwritten - the whole buffer is new, oldest label is at current pointer
		firstReadStart =  bufferStart + ((currentCount % entriesInRcvBuffer) * entrySize);
		//the oldest label is at the top of the buffer so one read of the whole buffer is needed
		if (firstReadStart >= bufferEnd) {
			firstReadStart =  bufferStart;
			firstReadSizeinBytes = (bufferEnd - bufferStart);
			secondReadStart =  (0);
		}
		//the oldest label is in the middle of the buffer, read from the oldest label till the end and then from the beginning till the oldest label (which is the end of the newest label)
		else {
			firstReadSizeinBytes = (bufferEnd - firstReadStart);
			secondReadStart =  bufferStart;
			secondReadSizeInBytes = firstReadStart - bufferStart;
		}
	}

	totalBytesRead = 0;
	if (cardpx[handlepx].isUNet) {
		//if we need to read more than can fit into a packet, use multiple reads
		while (firstReadSizeinBytes > 0) {
			if (firstReadSizeinBytes > bytesInMaxRead)
				bytesInThisRead = bytesInMaxRead;
			else
				bytesInThisRead = firstReadSizeinBytes;
			b2drmReadWordBufferFromDpr ((DWORD_PTR)(cardpx[handlepx].exc_pc_dpr + firstReadStart + totalBytesRead),
				(unsigned short *) (buffer + totalBytesRead),
				bytesInThisRead/2,
				"Read_RT_Block_Px1",
				B2DRMHANDLE );
			totalBytesRead += bytesInThisRead;
			firstReadSizeinBytes -= bytesInThisRead;
		}
		readSoFar = 0;
		while (secondReadSizeInBytes > 0) {
			if (secondReadSizeInBytes > bytesInMaxRead)
				bytesInThisRead = bytesInMaxRead;
			else
				bytesInThisRead = secondReadSizeInBytes;
			b2drmReadWordBufferFromDpr ((DWORD_PTR)(cardpx[handlepx].exc_pc_dpr + secondReadStart + readSoFar),
				(unsigned short *) (buffer + totalBytesRead),
				bytesInThisRead/2,
				"Read_RT_Block_Px2",
				B2DRMHANDLE );
			secondReadSizeInBytes -= bytesInThisRead;
			totalBytesRead += bytesInThisRead;
			readSoFar += bytesInThisRead;
		}
	}
	else {
		if ((cardpx[handlepx].dmaAvailable == TRUE) && (cardpx[handlepx].useDmaIfAvailable == TRUE))	{
			status = PerformDMARead(cardpx[handlepx].card_addr, cardpx[handlepx].module,
				(void *) buffer, firstReadSizeinBytes,
				(void *) firstReadStart);
			if (status == 0) 
				totalBytesRead = firstReadSizeinBytes;
			if (secondReadStart != 0) {
				status = PerformDMARead(cardpx[handlepx].card_addr, cardpx[handlepx].module,
					(void *) (buffer + firstReadSizeinBytes), secondReadSizeInBytes,
					(void *) secondReadStart);
				if (status == 0) 
					totalBytesRead += secondReadSizeInBytes;
			}
		}
	}
	//if DMA failed use RMA
	if (totalBytesRead == 0) {
		myMemCopy((void *)buffer, (void *)((char *)(cardpx[handlepx].exc_bd_rt)  + firstReadStart), firstReadSizeinBytes, cardpx[handlepx].is32bitAccess); 
		totalBytesRead = firstReadSizeinBytes;
		if (secondReadStart != 0) {
			myMemCopy((void *)(buffer + firstReadSizeinBytes), (void *)((char *)(cardpx[handlepx].exc_bd_rt)  + secondReadStart), secondReadSizeInBytes, cardpx[handlepx].is32bitAccess); 
			totalBytesRead += secondReadSizeInBytes;
		}
	}
	//update lastCountRead to reflect the entries we just read
	entriesRead = totalBytesRead / entrySize;
	cardpx[handlepx].lastRTstackCountRead = currentCount;
	//check counter again to see how many entries came in during our read processing
	Get_RT_Stack_Entry_Count_Px(handlepx, &endCount);
	countsDuringRead = endCount - currentCount;
	//if the number of entries that came in during our processing didn't overwrite anything we read, we're done
	if ((entriesRead + countsDuringRead) <= entriesInRcvBuffer) {
		*nummsgs = entriesRead;
		return (0);
	}
	//otherwise the first several entries we read may be corrupt, skip them
	corruptentries  =	(entriesRead + countsDuringRead) - entriesInRcvBuffer;
	//reduce the number of message returned by the number of corupt messages
	*nummsgs = entriesRead - corruptentries;
	//move the pointer of the buffer to skip over the corrupt entries
	*userbuffer = buffer + (corruptentries * entrySize);
	*overwritten = (*overwritten + corruptentries) % entriesInRcvBuffer;
	return (0);
}

#define DOUBLEWORDBOUNDARY 3 //low two bits of a double word boundary address are 0
#define WORDBOUNDARY 1 //low bit of a word boundary address is 0
int myMemCopy( void *destPointer, void *srcPointer, int count, int is32BitAccess){
	int Index;
	int bytesLeft;
	unsigned int *pSrcDW, *pDestDW;
	usint *pSrcW, *pDestW;
	uchar *pSrcCh, *pDestCh;
	if (((((unsigned int)srcPointer & DOUBLEWORDBOUNDARY) == 0) && (((unsigned int)destPointer & DOUBLEWORDBOUNDARY) == 0))  && (is32BitAccess)){
		pSrcDW = (unsigned int *)srcPointer;
		pDestDW = (unsigned int *)destPointer;
		for (Index = 0; Index < count /4; Index++){
			*pDestDW = *pSrcDW;
			pSrcDW++;
			pDestDW++;
		}
		pSrcCh = (uchar *)pSrcDW;
		pDestCh = (uchar *)pDestDW;
		bytesLeft = count % 4;
	}
	else 	if ((((usint)srcPointer & WORDBOUNDARY) == 0) && (((usint)destPointer & WORDBOUNDARY) == 0)) {
		pSrcW = (usint *)srcPointer;
		pDestW = (usint *)destPointer;
		for (Index = 0; Index < count /2; Index++){
			*pDestW = *pSrcW;
			pSrcW++;
			pDestW++;
		}
		pSrcCh = (uchar *)pSrcW;
		pDestCh = (uchar *)pDestW;
		bytesLeft = count % 2;
	}
	else {
		pSrcCh = (uchar *)srcPointer;
		pDestCh = (uchar *)destPointer;
		for (Index = 0; Index < count; Index++){
			*pDestCh = *pSrcCh;
			pSrcCh++;
			pDestCh++;
		}
		bytesLeft = 0;
	}
	for (Index = 0; Index < bytesLeft; Index++){
		*pDestCh = *pSrcCh;
		pSrcCh++;
		pDestCh++;
	}

	return 0;
}

/*
PerformLongDMARead:
	This routine allows the user to make a DMA call for buffers larger than 4k bytes.
	The routine makes multiple calls on	the deviceio.c functions of 4k bytes each.
*/
int PerformLongDMARead(int nDevice, int nModule, char *pBuffer, unsigned long dwLengthInBytes, char *pAddressOnCard) { 
// size of buffer that we read from DAM
#define FOUR_K_BYTES 0x1000
	
	unsigned int chars4dma, charsSent;
	unsigned int charsleft; //number of bytes of space left in user buffer for more messages
	int dmaError;

	charsSent = 0;
	charsleft = dwLengthInBytes;
	while (charsleft > 0) {
		if (charsleft > FOUR_K_BYTES){
			chars4dma = FOUR_K_BYTES;
			charsleft -= FOUR_K_BYTES;
		}
		else {
			chars4dma = charsleft;
			charsleft = 0;
		}
		dmaError = PerformDMARead (nDevice, nModule, (void *)(pBuffer + charsSent), chars4dma, (void *)(pAddressOnCard + charsSent)) ;
		if (dmaError)
			return dmaError;
		charsSent += chars4dma;
	}
	return 0;
}


