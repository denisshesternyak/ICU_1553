#include "Excalbr.h"
#define LODWORD(_qw)    ((DWORD)(_qw))
#define HIDWORD(_qw)    ((DWORD)(((_qw) >> 32) & 0xffffffff))

#define DBG_WRITE_REGISTER_ULONG(addr, data) {DebugMessage(TRUE, "Writing 0x%x to address 0x%p", (data), (addr)); WRITE_REGISTER_ULONG((addr), (data));}
NTSTATUS ExcInitiateDmaTransfer( IN PFDO_DATA        fdoData, IN WDFREQUEST       Request, IN  WDF_DMA_DIRECTION       Direction);
VOID
Excx64EvtIoRead (
    WDFQUEUE      Queue,
    WDFREQUEST    Request,
    size_t        Length
    )
{
    NTSTATUS    status;
    ULONG_PTR bytesCopied =0;
    WDFMEMORY memory;
    PFDO_DATA    fdoData;

    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Queue);

    fdoData = Excx64FdoGetData(WdfIoQueueGetDevice(Queue));
    if (fdoData->dwTotalDMAReadsReq<5)
	    DebugMessage(TRUE, "Excx64EvtIoRead: Request: 0x%p, Queue: 0x%p\n",
			    Request, Queue);

    fdoData->dwTotalDMAReadsReq++;
    fdoData->fDMAOpenRead = TRUE;
    status = ExcInitiateDmaTransfer(fdoData, Request, WdfDmaDirectionReadFromDevice);
    if(!NT_SUCCESS(status)) {
	    DebugMessage(TRUE, "ExcInitiateDmaTransfer failed %X\n", status);
    }

}

VOID
Excx64EvtIoWrite (
    WDFQUEUE      Queue,
    WDFREQUEST    Request,
    size_t        Length
    )

{
    NTSTATUS    status;
    ULONG_PTR bytesWritten =0;
    WDFMEMORY memory;
    PFDO_DATA    fdoData;

    UNREFERENCED_PARAMETER(Queue);

    fdoData = Excx64FdoGetData(WdfIoQueueGetDevice(Queue));
    if (fdoData->dwTotalDMAWritesReq<5)
	    DebugMessage(TRUE, "Excx64EvtIoWrite: Request: 0x%p, Queue: 0x%p\n",
			    Request, Queue);

    fdoData->dwTotalDMAWritesReq++;
    fdoData->fDMAOpenWrite = TRUE;
    status = ExcInitiateDmaTransfer(fdoData, Request, WdfDmaDirectionWriteToDevice);
    if(!NT_SUCCESS(status)) {
	    DebugMessage(TRUE, "ExcInitiateDmaTransfer failed %X\n", status);
    }

}

NTSTATUS
ExcInitiateDmaTransfer(
    IN PFDO_DATA        fdoData,
    IN WDFREQUEST       Request,
    IN  WDF_DMA_DIRECTION       Direction
    )
{
    WDFDMATRANSACTION   dmaTransaction;
    NTSTATUS            status;
    BOOLEAN             bCreated = FALSE;

    if ((fdoData->dwTotalDMAReadsReq<5) && (fdoData->dwTotalDMAWritesReq<5))
	    DebugMessage(TRUE, "ExcInitiateDmaTransfer \n");
    do {
        //
        // Create a new DmaTransaction.
        //
        status = WdfDmaTransactionCreate( fdoData->WdfDmaEnabler,
                                          WDF_NO_OBJECT_ATTRIBUTES,
                                          &dmaTransaction );

        if(!NT_SUCCESS(status)) {
            DebugMessage(TRUE, "WdfDmaTransactionCreate failed %X\n", status);
            break;
        }

        bCreated = TRUE;
        //
        // Initialize the new DmaTransaction.
        //

        status = WdfDmaTransactionInitializeUsingRequest(
                                     dmaTransaction,
                                     Request,
                                     ExcEvtProgramDmaFunction,
                                     Direction );

        if(!NT_SUCCESS(status)) {
            DebugMessage(TRUE, "WdfDmaTransactionInitalizeUsingRequest failed %X\n", status);
            break;
        }

	//Save the transaction handle for the DPC; this is how we get the request to complete
	fdoData->dmaTransaction = dmaTransaction;
        //
        // Execute this DmaTransaction.
        //
        status = WdfDmaTransactionExecute( dmaTransaction,
                                           dmaTransaction );

        if(!NT_SUCCESS(status)) {
            DebugMessage(TRUE, "WdfDmaTransactionExecute failed %X\n", status);
            break;
        }

    } while (FALSE);

    if(!NT_SUCCESS(status)){

        if(bCreated) {
		WdfObjectDelete( dmaTransaction );
		fdoData->dmaTransaction = NULL;

        }
    }
    return status;
}

#define PHYSICAL_START_OF_MODULES 0x2000000
#define PHYSICAL_START_OF_MODULES_1394 0x3000000
BOOLEAN
ExcEvtProgramDmaFunction(
    IN  WDFDMATRANSACTION       Transaction,
    IN  WDFDEVICE               Device,
    IN  PVOID                   Context,
    IN  WDF_DMA_DIRECTION       Direction,
    IN  PSCATTER_GATHER_LIST    ScatterGather
    )
{

	WDFREQUEST    Request;
	WDFMEMORY memory;
	NTSTATUS    status = STATUS_SUCCESS;
	ULONG_PTR bytesWritten = 0;
	DWORD dwBuffer, dwCardOffset, dwRepeatCode;
	size_t        dwLengthOfTransfer;
	WDF_REQUEST_PARAMETERS    params;
	int i;
	PFDO_DATA    fdoData;
	DWORD_PTR DMARegsBase;
	DWORD a2p_mask, ret, irq_mask; 
	fdoData = Excx64FdoGetData(Device);
	if ((fdoData->dwTotalDMAReadsReq<5) && (fdoData->dwTotalDMAWritesReq<5))
		DebugMessage(TRUE, "ExcEvtProgramDmaFunction \n");
	Request = WdfDmaTransactionGetRequest(Transaction);
	WDF_REQUEST_PARAMETERS_INIT(&params);
	// error condition: device does not support dma
	if (!fdoData->fSupportsDMA) {
		DebugMessage(TRUE, "Error: requested DMA operation on non-DMA card");
		(VOID )WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
		WdfObjectDelete(Transaction);
		fdoData->dmaTransaction = NULL;
		WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
		return FALSE;
	}


	WdfRequestGetParameters(
			Request,
			&params
			);

	switch (params.Type) {

		case WdfRequestTypeRead:
			dwLengthOfTransfer    = params.Parameters.Read.Length;
			dwCardOffset = LODWORD(params.Parameters.Read.DeviceOffset);
			dwRepeatCode = HIDWORD(params.Parameters.Read.DeviceOffset);
			break;

		case WdfRequestTypeWrite:
			dwLengthOfTransfer    = params.Parameters.Write.Length;
			dwCardOffset = LODWORD(params.Parameters.Write.DeviceOffset);
			dwRepeatCode = HIDWORD(params.Parameters.Write.DeviceOffset);
			break;

		default:
			DebugMessage(TRUE, "Request type not Read or Write\n");
			(VOID )WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
			WdfObjectDelete(Transaction);
			fdoData->dmaTransaction = NULL;
			WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
			return FALSE;
	}
	//DebugMessage(TRUE, "ExcEvtProgramDmaFunction: Request: 0x%p, Direction: %s, dwLengthOfTransfer: %d, dwCardOffset: %x, dwRepeatCode: %d\n", Request, Direction == WdfDmaDirectionReadFromDevice ? "WdfDmaDirectionReadFromDevice" : "WdfDmaDirectionWriteToDevice", dwLengthOfTransfer, dwCardOffset, dwRepeatCode);
	//DebugMessage(TRUE, "DMA Buffer address = %p", ScatterGather->Elements[0].Address.LowPart);

	// Validate that the requested length fits within the DPR
	if ((dwRepeatCode == 0) && (dwCardOffset + dwLengthOfTransfer >= fdoData->dwDPRSize)) { //TODO: Check when dwRepeatCode is not zero for legal address
		DebugMessage(TRUE, "ERROR: length goes past valid boundaries");
		(VOID )WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
		WdfObjectDelete(Transaction);
		fdoData->dmaTransaction = NULL;
		WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
		return FALSE;
	}

	// Validate that the requested length fits within our physical buffer
	if (dwLengthOfTransfer > fdoData->dwDMABuffSize) {
		DebugMessage(TRUE, "ERROR: cannot do DMA transfer greater than physical buffer length (req: %d; phybufferlen: %d)", dwLengthOfTransfer, fdoData->dwDMABuffSize);
		(VOID )WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
		WdfObjectDelete(Transaction);
		fdoData->dmaTransaction = NULL;
		WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
		return FALSE;
	}

	// Short-circuit a zero byte operation
	if (dwLengthOfTransfer == 0) {
		DebugMessage(TRUE, "ERROR: zero bytes specified");
		(VOID )WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
		WdfObjectDelete(Transaction);
		fdoData->dmaTransaction = NULL;
		WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
		return FALSE;
	}
	
	if (fdoData->dwBoardSubType == EXC_BOARD_807)
		DMARegsBase = fdoData->dwMappedAddr[EXCPCI_BANK_DMA] + 0x4000;
	else if (CardUsesNewGlobalDMARegs(fdoData->dwBoardType, fdoData->dwBoardSubType, fdoData->wFPGARev))
		DMARegsBase = fdoData->dwMappedAddr[EXCPCI_BANK_GLOBAL] + 0x1000;
	else
		DMARegsBase = fdoData->dwMappedAddr[EXCPCI_BANK_DMA];
	if (fdoData->dwBoardSubType == EXC_BOARD_807)
	{ 
		//The following code was copied from alt_pcie_qsys_simple_sw/altpcie_demo.cpp (including cryptic comments): 

		/*
		 * NOTE: In the original code, the Altera people do all their writes with this function:
		 *
		   void OnRCSlaveWrite(WDC_DEVICE_HANDLE hDev, int bar, unsigned int addr, unsigned int wdata){
		   WDC_WriteAddr8(hDev,bar,addr   , wdata    & 0xFF);
		   WDC_WriteAddr8(hDev,bar,addr+1 , (wdata>>8)  & 0xFF);
		   WDC_WriteAddr8(hDev,bar,addr+2 , (wdata>>16) & 0xFF);
		   WDC_WriteAddr8(hDev,bar,addr+3 , (wdata>>24) & 0xFF);
		   }

		 * We have been using WRITE_REGISTER_ULONG which does one 32-bit write, and that may be different than 4 8-bit writes (Dave).
		 */
		// trying and check the address translation path through
		int wordIndex;

		//The next line is an attempt to deal with endless interrupts when the DMA finishes:
		WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CONTROL_BITS), 0);

		WRITE_REGISTER_ULONG((ULONG*)(fdoData->dwMappedAddr[EXCPCI_BANK_DMA]+0x1000), 0xFFFFFFFC); //0x1000 is the offset for translation register

		// reading out the resulted mask of path through
		a2p_mask = READ_REGISTER_ULONG((ULONG*)(fdoData->dwMappedAddr[EXCPCI_BANK_DMA]+0x1000));
		//DebugMessage(TRUE, "a2p_mask = 0x%x", a2p_mask);

		// program address translation table
		// PCIe core limits the data length to be 1MByte, so it only needs 20bits of address.
		WRITE_REGISTER_ULONG((ULONG*)(fdoData->dwMappedAddr[EXCPCI_BANK_DMA]+0x1000), ScatterGather->Elements[0].Address.LowPart & a2p_mask);
		//Maybe we should write the high part like this (Dave):
		//WRITE_REGISTER_ULONG((ULONG*)(fdoData->dwMappedAddr[EXCPCI_BANK_DMA]+0x1004), ScatterGather->Elements[0].Address.HighPart & a2p_mask);
		WRITE_REGISTER_ULONG((ULONG*)(fdoData->dwMappedAddr[EXCPCI_BANK_DMA]+0x1004), 0); // setting upper address   limited at hardIP for now.
		// clear the DMA contoller
		ret = READ_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
		WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL), 0x200);
		ret = READ_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
		WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL2), 0x2); // issue reset dispatcher
		WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL), 0); //clear all the status
		WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL2), 0x10);   // set IRQ enable
		ret = READ_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
		//enable_global_interrupt_mask (alt_u32 csr_base)
		irq_mask = READ_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL2));
		irq_mask |= 0x10;
		WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL2), irq_mask);    //setting the IRQ enable flag at here
		ret = READ_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
		WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL), 0); //clear all the status
		ret = READ_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
	}

	// put info into the card to start the transfer
	// .. the address of our physbuffer
	// We use Elements[0] since we are not doing Scatter-Gather
	if(Direction == WdfDmaDirectionReadFromDevice)
	{
		if (fdoData->dwBoardSubType == EXC_BOARD_807)
		{
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_WRITE_ADDR), ScatterGather->Elements[0].Address.LowPart & ~a2p_mask);
		}
		else
		{
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA1AddrLo), ScatterGather->Elements[0].Address.LowPart);
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA1AddrHi), ScatterGather->Elements[0].Address.HighPart);
		}
	}
	else {
		if (fdoData->dwBoardSubType == EXC_BOARD_807)
		{
			if(fdoData->dwBoardType == EXC_BOARD_1394)
				WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_WRITE_ADDR), dwCardOffset + PHYSICAL_START_OF_MODULES_1394);
			else
				WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_WRITE_ADDR), dwCardOffset + PHYSICAL_START_OF_MODULES);
		}
		else
		{
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA0AddrLo), ScatterGather->Elements[0].Address.LowPart);
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA0AddrHi), ScatterGather->Elements[0].Address.HighPart);
		}
	}

	/*
	 * It is important that we write this 'repeat code' (for serial and 708 modules to do DMA from/to a FIFO) before we write the
	 * base address of the DMA.  This is because in the hardware which does not support this kind of DMA, writing to the (non-existent) 
	 * repeat code register has the bad side effect of writing to the address register itself.  If we do it before writing the address,
	 * it just gets overwritten with the right address.
	 */

	// .. repeat code 
	// TODO: 807 card
	if(Direction == WdfDmaDirectionReadFromDevice)
	{
		if (CardUsesNewGlobalDMARegs(fdoData->dwBoardType, fdoData->dwBoardSubType, fdoData->wFPGARev))
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA1GlobalBankRepeatCode), dwRepeatCode);
		else
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA1RepeatCode), dwRepeatCode);
	}
	else {
		if (CardUsesNewGlobalDMARegs(fdoData->dwBoardType, fdoData->dwBoardSubType, fdoData->wFPGARev))
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA0GlobalBankRepeatCode), dwRepeatCode);
		else
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA0RepeatCode), dwRepeatCode);
	}

	// .. the base address
	//
	// For AFDX we have to do a calculation to determine which BAR to use for the DMA:
	//   A] If the address is within 256MB, it will stay as is, and the operation will be
	//   executed on BAR 0.
	//   B] If the address is above 256MB, the operation will be executed on BAR 1. If it is
	//   within the first 512KB after 256MB, we will subtract 256MB to arrive at the correct
	//   offset.
	//   C] Any other address shall be considered invalid.

	if(fdoData->dwBoardType == EXC_BOARD_AFDX) {
		if (dwCardOffset + dwLengthOfTransfer >= EXCAFDXPCI_TOTAL_MEM) {
			DebugMessage(TRUE, "ERROR: length goes past valid boundaries: End of requested memory = 0x%x, end of memory on card = 0x%x", dwCardOffset + dwLengthOfTransfer, EXCAFDXPCI_TOTAL_MEM);
			(VOID )WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
			WdfObjectDelete(Transaction);
			fdoData->dmaTransaction = NULL;
			WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
			return FALSE;
		}
		if(dwCardOffset >= 2 * EXCAFDXPCI_DPRBANK_LENGTH) {
			dwCardOffset -= 2 * EXCAFDXPCI_DPRBANK_LENGTH;
			if ((fdoData->dwTotalDMAReadsReq<5) && (fdoData->dwTotalDMAWritesReq<5))
				DebugMessage(TRUE, "StartIo: about to write %d to DMABARNum register", EXCAFDXPCI_PHYSBANK_DPR_4000MODS_BARNUM);
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCAFDXPCI_DMABARNum), (DWORD)EXCAFDXPCI_PHYSBANK_DPR_4000MODS_BARNUM);
			dwCardOffset += EXC4000PCI_BANK4_OFFSET; //This gives the right place in the BAR corresponding to the M4K modules
		}
		else {
			if ((fdoData->dwTotalDMAReadsReq<5) && (fdoData->dwTotalDMAWritesReq<5))
				DebugMessage(TRUE, "StartIo: about to write %d to DMABARNum register", EXCAFDXPCI_PHYSBANK_DPR_AFDX_BARNUM);
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCAFDXPCI_DMABARNum), (DWORD)EXCAFDXPCI_PHYSBANK_DPR_AFDX_BARNUM);
		}
		if ((fdoData->dwTotalDMAReadsReq<5) && (fdoData->dwTotalDMAWritesReq<5))
			DebugMessage(TRUE, "StartIo: about to do AFDX DMA transfer.  dwCardOffset = %x", dwCardOffset);
	}
	if(Direction == WdfDmaDirectionReadFromDevice)
	{
		// NOTE: DMA1 base address register is not yet implemented; for now we write to DMA 0
		//WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA1CardAddr), dwCardOffset);
		if (fdoData->dwBoardSubType == EXC_BOARD_807)
		{
			//DbgPrint(TRUE, "StartIo: about to write %d to EXCPCI_ALTERA_DMA_READ_ADDR register", dwCardOffset + PHYSICAL_START_OF_MODULES);
			if(fdoData->dwBoardType == EXC_BOARD_1394)
				WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_READ_ADDR), dwCardOffset + PHYSICAL_START_OF_MODULES_1394);
			else
				WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_READ_ADDR), dwCardOffset + PHYSICAL_START_OF_MODULES);
                }
		else if (CardUsesNewGlobalDMARegs(fdoData->dwBoardType, fdoData->dwBoardSubType, fdoData->wFPGARev))
                {
                        WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMAGlobalBankCardAddr), dwCardOffset);
                }
                else
                {
                        WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA0CardAddr), dwCardOffset);
                }
	}
	else {
		if (fdoData->dwBoardSubType == EXC_BOARD_807)
		{
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_READ_ADDR), ScatterGather->Elements[0].Address.LowPart & ~a2p_mask);
                }
		else if (CardUsesNewGlobalDMARegs(fdoData->dwBoardType, fdoData->dwBoardSubType, fdoData->wFPGARev))
                {
                        WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMAGlobalBankCardAddr), dwCardOffset);
                }
                else
                {
                        WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA0CardAddr), dwCardOffset);
                }
	}

	// .. length 
	if(Direction == WdfDmaDirectionReadFromDevice)
	{
		if (fdoData->dwBoardSubType == EXC_BOARD_807)
		{
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_LENGTH), dwLengthOfTransfer);
		}
		else 
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA1Length), dwLengthOfTransfer);
	}
	else {
		if (fdoData->dwBoardSubType == EXC_BOARD_807)
		{
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_LENGTH), dwLengthOfTransfer);
		}
		else 
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA0Length), dwLengthOfTransfer);
	}

	// .. write the byte to start the transfer!
	if(Direction == WdfDmaDirectionReadFromDevice)
	{
		if (fdoData->dwBoardSubType == EXC_BOARD_807)
		{
			//WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CONTROL_BITS), (1 << 14) | (1 << 24) | (1<<31));
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CONTROL_BITS), (1 << 14) | (1<<31));
		}
		else 
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA1StartAddr), EXCPCI_DMA1StartMask);
	}
	else {
		if (fdoData->dwBoardSubType == EXC_BOARD_807)
		{
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_ALTERA_DMA_CONTROL_BITS), (1 << 14) | (1 << 24) | (1<<31));
		}
		else 
			WRITE_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA0StartAddr), EXCPCI_DMA0StartMask);
	}

	if ((fdoData->dwBoardSubType != EXC_BOARD_807) && (((Direction == WdfDmaDirectionReadFromDevice) && fdoData->fPollingDMARead) || ((Direction == WdfDmaDirectionWriteToDevice) && fdoData->fPollingDMAWrite))
			|| ((fdoData->dwBoardSubType == EXC_BOARD_807) && (((Direction == WdfDmaDirectionReadFromDevice) && fdoData->fPollingDMARead807) || ((Direction == WdfDmaDirectionWriteToDevice) && fdoData->fPollingDMAWrite807))))
	{
		// .. wait for the end of transfer
		WORD loopcounter = 10000;
		for (i=0; i<loopcounter; i++) {
			DWORD dwCurLen;
			if (fdoData->dwBoardSubType == EXC_BOARD_807)
			{
				DWORD_PTR *pdwMemRange = fdoData->dwMappedAddr;
				KeStallExecutionProcessor(1);
				WORD dwIntStatus = READ_REGISTER_USHORT((USHORT*)(pdwMemRange[EXCPCI_BANK_GLOBAL]+EXCPCI_807DMAIntStatus));
				if (dwIntStatus != 0)
				{
					WRITE_REGISTER_USHORT((USHORT*)(pdwMemRange[EXCPCI_BANK_GLOBAL]+EXCPCI_807DMAIntStatus), 1);
					break;
				}
			}
			else
			{
				KeStallExecutionProcessor(1);
				if(Direction == WdfDmaDirectionReadFromDevice)
					dwCurLen = READ_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA1Length));
				else
					dwCurLen = READ_REGISTER_ULONG((ULONG*)(DMARegsBase+EXCPCI_DMA0Length));

				if (!dwCurLen) {
					break;
				}
			}
		}

		if (i>=loopcounter) {
			DebugMessage(TRUE, "ERROR! looped %d times but transfer did not finish!", loopcounter);
			(VOID )WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
			WdfObjectDelete(Transaction);
			fdoData->dmaTransaction = NULL;
			WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
			return TRUE; //We started the DMA
		}


		(VOID )WdfDmaTransactionDmaCompleted(Transaction, &status);
		dwLengthOfTransfer = WdfDmaTransactionGetBytesTransferred(Transaction);
		WdfObjectDelete(Transaction);
		fdoData->dmaTransaction = NULL;
		status = STATUS_SUCCESS;
		WdfRequestCompleteWithInformation(Request, status, dwLengthOfTransfer); 

		if ((Direction == WdfDmaDirectionReadFromDevice))
			fdoData->dwTotalDMAReadsComp++;
		else
			fdoData->dwTotalDMAWritesComp++;
	}

	/*
   The EvtProgramDma callback function must return TRUE if it successfully starts the DMA transfer operation.
   Otherwise, this callback function must return FALSE. However, the framework currently ignores the return value. 
   */
	//DebugMessage(TRUE, "ExcEvtProgramDmaFunction: DMA Succeeded");
	return TRUE; //We started the DMA
}

BOOL CardUsesNewGlobalDMARegs(DWORD dwBoardType, DWORD dwBoardSubType, WORD wFPGARev)
{
	return ((dwBoardType == EXC_BOARD_4000PCI) && ((dwBoardSubType == EXC_BOARD_4000ETH) || (dwBoardSubType == EXC_BOARD_807))) || ((dwBoardType == EXC_BOARD_4000PCI) && (wFPGARev != EXC_FPGAREV_NONE) && (wFPGARev >= 0x50));
}
