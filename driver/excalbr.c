// vim:  sw=2 fdm=indent
#include "exclinuxdef.h"

const char* EXC_KD_VERSION = "2.9";
// Choose device type
// Possibilities: EXC_BOARD_429MX, EXC_BOARD_1553PCIPX, EXC_BOARD_429PCIMX, EXC_BOARD_4000PCI
#define EXC_DEVTYPE EXC_BOARD_4000PCI
//#define EXC_DEVTYPE EXC_BOARD_1553PCIPX

// ISA devices (such as the 429MX): choose resources. 
#define EXC_MEM        0
#define EXC_MEM_LENGTH 0
#define EXC_IRQ        0

// To allow output of debug messages, comment out the definition
// of DebugMessage as an empty function, and #define it to printk
#define DebugMessage printk
//void DebugMessage(char *buf, ...) {}

#define MODULE_BIT(mod)  (1 << (mod))
#define PHYSICAL_START_OF_MODULES 0x2000000 //For DMA on 807 card

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif
#ifndef CONFIG_PCIBIOS
#  define CONFIG_PCIBIOS
#endif

#define __NO_VERSION__ /* don't define kernel_version in module.h */
#include <linux/module.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#include <generated/utsrelease.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#include <linux/utsrelease.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
#include <linux/uaccess.h>
#endif

char kernel_version [] = UTS_RELEASE;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
#include <linux/config.h>
#endif

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>     
#include <linux/errno.h>  
#include <linux/types.h>  
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fcntl.h>     
#include <linux/vmalloc.h>
#include <linux/pci.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18)
#include <linux/sched.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
#include <linux/smp_lock.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
#include <asm/system.h>  
#endif
#include <asm/segment.h> 
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/irq.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/init.h>
#endif

// our own structures
#include "excalbr.h"
#include "excsysio.h"

// our functions
int exc_init_module(void);
void exc_cleanup_module(void); 
int   ExcGetOurPCIDevices(DWORD dwBoardType);
int   ExcAllocateMajorNumber(unsigned long *dwMajor);
int   ExcFindResources(int card);
int   ExcInitResources(int card);
void  ExcEnableInterrupts(int card);
void  ExcDisableInterrupts(int card);
DWORD ExcGetPCIBankAddress(struct exc_pci_dev *dev, int bank);
DWORD ExcGetPCIBankLength(int card, int bank);
int ExcIsIOBank(struct exc_pci_dev *dev, int bank);
int ExcIsValidPCIBank(struct exc_pci_dev *dev, int bank);
char ExcCheckCardID(int bank);
int mGetCardFromMinorNumberInode(struct inode *);
int mGetCardFromMinorNumberInt(int minor);
void QueueRequest(struct file *filp, char __user *buf, size_t count, loff_t *ppos, BOOL read);
void StartIo(ExcDMAPacket* ppkt);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
void EndIo(void *data);
#else /* LINUX2_6_27 */
void EndIo(struct work_struct *pws);
#endif /* LINUX2_6_27 */
int exc_read_procmem(char *buf, char **start, off_t offset, int count, int *eof, void *data);
static void exc_create_proc(void);
static BOOL CardUsesGlobalRegForDMAInterrupts(DWORD dwBoardType, WORD wFPGARev);
static BOOL CardUsesGlobalRegForAllDMARegs(DWORD dwBoardSubType, WORD wFPGARev);

// entry point handlers
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
int ExcIoctl(struct inode *, struct file *, unsigned int, unsigned long);
static long Exc_compat_ioctl(struct file *f, unsigned int cmd, unsigned long arg);
#else
long ExcIoctl(struct file *, unsigned int, unsigned long);
#endif
int ExcOpen(struct inode *, struct file *);
int ExcRelease(struct inode *, struct file *);
int ExcMmap(struct file *, struct vm_area_struct *);
ssize_t ExcRead(struct file *filp, char __user *buf, size_t count, loff_t *ppos);
ssize_t ExcWrite(struct file *filp, const char __user *buf, size_t count, loff_t *ppos);

// As of kernel 2.6, interrupt handlers should return an informative value!
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
void ExcIsr(int irq, void *dev_id, struct pt_regs *regs);
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
irqreturn_t ExcIsr(int irq, void *dev_id, struct pt_regs *regs);
#else /* LINUX2_6_27 */
irqreturn_t ExcIsr(int irq, void *dev_id);
#endif /* LINUX2_6_27 */
#endif

// A device context for each Excalibur device
#define MAXBOARDS 12
static EXCDEVICE ExcDevice[MAXBOARDS]; 
wait_queue_head_t multi_device_waitQueue;

// Debug variable
int gDebug1;

// The driver major number, to allow access from the outside world
static unsigned long dwExcMajor;

// 2.4.10 and beyond: declare GPL license
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,10)
MODULE_LICENSE("GPL");
#endif

// Dispatch functions
static struct file_operations exc_fops =
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
  owner:   THIS_MODULE,
#endif
  mmap:    ExcMmap,
  open:    ExcOpen,
  read:    ExcRead,
  write:    ExcWrite,
  llseek:    default_llseek,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
  ioctl:   ExcIoctl,
  compat_ioctl:    Exc_compat_ioctl,
#else
  unlocked_ioctl:   ExcIoctl,
  compat_ioctl:   ExcIoctl,
#endif
  release: ExcRelease,
};


#ifndef pgprot_noncached

/*
 * This should probably be per-architecture in <asm/pgtable.h>
 */
static inline pgprot_t pgprot_noncached(pgprot_t _prot)
{
	unsigned long prot = pgprot_val(_prot);

#if defined(__i386__) || defined(__x86_64__)
	/* On PPro and successors, PCD alone doesn't always mean 
	    uncached because of interactions with the MTRRs. PCD | PWT
	    means definitely uncached. */ 
	if (boot_cpu_data.x86 > 3)
		prot |= _PAGE_PCD | _PAGE_PWT;
#elif defined(__powerpc__)
	prot |= _PAGE_NO_CACHE | _PAGE_GUARDED;
#elif defined(__mc68000__)
#ifdef SUN3_PAGE_NOCACHE
	if (MMU_IS_SUN3)
		prot |= SUN3_PAGE_NOCACHE;
	else
#endif
	if (MMU_IS_851 || MMU_IS_030)
		prot |= _PAGE_NOCACHE030;
	/* Use no-cache mode, serialized */
	else if (MMU_IS_040 || MMU_IS_060)
		prot = (prot & _CACHEMASK040) | _PAGE_NOCACHE_S;
#endif

	return __pgprot(prot);
}

#endif /* !pgprot_noncached */


// Declare and initialize queue for IRQ alert
// note that this is currently stored as a global
// in order to service multiple interrupts from different cards,
// the queue will have to be localized

// CR1
// Prior to 2.6, this routine needed to be called "init_module" in order to be found by the system.
// Now, we use macros to recognize more specifically-named modules.
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  int init_module(void) {
#else
  int exc_init_module(void) {
#endif

  int iError;
  int card;

  DebugMessage("EXC: ******** INIT MODULE EXCALBR ********* Version %s\n", EXC_KD_VERSION);
  exc_create_proc();

  // Initialize structure
  for (card=0; card<MAXBOARDS; card++) {
    int bank;
    int nModule;
    ExcDevice[card].fExists = FALSE;
    ExcDevice[card].dwSocketNumber = 0;
    ExcDevice[card].dev = NULL;
    ExcDevice[card].dwRefCount = 0;
    ExcDevice[card].fSupportsDMA = FALSE;
    ExcDevice[card].cpu_addr = NULL;
    ExcDevice[card].dma_handle = 0;
    ExcDevice[card].dwTotalDPRAMMemLength = 0;
    ExcDevice[card].dma_count = 0;
    ExcDevice[card].dma_queue_size = 0;
    ExcDevice[card].max_dma_queue_size = 0;
    ExcDevice[card].endio_calls = 0;
    ExcDevice[card].startio_calls = 0;
    ExcDevice[card].dma_interrupt_count = 0;
    ExcDevice[card].isr_calls = 0;
    ExcDevice[card].false_interrupts = 0;
    ExcDevice[card].dwBoardSubType = 0;
    for(nModule = 0; nModule < MAX_MODULES; nModule++)
      ExcDevice[card].fModWakeup[nModule] = FALSE;
    INIT_LIST_HEAD(&ExcDevice[card].dma_queue);
    spin_lock_init(&ExcDevice[card].dma_queue_lock);

    // Initialize arrays
    for (bank=0; bank<MAX_BANKS; bank++){
      ExcDevice[card].dwPhysicalAddress[bank] = 0;
      ExcDevice[card].dwMappedAddress[bank] = 0;
      ExcDevice[card].dwMemLength[bank] = 0;
    }


    // initialize wait queues
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
    init_waitqueue_head(&(ExcDevice[card].waitQueue));
    init_waitqueue_head(&(ExcDevice[card].dma_waitQueue));
    init_waitqueue_head(&(multi_device_waitQueue));
#else
    init_waitqueue(&(ExcDevice[card].waitQueue));
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
    INIT_WORK(&(ExcDevice[card].work_dma), EndIo, &ExcDevice[card]);
#else /* LINUX2_6_27 */
    INIT_WORK(&(ExcDevice[card].work_dma), EndIo);
#endif /* LINUX2_6_27 */
  }

  // Based on board type define, set up device structure, and initalize our device
  switch (EXC_DEVTYPE) {

  case EXC_BOARD_1553PCIPX:
  case EXC_BOARD_429PCIMX:
  case EXC_BOARD_4000PCI:

    iError = ExcGetOurPCIDevices(EXC_DEVTYPE);
    if (iError) {
      DebugMessage("EXC: Could not find PCI device, returning ENODEV\n");
      return -ENODEV;
    }

    break;

  case EXC_BOARD_429MX:

    // for ISA cards, we only support one board
    // and no detection phase necessary
    ExcDevice[0].fExists = TRUE;
    ExcDevice[0].dwBoardType = EXC_BOARD_429MX;
    ExcDevice[0].dwBusType = EXC_BUS_ISA;

    break;

  default:

    // unsupported card
    DebugMessage("EXC: Unsupported Card. returning ENODEV\n");
    return -ENODEV;
  }

  // find and initialize resources for each card
  for (card=0; card<MAXBOARDS; card++) {
    
    DebugMessage("EXC: Finding Resources for Card %d\n", card);
    if (!ExcDevice[card].fExists) continue;
    

    iError = ExcFindResources(card);
    if (iError) {
      DebugMessage("EXC: Error finding resources\n");
      return -ERESTARTSYS;
    }

    DebugMessage("EXC: Initializing Resources for Card %d\n", card);
    
    iError = ExcInitResources(card);
    if (iError) {
      DebugMessage("EXC: Error initializing resources\n");
      return -ERESTARTSYS;
    }
  }

  iError = ExcAllocateMajorNumber(&dwExcMajor);
  if (iError) {
    DebugMessage("EXC: Error Allocating Major Number\n");
    return -ERESTARTSYS;
  }

  return 0;
}


// Prior to 2.6, this routine needed to be called "cleanup_module" in order to be found by the system.
// Now, we use macros to recognize more specifically-named modules.
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  void cleanup_module(void) {
#else
  void exc_cleanup_module(void) {
#endif

  int bank, card;

  // unregister our major number
  unregister_chrdev(dwExcMajor, "excalbr");

  // Traverse our boards
  for (card=0; card<MAXBOARDS; card++) {
    if (ExcDevice[card].fExists) {

      // unhook interrupts
      if (ExcDevice[card].fUsingIRQ) {
	ExcDisableInterrupts(card);
	free_irq(ExcDevice[card].dwIrqNumber, (void*)&(ExcDevice[card]));
      }

      // unmap memory
      for (bank=0; bank<MAX_BANKS; bank++) {
	if(ExcDevice[card].dwPhysicalAddress[bank] == 0)
	  continue;
	iounmap((void*)ExcDevice[card].dwMappedAddress[bank]);
      }
      if (ExcDevice[card].dma_handle)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
	pci_free_consistent(ExcDevice[card].dev, EXC_DMA_BUFFER_SIZE, ExcDevice[card].cpu_addr, ExcDevice[card].dma_handle);
#else
	dma_free_coherent(&ExcDevice[card].dev->dev, EXC_DMA_BUFFER_SIZE, ExcDevice[card].cpu_addr, ExcDevice[card].dma_handle);
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
    }
  }
  remove_proc_entry("excalbr", NULL /* parent dir */);
}

// 2.6 and beyond - explicitly declare startup/shutdown functions
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
module_init(exc_init_module);
module_exit(exc_cleanup_module);
#endif

// CR1
int ExcGetOurPCIDevices(DWORD dwBoardType) {

  DWORD dwID;
  int nextcard;
  int i;
  DWORD alteraIDs[] = {0xE800, 0xE801, 0xE807, 0xE810, 0xE850};

  DebugMessage("EXC: Entering GetOurPCIDevices\n");
    
  // First make sure the computer supports PCI (pre-2.6 only)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  if (!pcibios_present()) {
    DebugMessage("EXC: Error, No PCI Support \n");
    return -EPERM;
  }
#endif
  
  switch (dwBoardType) {
    case (EXC_BOARD_1553PCIPX):

    // look for PCIPX (RevA or Rev B)
    // Only one board supported at a time
    for (dwID = EXC_PCI_PCIPX_DEVICEID_START; dwID <= EXC_PCI_PCIPX_DEVICEID_STOP; dwID++) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
      ExcDevice[0].dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, NULL);
#else /* LINUX2_6_27 */
      ExcDevice[0].dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, NULL);
#endif /* LINUX2_6_27 */

      if (ExcDevice[0].dev != NULL) {
	ExcDevice[0].fExists = TRUE;
	ExcDevice[0].dwBusType = EXC_BUS_PCI;
	ExcDevice[0].dwBoardType = dwBoardType;
	ExcDevice[0].dwPCIInterfaceType = EXC_INTERFACE_AMCC;
	DebugMessage("EXC: Found PCIPx (Rev A or Rev B) series board \n");
	return 0;
      }
    } // end loop through various id's

    // look for PCIPX (Rev C)
    // only one board supported at a time
    for (dwID = EXC_PCI_PCIPX_REVC_DEVICEID_START; dwID <= EXC_PCI_PCIPX_REVC_DEVICEID_STOP; dwID++) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
      ExcDevice[0].dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, NULL);
#else /* LINUX2_6_27 */
      ExcDevice[0].dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, NULL);
#endif /* LINUX2_6_27 */

      if (ExcDevice[0].dev != NULL) {
	ExcDevice[0].fExists = TRUE;
	ExcDevice[0].dwBusType = EXC_BUS_PCI;
	ExcDevice[0].dwBoardType = dwBoardType;
	ExcDevice[0].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	DebugMessage("EXC: Found PCIPx (Rev C) series board \n");
	return 0;
      }
    } // end loop through various id's
    break;    

    case (EXC_BOARD_4000PCI):

    // look for 4000 series card
    // supporting multiple devices - loop until we find all of them
    nextcard = 0;

    for (dwID = EXC_PCI_4000PCI_DEVICEID_START; dwID <= EXC_PCI_4000PCI_DEVICEID_STOP; dwID++) {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = dwBoardType;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	DebugMessage("EXC: Found 4000 series board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } // end loop through various id's

    //Check for 8000 CPCI card with ID 8001 which doesn't fit into previous loop:
    do {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = 0x8001;

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = dwBoardType;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	DebugMessage("EXC: Found 8000 series board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } while(0);

    //Check for express cards
    for (dwID = EXC_PCI_4000PCIe_DEVICEID_START; dwID <= EXC_PCI_4000PCIe_DEVICEID_STOP; dwID++) {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = dwBoardType;
	ExcDevice[nextcard].dwBoardSubType = EXC_BOARD_4000PCIE;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = TRUE;
	DebugMessage("EXC: Found 4000 express board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } // end loop through various id's

    //Check for express card with ID E450 which doesn't fit into previous loop:
    do {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = 0xE450;

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = dwBoardType;
	ExcDevice[nextcard].dwBoardSubType = EXC_BOARD_4000PCIE;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = TRUE;
	DebugMessage("EXC: Found 4000 express board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } while(0); 

    do {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = 0xE427; //708 mPCIe

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = dwBoardType;
	ExcDevice[nextcard].dwBoardSubType = EXC_BOARD_4000PCIE;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = TRUE;
	DebugMessage("EXC: Found 708 mini-PCIe board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } while(0); 

    do {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = 0xE423; //1553 2-module mPCIe

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = dwBoardType;
	ExcDevice[nextcard].dwBoardSubType = EXC_BOARD_4000PCIE;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = TRUE;
	DebugMessage("EXC: Found 708 mini-PCIe board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } while(0); 

    //Check for AFDX cards
    do  {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = 0xE664;

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = dwBoardType;
	ExcDevice[nextcard].dwBoardSubType = EXC_BOARD_AFDX;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = TRUE;
	DebugMessage("EXC: Found AFDX board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } while(0); 

    //Check for the 4000PCIe card with 512M memory space for AFDX modules
    do  {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = 0xE464;

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = dwBoardType;
	ExcDevice[nextcard].dwBoardSubType = EXC_BOARD_4000ETH;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = TRUE;
	DebugMessage("EXC: Found Modular Ethernet board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } while(0); 

    //Check for the 1394 card
    do  {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = 0x1394;

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = EXC_BOARD_1394;
	//ExcDevice[nextcard].dwBoardSubType = EXC_BOARD_1394;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = FALSE;
	DebugMessage("EXC: Found 1394 board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } while(0); 

    //Check for the 1394e card
    do  {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = 0xEF00;

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = EXC_BOARD_1394;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = TRUE;
	DebugMessage("EXC: Found 1394 board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } while(0); 

    //Check for the new 1394e card
    do  {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = 0xE394;

      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = EXC_BOARD_1394;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = TRUE;
	DebugMessage("EXC: Found 1394 board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } while(0); 

    //Check for new Altera-based cards
    for (i = 0; i < (sizeof(alteraIDs) / sizeof(alteraIDs[0])); i++) {
      struct pci_dev *dev = NULL;
      struct pci_dev *prevdev = NULL;
      dwID = alteraIDs[i];

      DebugMessage("EXC: Looking for dwID=0x%lx, address of alteraIDs = %p\n",dwID, alteraIDs);
      while (1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
	dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#else /* LINUX2_6_27 */
	dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, prevdev);
#endif /* LINUX2_6_27 */
	if (dev == NULL) break;

	ExcDevice[nextcard].dev = dev;
	ExcDevice[nextcard].fExists = TRUE;
	ExcDevice[nextcard].dwBusType = EXC_BUS_PCI;
	ExcDevice[nextcard].dwBoardType = dwBoardType;
	ExcDevice[nextcard].dwBoardSubType = EXC_BOARD_807;
	ExcDevice[nextcard].dwPCIInterfaceType = EXC_INTERFACE_CORE;
	ExcDevice[nextcard].fSupportsDMA = TRUE; 
	DebugMessage("EXC: Found 8000 express board (Card %d) \n",nextcard);
	nextcard++;
	prevdev = dev;
      }
    } // end loop through various id's

    // if we have found at least one card, we can return success
    if (nextcard > 0)
      return 0;

    break;    

    case (EXC_BOARD_429PCIMX):

    // look for 429PCI device
    // only one device supported at a time
    for (dwID = EXC_PCI_429PCI_DEVICEID_START; dwID <= EXC_PCI_429PCI_DEVICEID_STOP; dwID++) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
      ExcDevice[0].dev = (struct pci_dev *) pci_find_device(EXC_PCI_VENDOR_ID, dwID, NULL);
#else /* LINUX2_6_27 */
      ExcDevice[0].dev = (struct pci_dev *) pci_get_device(EXC_PCI_VENDOR_ID, dwID, NULL);
#endif /* LINUX2_6_27 */

      if (ExcDevice[0].dev != NULL) {
	ExcDevice[0].fExists = TRUE;
	ExcDevice[0].dwBusType = EXC_BUS_PCI;
	ExcDevice[0].dwBoardType = dwBoardType;
	ExcDevice[0].dwPCIInterfaceType = EXC_INTERFACE_AMCC;
	DebugMessage("EXC: Found 429PCI device\n");
	return 0;
      }
    } // end loop through various 429PCI id's
    break;

    default:
    DebugMessage("EXC: Unsupported PCI card!\n");
    break;

  } // end switch

  return -ENOENT;
}

  // debug function!
char ExcCheckCardID(int bank) {
  char value;
  if(ExcDevice[0].dwPhysicalAddress[bank] == 0){
    DebugMessage("EXC: Invalid bank\n");
    return 0;
  }
  value = readb((unsigned char*)(ExcDevice[0].dwMappedAddress[bank]+0x3FFE));
  DebugMessage("EXC: Found value at 0x3FFE: 0x%x\n", value);
  return value;
}

int ExcFindResources(int card) {

  // Extract information regarding our MEM, IO, and IRQ resources
  // Fill the ExcDevice structure with the results

    int iBank;

    DWORD dwTotalMemBanks = 0;
    DebugMessage("EXC: ExcFindResources for card %d", card);
    ExcDevice[card].fUsingIO = FALSE;
    ExcDevice[card].fUsingIRQ = FALSE;
    
    if (ExcDevice[card].dwBusType == EXC_BUS_ISA) {
      dwTotalMemBanks = 1;
      ExcDevice[card].dwPhysicalAddress[0] = EXC_MEM;
      ExcDevice[card].dwMemLength[0] = EXC_MEM_LENGTH;
      ExcDevice[card].dwTotalDPRAMMemLength = EXC_MEM_LENGTH;
      
      if (EXC_IRQ) {
	ExcDevice[card].fUsingIRQ = TRUE;
	ExcDevice[card].dwIrqNumber = EXC_IRQ;
      }
    }
    
    else {
      
      // Find Memory and IO Banks
      for (iBank=0; iBank < EXC_PCI_MAX_BANKS; iBank++) {
	
	// Is this bank valid on the PCI card?
	if (!ExcIsValidPCIBank(ExcDevice[card].dev, iBank))
	  continue;
	
	// Is this an IO Bank?
	if (ExcIsIOBank(ExcDevice[card].dev, iBank)) {
	  ExcDevice[card].fUsingIO = TRUE;
	  ExcDevice[card].dwIobase = ExcGetPCIBankAddress(ExcDevice[card].dev, iBank);
	  DebugMessage("EXC: PCI Register %d is an I/O space, at 0x%x\n", iBank, (unsigned int) (ExcDevice[card].dwIobase));
	}
	
	// Otherwise, it is a Memory Bank.
	else {
	  DWORD dwAddr, dwLength;
	  dwAddr = ExcGetPCIBankAddress(ExcDevice[card].dev, iBank);
	  dwLength = ExcGetPCIBankLength(card, iBank);
	  DebugMessage("EXC: PCI Register %d is memory bank at 0x%x with length 0x%x\n", iBank, (unsigned int) dwAddr, (unsigned int) dwLength);
	  
	  // Special processing for 4000PCI card
	  if (ExcDevice[card].dwBoardType == EXC_BOARD_4000PCI) {
	    if(ExcDevice[card].dwBoardSubType  == EXC_BOARD_AFDX) {

	      //The AFDX card has two modules for AFDX with 128MB of DPRAM, and then two more 4000 modules of 128KB each
	      //We split each AFDX 'module' into transmit and receive, since these function separately
	      if (iBank == EXCAFDXPCI_PHYSBANK_DPR_AFDX) {

		// Break the area down into four modules

		ExcDevice[card].f4000ExpandedMem = TRUE;
		ExcDevice[card].dwPhysicalAddress[0] = dwAddr;
		ExcDevice[card].dwPhysicalAddress[1] = dwAddr+EXCAFDXPCI_TXDPRBANK_LENGTH;
		ExcDevice[card].dwPhysicalAddress[2] = dwAddr+EXCAFDXPCI_DPRBANK_LENGTH;
		ExcDevice[card].dwPhysicalAddress[3] = dwAddr+EXCAFDXPCI_DPRBANK_LENGTH+EXCAFDXPCI_TXDPRBANK_LENGTH;

		// .. Assign the lengths
		ExcDevice[card].dwMemLength[0] = EXCAFDXPCI_TXDPRBANK_LENGTH;
		ExcDevice[card].dwMemLength[1] = EXCAFDXPCI_RXDPRBANK_LENGTH;
		ExcDevice[card].dwMemLength[2] = EXCAFDXPCI_TXDPRBANK_LENGTH;
		ExcDevice[card].dwMemLength[3] = EXCAFDXPCI_RXDPRBANK_LENGTH;

		ExcDevice[card].dwTotalDPRAMMemLength += EXCAFDXPCI_TOTAL_MEM;
		dwTotalMemBanks += 4;

	      }	
	      else if (iBank == EXCAFDXPCI_PHYSBANK_DPR_4000MODS) {

		// Break the area down into two modules

		ExcDevice[card].dwPhysicalAddress[5] = dwAddr + EXC4000PCI_BANK4_OFFSET;
		ExcDevice[card].dwPhysicalAddress[6] = dwAddr + EXC4000PCI_BANK5_OFFSET;

		// .. Assign the lengths
		ExcDevice[card].dwMemLength[5] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[6] = EXC4000PCI_BANK_LENGTH;

		dwTotalMemBanks += 2;

	      }	
	      else if (iBank == EXCAFDXPCI_PHYSBANK_GLOBAL_AND_DMA) {

		// First global registers
		ExcDevice[card].dwPhysicalAddress[EXC4000PCI_BANK_GLOBAL] = dwAddr;
		ExcDevice[card].dwMemLength[EXC4000PCI_BANK_GLOBAL] = EXCAFDXPCI_GLOBALBANK_LENGTH;
		dwTotalMemBanks ++;

		// DMA registers
		ExcDevice[card].dwPhysicalAddress[EXC4000PCI_BANK_DMA] = dwAddr + EXCAFDXPCI_GLOBALBANK_LENGTH;
		ExcDevice[card].dwMemLength[EXC4000PCI_BANK_DMA] = EXCAFDXPCI_DMABANK_LENGTH;
		dwTotalMemBanks ++;

	      }	
	    }	

	    else if(ExcDevice[card].dwBoardSubType  == EXC_BOARD_4000ETH) {
	      if (iBank == EXC4000PCI_PHYSBANK_DPR) {
		DebugMessage("EXC: Found dual port ram of length 0x%x\n", (unsigned int) dwLength);

		// Break the area down into modules
		ExcDevice[card].dwPhysicalAddress[0] = dwAddr + EXC4000PCI_EXT_BANK0_OFFSET;
		ExcDevice[card].dwPhysicalAddress[1] = dwAddr + EXC4000PCI_EXT_BANK1_OFFSET;
		ExcDevice[card].dwPhysicalAddress[2] = dwAddr + EXC4000PCI_EXT_BANK2_OFFSET;
		ExcDevice[card].dwPhysicalAddress[3] = dwAddr + EXC4000PCI_EXT_BANK3_OFFSET;

		ExcDevice[card].dwMemLength[0] = EXC4000PCI_EXT_BANK_LENGTH;
		ExcDevice[card].dwMemLength[1] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[2] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[3] = EXC4000PCI_EXT_BANK_LENGTH;
		ExcDevice[card].dwTotalDPRAMMemLength += 2 * EXC4000PCI_BANK_LENGTH + 2 * EXC4000PCI_EXT_BANK_LENGTH;
		dwTotalMemBanks += 4;
	      }
	      else if (iBank == EXC4000PCIE_PHYSBANK_GLOBAL)
	      {
		DebugMessage("EXC: Found 4000PCI Global Registers\n");
		ExcDevice[card].dwPhysicalAddress[EXC4000PCI_BANK_GLOBAL] = dwAddr;
		ExcDevice[card].dwMemLength[EXC4000PCI_BANK_GLOBAL] = dwLength;
		dwTotalMemBanks++;
	      }
	      else if (iBank == EXC4000PCI_PHYSBANK_DMA) {

		DebugMessage("EXC: Found 4000PCI DMA Registers\n");

		ExcDevice[card].dwPhysicalAddress[EXCPCI_BANK_DMA] = dwAddr;
		ExcDevice[card].dwMemLength[EXCPCI_BANK_DMA] = dwLength;
		dwTotalMemBanks++;
	      }
	    }

	    else if(ExcDevice[card].dwBoardSubType  == EXC_BOARD_807) {
	      ExcDevice[card].f4000ExpandedMem = TRUE;
	      if (iBank == EXC4000PCI_PHYSBANK_DPR) { 

		if (dwLength != EXC4000PCI_TOTAL_MEM_807){
		  DebugMessage("EXC: Incorrect memory length\n");
		  return -1;
		}

		DebugMessage("EXC: Found dual port ram of length 0x%x\n", (unsigned int) dwLength);

		// Break the area down into modules
		ExcDevice[card].dwPhysicalAddress[0] = dwAddr + EXC4000PCI_BANK0_OFFSET;
		ExcDevice[card].dwPhysicalAddress[1] = dwAddr + EXC4000PCI_BANK1_OFFSET;
		ExcDevice[card].dwPhysicalAddress[2] = dwAddr + EXC4000PCI_BANK2_OFFSET;
		ExcDevice[card].dwPhysicalAddress[3] = dwAddr + EXC4000PCI_BANK3_OFFSET;
		ExcDevice[card].dwPhysicalAddress[5] = dwAddr + EXC4000PCI_BANK4_OFFSET;
		ExcDevice[card].dwPhysicalAddress[6] = dwAddr + EXC4000PCI_BANK5_OFFSET;
		ExcDevice[card].dwPhysicalAddress[7] = dwAddr + EXC4000PCI_BANK6_OFFSET;
		ExcDevice[card].dwPhysicalAddress[8] = dwAddr + EXC4000PCI_BANK7_OFFSET;


		ExcDevice[card].dwMemLength[0] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[1] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[2] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[3] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwTotalDPRAMMemLength += 4 * EXC4000PCI_BANK_LENGTH;
		dwTotalMemBanks += 4;
		ExcDevice[card].dwMemLength[5] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[6] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[7] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[8] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwTotalDPRAMMemLength += 4 * EXC4000PCI_BANK_LENGTH;
		dwTotalMemBanks += 4;
	      }
	      else if (iBank == EXC4000PCI_PHYSBANK_GLOBAL_807_LINUX)
	      {
		DebugMessage("EXC: Found 4000PCI Global Registers\n");
		ExcDevice[card].dwPhysicalAddress[EXC4000PCI_BANK_GLOBAL] = dwAddr;
		ExcDevice[card].dwMemLength[EXC4000PCI_BANK_GLOBAL] = dwLength;
		dwTotalMemBanks++;
	      }
	      else if (iBank == EXC4000PCI_PHYSBANK_DMA_807_LINUX) {

		DebugMessage("EXC: Found 4000PCI DMA Registers\n");

		ExcDevice[card].dwPhysicalAddress[EXCPCI_BANK_DMA] = dwAddr;
		ExcDevice[card].dwMemLength[EXCPCI_BANK_DMA] = dwLength;
		dwTotalMemBanks++;
	      }
	    }

	    else if (iBank == EXC4000PCI_PHYSBANK_DPR) {

	      if (dwLength == EXC4000PCI_MEMLENGTH_DPR2) 
		ExcDevice[card].f4000ExpandedMem = TRUE;
	      else 
		ExcDevice[card].f4000ExpandedMem = FALSE;

	      if ((dwLength != EXC4000PCI_MEMLENGTH_DPR) && (dwLength != EXC4000PCI_MEMLENGTH_DPR2)) {
		DebugMessage("EXC: Incorrect memory length\n");
		return -1;
	      }

	      DebugMessage("EXC: Found dual port ram of length 0x%x\n", (unsigned int) dwLength);

	      // Break the area down into modules
	      ExcDevice[card].dwPhysicalAddress[0] = dwAddr + EXC4000PCI_BANK0_OFFSET;
	      ExcDevice[card].dwPhysicalAddress[1] = dwAddr + EXC4000PCI_BANK1_OFFSET;
	      ExcDevice[card].dwPhysicalAddress[2] = dwAddr + EXC4000PCI_BANK2_OFFSET;
	      ExcDevice[card].dwPhysicalAddress[3] = dwAddr + EXC4000PCI_BANK3_OFFSET;
	      if (dwLength == EXC4000PCI_MEMLENGTH_DPR2) {
		ExcDevice[card].dwPhysicalAddress[5] = dwAddr + EXC4000PCI_BANK4_OFFSET;
		ExcDevice[card].dwPhysicalAddress[6] = dwAddr + EXC4000PCI_BANK5_OFFSET;
		ExcDevice[card].dwPhysicalAddress[7] = dwAddr + EXC4000PCI_BANK6_OFFSET;
		ExcDevice[card].dwPhysicalAddress[8] = dwAddr + EXC4000PCI_BANK7_OFFSET;
	      }


	      ExcDevice[card].dwMemLength[0] = EXC4000PCI_BANK_LENGTH;
	      ExcDevice[card].dwMemLength[1] = EXC4000PCI_BANK_LENGTH;
	      ExcDevice[card].dwMemLength[2] = EXC4000PCI_BANK_LENGTH;
	      ExcDevice[card].dwMemLength[3] = EXC4000PCI_BANK_LENGTH;
	      ExcDevice[card].dwTotalDPRAMMemLength += 4 * EXC4000PCI_BANK_LENGTH;
	      dwTotalMemBanks += 4;
	      if (dwLength == EXC4000PCI_MEMLENGTH_DPR2) {
		ExcDevice[card].dwMemLength[5] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[6] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[7] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwMemLength[8] = EXC4000PCI_BANK_LENGTH;
		ExcDevice[card].dwTotalDPRAMMemLength += 4 * EXC4000PCI_BANK_LENGTH;
		dwTotalMemBanks += 4;
	      }		      
	    }

	    else if ( (ExcDevice[card].dwBoardSubType == 0 && iBank == EXC4000PCI_PHYSBANK_GLOBAL) || (ExcDevice[card].dwBoardSubType == EXC_BOARD_4000PCIE && iBank == EXC4000PCIE_PHYSBANK_GLOBAL) )
	    {

	      DebugMessage("EXC: Found 4000PCI Global Registers\n");

	      ExcDevice[card].dwPhysicalAddress[EXC4000PCI_BANK_GLOBAL] = dwAddr;
	      ExcDevice[card].dwMemLength[EXC4000PCI_BANK_GLOBAL] = dwLength;
	      dwTotalMemBanks++;
	    }
	    else if (iBank == EXC4000PCI_PHYSBANK_DMA) {

	      DebugMessage("EXC: Found 4000PCI DMA Registers\n");

	      ExcDevice[card].dwPhysicalAddress[EXCPCI_BANK_DMA] = dwAddr;
	      ExcDevice[card].dwMemLength[EXCPCI_BANK_DMA] = dwLength;
	      dwTotalMemBanks++;
	    }
	    else {
	      DebugMessage("EXC: Extra bank on 4000PCI card, ignoring\n");
	    }
	  }
	  
	  else if(ExcDevice[card].dwBoardType  == EXC_BOARD_1394) {
	    DebugMessage("EXC: Found dual port ram of length 0x%x\n", (unsigned int) dwLength);


	    // First bank - dual port ram for the four modules
	    if (iBank == EXC1394PCI_PHYSBANK_DPR) {

	      // Make sure the total length is correct
	      if (dwLength != EXC1394_TOTAL_MEM) {
		DebugMessage("EXC: Incorrect memory length\n");
		return FALSE;
	      }

	      // Break the area down into three modules

	      ExcDevice[card].dwPhysicalAddress[0] = dwAddr + EXC1394_BANK0_OFFSET;
	      ExcDevice[card].dwPhysicalAddress[1] = dwAddr + EXC1394_BANK1_OFFSET;
	      ExcDevice[card].dwPhysicalAddress[2] = dwAddr + EXC1394_BANK2_OFFSET;

	      // .. Assign the lengths
	      ExcDevice[card].dwMemLength[0] = EXC1394_BANK_LENGTH;
	      ExcDevice[card].dwMemLength[1] = EXC1394_BANK_LENGTH;
	      ExcDevice[card].dwMemLength[2] = EXC1394_BANK_LENGTH;

	      dwTotalMemBanks += 3;
	      ExcDevice[card].dwTotalDPRAMMemLength += EXC1394_TOTAL_MEM;
	    }

	    // Second bank: global registers
	    else if (iBank == EXC1394PCI_PHYSBANK_GLOBAL_LINUX) {

	      DebugMessage("EXC: Found 1394 Global Registers\n");
	      ExcDevice[card].dwPhysicalAddress[EXC4000PCI_BANK_GLOBAL] = dwAddr;
	      ExcDevice[card].dwMemLength[EXC4000PCI_BANK_GLOBAL] = dwLength;
	      dwTotalMemBanks++;
	    }

	    else if ((ExcDevice[card].fSupportsDMA == TRUE) && (iBank == EXC1394PCI_PHYSBANK_DMA_LINUX)) {
	      DebugMessage("EXC: Found 1394 DMA Registers\n");

	      ExcDevice[card].dwPhysicalAddress[EXCPCI_BANK_DMA] = dwAddr;
	      ExcDevice[card].dwMemLength[EXCPCI_BANK_DMA] = dwLength;
	      dwTotalMemBanks++;
	    }

	    // Extra bank for config registers - we don't need to map it
	    else {
	      DebugMessage("EXC: Not mapping extra bank on 1394 board");
	    }
	  }
	  // Special processing for PCIPx Rev C card (if Core based)
	  else if ((ExcDevice[card].dwBoardType == EXC_BOARD_1553PCIPX) && (ExcDevice[card].dwPCIInterfaceType == EXC_INTERFACE_CORE)) {
	    if (iBank == 0) {
	      
	    ExcDevice[card].dwPhysicalAddress[EXC_PCIPX_CONFIG] = dwAddr;
	    ExcDevice[card].dwMemLength[EXC_PCIPX_CONFIG] = dwLength;
	    dwTotalMemBanks++;
	    
	    DebugMessage("EXC: Sorted Config Bank for PCIPX\n");
	    }
	    else if ((iBank>0) && (iBank<5)) {
	      
	      ExcDevice[card].dwPhysicalAddress[dwTotalMemBanks-1] = dwAddr;
	      ExcDevice[card].dwMemLength[dwTotalMemBanks-1] = dwLength;
	      ExcDevice[card].dwTotalDPRAMMemLength += dwLength;
	      dwTotalMemBanks++;
	      
	      DebugMessage("EXC: Sorted Memory Bank %d for PCIPX\n", (unsigned short)(dwTotalMemBanks-1));
	    }
	    else {
	      DebugMessage("EXC: Extra bank on 4000PCI card, ignoring\n");
	    }
	  }
	  
	  // Other cards
	  else {
	    ExcDevice[card].dwPhysicalAddress[dwTotalMemBanks] = dwAddr;
	    ExcDevice[card].dwMemLength[dwTotalMemBanks] = dwLength;
	    ExcDevice[card].dwTotalDPRAMMemLength += dwLength;
	    dwTotalMemBanks++;
	    DebugMessage("EXC: adding mem bank for default card\n");
	    DebugMessage("EXC: bank number = %ld\n", dwTotalMemBanks - 1);
	    DebugMessage("EXC: dwAddr = %lx\n", dwAddr);
	    DebugMessage("EXC: dwLength = %lx\n", dwLength);
	  }
	}
      }
    
      // Find our IRQ

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
	// enable PCI interrupt routing
	{
	int error;
	error = pci_enable_device(ExcDevice[card].dev);
	DebugMessage("EXC: pci_enable_device returns: 0x%x", error);
	}
#endif

      if (ExcDevice[card].dev->irq) {
	DebugMessage("EXC: IRQ at 0x%x\n", ExcDevice[card].dev->irq);
	ExcDevice[card].fUsingIRQ = TRUE;
	ExcDevice[card].dwIrqNumber = ExcDevice[card].dev->irq;
      }
    }
    
    // Summarize findings:
    DebugMessage("EXC: Found %d memory banks\n", (unsigned short int)(dwTotalMemBanks));
    DebugMessage("EXC: Using IO: %s\n", ExcDevice[card].fUsingIO ? "YES" : "NO");
    DebugMessage("EXC: Using IRQ: %s\n", ExcDevice[card].fUsingIRQ ? "YES" : "NO");

    return 0;
}
 
DWORD ExcGetPCIBankAddress(struct exc_pci_dev *dev, int bank) {
  DWORD dwAddrRegister;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

  dwAddrRegister = pci_resource_start(dev,bank);

#else

  dwAddrRegister = dev->base_address[bank];

  // remove low bit, which signifies memory or IO
  dwAddrRegister &= 0xFFFFFFFE;

#endif

  return dwAddrRegister;
}

int ExcIsIOBank(struct exc_pci_dev *dev, int bank) {
  DWORD dwAddrRegister;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

  dwAddrRegister = pci_resource_flags(dev,bank);

#else

  dwAddrRegister = dev->base_address[bank];

#endif
  
  // low bit signifies IO Space
  if (dwAddrRegister & 0x1)
    return TRUE;
  else
    return FALSE;
}

int ExcIsValidPCIBank(struct exc_pci_dev *dev, int bank) {
  DWORD dwAddrRegister;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

  dwAddrRegister = pci_resource_start(dev,bank);

#else

  dwAddrRegister = dev->base_address[bank];

#endif

  DebugMessage("EXC: ExcIsValidPCIBank: dwAddrRegister = %lx\n", dwAddrRegister);
  if (dwAddrRegister)
    return TRUE;
  else
    return FALSE;
}

DWORD ExcGetPCIBankLength(int card, int bank) {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

  // kernels 2.4 and above: get it programmatically  
  return pci_resource_len(ExcDevice[card].dev, bank);

#else

  // Linux 2.2 makes it hard to retrieve actual lengths, so we just hard code the values.

  if (ExcDevice[card].dwBoardType == EXC_BOARD_4000PCI) {

    // note that under linux 2.2 we will not be able to detect the expanded memory on some 4000PCI cards.
    // however, this is unlikely to be an issue, because the exapanded memory cards only started shipping 
    // in 2008, and it is unlikely that there are still users using 2.2 at this point.
    if (bank == EXC4000PCI_PHYSBANK_DPR) 
      return EXC4000PCI_MEMLENGTH_DPR;
    else if (bank == EXC4000PCI_PHYSBANK_GLOBAL)
      return EXC4000PCI_MEMLENGTH_GLOBAL;
    else
      return 0;
  }
  else if (ExcDevice[card].dwBoardType == EXC_BOARD_1553PCIPX) {
    if (bank == 0)
      return 0x100;
    else
      return 0x8000;
  }
  else {
    return 0x8000;
  }
#endif

}

int ExcInitResources(int card) {

  int i;
  
  // map the memory
  for (i=0; i<MAX_BANKS; i++) {
    DWORD dwPhysAddr, dwLength;
    dwPhysAddr = ExcDevice[card].dwPhysicalAddress[i];
    dwLength = ExcDevice[card].dwMemLength[i];
    if(dwPhysAddr == 0)
      continue;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
    ExcDevice[card].dwMappedAddress[i] = (unsigned long) __ioremap(dwPhysAddr, dwLength,0);
#else /* LINUX2_6_27 */
    ExcDevice[card].dwMappedAddress[i] = (unsigned long) ioremap(dwPhysAddr, dwLength);
#endif /* LINUX2_6_27 */
    DebugMessage("EXC: Got mapped address 0x%p\n", (void*) (ExcDevice[card].dwMappedAddress[i]));

    DebugMessage("EXC: Mapped addr 0x%x of length 0x%x\n", (unsigned int) dwPhysAddr, (unsigned int) dwLength);
    if(ExcDevice[card].dwMappedAddress[i] == (unsigned long)NULL){
      DebugMessage("EXC: Error: Null pointer for bank %d\n", i);
      return -1;
    }
    else
    DebugMessage("EXC: First two words: 0x%x 0x%x\n",
		 *((unsigned short int*)(ExcDevice[card].dwMappedAddress[i])),
		 readw((unsigned short int*)(ExcDevice[card].dwMappedAddress[i]+2)));
  }
  
  // Determine unique ID (M4K card only)
  if (ExcDevice[card].dwBoardType == EXC_BOARD_4000PCI) {

    WORD wBoardIDRegister, wFPGARevRegister;
    wBoardIDRegister = readw((unsigned short int*)(ExcDevice[card].dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+EXC4000PCI_BOARDID));
    //DebugMessage("Reading Unique ID from mapped address %p\n", (unsigned short int*)(ExcDevice[card].dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+EXC4000PCI_BOARDID));

    // extract board ID via mask
    ExcDevice[card].dwSocketNumber = (wBoardIDRegister & EXC4000PCI_BOARDIDMASK);

    DebugMessage("EXC: Determined Unique ID: %d\n",(unsigned int)(ExcDevice[card].dwSocketNumber));
    
    //Get FPGA Revision for changes which depend on it
    wFPGARevRegister = readw((unsigned short int*)(ExcDevice[card].dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+EXC4000PCI_FPGAREV));
    DebugMessage("EXC: Determined FPGA Version: 0x%x\n",(unsigned int)wFPGARevRegister);
    ExcDevice[card].wFPGARev = wFPGARevRegister;
  }

  // take the interrupt
  if (ExcDevice[card].fUsingIRQ) {
    
    int iError;

    DebugMessage("EXC: Hooking allocated IRQ line (#%d)\n", (unsigned int)(ExcDevice[card].dwIrqNumber));
    //disable_irq(ExcDevice[card].dwIrqNumber); 
    iError = request_irq(ExcDevice[card].dwIrqNumber, 
			 ExcIsr,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
			 SA_INTERRUPT | SA_SAMPLE_RANDOM | SA_SHIRQ,
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
			 IRQF_DISABLED | IRQF_SAMPLE_RANDOM | IRQF_SHARED,
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
			 IRQF_DISABLED | IRQF_SHARED,
#else 
			 IRQF_SHARED,
#endif /* LINUX2_6_27 */
			 "Excalibur",
			 (void*)&(ExcDevice[card]));
    if (iError) {
      DebugMessage("EXC: Error hooking IRQ (0x%x)\n", iError);
      ExcDevice[card].fUsingIRQ = FALSE;
    }
    
    ExcEnableInterrupts(card);
  }

  //Allocate DMA buffer if relevant
  if(ExcDevice[card].fSupportsDMA == TRUE){
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
    ExcDevice[card].cpu_addr = pci_alloc_consistent(ExcDevice[card].dev, EXC_DMA_BUFFER_SIZE, &ExcDevice[card].dma_handle);
#else
    ExcDevice[card].cpu_addr = dma_alloc_coherent(&ExcDevice[card].dev->dev, EXC_DMA_BUFFER_SIZE, &ExcDevice[card].dma_handle, GFP_DMA);
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
    DebugMessage("EXC: Allocated DMA buffer: cpu_addr = %lx, dma_handle = %lx\n",(unsigned long)(ExcDevice[card].cpu_addr), (unsigned long)(ExcDevice[card].dma_handle));
  }

  return 0;
}

void ExcEnableInterrupts(int card) {

  if (ExcDevice[card].dwBusType == EXC_BUS_PCI) {
    if (ExcDevice[card].dwPCIInterfaceType == EXC_INTERFACE_AMCC) {
      
      // Enable interrupts on the AMCC
      DWORD dwControl;
      
      dwControl = inl(ExcDevice[card].dwIobase + PCI_IO_INTERRUPT_CONTROL);
      DebugMessage("EXC: Initial value of control register: 0x%x\n", (unsigned int) dwControl);
      
      outl(PCI_IO_INTCTL_ENABLE, ExcDevice[card].dwIobase + PCI_IO_INTERRUPT_CONTROL);
      
      dwControl = inl(ExcDevice[card].dwIobase + PCI_IO_INTERRUPT_CONTROL);
      DebugMessage("EXC: Enabled interrupts. Current value of control register: 0x%x\n", (unsigned int) dwControl);
      
      // make sure that any previous interrupt is reset
      outl(dwControl, ExcDevice[card].dwIobase + PCI_IO_INTERRUPT_CONTROL);
    }
  }
}

void ExcDisableInterrupts(int card) {

  if (ExcDevice[card].dwBusType == EXC_BUS_PCI) {
    if (ExcDevice[card].dwPCIInterfaceType == EXC_INTERFACE_AMCC) {

      // Disable Interrupts on the AMCC chip
      outl(PCI_IO_INTCTL_DISABLE, ExcDevice[card].dwIobase + PCI_IO_INTERRUPT_CONTROL);
    }
  }
}

int ExcAllocateMajorNumber(unsigned long *dwMajor) {
  
  int result;
    
  // ask for a dynamically allocated major number
  result = register_chrdev(0, "excalbr", &exc_fops);
  if (result < 0) {
    DebugMessage("EXC: error, cannot get major number\n");
    return -EPERM;
  }

  DebugMessage("EXC: got major number %d\n", result);
  *dwMajor = result;
  return 0;
}

int ExcOpen(struct inode *inode, struct file *filp){

   int i;

   // get minor number
   int card = mGetCardFromMinorNumberInode(inode);
   if (card < 0) return card;

   DebugMessage("EXC: Received Open() call from Application. Card: %d\n", card);

   // Is this the first open?
   if (!ExcDevice[card].dwRefCount) {

     DebugMessage("EXC: First handle open, performing initialization\n");

     // reset interrupt counts
     ExcDevice[card].dwTotalInterrupts = 0;
     ExcDevice[card].MchIntShadowRegister = 0;
     for (i=0; i<MAX_BANKS; i++) 
       ExcDevice[card].MchInterruptCount[i] = 0;
   }

   // debug
   gDebug1 = card;

   // increment reference count
   ExcDevice[card].dwRefCount++;
   filp->private_data = &ExcDevice[card];

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)

   // prevent unloading of driver until the handle is closed
   MOD_INC_USE_COUNT;

#endif

   return 0;
 }

 int ExcRelease(struct inode *inode, struct file *filp) {

   // get minor number
   int card = mGetCardFromMinorNumberInode(inode);
   if (card < 0) return card;

   DebugMessage("EXC: Recevied Close() call from Application. Card: %d\n", card);
   DebugMessage("EXC: Total interrupt count %d\n", (unsigned int)(ExcDevice[card].dwTotalInterrupts));

   ExcDevice[card].dwRefCount--;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
   MOD_DEC_USE_COUNT;
#endif

   return 0;
 }

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)

// In pre-2.6 versions of the kernel, this function was not automatically available for use
// (this particular version is copied from /drivers/char/mem.c). From 2.6, however, it appears
// that we can rely on the function existing for use from within the kernel headers.
static inline unsigned long makemask_pgprot_noncached(unsigned long prot) {

#if defined(__i386__)
	if (boot_cpu_data.x86 > 3)
		prot |= _PAGE_PCD;
#elif defined(__powerpc__)
	prot |= _PAGE_NO_CACHE | _PAGE_GUARDED;
#elif defined(__mc68000__)
	if (CPU_IS_020_OR_030)
		prot |= _PAGE_NOCACHE030;
	/* Use no-cache mode, serialized */
	if (CPU_IS_040_OR_060)
		prot = (prot & _CACHEMASK040) | _PAGE_NOCACHE_S;
#elif defined(__mips__)
	prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;
#endif

	return prot;
}
#endif

static inline int range_is_allowed(unsigned long pfn, unsigned long size, EXCDEVICE* pdx)
{
    u64 from = ((u64)pfn) << PAGE_SHIFT;
    u64 to = from + size;
    u64 cursor = from;
    DebugMessage("Exc: " "range_is_allowed: Program %s trying to access memory between %Lx->%Lx.\n", current->comm, from, to);

    while (cursor < to) {
	BOOL foundpage = FALSE;
	int i = 0;
	for (i=0; i<MAX_BANKS; i++) {
	    if ((cursor >= pdx->dwPhysicalAddress[i]) && (cursor + min(size, PAGE_SIZE) <= pdx->dwPhysicalAddress[i] + max(pdx->dwMemLength[i], PAGE_SIZE)))
	    {
		foundpage = TRUE;
		break;
	    }
	}
	if (!foundpage) {
	    DebugMessage("Exc: " "Program %s tried to access memory between %Lx->%Lx.\n", current->comm, from, to);
	    return 0;
	}
	cursor += PAGE_SIZE;
    }
    return 1;
}

int ExcMmap(struct file *file, struct vm_area_struct *vma) {

  // This mmap code comes from /drivers/char/mem.c 
  // with modifications for kernel 2.4
	size_t size = vma->vm_end - vma->vm_start;
	EXCDEVICE* pdx = file->private_data;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
        unsigned long offset = vma->vm_offset;
#else
        unsigned long offset = vma->vm_pgoff<<PAGE_SHIFT;
	if (!range_is_allowed(vma->vm_pgoff, size, pdx))
		return -EPERM;
#endif
    
   
   DebugMessage("EXC: Received request to map memory at offset 0x%x\n", (unsigned int) offset);

   if (offset & ~PAGE_MASK)
     return -ENXIO;

   /*
    * Accessing memory above the top the kernel knows about or
    * through a file pointer that was marked O_SYNC will be
    * done non-cached.
    *
    * Set VM_IO, as this is likely a non-cached access to an
    * I/O area, and we don't want to include that in a core
    * file.
    */


   if (offset >= __pa(high_memory) || (file->f_flags & O_SYNC)) {

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,18)
     pgprot_val(vma->vm_page_prot) = pgprot_noncached(pgprot_val(vma->vm_page_prot));
#else
     // in kernel 2.6 we use the internal pgprot_noncached function, which does not need
     // the pgprot_val wrappers.
     vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif

//This makes sure that if we are not in RHEL, the conditional about RHEL_RELEASE_CODE is always false:
#ifndef RHEL_RELEASE_VERSION
#define RHEL_RELEASE_VERSION(a,b) 1
#define RHEL_RELEASE_CODE 0
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)
     vm_flags_set(vma, VM_IO);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0)) && (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9,5))
     vm_flags_set(vma, VM_IO); //same as above since RHEL backported this feature
#else
     vma->vm_flags |= VM_IO;
#endif
   }

   // Call remap_page_range to effect the mapping
   // Kernel 2.5.3 added an extra parameter - the vma needs to be passed as the initial 
   // parameter to remap_page_range.
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,20)
   if (remap_page_range(vma->vm_start, offset, vma->vm_end-vma->vm_start,
			vma->vm_page_prot))
     return -EAGAIN;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
   if (remap_page_range(vma, vma->vm_start, offset, vma->vm_end-vma->vm_start,
			vma->vm_page_prot))
     return -EAGAIN;
#else 
   if (io_remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT, vma->vm_end-vma->vm_start, vma->vm_page_prot))
	return -EAGAIN;
#endif

   // Debug:
   // Read first 10 words of the global registers
   // 4000 card only!
  if (EXC_DEVTYPE == EXC_BOARD_4000PCI) {

     WORD w[10];
     int i;
     for (i=0; i<10; i++) {
       if(ExcDevice[gDebug1].dwMappedAddress[EXC4000PCI_BANK_GLOBAL] != 0) //Safety check
	 w[i]=readw((unsigned short int*)(ExcDevice[gDebug1].dwMappedAddress[EXC4000PCI_BANK_GLOBAL] + i*2));
     }
     DebugMessage("EXC: GlobalRegDump: 0x%x 0x%x 0x%x 0x%x 0x%x\n", w[0],w[1],w[2],w[3],w[4]);
     DebugMessage("EXC: GlobalRegDump: 0x%x 0x%x 0x%x 0x%x 0x%x\n", w[5],w[6],w[7],w[8],w[9]);
  }

   DebugMessage("EXC: Successfully mapped memory\n");
   return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
static long Exc_compat_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
  long retval;

  switch (cmd) {
    case EXC_IOCTL_GETBANKINFO:
    case EXC_IOCTL_GET_INTERRUPT_COUNT:
    case EXC_IOCTL_GET_MCH_INT_COUNT:
    case EXC_IOCTL_GET_MCH_SHADOW:
    case EXC_IOCTL_GET_IRQ_NUMBER:
    case EXC_IOCTL_WAIT_FOR_IRQ:
    case EXC_IOCTL_CALL_ENDIO:
    case EXC_IOCTL_WAIT_FOR_MOD_IRQ:
    case EXC_IOCTL_WAIT_FOR_MULT_IRQ_MULT_DEV:
      lock_kernel();
      retval = ExcIoctl(f->f_dentry->d_inode, f, cmd, arg);
      unlock_kernel();
      return retval;

    default:
      return -ENOIOCTLCMD;
  }
}
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
int ExcIoctl (struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
#else
long ExcIoctl (struct file *filp,
                 unsigned int cmd, unsigned long arg)
#endif
{
    // get minor number
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
    int card = mGetCardFromMinorNumberInode(inode);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
    int card = mGetCardFromMinorNumberInode(filp->f_dentry->d_inode);
#else
    int card = mGetCardFromMinorNumberInode(filp->f_path.dentry->d_inode);
#endif
    if (card < 0) return card;

    DebugMessage("EXC: Received IOCTL (0x%x). Card: %d\n", cmd, card);

    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return EINVAL before verify_area()
     */
    if (_IOC_TYPE(cmd) != EXC_IOCTL_MAGIC) return -EINVAL;
    if (_IOC_NR(cmd) > EXC_IOCTL_MAX) return -EINVAL;

    switch(cmd) {

      case EXC_IOCTL_GETBANKINFO:
	{
	  ExcBankInfo BankInfo;
	  unsigned long dwBank;

	  DebugMessage("EXC: EXC_IOCTL_GETBANKADDR\n");
	  //DebugMessage("EXC: Input Buffer from User: 0x%x\n", arg);

	  if (copy_from_user(&BankInfo, (unsigned long *) arg, sizeof(ExcBankInfo))){
	    DebugMessage("EXC: Error reading input from user\n");
	    return -EFAULT;
	  }

	  dwBank = BankInfo.dwBank; 
	  DebugMessage("EXC: Request for Physical Address of bank %d\n", (unsigned int) dwBank);

	  // PCI/Px core based: refuse request for configuration range
	  // Config range is used only for interrupt reset, not of interest to user program
	  if ( (ExcDevice[card].dwBoardType == EXC_BOARD_1553PCIPX) && (!ExcDevice[card].fUsingIO) ) {
	    if (dwBank == EXC_PCIPX_CONFIG) {
	      DebugMessage("Config space requested on core card, failing request");
	      return -EINVAL;
	    }
	  }
	  
	  if(ExcDevice[card].dwPhysicalAddress[dwBank] == 0){
	    DebugMessage("EXC: This bank does not exist!\n");
	    return -EINVAL;
	  }

	  BankInfo.dwAddress = ExcDevice[card].dwPhysicalAddress[dwBank];
	  BankInfo.dwLength = ExcDevice[card].dwMemLength[dwBank];

	  if (copy_to_user((unsigned long *) arg, &BankInfo, sizeof(ExcBankInfo))) {
	    DebugMessage("EXC: Error putting output to user\n");
	    return -EFAULT;
	  }
	  DebugMessage("EXC: Returning address 0x%x\n", (unsigned int) (BankInfo.dwAddress));
	  
	  break;
	}
    case EXC_IOCTL_GET_INTERRUPT_COUNT:

      // Copy interrupt count to user's buffer
      if (copy_to_user((unsigned int *) arg, &(ExcDevice[card].dwTotalInterrupts), sizeof(unsigned int))) {
	DebugMessage("EXC: GETINTTOTAL, error putting output to user\n");
	return -EFAULT;
      }
      break;

    case EXC_IOCTL_GET_MCH_INT_COUNT:
      {
	unsigned int dwBank;

	DebugMessage("EXC: EXC_IOCTL_GET_MCH_INT_COUNT\n");
	// get bank number from user's request
	if (copy_from_user(&dwBank, (unsigned int *) arg, sizeof(unsigned int))){
	  DebugMessage("EXC: Error reading input from user\n");
	  return -EFAULT;
	}

	if(ExcDevice[card].dwPhysicalAddress[dwBank] == 0){
	  DebugMessage("EXC: GetMchIntCount(): this bank does not exist!\n");
	  return -EINVAL;
	}

	if (copy_to_user((unsigned int *) arg, &(ExcDevice[card].MchInterruptCount[dwBank]), sizeof(unsigned int))) {
	  DebugMessage("EXC: GetMchIntCount(): Error putting output to user\n");
	  return -EFAULT;
	}
	break;
      }

    case EXC_IOCTL_GET_MCH_SHADOW:
      {

      // the "mch shadow" is a shadow of a (possibly virtual) register which
      // sets a bit for every module which interrupts, allowing the user to
      // decipher multiple, possible simultaenous, interrupts from multiple
      // modules.
      // The actual register may be reset by the kernel, as the kernel
      // processes the interrupts.  Thus, the need for a shadow register,
      // which preserves the information for the user mode program.

      // In our operation here, we use a shadow of the shadow, in order
      // to ensure that information is not lost in between the info transfer
      // and the reset.
      unsigned char ShadowOfShadow = ExcDevice[card].MchIntShadowRegister;
      
      // copy interrupt mask to user's buffer
      if (copy_to_user((unsigned char *) arg, &ShadowOfShadow, sizeof(unsigned char))) {
	DebugMessage("EXC: GetMchShadow(): Error putting output to user\n");
	return -EFAULT;
      }

      // reset the shadow register
      ExcDevice[card].MchIntShadowRegister &= (~(ShadowOfShadow));

      break;
      }
      
    case EXC_IOCTL_GET_IRQ_NUMBER:

      {
	unsigned int dwIrq;
	DebugMessage("EXC: EXC_IOCTL_GET_IRQ_NUMBER\n");

	if (ExcDevice[card].fUsingIRQ)
	  dwIrq = ExcDevice[card].dwIrqNumber;
	else
	  dwIrq = 0;

	// copy IRQ number to user's buffer
	if (copy_to_user((unsigned int *) arg, &dwIrq, sizeof(unsigned int))) {
	  DebugMessage("EXC: GetIRQNumber(): Error putting output to user\n");
	  return -EFAULT;
	}
	DebugMessage("EXC: Returning %d\n", (unsigned int) dwIrq);
	break;
      }
    case EXC_IOCTL_WAIT_FOR_IRQ:
      // put process to sleep
      // process will be woken up in Isr when interrupt occurs

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
      interruptible_sleep_on(&(ExcDevice[card].waitQueue));
#else
      // new wait mechanism: use a waitqueue object and the prepare/finish funcs
      // init the waitqueue object
      init_wait((&(ExcDevice[card].wait)));

      prepare_to_wait(&(ExcDevice[card].waitQueue), &(ExcDevice[card].wait), TASK_INTERRUPTIBLE);
      schedule();
      finish_wait(&(ExcDevice[card].waitQueue), &(ExcDevice[card].wait));
#endif

      break;
    case EXC_IOCTL_CALL_ENDIO:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
      EndIo(&ExcDevice[card]);
#else /* LINUX2_6_27 */
      EndIo(&ExcDevice[card].work_dma);
#endif /* LINUX2_6_27 */
      break;
    case EXC_IOCTL_WAIT_FOR_MOD_IRQ:

      {
	ExcIRQRequest irqreq;
	unsigned int nModule, timeoutjiffies;
	unsigned long		starttime = jiffies;
	DEFINE_WAIT(wait);
	DebugMessage("EXC: EXC_IOCTL_WAIT_FOR_MOD_IRQ\n");
	//DebugMessage("EXC: Input Buffer from User: 0x%x\n", arg);

	//if (copy_from_user(&nModule, (unsigned long *) arg, sizeof(nModule))){
	if (copy_from_user(&irqreq, (unsigned long *) arg, sizeof(irqreq))){
	  DebugMessage("EXC: Error reading input from user\n");
	  return -EFAULT;
	}
	nModule = (unsigned int)irqreq.nModule;
	timeoutjiffies = ((unsigned int)irqreq.timeout * HZ) / 1000;
	DebugMessage("EXC: timeoutjiffies = %d\n", timeoutjiffies);
	irqreq.timedOut = FALSE;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	interruptible_sleep_on(&(ExcDevice[card].waitQueue));
#else
	// new wait mechanism: use a waitqueue object and the prepare/finish funcs
	// init the waitqueue object
	init_wait((&(ExcDevice[card].wait)));

	ExcDevice[card].fModWakeup[nModule] = FALSE;
	while((!ExcDevice[card].fModWakeup[nModule]) && (jiffies < starttime + timeoutjiffies)){
	  prepare_to_wait(&(ExcDevice[card].waitQueue), &(wait), TASK_INTERRUPTIBLE);
	  if(ExcDevice[card].fModWakeup[nModule]){
	    finish_wait(&(ExcDevice[card].waitQueue), &(wait));
	    break;
	  }
	  schedule_timeout(timeoutjiffies);
	  finish_wait(&(ExcDevice[card].waitQueue), &(wait));
	}
	if(!ExcDevice[card].fModWakeup[nModule]){
	  DebugMessage("EXC: EXC_IOCTL_WAIT_FOR_MOD_IRQ timed out\n");
	  irqreq.timedOut = TRUE;
	}
	else
	  ExcDevice[card].fModWakeup[nModule] = FALSE;
	if (copy_to_user((unsigned long *) arg, &irqreq, sizeof(irqreq))) {
	  DebugMessage("EXC: Error putting output to user\n");
	  return -EFAULT;
	}
#endif

      }

      break;

    case EXC_IOCTL_WAIT_FOR_MULT_IRQ_MULT_DEV:
      {
	ExcMultipleIRQMultipleDeviceRequest multirqreq;
	WORD modulesInterrupting = 0;
	int timeoutjiffies, pairnum;
	unsigned long		starttime = jiffies;
	DEFINE_WAIT(wait);
	DebugMessage("EXC: EXC_IOCTL_WAIT_FOR_MULT_IRQ_MULT_DEV\n");

	if (copy_from_user(&multirqreq, (unsigned long *) arg, sizeof(multirqreq))){
	  DebugMessage("EXC: Error reading input from user\n");
	  return -EFAULT;
	}
	
	for(pairnum = 0; pairnum < sizeof(DWORD)*8; pairnum++){
	  //DebugMessage("nDevice = %d, nModule = %d\n", multirqreq.DevModPairs[pairnum].nDevice , multirqreq.DevModPairs[pairnum].nModule );
	}
	timeoutjiffies = (multirqreq.timeout * HZ) / 1000;
	DebugMessage("EXC: timeoutjiffies = %d\n", timeoutjiffies);
	multirqreq.timedOut = FALSE;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	return -EINVAL;
#else
	// new wait mechanism: use a waitqueue object and the prepare/finish funcs
	// init the waitqueue object

	//while((!ExcDevice[card].fModWakeup[moduleBitfield]) && (jiffies < starttime + timeoutjiffies)){
	while(jiffies < starttime + timeoutjiffies){
	  int nDevice;
	  int nModule;
	  int cardnum;
	  for(pairnum = 0; pairnum < multirqreq.numpairs; pairnum++){
	    DebugMessage("nDevice = %d, nModule = %d\n", multirqreq.DevModPairs[pairnum].nDevice , multirqreq.DevModPairs[pairnum].nModule );
	    nDevice = multirqreq.DevModPairs[pairnum].nDevice;
	    nModule = multirqreq.DevModPairs[pairnum].nModule;
	    if(nDevice == -1 || nModule == -1)
	      break;
	    cardnum = mGetCardFromMinorNumberInt(nDevice);
	    if(ExcDevice[cardnum].fModWakeup[nModule]){
	      modulesInterrupting |= 1 << pairnum;
	      ExcDevice[cardnum].fModWakeup[nModule] = FALSE;
	    }
	  }
	  if(modulesInterrupting != 0)
	    break;
	  prepare_to_wait(&(multi_device_waitQueue), &(wait), TASK_INTERRUPTIBLE);
	  for(pairnum = 0; pairnum < multirqreq.numpairs; pairnum++){
	    DebugMessage("nDevice = %d, nModule = %d\n", multirqreq.DevModPairs[pairnum].nDevice , multirqreq.DevModPairs[pairnum].nModule );
	    nDevice = multirqreq.DevModPairs[pairnum].nDevice;
	    nModule = multirqreq.DevModPairs[pairnum].nModule;
	    if(nDevice == -1 || nModule == -1)
	      break;
	    cardnum = mGetCardFromMinorNumberInt(nDevice);
	    if(ExcDevice[cardnum].fModWakeup[nModule]){
	      modulesInterrupting |= 1 << pairnum;
	      ExcDevice[cardnum].fModWakeup[nModule] = FALSE;
	    }
	  }
	  if(modulesInterrupting != 0){
	    finish_wait(&(multi_device_waitQueue), &(wait));
	    break;
	  }
	  schedule_timeout(timeoutjiffies);
	  finish_wait(&(multi_device_waitQueue), &(wait));
	}
	if(modulesInterrupting == 0){
	  DebugMessage("EXC: EXC_IOCTL_WAIT_FOR_MOD_IRQ timed out\n");
	  multirqreq.timedOut = TRUE;
	}
	multirqreq.modulesInterrupting = modulesInterrupting;
	if (copy_to_user((unsigned long *) arg, &multirqreq, sizeof(multirqreq))) {
	  DebugMessage("EXC: Error putting output to user\n");
	  return -EFAULT;
	}
#endif

      }

      break;
    default:  /* redundant, as cmd was checked against MAXNR */
      return -EINVAL;
    }
    return 0;
}


//CR1
// As of kernel 2.6, interrupt handlers should return an informative value!
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  void ExcIsr(int irq, void *dev_id, struct pt_regs *regs) {
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
  irqreturn_t ExcIsr(int irq, void *dev_id, struct pt_regs *regs) {
#else /* LINUX2_6_27 */
  irqreturn_t ExcIsr(int irq, void *dev_id) {
#endif /* LINUX2_6_27 */
    irqreturn_t retval = IRQ_NONE;
#endif

  // ISR routine
  // Process differs based on board type

  EXCDEVICE *pInterruptingDevice = (EXCDEVICE*) dev_id;
  pInterruptingDevice->isr_calls++;

  //DebugMessage("EXC: We're in the ISR!\n");

  //Handle DMA Interrupt if supported
  if (((pInterruptingDevice->dwBoardType==EXC_BOARD_4000PCI) || (pInterruptingDevice->dwBoardType==EXC_BOARD_1394)) && (pInterruptingDevice->fSupportsDMA == TRUE) && (!CardUsesGlobalRegForDMAInterrupts(pInterruptingDevice->dwBoardSubType, pInterruptingDevice->wFPGARev)) && (!CardUsesGlobalRegForAllDMARegs(pInterruptingDevice->dwBoardSubType, pInterruptingDevice->wFPGARev))){

    DWORD dwStatus = readl((ULONG*)(pInterruptingDevice->dwMappedAddress[EXCPCI_BANK_DMA]+EXCPCI_DMAIntStatus));
    if (dwStatus & EXCPCI_DMAIntMask) {


      // DMA interrupt has occured!
      pInterruptingDevice->dma_interrupt_count++;
      pInterruptingDevice->dwTotalInterrupts++;

      // .. write bits back to reset it
      writel(dwStatus, (ULONG*)(pInterruptingDevice->dwMappedAddress[EXCPCI_BANK_DMA]+EXCPCI_DMAIntReset));
      schedule_work(&pInterruptingDevice->work_dma);
      retval = IRQ_HANDLED;
    }
  }
  else if ((pInterruptingDevice->dwBoardType==EXC_BOARD_4000PCI) && (pInterruptingDevice->fSupportsDMA == TRUE) && (CardUsesGlobalRegForAllDMARegs(pInterruptingDevice->dwBoardSubType, pInterruptingDevice->wFPGARev))){
	
    DWORD dwStatus = readl((ULONG*)(pInterruptingDevice->dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+0x1000+EXCPCI_DMAIntStatus));
    if (dwStatus & EXCPCI_DMAIntMask) {


      // DMA interrupt has occured!
      pInterruptingDevice->dma_interrupt_count++;
      pInterruptingDevice->dwTotalInterrupts++;

      // .. write bits back to reset it
      writel(dwStatus, (ULONG*)(pInterruptingDevice->dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+0x1000+EXCPCI_DMAIntReset));
      schedule_work(&pInterruptingDevice->work_dma);
      retval = IRQ_HANDLED;
    }
  }

  if (pInterruptingDevice->dwBoardType==EXC_BOARD_1553PCIPX) {

    BOOL        fLineHigh;
    int	        numLoops=0;
    DWORD	dwCtlRegister=0;
  
    // First check that this is our interrupt
    // .. RevA/B check
    if (pInterruptingDevice->fUsingIO) {
      dwCtlRegister = inl(pInterruptingDevice->dwIobase + PCI_IO_INTERRUPT_CONTROL);
      if (!(dwCtlRegister & PCI_IO_INTCTL_INTMASK)) {

	// 2.6 and beyond - indicate that this is not our interrupt!
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	return;
#else
	return retval;
#endif
      }
    }
    // .. Rev C check
    else {
      DWORD	dwIntStatus;
      dwIntStatus = readl((unsigned long*)(pInterruptingDevice->dwMappedAddress[EXC_PCIPX_CONFIG]+EXC_PCIPX_INTSTATUS));
      if ( (dwIntStatus & EXC_PCIPX_INTMASK_ANYMOD) == 0) {
	// 2.6 and beyond - indicate that this is not our interrupt!
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	return;
#else
	return retval;
#endif
      }
    }

    // Process interrupt: Rev A/B
    if (pInterruptingDevice->fUsingIO) {
      
      // Loop while the line is high
      do {
	
	unsigned int  i;
	BYTE	    IrqBit;
	
	fLineHigh = FALSE;
	
	// Reset AMCC
	outl(dwCtlRegister, pInterruptingDevice->dwIobase + PCI_IO_INTERRUPT_CONTROL);
	
	// Check the channels, reset if applicable
	for (i=0, IrqBit=1; i<MAX_BANKS; i++, IrqBit*=2) {
	  DWORD dwPhysAddr = pInterruptingDevice->dwPhysicalAddress[i];
	  if(dwPhysAddr == 0)
	    continue;
	  
	  if (readw((unsigned short int*)(pInterruptingDevice->dwMappedAddress[i]+PCI_MEM_INTERRUPT_STATUS)) & 1) {
	    fLineHigh=TRUE;
	    pInterruptingDevice->MchIntShadowRegister |= IrqBit;
	    pInterruptingDevice->MchInterruptCount[i]++;
	    pInterruptingDevice->dwTotalInterrupts++;
	    writeb(0xFF, (unsigned char *)(pInterruptingDevice->dwMappedAddress[i]+PCI_MEM_INTERRUPT_RESET));
	  }
	}
	
	// check if we are looping unreasonably
	// may happen if too few modules and no real interrupt is represented
	if (numLoops++ > 3) break;
	
      } while (fLineHigh);
    }
    // Rev C process interrupt
    else {
      DWORD	dwIntStatus;
      DWORD	mod;
      
      // Get status register
      dwIntStatus = readl((unsigned long *)(pInterruptingDevice->dwMappedAddress[EXC_PCIPX_CONFIG]+EXC_PCIPX_INTSTATUS));
      
      // Check each of the modules
      for (mod=0; mod<MAX_BANKS; mod++) {
	DWORD dwIntMask = MODULE_BIT(mod);
	DWORD dwPhysAddr = pInterruptingDevice->dwPhysicalAddress[mod];
	if(dwPhysAddr == 0 || mod == EXC_PCIPX_CONFIG)
	  continue;
	if ( dwIntStatus & dwIntMask) {
	  
	  // reset module
	  writeb(0xFF, (unsigned char*) (pInterruptingDevice->dwMappedAddress[mod]+PCI_MEM_INTERRUPT_RESET));
	  
	  // update counters
	  pInterruptingDevice->MchIntShadowRegister |= dwIntMask;
	  pInterruptingDevice->MchInterruptCount[mod]++;
	  pInterruptingDevice->dwTotalInterrupts++;

	} // end if this is an interrupting module
      } // end for each module
    } // end Rev C processing
  } // end PCI/Px cards

  if ((pInterruptingDevice->dwBoardType==EXC_BOARD_4000PCI) || (pInterruptingDevice->dwBoardType==EXC_BOARD_1394)) { //The logic of the 4000 and 1394 cards is similar

    WORD wIntStatus;
    WORD wFullMask;
    int  mod;
    
    // First check that this is our interrupt
    if(pInterruptingDevice->dwBoardType==EXC_BOARD_1394)
    {
      wIntStatus = readw((unsigned int *)(pInterruptingDevice->dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+EXC1394PCI_INTSTATUS));
      wFullMask = EXC1394PCI_INTMASK_ANYMOD;
    }
    else
    {
      wIntStatus = readw((unsigned int *)(pInterruptingDevice->dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+EXC4000PCI_INTSTATUS));
      wFullMask = pInterruptingDevice->f4000ExpandedMem ? EXC4000PCI_INTMASK_ANYMOD_EXPANDED : EXC4000PCI_INTMASK_ANYMOD;
    }
    if (CardUsesGlobalRegForDMAInterrupts(pInterruptingDevice->dwBoardSubType, pInterruptingDevice->wFPGARev) && (wIntStatus & EXCPCI_GlobalDMAIntMask))
    {
      // DMA interrupt has occured!
      pInterruptingDevice->dma_interrupt_count++;
      pInterruptingDevice->dwTotalInterrupts++;

      schedule_work(&pInterruptingDevice->work_dma);
      retval = IRQ_HANDLED;
    }
    else if ((wIntStatus & wFullMask) == 0) {

      // 2.6 and beyond - indicate that this is a spurious interrupt!
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	return;
#else
	pInterruptingDevice->false_interrupts++;
	return retval;
#endif
    }

    // Reset modules
    if(pInterruptingDevice->dwBoardType==EXC_BOARD_1394)
    {
      writew(wIntStatus, (unsigned short int*)(pInterruptingDevice->dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+EXC1394PCI_INTRESET));
    }
    else
    {
      writew(wIntStatus, (unsigned short int*)(pInterruptingDevice->dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+EXC4000PCI_INTRESET));
    }

    // Check which bits are set
    for (mod=0; mod < EXC4000PCI_MAX_INTERRUPT_BITS; mod++) {
      WORD wModuleMask = MODULE_BIT(mod);
      
      // if this bit is not part of our mask to begin with, ignore it
      if ((wFullMask & wModuleMask) == 0) continue;

      // 
      if (wIntStatus & wModuleMask) {
	pInterruptingDevice->MchIntShadowRegister |= wModuleMask;
	pInterruptingDevice->MchInterruptCount[mod]++;
	pInterruptingDevice->dwTotalInterrupts++;
	pInterruptingDevice->fModWakeup[mod] = TRUE;
      }
    }
  } // end 4000PCI card

  else if (pInterruptingDevice->dwBoardType==EXC_BOARD_429PCIMX) {

    BOOL        fLineHigh;
    int	        numLoops=0;
    DWORD	dwCtlRegister;
  
    // First check that this is our interrupt
    dwCtlRegister = inl(pInterruptingDevice->dwIobase + PCI_IO_INTERRUPT_CONTROL);
    if (!(dwCtlRegister & PCI_IO_INTCTL_INTMASK)) {
      // 2.6 and beyond - indicate that this is a spurious interrupt!
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	return;
#else
	return retval;
#endif
    }

    // Loop while the line is high
    do {
      
      unsigned int		i;
      BYTE	IrqBit;
      WORD	wStatus;

      fLineHigh = FALSE;
      
      // Reset AMCC
      outl(dwCtlRegister, pInterruptingDevice->dwIobase + PCI_IO_INTERRUPT_CONTROL);

      // Get status register
      wStatus = readw((unsigned short int *)(pInterruptingDevice->dwMappedAddress[0]+DAS429PCI_INTSTATUS));

      // Reset module(s)
      writew(wStatus, (unsigned short int*)(pInterruptingDevice->dwMappedAddress[0]+DAS429PCI_INTRESET));

      // Check which channels
      for (i=0, IrqBit=1; i<MAX_BANKS; i++, IrqBit*=2) {
	DWORD dwPhysAddr = pInterruptingDevice->dwPhysicalAddress[i];
	if(dwPhysAddr == 0)
	  continue;

	if (wStatus & IrqBit) {

	  fLineHigh=TRUE;
	  pInterruptingDevice->MchIntShadowRegister |= IrqBit;
	  pInterruptingDevice->MchInterruptCount[i]++;
	  pInterruptingDevice->dwTotalInterrupts++;
	}
      }
      
      // check if we are looping unreasonably
      // may happen if too few modules and no real interrupt is represented
      if (numLoops++ > 3) break;
      
    } while (fLineHigh);
  }
  else if (pInterruptingDevice->dwBoardType == EXC_BOARD_429MX) {
    WORD wInterruptingChannels = 0;
    WORD i;

    // get interrupting channels
    wInterruptingChannels = readw((unsigned short int*)(pInterruptingDevice->dwMappedAddress[0]+DAS429MX_INT_STATUS));

    // reset channels
    writew(wInterruptingChannels, (unsigned short int*)(pInterruptingDevice->dwMappedAddress[0]+DAS429MX_INT_RESET));

    pInterruptingDevice->MchIntShadowRegister |= wInterruptingChannels;

    for (i=0; i<8; i++) {
      if (wInterruptingChannels & MODULE_BIT(i)) {
	pInterruptingDevice->MchInterruptCount[i]++;
	pInterruptingDevice->dwTotalInterrupts++;
      }
    }
  }

  // wake up any processes waiting for IRQ
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  wake_up_interruptible(&(pInterruptingDevice->waitQueue));
#else
  wake_up(&(pInterruptingDevice->waitQueue));
  wake_up(&multi_device_waitQueue);
#endif

  // 2.6 and beyond - indicate that we handled the interrupt!
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	return IRQ_HANDLED;
#endif
  }

int mGetCardFromMinorNumberInode(struct inode *pInode) {

  int minor;

  // Prior to 2.6, the method of finding the minor number was via the i_rdev field
  // In kernel 2.6, these new macros encapsulate minor number extraction in prepration for the 
  // widening of dev_t.
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  minor = MINOR(pInode->i_rdev);
#else
  minor = (int) iminor(pInode);
#endif
   DebugMessage("EXC: mGetCardFromMinorNumberInode: minor = %d\n", minor);
   return mGetCardFromMinorNumberInt(minor);
}

int mGetCardFromMinorNumberInt(int minor) {
  int card;
  DebugMessage("EXC: mGetCardFromMinorNumberInt: minor = %d\n", minor);

  // given a minor number, scans existing devices for a matching socket
  // DebugMessage("EXC: Requested minor number %d\n", minor);

  // .. special case: minor number 25 takes the first available 4000 series card
  if (minor == EXC_4000PCI) {
    for (card=0; card<MAXBOARDS; card++) {
      if ((ExcDevice[card].fExists) && (ExcDevice[card].dwBoardType == EXC_BOARD_4000PCI)) {
	return card;
      }
    }
  }

  // .. special case: EXC_1553PCIPX define
  if (minor == EXC_1553PCIPX) {
    for (card=0; card<MAXBOARDS; card++) {
      if ((ExcDevice[card].fExists) && (ExcDevice[card].dwBoardType == EXC_BOARD_1553PCIPX)) {
	return card;
      }
    }    
  }

  // .. special case: 429PCIMX define
  if (minor == DAS_429PCI) {
    for (card=0; card<MAXBOARDS; card++) {
      if ((ExcDevice[card].fExists) && (ExcDevice[card].dwBoardType == EXC_BOARD_429PCIMX)) {
	return card;
      }
    }    
  }

  // .. normal case: find matching device
  for (card = 0; card < MAXBOARDS; card++) {
    if ((ExcDevice[card].fExists) && (ExcDevice[card].dwSocketNumber == minor)) {
      return card;
    }
  }

  // no matching device
  return -ENODEV;
}



ssize_t ExcRead(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
  EXCDEVICE* pdx = filp->private_data;
  ssize_t retv = count;
  DWORD dwCardOffset, dwRepeatCode;

#ifdef __LP64__
  DebugMessage("ExcRead: pdx = %p, buf = %p, count = %ld, pos = %d\n", pdx, buf, count, (int)*ppos);
#else
  DebugMessage("ExcRead: pdx = %p, buf = %p, count = %d, pos = %d\n", pdx, buf, count, (int)*ppos);
#endif
  // error condition: device does not support dma
  if (!pdx->fSupportsDMA) {
    DebugMessage("Error: requested DMA operation on non-DMA card");
    return -EINVAL;
  }

  if(count == 0){
    DebugMessage("ERROR: zero bytes specified\n");
    return -EINVAL;
  }

  if(count > EXC_DMA_BUFFER_SIZE){
#ifdef __LP64__
    DebugMessage("ERROR: cannot do DMA transfer greater than physical buffer length (req: %ld; phybufferlen: %d) \n", count, EXC_DMA_BUFFER_SIZE);
#else
    DebugMessage("ERROR: cannot do DMA transfer greater than physical buffer length (req: %d; phybufferlen: %d) \n", count, EXC_DMA_BUFFER_SIZE);
#endif
    return -EINVAL;
  }

  dwRepeatCode = (DWORD)(((*ppos) & 0xFFFFFFFF00000000LL) >> 32);
  dwCardOffset = (DWORD)((*ppos) & 0x00000000FFFFFFFF);
  if((dwRepeatCode == 0) && count + dwCardOffset > pdx->dwTotalDPRAMMemLength){
    DebugMessage("Error: length goes past valid boundaries");
    return -EINVAL;
  }

  QueueRequest(filp, buf, count, ppos, TRUE);
  *ppos += count;
  return retv;
}

ssize_t ExcWrite(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
  EXCDEVICE* pdx = filp->private_data;
  ssize_t retv = count;
  DWORD dwCardOffset, dwRepeatCode;

#ifdef __LP64__
  DebugMessage("ExcWrite: pdx = %p, buf = %p, count = %ld, pos = %d\n", pdx, buf, count, (int)*ppos);
#else
  DebugMessage("ExcWrite: pdx = %p, buf = %p, count = %d, pos = %d\n", pdx, buf, count, (int)*ppos);
#endif
  // error condition: device does not support dma
  if (!pdx->fSupportsDMA) {
    DebugMessage("Error: requested DMA operation on non-DMA card");
    return -EINVAL;
  }


  if(count == 0){
    DebugMessage("ERROR: zero bytes specified\n");
    return -EINVAL;
  }

  if(count > EXC_DMA_BUFFER_SIZE){
#ifdef __LP64__
    DebugMessage("ERROR: cannot do DMA transfer greater than physical buffer length (req: %ld; phybufferlen: %d) \n", count, EXC_DMA_BUFFER_SIZE);
#else
    DebugMessage("ERROR: cannot do DMA transfer greater than physical buffer length (req: %d; phybufferlen: %d) \n", count, EXC_DMA_BUFFER_SIZE);
#endif
    return -EINVAL;
  }

  dwRepeatCode = (DWORD)(((*ppos) & 0xFFFFFFFF00000000LL) >> 32);
  dwCardOffset = (DWORD)((*ppos) & 0x00000000FFFFFFFF);
  if((dwRepeatCode == 0) && count + dwCardOffset > pdx->dwTotalDPRAMMemLength){
    DebugMessage("Error: length goes past valid boundaries");
    return -EINVAL;
  }

  QueueRequest(filp, (char __user *)buf, count, ppos, FALSE);
  *ppos += count;
  return retv;
}

void StartIo(ExcDMAPacket* ppkt){
  EXCDEVICE* pdx = ppkt->filp->private_data;
  unsigned char *pRing0Buffer = (unsigned char*)pdx->cpu_addr;
  unsigned int count = ppkt->count;
  int i;
  DWORD dwCardOffset, dwRepeatCode;
  DWORD DMARegsBase;
  DWORD a2p_mask, ret, irq_mask; 
  if (CardUsesGlobalRegForAllDMARegs(pdx->dwBoardSubType, pdx->wFPGARev))
    DMARegsBase = pdx->dwMappedAddress[EXC4000PCI_BANK_GLOBAL] + 0x1000;
  else if (pdx->dwBoardSubType == EXC_BOARD_807)
    DMARegsBase = pdx->dwMappedAddress[EXCPCI_BANK_DMA] + 0x4000;
  else
    DMARegsBase = pdx->dwMappedAddress[EXCPCI_BANK_DMA];
  pdx->startio_calls++;

  DebugMessage("EXC: DMARegsBase = %lx", DMARegsBase);
  if(!ppkt->read){
    for(i = 0; i < count; i++){
      pRing0Buffer[i] = ppkt->internal_buf[i];
    }
  }

  // Do the DMA!


  DebugMessage("EXC: StartIo: About to start DMA!\n");
  if (pdx->dwBoardSubType == EXC_BOARD_807) {
    //The following code was copied from alt_pcie_qsys_simple_sw/altpcie_demo.cpp (including cryptic comments): 
    //
    // trying and check the address translation path through
    writel(0xFFFFFFFC, (u32*)(pdx->dwMappedAddress[EXCPCI_BANK_DMA]+0x1000)); 
    //0x1000 is the offset for translation register
    //The next line is an attempt to deal with endless interrupts when the DMA finishes:
    writel(0, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CONTROL_BITS));
    // reading out the resulted mask of path through
    a2p_mask = readl((u32*)(pdx->dwMappedAddress[EXCPCI_BANK_DMA]+0x1000));
    // program address translation table
    // PCIe core limits the data length to be 1MByte, so it only needs 20bits of address.
    writel(pdx->dma_handle & a2p_mask, (u32*)(pdx->dwMappedAddress[EXCPCI_BANK_DMA]+0x1000)); 
    writel(0, (u32*)(pdx->dwMappedAddress[EXCPCI_BANK_DMA]+0x1004));  // setting upper address   limited at hardIP for now.
    // clear the DMA controller
    ret = readl((u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
    writel(0x200, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));  
    ret = readl((u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
    writel(0x2, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL2));   // issue reset dispatcher
    writel(0x0, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));  //clear all the status
    writel(0x10, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL2));  // set IRQ enable
    ret = readl((u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
    //enable_global_interrupt_mask (alt_u32 csr_base)
    irq_mask = readl((u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL2));
    irq_mask |= 0x10;
    //writel(irq_mask, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL2));  //setting the IRQ enable flag at here
    ret = readl((u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
    writel(0x0, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));  //clear all the status
    ret = readl((u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CTRL));
  }
  // put info into the card to start the transfer
  // .. the address of our physbuffer
  if (ppkt->read) {
    if (pdx->dwBoardSubType == EXC_BOARD_807) {
      writel(pdx->dma_handle & ~a2p_mask, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_WRITE_ADDR));
    }
    else {
      writel(pdx->dma_handle, (ULONG*)(DMARegsBase+EXCPCI_DMA1AddrLo));
      writel(0L, (ULONG*)(DMARegsBase+EXCPCI_DMA1AddrHi));
    }
  }
  else {
    if (pdx->dwBoardSubType == EXC_BOARD_807) {
      writel(pdx->dma_handle & ~a2p_mask, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_READ_ADDR));
    }
    else {
      writel(pdx->dma_handle, (ULONG*)(DMARegsBase+EXCPCI_DMA0AddrLo));
      writel(0L, (ULONG*)(DMARegsBase+EXCPCI_DMA0AddrHi));
    }
  }

  dwCardOffset = (DWORD)((*ppkt->ppos) & 0x00000000FFFFFFFF);
  dwRepeatCode = (DWORD)(((*ppkt->ppos) & 0xFFFFFFFF00000000LL) >> 32);

  /*
   * It is important that we write this 'repeat code' (for serial and 708 modules to do DMA from/to a FIFO) before we write the
   * base address of the DMA.  This is because in the hardware which does not support this kind of DMA, writing to the (non-existent) 
   * repeat code register has the bad side effect of writing to the address register itself.  If we do it before writing the address,
   * it just gets overwritten with the right address.
   */

  // .. repeat code 
  // TODO: 807 card
  if (pdx->dwBoardSubType != EXC_BOARD_807){
    if (ppkt->read) {
      writel(dwRepeatCode, (ULONG*)(DMARegsBase+EXCPCI_DMA1RepeatCode));
    }
    else {
      writel(dwRepeatCode, (ULONG*)(DMARegsBase+EXCPCI_DMA0RepeatCode));
    }
  }

  if(pdx->dwBoardSubType == EXC_BOARD_AFDX) {
    if ((*ppkt->ppos) + (ppkt->count) >= EXCAFDXPCI_TOTAL_MEM) {
      DebugMessage("EXC: ERROR: length goes past valid boundaries:");
      return;
    }
    if(*ppkt->ppos >= 2 * EXCAFDXPCI_DPRBANK_LENGTH) {
      *ppkt->ppos -= 2 * EXCAFDXPCI_DPRBANK_LENGTH;
      writel((DWORD)EXCAFDXPCI_PHYSBANK_DPR_4000MODS_BARNUM, (ULONG*)(DMARegsBase+EXCAFDXPCI_DMABARNum));
      *ppkt->ppos += EXC4000PCI_BANK4_OFFSET; //This gives the right place in the BAR corresponding to the M4K modules
    }
    else {
      writel((DWORD)EXCAFDXPCI_PHYSBANK_DPR_AFDX_BARNUM, (ULONG*)(DMARegsBase+EXCAFDXPCI_DMABARNum));
    }
  }

  // .. the base address
  if (ppkt->read) {
    // NOTE: DMA1 base address register is not yet implemented; for now we write to DMA 0
    //writel(*ppkt->ppos, (ULONG*)(DMARegsBase+EXCPCI_DMA1CardAddr));
    if (pdx->dwBoardSubType == EXC_BOARD_807){
      writel(*ppkt->ppos + PHYSICAL_START_OF_MODULES, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_READ_ADDR));
    }
    else if (CardUsesGlobalRegForAllDMARegs(pdx->dwBoardSubType, pdx->wFPGARev))
      writel(*ppkt->ppos, (ULONG*)(DMARegsBase+EXCPCI_DMAGlobalBankCardAddr));
    else
      writel(*ppkt->ppos, (ULONG*)(DMARegsBase+EXCPCI_DMA0CardAddr));
  }
  else {
    if (pdx->dwBoardSubType == EXC_BOARD_807){
      writel(*ppkt->ppos + PHYSICAL_START_OF_MODULES, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_WRITE_ADDR));
    }
    else if (CardUsesGlobalRegForAllDMARegs(pdx->dwBoardSubType, pdx->wFPGARev))
      writel(*ppkt->ppos, (ULONG*)(DMARegsBase+EXCPCI_DMAGlobalBankCardAddr));
    else
      writel(*ppkt->ppos, (ULONG*)(DMARegsBase+EXCPCI_DMA0CardAddr));
  }

  // .. length 
  if (pdx->dwBoardSubType == EXC_BOARD_807){
    writel(ppkt->count, (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_LENGTH));
  }
  else if (ppkt->read) {
    writel(ppkt->count, (ULONG*)(DMARegsBase+EXCPCI_DMA1Length));
  }
  else {
    writel(ppkt->count, (ULONG*)(DMARegsBase+EXCPCI_DMA0Length));
  }

  // .. write the byte to start the transfer!
  if (ppkt->read) {
    if (pdx->dwBoardSubType == EXC_BOARD_807)
    {
      //DebugMessage("Writing 0x%x to 0x%llx to start DMA transfer", (1 << 14) | (1<<31), (ULONG)(u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CONTROL_BITS));
      writel((1 << 14) | (1<<31), (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CONTROL_BITS));
    }
    else
      writel(EXCPCI_DMA1StartMask, (ULONG*)(DMARegsBase+EXCPCI_DMA1StartAddr));
  }
  else {
    if (pdx->dwBoardSubType == EXC_BOARD_807)
      writel((1 << 14) | (1 << 24) | (1<<31), (u32*)(DMARegsBase+EXCPCI_ALTERA_DMA_CONTROL_BITS));
    else
      writel(EXCPCI_DMA0StartMask, (ULONG*)(DMARegsBase+EXCPCI_DMA0StartAddr));
  }
  //We are using polling on the new card since interrupts have an unidentified HW problem:
  if (pdx->dwBoardSubType == EXC_BOARD_807){
    // .. wait for the end of transfer
    for (i=0; i<100; i++) {
      WORD dwIntStatus;
      msleep(1); 
      dwIntStatus  = readw((WORD*)(pdx->dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+EXCPCI_807DMAIntStatus));
      if (dwIntStatus != 0) {
	writew(1, (WORD*)(pdx->dwMappedAddress[EXC4000PCI_BANK_GLOBAL]+EXCPCI_807DMAIntStatus)); 
	DebugMessage("Looped %d times and transfer did finish!", i);
	break;
      }
    }
    if (i==100) {
      DebugMessage("ERROR! looped 100 times but transfer did not finish!");
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
    EndIo(pdx);
#else /* LINUX2_6_27 */
    EndIo(&pdx->work_dma);
#endif /* LINUX2_6_27 */
  }
}

//TODO: This function really needs to return a value so that the caller can return the number of bytes read or written and increment *ppos
void QueueRequest(struct file *filp, char __user *buf, size_t count, loff_t *ppos, BOOL read){
  ExcDMAPacket* ppkt = (ExcDMAPacket*)kmalloc(sizeof(ExcDMAPacket), GFP_KERNEL);
  EXCDEVICE* pdx = filp->private_data;
  BOOL queue_empty = TRUE;
  DEFINE_WAIT(dma_wait);
  DebugMessage("EXC: QueueRequest\n");
  ppkt->filp = filp;
  ppkt->buf = buf;
  ppkt->count = count;
  ppkt->ppos = ppos;
  ppkt->read = read;
  ppkt->f_completed = FALSE;

  if(!read){
    //Copy data from user buffer
    if(copy_from_user(ppkt->internal_buf, buf, count)){
      DebugMessage("EXC: Error reading input from user\n");
    }
  }

  spin_lock(&pdx->dma_queue_lock);
  queue_empty = list_empty(&pdx->dma_queue);
  list_add_tail(&ppkt->list, &pdx->dma_queue);
  pdx->dma_queue_size++;
  if(pdx->dma_queue_size > pdx->max_dma_queue_size)
    pdx->max_dma_queue_size = pdx->dma_queue_size;
  spin_unlock(&pdx->dma_queue_lock);
  if(queue_empty){
    DebugMessage("EXC: QueueRequest about to call StartIo because queue was empty\n");
    StartIo(ppkt);
  }

  //spin_lock(&pdx->dma_waitqueue_lock);
  while(ppkt->f_completed == FALSE){
    prepare_to_wait(&(pdx->dma_waitQueue), &(dma_wait), TASK_INTERRUPTIBLE);
    schedule();
    finish_wait(&(pdx->dma_waitQueue), &(dma_wait));
  }
  //spin_unlock(&pdx->dma_waitqueue_lock);
  DebugMessage("EXC: returned from wait in QueueRequest\n");
  DebugMessage("EXC: Completing QueueRequest\n");
  pdx->dma_count++;
  if(read){
    //Copy data to user buffer
    if(copy_to_user(buf, (const void *)ppkt->internal_buf, count)){
      DebugMessage("EXC: Error writing input to user\n");
    }
  }
  kfree(ppkt);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
void EndIo(void *data){
  EXCDEVICE * pdx = (EXCDEVICE*)data;
#else /* LINUX2_6_27 */
void EndIo(struct work_struct *pws){
  //struct apanel *ap = container_of(work, struct apanel, led_work);
  EXCDEVICE * pdx = container_of(pws, EXCDEVICE, work_dma);
#endif /* LINUX2_6_27 */
  ExcDMAPacket* ppkt;
  BOOL queue_empty = TRUE;
  unsigned char *pRing0Buffer;
  unsigned int count;
  int i;
  pdx->endio_calls++;

  DebugMessage("EXC: EndIo\n");
  //spin_lock(&pdx->dma_queue_lock); //We need the lock even before we delete the entry to insure that QueueRequest has had time to add its new packet to the queue.
  if (list_empty_careful(&pdx->dma_queue)) //We seem to get called on initialization when no packets are in the queue
    return;
  ppkt = list_entry(pdx->dma_queue.next, ExcDMAPacket, list);
  pRing0Buffer = (unsigned char*)pdx->cpu_addr;
  count = ppkt->count;

  if(ppkt->read){
    for(i = 0; i < count; i++){
      ppkt->internal_buf[i] = pRing0Buffer[i];
    }
  }
  spin_lock(&pdx->dma_queue_lock);
  list_del(&ppkt->list);
  pdx->dma_queue_size--;
  ppkt->f_completed = TRUE;
  spin_unlock(&pdx->dma_queue_lock);
  queue_empty = list_empty(&pdx->dma_queue);
  if(!queue_empty){
    ppkt = list_entry(pdx->dma_queue.next, ExcDMAPacket, list);
    StartIo(ppkt);
  }
  //TODO: Make sure this wakes up only the first process that was put to sleep (see code)
  wake_up(&pdx->dma_waitQueue);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
static int proc_excalbr_show(struct seq_file *m, void *v)
{
	int card, bank, len = 0;
	int limit = 0x1000; /* Don't print more than this */
	unsigned short tempword[2];
	int time_tag;
	seq_printf(m,"Excalbur Systems: Kernel Driver Version: %s\n", EXC_KD_VERSION);
	seq_printf(m,"Excalbur Systems: Major number for our devices: %ld\n", dwExcMajor);
	seq_printf(m,"ExcDevice array at 0x%p\n", ExcDevice);
	for (card=0; card<MAXBOARDS; card++) {
	  if (ExcDevice[card].fExists){
	    seq_printf(m,"ExcDevice[%d] at  %p\n", card, &ExcDevice[card]);
	    seq_printf(m,"ExcDevice[%d] card type = %ld\n", card, ExcDevice[card].dwBoardType);
	    seq_printf(m,"ExcDevice[%d] card subtype = %ld\n", card, ExcDevice[card].dwBoardSubType);
	    seq_printf(m,"ExcDevice[%d] Unique ID = %ld\n", card, ExcDevice[card].dwSocketNumber);
	    seq_printf(m,"ExcDevice[%d] dwTotalDPRAMMemLength = 0x%lx\n", card, ExcDevice[card].dwTotalDPRAMMemLength);
	    seq_printf(m,"ExcDevice[%d] %s DMA\n", card, ExcDevice[card].fSupportsDMA ? "supports" : "does not support");
	    seq_printf(m,"ExcDevice[%d] dma_count = %u\n", card, ExcDevice[card].dma_count);
	    tempword[0] = *((unsigned short*)(ExcDevice[card].dwMappedAddress[0] + 0x7008));
	    /* read ttagLo twice, to help verify that we freeze it for reading */
	    tempword[0] = *((unsigned short*)(ExcDevice[card].dwMappedAddress[0] + 0x7008));
	    tempword[1] = *((unsigned short*)(ExcDevice[card].dwMappedAddress[0] + 0x700a));
	    time_tag = ((unsigned int)tempword[1]<<16) + tempword[0];
	    seq_printf(m,"ExcDevice[%d] current timetag = 0x%x\n", card, time_tag);
	    seq_printf(m,"ExcDevice[%d] endio_calls = %d\n", card, ExcDevice[card].endio_calls);
	    seq_printf(m,"ExcDevice[%d] startio_calls = %d\n", card, ExcDevice[card].startio_calls);
	    seq_printf(m,"ExcDevice[%d] dma_interrupt_count = %d\n", card, ExcDevice[card].dma_interrupt_count);
	    seq_printf(m,"ExcDevice[%d] isr_calls = %d\n", card, ExcDevice[card].isr_calls);
	    seq_printf(m,"ExcDevice[%d] dwTotalInterrupts = %u\n", card, ExcDevice[card].dwTotalInterrupts);
	    seq_printf(m,"ExcDevice[%d] dma_queue_size = %u\n", card, ExcDevice[card].dma_queue_size);
	    seq_printf(m,"ExcDevice[%d] max_dma_queue_size = %u\n", card, ExcDevice[card].max_dma_queue_size);
	    for (bank=0; bank<MAX_BANKS; bank++){
	      if(ExcDevice[card].dwPhysicalAddress[bank] != 0){
		seq_printf(m,"ExcDevice[%d], bank %d dwPhysicalAddress = 0x%lx\n", card, bank, ExcDevice[card].dwPhysicalAddress[bank]);
		seq_printf(m,"ExcDevice[%d], bank %d dwMappedAddress = 0x%lx\n", card, bank, ExcDevice[card].dwMappedAddress[bank]);
		seq_printf(m,"ExcDevice[%d], bank %d dwMemLength = 0x%lx\n", card, bank, ExcDevice[card].dwMemLength[bank]);
	      }
	    }
	  } 
	  if(len > limit)
	    break;
	}
	return len;
}
#else
static int proc_excalbr_show(struct seq_file *m, void *v)
{
	int card, bank, len = 0;
	int limit = 0x1000; /* Don't print more than this */
	unsigned short tempword[2];
	int time_tag;
	len += seq_printf(m,"Excalbur Systems: Major number for our devices: %ld\n", dwExcMajor);
	len += seq_printf(m,"ExcDevice array at 0x%p\n", ExcDevice);
	for (card=0; card<MAXBOARDS; card++) {
	  if (ExcDevice[card].fExists){
	    len += seq_printf(m,"ExcDevice[%d] at  %p\n", card, &ExcDevice[card]);
	    len += seq_printf(m,"ExcDevice[%d] card type = %ld\n", card, ExcDevice[card].dwBoardType);
	    len += seq_printf(m,"ExcDevice[%d] card subtype = %ld\n", card, ExcDevice[card].dwBoardSubType);
	    len += seq_printf(m,"ExcDevice[%d] Unique ID = %ld\n", card, ExcDevice[card].dwSocketNumber);
	    len += seq_printf(m,"ExcDevice[%d] dwTotalDPRAMMemLength = 0x%lx\n", card, ExcDevice[card].dwTotalDPRAMMemLength);
	    len += seq_printf(m,"ExcDevice[%d] %s DMA\n", card, ExcDevice[card].fSupportsDMA ? "supports" : "does not support");
	    len += seq_printf(m,"ExcDevice[%d] dma_count = %u\n", card, ExcDevice[card].dma_count);
	    tempword[0] = *((unsigned short*)(ExcDevice[card].dwMappedAddress[0] + 0x7008));
	    /* read ttagLo twice, to help verify that we freeze it for reading */
	    tempword[0] = *((unsigned short*)(ExcDevice[card].dwMappedAddress[0] + 0x7008));
	    tempword[1] = *((unsigned short*)(ExcDevice[card].dwMappedAddress[0] + 0x700a));
	    time_tag = ((unsigned int)tempword[1]<<16) + tempword[0];
	    len += seq_printf(m,"ExcDevice[%d] current timetag = 0x%x\n", card, time_tag);
	    len += seq_printf(m,"ExcDevice[%d] endio_calls = %d\n", card, ExcDevice[card].endio_calls);
	    len += seq_printf(m,"ExcDevice[%d] startio_calls = %d\n", card, ExcDevice[card].startio_calls);
	    len += seq_printf(m,"ExcDevice[%d] dma_interrupt_count = %d\n", card, ExcDevice[card].dma_interrupt_count);
	    len += seq_printf(m,"ExcDevice[%d] isr_calls = %d\n", card, ExcDevice[card].isr_calls);
	    len += seq_printf(m,"ExcDevice[%d] dwTotalInterrupts = %u\n", card, ExcDevice[card].dwTotalInterrupts);
	    len += seq_printf(m,"ExcDevice[%d] dma_queue_size = %u\n", card, ExcDevice[card].dma_queue_size);
	    len += seq_printf(m,"ExcDevice[%d] max_dma_queue_size = %u\n", card, ExcDevice[card].max_dma_queue_size);
	    for (bank=0; bank<MAX_BANKS; bank++){
	      if(ExcDevice[card].dwPhysicalAddress[bank] != 0){
		len += seq_printf(m,"ExcDevice[%d], bank %d dwPhysicalAddress = 0x%lx\n", card, bank, ExcDevice[card].dwPhysicalAddress[bank]);
		len += seq_printf(m,"ExcDevice[%d], bank %d dwMappedAddress = 0x%lx\n", card, bank, ExcDevice[card].dwMappedAddress[bank]);
		len += seq_printf(m,"ExcDevice[%d], bank %d dwMemLength = 0x%lx\n", card, bank, ExcDevice[card].dwMemLength[bank]);
	      }
	    }
	  } 
	  if(len > limit)
	    break;
	}
	return len;
}
#endif

static int proc_excalbr_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_excalbr_show, NULL);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,5,0)
static const struct proc_ops proc_excalbr_fops = {
	.proc_open		= proc_excalbr_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};
#else
static const struct file_operations proc_excalbr_fops = {
	.owner		= THIS_MODULE,
	.open		= proc_excalbr_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif
#endif
static void exc_create_proc(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
	struct proc_dir_entry *pde;

	pde = proc_create("excalbr", 0, NULL, &proc_excalbr_fops);
	if (!pde) {
	  //TODO: error handling
	}
#else
	create_proc_read_entry("excalbr", 0 /* default mode */,
			NULL /* parent dir */, exc_read_procmem,
			NULL /* client data */);
#endif
}

int exc_read_procmem(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int card, bank, len = 0;
	int limit = count - 80; /* Don't print more than this */
	unsigned short tempword[2];
	int time_tag;
	len += sprintf(buf+len,"Excalbur Systems: Major number for our devices: %ld\n", dwExcMajor);
	len += sprintf(buf+len,"ExcDevice array at 0x%p\n", ExcDevice);
	for (card=0; card<MAXBOARDS; card++) {
	  if (ExcDevice[card].fExists){
	    len += sprintf(buf+len,"ExcDevice[%d] at  %p\n", card, &ExcDevice[card]);
	    len += sprintf(buf+len,"ExcDevice[%d] card type = %ld\n", card, ExcDevice[card].dwBoardType);
	    len += sprintf(buf+len,"ExcDevice[%d] card subtype = %ld\n", card, ExcDevice[card].dwBoardSubType);
	    len += sprintf(buf+len,"ExcDevice[%d] Unique ID = %ld\n", card, ExcDevice[card].dwSocketNumber);
	    len += sprintf(buf+len,"ExcDevice[%d] dwTotalDPRAMMemLength = 0x%lx\n", card, ExcDevice[card].dwTotalDPRAMMemLength);
	    len += sprintf(buf+len,"ExcDevice[%d] %s DMA\n", card, ExcDevice[card].fSupportsDMA ? "supports" : "does not support");
	    len += sprintf(buf+len,"ExcDevice[%d] dma_count = %u\n", card, ExcDevice[card].dma_count);
	    tempword[0] = *((unsigned short*)(ExcDevice[card].dwMappedAddress[0] + 0x7008));
	    /* read ttagLo twice, to help verify that we freeze it for reading */
	    tempword[0] = *((unsigned short*)(ExcDevice[card].dwMappedAddress[0] + 0x7008));
	    tempword[1] = *((unsigned short*)(ExcDevice[card].dwMappedAddress[0] + 0x700a));
	    time_tag = ((unsigned int)tempword[1]<<16) + tempword[0];
	    len += sprintf(buf+len,"ExcDevice[%d] current timetag = 0x%x\n", card, time_tag);
	    len += sprintf(buf+len,"ExcDevice[%d] endio_calls = %d\n", card, ExcDevice[card].endio_calls);
	    len += sprintf(buf+len,"ExcDevice[%d] startio_calls = %d\n", card, ExcDevice[card].startio_calls);
	    len += sprintf(buf+len,"ExcDevice[%d] dma_interrupt_count = %d\n", card, ExcDevice[card].dma_interrupt_count);
	    len += sprintf(buf+len,"ExcDevice[%d] isr_calls = %d\n", card, ExcDevice[card].isr_calls);
	    len += sprintf(buf+len,"ExcDevice[%d] dwTotalInterrupts = %u\n", card, ExcDevice[card].dwTotalInterrupts);
	    len += sprintf(buf+len,"ExcDevice[%d] dma_queue_size = %u\n", card, ExcDevice[card].dma_queue_size);
	    len += sprintf(buf+len,"ExcDevice[%d] max_dma_queue_size = %u\n", card, ExcDevice[card].max_dma_queue_size);
	    for (bank=0; bank<MAX_BANKS; bank++){
	      if(ExcDevice[card].dwPhysicalAddress[bank] != 0){
		len += sprintf(buf+len,"ExcDevice[%d], bank %d dwPhysicalAddress = 0x%lx\n", card, bank, ExcDevice[card].dwPhysicalAddress[bank]);
		len += sprintf(buf+len,"ExcDevice[%d], bank %d dwMappedAddress = 0x%lx\n", card, bank, ExcDevice[card].dwMappedAddress[bank]);
		len += sprintf(buf+len,"ExcDevice[%d], bank %d dwMemLength = 0x%lx\n", card, bank, ExcDevice[card].dwMemLength[bank]);
	      }
	    }
	  } 
	  if(len > limit)
	    break;
	}
	*eof = 1;
	return len;
}


BOOL CardUsesGlobalRegForDMAInterrupts(DWORD dwBoardSubType, WORD wFPGARev)
{
	//DebugMessage("EXC: CardUsesGlobalRegForDMAInterrupts about to return %d\n", ((dwBoardType == EXC_BOARD_AFDX) && (wFPGARev != EXC_FPGAREV_NONE) && (wFPGARev >= 0x10)));
	return ((dwBoardSubType == EXC_BOARD_AFDX) && (wFPGARev != EXC_FPGAREV_NONE) && (wFPGARev >= 0x10));
}

BOOL CardUsesGlobalRegForAllDMARegs(DWORD dwBoardSubType, WORD wFPGARev)
{
	//DebugMessage("EXC: CardUsesGlobalRegForAllDMARegs about to return %d, wFPGARev = 0x%x\n", ((dwBoardSubType == EXC_BOARD_4000PCIE) && (wFPGARev != EXC_FPGAREV_NONE) && (wFPGARev >= 0x50)), wFPGARev);
	return ((dwBoardSubType == EXC_BOARD_4000ETH) || ((dwBoardSubType == EXC_BOARD_4000PCIE) && ((wFPGARev != EXC_FPGAREV_NONE) && (wFPGARev >= 0x50))));
}
