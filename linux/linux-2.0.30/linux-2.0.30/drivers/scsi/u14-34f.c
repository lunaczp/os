/*
 *      u14-34f.c - Low-level driver for UltraStor 14F/34F SCSI host adapters.
 *
 *      24 Feb 1997 rev. 3.00 for linux 2.0.29 and 2.1.26
 *          When loading as a module, parameter passing is now supported
 *          both in 2.0 and in 2.1 style.
 *          Fixed data transfer direction for some SCSI opcodes.
 *          Immediate acknowledge to request sense commands.
 *          Linked commands to each disk device are now reordered by elevator
 *          sorting. Rare cases in which reordering of write requests could 
 *          cause wrong results are managed.
 *
 *      18 Jan 1997 rev. 2.60 for linux 2.1.21 and 2.0.28
 *          Added command line options to enable/disable linked commands
 *          (lc:[y|n]), old firmware support (of:[y|n]) and to set the max
 *          queue depth (mq:xx). Default is "u14-34f=lc:n,of:n,mq:8".
 *          Improved command linking.
 *
 *       8 Jan 1997 rev. 2.50 for linux 2.1.20 and 2.0.27
 *          Added linked command support.
 *
 *       3 Dec 1996 rev. 2.40 for linux 2.1.14 and 2.0.27
 *          Added queue depth adjustment.
 *
 *      22 Nov 1996 rev. 2.30 for linux 2.1.12 and 2.0.26
 *          The list of i/o ports to be probed can be overwritten by the
 *          "u14-34f=port0,port1,...." boot command line option.
 *          Scatter/gather lists are now allocated by a number of kmalloc
 *          calls, in order to avoid the previous size limit of 64Kb.
 *
 *      16 Nov 1996 rev. 2.20 for linux 2.1.10 and 2.0.25
 *          Added multichannel support.
 *
 *      27 Sep 1996 rev. 2.12 for linux 2.1.0
 *          Portability cleanups (virtual/bus addressing, little/big endian
 *          support).
 *
 *      09 Jul 1996 rev. 2.11 for linux 2.0.4
 *          "Data over/under-run" no longer implies a redo on all targets.
 *          Number of internal retries is now limited.
 *
 *      16 Apr 1996 rev. 2.10 for linux 1.3.90
 *          New argument "reset_flags" to the reset routine.
 *
 *      21 Jul 1995 rev. 2.02 for linux 1.3.11
 *          Fixed Data Transfer Direction for some SCSI commands.
 *
 *      13 Jun 1995 rev. 2.01 for linux 1.2.10
 *          HAVE_OLD_UX4F_FIRMWARE should be defined for U34F boards when
 *          the firmware prom is not the latest one (28008-006).
 *
 *      11 Mar 1995 rev. 2.00 for linux 1.2.0
 *          Fixed a bug which prevented media change detection for removable
 *          disk drives.
 *
 *      23 Feb 1995 rev. 1.18 for linux 1.1.94
 *          Added a check for scsi_register returning NULL.
 *
 *      11 Feb 1995 rev. 1.17 for linux 1.1.91
 *          U14F qualified to run with 32 sglists.
 *          Now DEBUG_RESET is disabled by default.
 *
 *       9 Feb 1995 rev. 1.16 for linux 1.1.90
 *          Use host->wish_block instead of host->block.
 *
 *       8 Feb 1995 rev. 1.15 for linux 1.1.89
 *          Cleared target_time_out counter while performing a reset.
 *
 *      28 Jan 1995 rev. 1.14 for linux 1.1.86
 *          Added module support.
 *          Log and do a retry when a disk drive returns a target status
 *          different from zero on a recovered error.
 *          Auto detects if U14F boards have an old firmware revision.
 *          Max number of scatter/gather lists set to 16 for all boards
 *          (most installation run fine using 33 sglists, while other
 *          has problems when using more then 16).
 *
 *      16 Jan 1995 rev. 1.13 for linux 1.1.81
 *          Display a message if check_region detects a port address
 *          already in use.
 *
 *      15 Dec 1994 rev. 1.12 for linux 1.1.74
 *          The host->block flag is set for all the detected ISA boards.
 *
 *      30 Nov 1994 rev. 1.11 for linux 1.1.68
 *          Redo i/o on target status CHECK_CONDITION for TYPE_DISK only.
 *          Added optional support for using a single board at a time.
 *
 *      14 Nov 1994 rev. 1.10 for linux 1.1.63
 *
 *      28 Oct 1994 rev. 1.09 for linux 1.1.58  Final BETA release.
 *      16 Jul 1994 rev. 1.00 for linux 1.1.29  Initial ALPHA release.
 *
 *          This driver is a total replacement of the original UltraStor 
 *          scsi driver, but it supports ONLY the 14F and 34F boards.
 *          It can be configured in the same kernel in which the original
 *          ultrastor driver is configured to allow the original U24F
 *          support.
 * 
 *          Multiple U14F and/or U34F host adapters are supported.
 *
 *  Copyright (C) 1994-1997 Dario Ballabio (dario@milano.europe.dg.com)
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that redistributions of source
 *  code retain the above copyright notice and this comment without
 *  modification.
 *
 *      WARNING: if your 14/34F board has an old firmware revision (see below)
 *               you must change "#undef" into "#define" in the following
 *               statement.
 */
#undef HAVE_OLD_UX4F_FIRMWARE
/*
 *  The UltraStor 14F, 24F, and 34F are a family of intelligent, high
 *  performance SCSI-2 host adapters.
 *  Here is the scoop on the various models:
 *
 *  14F - ISA first-party DMA HA with floppy support and WD1003 emulation.
 *  24F - EISA Bus Master HA with floppy support and WD1003 emulation.
 *  34F - VESA Local-Bus Bus Master HA (no WD1003 emulation).
 *
 *  This code has been tested with up to two U14F boards, using both 
 *  firmware 28004-005/38004-004 (BIOS rev. 2.00) and the latest firmware
 *  28004-006/38004-005 (BIOS rev. 2.01). 
 *
 *  The latest firmware is required in order to get reliable operations when 
 *  clustering is enabled. ENABLE_CLUSTERING provides a performance increase
 *  up to 50% on sequential access.
 *
 *  Since the Scsi_Host_Template structure is shared among all 14F and 34F,
 *  the last setting of use_clustering is in effect for all of these boards.
 *
 *  Here a sample configuration using two U14F boards:
 *
 U14F0: ISA 0x330, BIOS 0xc8000, IRQ 11, DMA 5, SG 32, MB 16, of:n, lc:y, mq:8.
 U14F1: ISA 0x340, BIOS 0x00000, IRQ 10, DMA 6, SG 32, MB 16, of:n, lc:y, mq:8.
 *
 *  The boot controller must have its BIOS enabled, while other boards can
 *  have their BIOS disabled, or enabled to an higher address.
 *  Boards are named Ux4F0, Ux4F1..., according to the port address order in
 *  the io_port[] array.
 *  
 *  The following facts are based on real testing results (not on
 *  documentation) on the above U14F board.
 *  
 *  - The U14F board should be jumpered for bus on time less or equal to 7 
 *    microseconds, while the default is 11 microseconds. This is order to 
 *    get acceptable performance while using floppy drive and hard disk 
 *    together. The jumpering for 7 microseconds is: JP13 pin 15-16, 
 *    JP14 pin 7-8 and pin 9-10.
 *    The reduction has a little impact on scsi performance.
 *  
 *  - If scsi bus length exceeds 3m., the scsi bus speed needs to be reduced
 *    from 10Mhz to 5Mhz (do this by inserting a jumper on JP13 pin 7-8).
 *
 *  - If U14F on board firmware is older than 28004-006/38004-005,
 *    the U14F board is unable to provide reliable operations if the scsi 
 *    request length exceeds 16Kbyte. When this length is exceeded the
 *    behavior is: 
 *    - adapter_status equal 0x96 or 0xa3 or 0x93 or 0x94;
 *    - adapter_status equal 0 and target_status equal 2 on for all targets
 *      in the next operation following the reset.
 *    This sequence takes a long time (>3 seconds), so in the meantime
 *    the SD_TIMEOUT in sd.c could expire giving rise to scsi aborts
 *    (SD_TIMEOUT has been increased from 3 to 6 seconds in 1.1.31).
 *    Because of this I had to DISABLE_CLUSTERING and to work around the
 *    bus reset in the interrupt service routine, returning DID_BUS_BUSY
 *    so that the operations are retried without complains from the scsi.c
 *    code.
 *    Any reset of the scsi bus is going to kill tape operations, since
 *    no retry is allowed for tapes. Bus resets are more likely when the
 *    scsi bus is under heavy load.
 *    Requests using scatter/gather have a maximum length of 16 x 1024 bytes 
 *    when DISABLE_CLUSTERING is in effect, but unscattered requests could be
 *    larger than 16Kbyte.
 *
 *    The new firmware has fixed all the above problems.
 *
 *  For U34F boards the latest bios prom is 38008-002 (BIOS rev. 2.01),
 *  the latest firmware prom is 28008-006. Older firmware 28008-005 has
 *  problems when using more then 16 scatter/gather lists.
 *
 *  The list of i/o ports to be probed can be totally replaced by the
 *  boot command line option: "u14-34f=port0,port1,port2,...", where the
 *  port0, port1... arguments are ISA/VESA addresses to be probed.
 *  For example using "u14-34f=0x230,0x340", the driver probes only the two
 *  addresses 0x230 and 0x340 in this order; "u14-34f=0" totally disables
 *  this driver.
 *
 *  After the optional list of detection probes, other possible command line
 *  options are:
 *
 *  lc:y  enables linked commands;
 *  lc:n  disables linked commands;
 *  of:y  enables old firmware support;
 *  of:n  disables old firmware support;
 *  mq:xx set the max queue depth to the value xx (2 <= xx <= 8).
 *
 *  The default value is: "u14-34f=lc:n,of:n,mq:8". An example using the list
 *  of detection probes could be: "u14-34f=0x230,0x340,lc:y,of:n,mq:4".
 *
 *  When loading as a module, parameters can be specified as well.
 *  The above example would be (use 1 in place of y and 0 in place of n):
 *
 *  modprobe u14-34f io_port=0x230,0x340 linked_comm=1 have_old_firmware=0 \
 *                max_queue_depth=4
 *
 *  ----------------------------------------------------------------------------
 *  In this implementation, linked commands are designed to work with any DISK
 *  or CD-ROM, since this linking has only the intent of clustering (time-wise)
 *  and reordering by elevator sorting commands directed to each device,
 *  without any relation with the actual SCSI protocol between the controller
 *  and the device.
 *  If Q is the queue depth reported at boot time for each device (also named
 *  cmds/lun) and Q > 2, whenever there is already an active command to the
 *  device all other commands to the same device  (up to Q-1) are kept waiting
 *  in the elevator sorting queue. When the active command completes, the
 *  commands in this queue are sorted by sector address. The sort is chosen
 *  between increasing or decreasing by minimizing the seek distance between
 *  the sector of the commands just completed and the sector of the first 
 *  command in the list to be sorted. 
 *  Trivial math assures that if there are (Q-1) outstanding request for
 *  random seeks over S sectors, the unsorted average seek distance is S/2,
 *  while the sorted average seek distance is S/(Q-1). The seek distance is
 *  hence divided by a factor (Q-1)/2.
 *  The above pure geometric remarks are valid in all cases and the 
 *  driver effectively reduces the seek distance by the predicted factor
 *  when there are Q concurrent read i/o operations on the device, but this
 *  does not necessarily results in a noticeable performance improvement:
 *  your mileage may vary....
 *
 *  Note: command reordering inside a batch of queued commands could cause
 *        wrong results only if there is at least one write request and the
 *        intersection (sector-wise) of all requests is not empty. 
 *        When the driver detects a batch including overlapping requests
 *        (a really rare event) strict serial (pid) order is enforced.
 *  ----------------------------------------------------------------------------
 *
 *  The boards are named Ux4F0, Ux4F1,... according to the detection order.
 *
 *  In order to support multiple ISA boards in a reliable way,
 *  the driver sets host->wish_block = TRUE for all ISA boards.
 */

#define LinuxVersionCode(v, p, s) (((v)<<16)+((p)<<8)+(s))
#define MAX_INT_PARAM 10
#if defined(MODULE)
#include <linux/module.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= LinuxVersionCode(2,1,26)
MODULE_PARM(io_port, "1-" __MODULE_STRING(MAX_INT_PARAM) "i");
MODULE_PARM(linked_comm, "i");
MODULE_PARM(have_old_firmware, "i");
MODULE_PARM(link_statistics, "i");
MODULE_PARM(max_queue_depth, "i");
MODULE_AUTHOR("Dario Ballabio");
#endif
#endif

#include <linux/string.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/byteorder.h>
#include <linux/proc_fs.h>
#include <linux/blk.h>
#include "scsi.h"
#include "hosts.h"
#include "sd.h"
#include <asm/dma.h>
#include <asm/irq.h>
#include "u14-34f.h"
#include<linux/stat.h>
#include<linux/config.h>

struct proc_dir_entry proc_scsi_u14_34f = {
    PROC_SCSI_U14_34F, 6, "u14_34f",
    S_IFDIR | S_IRUGO | S_IXUGO, 2
};

/* Values for the PRODUCT_ID ports for the 14/34F */
#define PRODUCT_ID1  0x56
#define PRODUCT_ID2  0x40        /* NOTE: Only upper nibble is used */

/* Subversion values */
#define ISA  0
#define ESA 1

#define OP_HOST_ADAPTER   0x1
#define OP_SCSI           0x2
#define OP_RESET          0x4
#define DTD_SCSI          0x0
#define DTD_IN            0x1
#define DTD_OUT           0x2
#define DTD_NONE          0x3
#define HA_CMD_INQUIRY    0x1
#define HA_CMD_SELF_DIAG  0x2
#define HA_CMD_READ_BUFF  0x3
#define HA_CMD_WRITE_BUFF 0x4

#undef  DEBUG_LINKED_COMMANDS
#undef  DEBUG_DETECT
#undef  DEBUG_INTERRUPT
#undef  DEBUG_STATISTICS
#undef  DEBUG_RESET

#define MAX_ISA 3
#define MAX_VESA 1 
#define MAX_EISA 0
#define MAX_PCI 0
#define MAX_BOARDS (MAX_ISA + MAX_VESA + MAX_EISA + MAX_PCI)
#define MAX_CHANNEL 1
#define MAX_LUN 8
#define MAX_TARGET 8
#define MAX_IRQ 16
#define MAX_MAILBOXES 16
#define MAX_SGLIST 32
#define MAX_SAFE_SGLIST 16
#define MAX_INTERNAL_RETRIES 64
#define MAX_CMD_PER_LUN 2
#define MAX_TAGGED_CMD_PER_LUN (MAX_MAILBOXES - MAX_CMD_PER_LUN)

#define SKIP UINT_MAX
#define FALSE 0
#define TRUE 1
#define FREE 0
#define IN_USE   1
#define LOCKED   2
#define IN_RESET 3
#define IGNORE   4
#define READY    5
#define ABORTING 6
#define NO_DMA  0xff
#define MAXLOOP 200000

#define REG_LCL_MASK      0
#define REG_LCL_INTR      1
#define REG_SYS_MASK      2
#define REG_SYS_INTR      3
#define REG_PRODUCT_ID1   4
#define REG_PRODUCT_ID2   5
#define REG_CONFIG1       6
#define REG_CONFIG2       7
#define REG_OGM           8
#define REG_ICM           12
#define REGION_SIZE       13
#define BSY_ASSERTED      0x01
#define IRQ_ASSERTED      0x01
#define CMD_RESET         0xc0
#define CMD_OGM_INTR      0x01
#define CMD_CLR_INTR      0x01
#define CMD_ENA_INTR      0x81
#define ASOK              0x00
#define ASST              0x91

#define ARRAY_SIZE(arr) (sizeof (arr) / sizeof (arr)[0])
#define YESNO(a) ((a) ? 'y' : 'n')
#define TLDEV(type) ((type) == TYPE_DISK || (type) == TYPE_ROM)

#define PACKED          __attribute__((packed))

struct sg_list {
   unsigned int address;                /* Segment Address */
   unsigned int num_bytes;              /* Segment Length */
   };

/* MailBox SCSI Command Packet */
struct mscp {
   unsigned char opcode: 3;             /* type of command */
   unsigned char xdir: 2;               /* data transfer direction */
   unsigned char dcn: 1;                /* disable disconnect */
   unsigned char ca: 1;                 /* use cache (if available) */
   unsigned char sg: 1;                 /* scatter/gather operation */
   unsigned char target: 3;             /* SCSI target id */
   unsigned char channel: 2;            /* SCSI channel number */
   unsigned char lun: 3;                /* SCSI logical unit number */
   unsigned int data_address PACKED;    /* transfer data pointer */
   unsigned int data_len PACKED;        /* length in bytes */
   unsigned int link_address PACKED;    /* for linking command chains */
   unsigned char clink_id;              /* identifies command in chain */
   unsigned char use_sg;                /* (if sg is set) 8 bytes per list */
   unsigned char sense_len;
   unsigned char scsi_cdbs_len;         /* 6, 10, or 12 */
   unsigned char scsi_cdbs[12];         /* SCSI commands */
   unsigned char adapter_status;        /* non-zero indicates HA error */
   unsigned char target_status;         /* non-zero indicates target error */
   unsigned int sense_addr PACKED;
   Scsi_Cmnd *SCpnt;
   unsigned int index;                  /* cp index */
   struct sg_list *sglist;
   };

struct hostdata {
   struct mscp cp[MAX_MAILBOXES];       /* Mailboxes for this board */
   unsigned int cp_stat[MAX_MAILBOXES]; /* FREE, IN_USE, LOCKED, IN_RESET */
   unsigned int last_cp_used;           /* Index of last mailbox used */
   unsigned int iocount;                /* Total i/o done for this board */
   unsigned int multicount;             /* Total ... in second ihdlr loop */
   int board_number;                    /* Number of this board */
   char board_name[16];                 /* Name of this board */
   char board_id[256];                  /* data from INQUIRY on this board */
   int in_reset;                        /* True if board is doing a reset */
   int target_to[MAX_TARGET][MAX_CHANNEL]; /* N. of timeout errors on target */
   int target_redo[MAX_TARGET][MAX_CHANNEL]; /* If TRUE redo i/o on target */
   unsigned int retries;                /* Number of internal retries */
   unsigned long last_retried_pid;      /* Pid of last retried command */
   unsigned char subversion;            /* Bus type, either ISA or ESA */
   unsigned char heads;
   unsigned char sectors;

   /* slot != 0 for the U24F, slot == 0 for both the U14F and U34F */
   unsigned char slot;
   };

static struct Scsi_Host *sh[MAX_BOARDS + 1];
static const char *driver_name = "Ux4F";
static unsigned int irqlist[MAX_IRQ], calls[MAX_IRQ];

static unsigned int io_port[] = { 

   /* Space for MAX_INT_PARAM ports usable while loading as a module */
   SKIP,    SKIP,   SKIP,   SKIP,   SKIP,   SKIP,   SKIP,   SKIP,
   SKIP,    SKIP,

   /* Possible ISA/VESA ports */
   0x330, 0x340, 0x230, 0x240, 0x210, 0x130, 0x140,

   /* End of list */
   0x0
   };

#define HD(board) ((struct hostdata *) &sh[board]->hostdata)
#define BN(board) (HD(board)->board_name)

#define SWAP_BYTE(x) ((unsigned long)( \
	(((unsigned long)(x) & 0x000000ffU) << 24) | \
	(((unsigned long)(x) & 0x0000ff00U) <<  8) | \
	(((unsigned long)(x) & 0x00ff0000U) >>  8) | \
	(((unsigned long)(x) & 0xff000000U) >> 24)))

#if defined(__BIG_ENDIAN)
#define H2DEV(x) SWAP_BYTE(x)
#else
#define H2DEV(x) (x)
#endif

#define DEV2H(x) H2DEV(x)
#define V2DEV(addr) ((addr) ? H2DEV(virt_to_bus((void *)addr)) : 0)
#define DEV2V(addr) ((addr) ? DEV2H(bus_to_virt((unsigned long)addr)) : 0)

static void u14_34f_interrupt_handler(int, void *, struct pt_regs *);
static void flush_dev(Scsi_Device *, unsigned long, unsigned int, unsigned int);
static int do_trace = FALSE;
static int setup_done = FALSE;
static int link_statistics = 0;

#if defined (HAVE_OLD_UX4F_FIRMWARE)
static int have_old_firmware = TRUE;
#else
static int have_old_firmware = FALSE;
#endif

#if defined (CONFIG_SCSI_U14_34F_LINKED_COMMANDS)
static int linked_comm = TRUE;
#else
static int linked_comm = FALSE;
#endif

#if defined CONFIG_SCSI_U14_34F_MAX_TAGS
static int max_queue_depth = CONFIG_SCSI_U14_34F_MAX_TAGS;
#else
static int max_queue_depth = MAX_CMD_PER_LUN;
#endif

static void select_queue_depths(struct Scsi_Host *host, Scsi_Device *devlist) {
   Scsi_Device *dev;
   int j, ntag = 0, nuntag = 0, tqd, utqd; 
   unsigned long flags;

   save_flags(flags);
   cli();

   j = ((struct hostdata *) host->hostdata)->board_number;

   for(dev = devlist; dev; dev = dev->next) {

      if (dev->host != host) continue;

      if (TLDEV(dev->type) && (dev->tagged_supported || linked_comm))
         ntag++;
      else
         nuntag++;
      }

   utqd = MAX_CMD_PER_LUN;

   tqd = (host->can_queue - utqd * nuntag) / (ntag ? ntag : 1);

   if (tqd > max_queue_depth) tqd = max_queue_depth;

   if (tqd < MAX_CMD_PER_LUN) tqd = MAX_CMD_PER_LUN;

   for(dev = devlist; dev; dev = dev->next) {
      char *tag_suffix = "", *link_suffix = "";

      if (dev->host != host) continue;

      if (TLDEV(dev->type) && (dev->tagged_supported || linked_comm))
         dev->queue_depth = tqd;
      else
         dev->queue_depth = utqd;

      if (TLDEV(dev->type)) {
         if (linked_comm && dev->queue_depth > 2)
            link_suffix = ", linked";
         else
            link_suffix = ", unlinked";
         }

      if (dev->tagged_supported && TLDEV(dev->type) && dev->tagged_queue)
         tag_suffix = ", tagged";
      else if (dev->tagged_supported && TLDEV(dev->type))
         tag_suffix = ", untagged";

      printk("%s: scsi%d, channel %d, id %d, lun %d, cmds/lun %d%s%s.\n",
             BN(j), host->host_no, dev->channel, dev->id, dev->lun,
             dev->queue_depth, link_suffix, tag_suffix);
      }

   restore_flags(flags);
   return;
}

static inline int wait_on_busy(unsigned int iobase) {
   unsigned int loop = MAXLOOP;

   while (inb(iobase + REG_LCL_INTR) & BSY_ASSERTED)
      if (--loop == 0) return TRUE;

   return FALSE;
}

static int board_inquiry(unsigned int j) {
   struct mscp *cpp;
   unsigned int time, limit = 0;

   cpp = &HD(j)->cp[0];
   memset(cpp, 0, sizeof(struct mscp));
   cpp->opcode = OP_HOST_ADAPTER;
   cpp->xdir = DTD_IN;
   cpp->data_address = V2DEV(HD(j)->board_id);
   cpp->data_len = H2DEV(sizeof(HD(j)->board_id));
   cpp->scsi_cdbs_len = 6;
   cpp->scsi_cdbs[0] = HA_CMD_INQUIRY;

   if (wait_on_busy(sh[j]->io_port)) {
      printk("%s: board_inquiry, adapter busy.\n", BN(j));
      return TRUE;
      }

   HD(j)->cp_stat[0] = IGNORE;

   /* Clear the interrupt indication */
   outb(CMD_CLR_INTR, sh[j]->io_port + REG_SYS_INTR);

   /* Store pointer in OGM address bytes */
   outl(V2DEV(cpp), sh[j]->io_port + REG_OGM);

   /* Issue OGM interrupt */
   outb(CMD_OGM_INTR, sh[j]->io_port + REG_LCL_INTR);

   sti();
   time = jiffies;
   while ((jiffies - time) < HZ && limit++ < 100000000);
   cli();

   if (cpp->adapter_status || HD(j)->cp_stat[0] != FREE) {
      HD(j)->cp_stat[0] = FREE;
      printk("%s: board_inquiry, err 0x%x.\n", BN(j), cpp->adapter_status);
      return TRUE;
      }

   return FALSE;
}

static inline int port_detect(unsigned int port_base, unsigned int j, 
                              Scsi_Host_Template *tpnt) {
   unsigned char irq, dma_channel, subversion, i;
   unsigned char in_byte;
   char *bus_type, dma_name[16];

   /* Allowed BIOS base addresses (NULL indicates reserved) */
   void *bios_segment_table[8] = { 
      NULL, 
      (void *) 0xc4000, (void *) 0xc8000, (void *) 0xcc000, (void *) 0xd0000,
      (void *) 0xd4000, (void *) 0xd8000, (void *) 0xdc000
      };
   
   /* Allowed IRQs */
   unsigned char interrupt_table[4] = { 15, 14, 11, 10 };
   
   /* Allowed DMA channels for ISA (0 indicates reserved) */
   unsigned char dma_channel_table[4] = { 5, 6, 7, 0 };
   
   /* Head/sector mappings */
   struct {
      unsigned char heads;
      unsigned char sectors;
      } mapping_table[4] = { 
	   { 16, 63 }, { 64, 32 }, { 64, 63 }, { 64, 32 }
	   };

   struct config_1 {
      unsigned char bios_segment: 3;
      unsigned char removable_disks_as_fixed: 1;
      unsigned char interrupt: 2;
      unsigned char dma_channel: 2;
      } config_1;

   struct config_2 {
      unsigned char ha_scsi_id: 3;
      unsigned char mapping_mode: 2;
      unsigned char bios_drive_number: 1;
      unsigned char tfr_port: 2;
      } config_2;

   char name[16];

   sprintf(name, "%s%d", driver_name, j);

   if(check_region(port_base, REGION_SIZE)) {
      printk("%s: address 0x%03x in use, skipping probe.\n", name, port_base);
      return FALSE;
      }

   if (inb(port_base + REG_PRODUCT_ID1) != PRODUCT_ID1) return FALSE;

   in_byte = inb(port_base + REG_PRODUCT_ID2);

   if ((in_byte & 0xf0) != PRODUCT_ID2) return FALSE;

   *(char *)&config_1 = inb(port_base + REG_CONFIG1);
   *(char *)&config_2 = inb(port_base + REG_CONFIG2);

   irq = interrupt_table[config_1.interrupt];
   dma_channel = dma_channel_table[config_1.dma_channel];
   subversion = (in_byte & 0x0f);

   /* Board detected, allocate its IRQ if not already done */
   if ((irq >= MAX_IRQ) || (!irqlist[irq] && request_irq(irq,
              u14_34f_interrupt_handler, SA_INTERRUPT, driver_name, NULL))) {
      printk("%s: unable to allocate IRQ %u, detaching.\n", name, irq);
      return FALSE;
      }

   if (subversion == ISA && request_dma(dma_channel, driver_name)) {
      printk("%s: unable to allocate DMA channel %u, detaching.\n",
	     name, dma_channel);
      free_irq(irq, NULL);
      return FALSE;
      }

   sh[j] = scsi_register(tpnt, sizeof(struct hostdata));

   if (sh[j] == NULL) {
      printk("%s: unable to register host, detaching.\n", name);

      if (!irqlist[irq]) free_irq(irq, NULL);

      if (subversion == ISA) free_dma(dma_channel);

      return FALSE;
      }

   sh[j]->io_port = port_base;
   sh[j]->unique_id = port_base;
   sh[j]->n_io_port = REGION_SIZE;
   sh[j]->base = bios_segment_table[config_1.bios_segment];
   sh[j]->irq = irq;
   sh[j]->sg_tablesize = MAX_SGLIST;
   sh[j]->this_id = config_2.ha_scsi_id;
   sh[j]->can_queue = MAX_MAILBOXES;
   sh[j]->cmd_per_lun = MAX_CMD_PER_LUN;
   sh[j]->select_queue_depths = select_queue_depths;

#if defined(DEBUG_DETECT)
   {
   unsigned char sys_mask, lcl_mask;

   sys_mask = inb(sh[j]->io_port + REG_SYS_MASK);
   lcl_mask = inb(sh[j]->io_port + REG_LCL_MASK);
   printk("SYS_MASK 0x%x, LCL_MASK 0x%x.\n", sys_mask, lcl_mask);
   }
#endif

   /* If BIOS is disabled, force enable interrupts */
   if (sh[j]->base == 0) outb(CMD_ENA_INTR, sh[j]->io_port + REG_SYS_MASK);

   /* Register the I/O space that we use */
   request_region(sh[j]->io_port, sh[j]->n_io_port, driver_name);

   memset(HD(j), 0, sizeof(struct hostdata));
   HD(j)->heads = mapping_table[config_2.mapping_mode].heads;
   HD(j)->sectors = mapping_table[config_2.mapping_mode].sectors;
   HD(j)->subversion = subversion;
   HD(j)->board_number = j;
   irqlist[irq]++;

   if (have_old_firmware) sh[j]->sg_tablesize = MAX_SAFE_SGLIST;

   if (HD(j)->subversion == ESA) {
      sh[j]->unchecked_isa_dma = FALSE;
      sh[j]->dma_channel = NO_DMA;
      sprintf(BN(j), "U34F%d", j);
      bus_type = "VESA";
      }
   else {
      sh[j]->wish_block = TRUE;
      sh[j]->unchecked_isa_dma = TRUE;
      disable_dma(dma_channel);
      clear_dma_ff(dma_channel);
      set_dma_mode(dma_channel, DMA_MODE_CASCADE);
      enable_dma(dma_channel);

      if (have_old_firmware) sh[j]->hostt->use_clustering = DISABLE_CLUSTERING;

      sh[j]->dma_channel = dma_channel;
      sprintf(BN(j), "U14F%d", j);
      bus_type = "ISA";
      }

   sh[j]->max_channel = MAX_CHANNEL - 1;
   sh[j]->max_id = MAX_TARGET;
   sh[j]->max_lun = MAX_LUN;

   if (HD(j)->subversion == ISA && !board_inquiry(j)) {
      HD(j)->board_id[40] = 0;

      if (strcmp(&HD(j)->board_id[32], "06000600")) {
	 printk("%s: %s.\n", BN(j), &HD(j)->board_id[8]);
	 printk("%s: firmware %s is outdated, FW PROM should be 28004-006.\n",
		BN(j), &HD(j)->board_id[32]);
	 sh[j]->hostt->use_clustering = DISABLE_CLUSTERING;
	 sh[j]->sg_tablesize = MAX_SAFE_SGLIST;
	 }
      }

   if (dma_channel == NO_DMA) sprintf(dma_name, "%s", "NO DMA");
   else                       sprintf(dma_name, "DMA %u", dma_channel);

   for (i = 0; i < sh[j]->can_queue; i++)
      if (! ((&HD(j)->cp[i])->sglist = kmalloc(
            sh[j]->sg_tablesize * sizeof(struct sg_list), 
            (sh[j]->unchecked_isa_dma ? GFP_DMA : 0) | GFP_ATOMIC))) {
         printk("%s: kmalloc SGlist failed, mbox %d, detaching.\n", BN(j), i);
         u14_34f_release(sh[j]);
         return FALSE;
         }
      
   if (max_queue_depth > MAX_TAGGED_CMD_PER_LUN) 
       max_queue_depth = MAX_TAGGED_CMD_PER_LUN;

   if (max_queue_depth < MAX_CMD_PER_LUN) max_queue_depth = MAX_CMD_PER_LUN;

   printk("%s: %s 0x%03x, BIOS 0x%05x, IRQ %u, %s, SG %d, MB %d, of:%c, "\
          "lc:%c, mq:%d.\n", BN(j), bus_type, sh[j]->io_port, (int)sh[j]->base,
          sh[j]->irq, dma_name, sh[j]->sg_tablesize, sh[j]->can_queue,
          YESNO(have_old_firmware), YESNO(linked_comm), max_queue_depth);

   if (sh[j]->max_id > 8 || sh[j]->max_lun > 8)
      printk("%s: wide SCSI support enabled, max_id %u, max_lun %u.\n",
             BN(j), sh[j]->max_id, sh[j]->max_lun);

   for (i = 0; i <= sh[j]->max_channel; i++)
      printk("%s: SCSI channel %u enabled, host target ID %u.\n",
             BN(j), i, sh[j]->this_id);

   return TRUE;
}

void u14_34f_setup(char *str, int *ints) {
   int i, argc = ints[0];
   char *cur = str, *pc;

   if (argc > 0) {

      if (argc > MAX_INT_PARAM) argc = MAX_INT_PARAM;

      for (i = 0; i < argc; i++) io_port[i] = ints[i + 1]; 
   
      io_port[i] = 0;
      setup_done = TRUE;
      }

   while (cur && (pc = strchr(cur, ':'))) {
      int val = 0, c = *++pc;

      if (c == 'n' || c == 'N') val = FALSE;
      else if (c == 'y' || c == 'Y') val = TRUE;
      else val = (int) simple_strtoul(pc, NULL, 0);

      if (!strncmp(cur, "lc:", 3)) linked_comm = val;
      else if (!strncmp(cur, "of:", 3)) have_old_firmware = val;
      else if (!strncmp(cur, "mq:", 3))  max_queue_depth = val;
      else if (!strncmp(cur, "ls:", 3))  link_statistics = val;

      if ((cur = strchr(cur, ','))) ++cur;
      }

   return;
}

int u14_34f_detect(Scsi_Host_Template *tpnt) {
   unsigned long flags;
   unsigned int j = 0, k;

   tpnt->proc_dir = &proc_scsi_u14_34f;

   save_flags(flags);
   cli();

#if defined(MODULE)
   /* io_port could have been modified when loading as a module */
   if(io_port[0] != SKIP) {
      setup_done = TRUE;
      io_port[MAX_INT_PARAM] = 0;
      }
#endif

   for (k = 0; k < MAX_IRQ; k++) {
      irqlist[k] = 0;
      calls[k] = 0;
      }

   for (k = 0; k < MAX_BOARDS + 1; k++) sh[k] = NULL;

   for (k = 0; io_port[k]; k++) {

      if (io_port[k] == SKIP) continue;

      if (j < MAX_BOARDS && port_detect(io_port[k], j, tpnt)) j++;
      }

   if (j > 0) 
      printk("UltraStor 14F/34F: Copyright (C) 1994-1997 Dario Ballabio.\n");

   restore_flags(flags);
   return j;
}

static inline void build_sg_list(struct mscp *cpp, Scsi_Cmnd *SCpnt) {
   unsigned int k, data_len = 0;
   struct scatterlist *sgpnt;

   sgpnt = (struct scatterlist *) SCpnt->request_buffer;

   for (k = 0; k < SCpnt->use_sg; k++) {
      cpp->sglist[k].address = V2DEV(sgpnt[k].address);
      cpp->sglist[k].num_bytes = H2DEV(sgpnt[k].length);
      data_len += sgpnt[k].length;
      }

   cpp->use_sg = SCpnt->use_sg;
   cpp->data_address = V2DEV(cpp->sglist);
   cpp->data_len = H2DEV(data_len);
}

int u14_34f_queuecommand(Scsi_Cmnd *SCpnt, void (*done)(Scsi_Cmnd *)) {
   unsigned long flags;
   unsigned int i, j, k;
   struct mscp *cpp;

   static const unsigned char data_out_cmds[] = {
      0x0a, 0x2a, 0x15, 0x55, 0x04, 0x07, 0x18, 0x1d, 0x24, 0x2e,
      0x30, 0x31, 0x32, 0x38, 0x39, 0x3a, 0x3b, 0x3d, 0x3f, 0x40, 
      0x41, 0x4c, 0xaa, 0xae, 0xb0, 0xb1, 0xb2, 0xb6, 0xea, 0x1b
      };

   static const unsigned char data_none_cmds[] = {
      0x01, 0x0b, 0x10, 0x11, 0x13, 0x16, 0x17, 0x19, 0x2b, 0x1e,
      0x2c, 0xac, 0x2f, 0xaf, 0x33, 0xb3, 0x35, 0x36, 0x45, 0x47,
      0x48, 0x49, 0xa9, 0x4b, 0xa5, 0xa6, 0xb5
      };

   save_flags(flags);
   cli();
   /* j is the board number */
   j = ((struct hostdata *) SCpnt->host->hostdata)->board_number;

   if (!done) panic("%s: qcomm, pid %ld, null done.\n", BN(j), SCpnt->pid);

   if (SCpnt->cmnd[0] == REQUEST_SENSE && SCpnt->sense_buffer[0]) {
      SCpnt->result = DID_OK << 16; 
      SCpnt->host_scribble = NULL;
      printk("%s: qcomm, target %d.%d:%d, pid %ld, request sense ignored.\n",
             BN(j), SCpnt->channel, SCpnt->target, SCpnt->lun, SCpnt->pid);
      restore_flags(flags);
      done(SCpnt);    
      return 0;
      }

   /* i is the mailbox number, look for the first free mailbox 
      starting from last_cp_used */
   i = HD(j)->last_cp_used + 1;

   for (k = 0; k < sh[j]->can_queue; k++, i++) {

      if (i >= sh[j]->can_queue) i = 0;

      if (HD(j)->cp_stat[i] == FREE) {
	 HD(j)->last_cp_used = i;
	 break;
	 }
      }

   if (k == sh[j]->can_queue) {
      printk("%s: qcomm, no free mailbox, resetting.\n", BN(j));

      if (HD(j)->in_reset) 
	 printk("%s: qcomm, already in reset.\n", BN(j));
      else if (u14_34f_reset(SCpnt, SCSI_RESET_SUGGEST_BUS_RESET) 
               == SCSI_RESET_SUCCESS) 
	 panic("%s: qcomm, SCSI_RESET_SUCCESS.\n", BN(j));

      SCpnt->result = DID_BUS_BUSY << 16; 
      SCpnt->host_scribble = NULL;
      printk("%s: qcomm, pid %ld, DID_BUS_BUSY, done.\n", BN(j), SCpnt->pid);
      restore_flags(flags);
      done(SCpnt);    
      return 0;
      }

   /* Set pointer to control packet structure */
   cpp = &HD(j)->cp[i];

   memset(cpp, 0, sizeof(struct mscp) - sizeof(struct sg_list *));
   SCpnt->scsi_done = done;
   cpp->index = i;
   SCpnt->host_scribble = (unsigned char *) &cpp->index;

   if (do_trace) printk("%s: qcomm, mbox %d, target %d.%d:%d, pid %ld.\n",
			BN(j), i, SCpnt->channel, SCpnt->target, 
                        SCpnt->lun, SCpnt->pid);

   cpp->xdir = DTD_IN;

   for (k = 0; k < ARRAY_SIZE(data_out_cmds); k++)
      if (SCpnt->cmnd[0] == data_out_cmds[k]) {
	 cpp->xdir = DTD_OUT;
	 break;
	 }

   if (cpp->xdir == DTD_IN)
      for (k = 0; k < ARRAY_SIZE(data_none_cmds); k++)
         if (SCpnt->cmnd[0] == data_none_cmds[k]) {
            cpp->xdir = DTD_NONE;
            break;
            }

   cpp->opcode = OP_SCSI;
   cpp->channel = SCpnt->channel;
   cpp->target = SCpnt->target;
   cpp->lun = SCpnt->lun;
   cpp->SCpnt = SCpnt;
   cpp->sense_addr = V2DEV(SCpnt->sense_buffer);
   cpp->sense_len = sizeof SCpnt->sense_buffer;

   if (SCpnt->use_sg) {
      cpp->sg = TRUE;
      build_sg_list(cpp, SCpnt);
      }
   else {
      cpp->data_address = V2DEV(SCpnt->request_buffer);
      cpp->data_len = H2DEV(SCpnt->request_bufflen);
      }

   cpp->scsi_cdbs_len = SCpnt->cmd_len;
   memcpy(cpp->scsi_cdbs, SCpnt->cmnd, cpp->scsi_cdbs_len);

   if (linked_comm && SCpnt->device->queue_depth > 2
                                     && TLDEV(SCpnt->device->type)) {
      HD(j)->cp_stat[i] = READY;
      flush_dev(SCpnt->device, 0, j, FALSE);
      restore_flags(flags);
      return 0;
      }

   if (wait_on_busy(sh[j]->io_port)) {
      SCpnt->result = DID_ERROR << 16;
      SCpnt->host_scribble = NULL;
      printk("%s: qcomm, target %d.%d:%d, pid %ld, adapter busy, DID_ERROR,"\
             " done.\n", BN(j), SCpnt->channel, SCpnt->target, SCpnt->lun,
             SCpnt->pid);
      restore_flags(flags);
      done(SCpnt);
      return 0;
      }

   /* Store pointer in OGM address bytes */
   outl(V2DEV(cpp), sh[j]->io_port + REG_OGM);

   /* Issue OGM interrupt */
   outb(CMD_OGM_INTR, sh[j]->io_port + REG_LCL_INTR);

   HD(j)->cp_stat[i] = IN_USE;
   restore_flags(flags);
   return 0;
}

int u14_34f_abort(Scsi_Cmnd *SCarg) {
   unsigned long flags;
   unsigned int i, j;

   save_flags(flags);
   cli();
   j = ((struct hostdata *) SCarg->host->hostdata)->board_number;

   if (SCarg->host_scribble == NULL) {
      printk("%s: abort, target %d.%d:%d, pid %ld inactive.\n",
	     BN(j), SCarg->channel, SCarg->target, SCarg->lun, SCarg->pid);
      restore_flags(flags);
      return SCSI_ABORT_NOT_RUNNING;
      }

   i = *(unsigned int *)SCarg->host_scribble;
   printk("%s: abort, mbox %d, target %d.%d:%d, pid %ld.\n",
	  BN(j), i, SCarg->channel, SCarg->target, SCarg->lun, SCarg->pid);

   if (i >= sh[j]->can_queue)
      panic("%s: abort, invalid SCarg->host_scribble.\n", BN(j));

   if (wait_on_busy(sh[j]->io_port)) {
      printk("%s: abort, timeout error.\n", BN(j));
      restore_flags(flags);
      return SCSI_ABORT_ERROR;
      }

   if (HD(j)->cp_stat[i] == FREE) {
      printk("%s: abort, mbox %d is free.\n", BN(j), i);
      restore_flags(flags);
      return SCSI_ABORT_NOT_RUNNING;
      }

   if (HD(j)->cp_stat[i] == IN_USE) {
      printk("%s: abort, mbox %d is in use.\n", BN(j), i);

      if (SCarg != HD(j)->cp[i].SCpnt)
	 panic("%s: abort, mbox %d, SCarg %p, cp SCpnt %p.\n",
	       BN(j), i, SCarg, HD(j)->cp[i].SCpnt);

      if (inb(sh[j]->io_port + REG_SYS_INTR) & IRQ_ASSERTED)
         printk("%s: abort, mbox %d, interrupt pending.\n", BN(j), i);

      restore_flags(flags);
      return SCSI_ABORT_SNOOZE;
      }

   if (HD(j)->cp_stat[i] == IN_RESET) {
      printk("%s: abort, mbox %d is in reset.\n", BN(j), i);
      restore_flags(flags);
      return SCSI_ABORT_ERROR;
      }

   if (HD(j)->cp_stat[i] == LOCKED) {
      printk("%s: abort, mbox %d is locked.\n", BN(j), i);
      restore_flags(flags);
      return SCSI_ABORT_NOT_RUNNING;
      }

   if (HD(j)->cp_stat[i] == READY || HD(j)->cp_stat[i] == ABORTING) {
      SCarg->result = DID_ABORT << 16;
      SCarg->host_scribble = NULL;
      HD(j)->cp_stat[i] = FREE;
      printk("%s, abort, mbox %d ready, DID_ABORT, pid %ld done.\n",
	     BN(j), i, SCarg->pid);
      SCarg->scsi_done(SCarg);
      restore_flags(flags);
      return SCSI_ABORT_SUCCESS;
      }

   restore_flags(flags);
   panic("%s: abort, mbox %d, invalid cp_stat.\n", BN(j), i);
}

int u14_34f_reset(Scsi_Cmnd *SCarg, unsigned int reset_flags) {
   unsigned long flags;
   unsigned int i, j, time, k, c, limit = 0;
   int arg_done = FALSE;
   Scsi_Cmnd *SCpnt;

   save_flags(flags);
   cli();
   j = ((struct hostdata *) SCarg->host->hostdata)->board_number;
   printk("%s: reset, enter, target %d.%d:%d, pid %ld, reset_flags %u.\n", 
	  BN(j), SCarg->channel, SCarg->target, SCarg->lun, SCarg->pid,
          reset_flags);

   if (SCarg->host_scribble == NULL)
      printk("%s: reset, pid %ld inactive.\n", BN(j), SCarg->pid);

   if (HD(j)->in_reset) {
      printk("%s: reset, exit, already in reset.\n", BN(j));
      restore_flags(flags);
      return SCSI_RESET_ERROR;
      }

   if (wait_on_busy(sh[j]->io_port)) {
      printk("%s: reset, exit, timeout error.\n", BN(j));
      restore_flags(flags);
      return SCSI_RESET_ERROR;
      }

   HD(j)->retries = 0;

   for (c = 0; c <= sh[j]->max_channel; c++)
      for (k = 0; k < sh[j]->max_id; k++) {
         HD(j)->target_redo[k][c] = TRUE;
         HD(j)->target_to[k][c] = 0;
         }

   for (i = 0; i < sh[j]->can_queue; i++) {

      if (HD(j)->cp_stat[i] == FREE) continue;

      if (HD(j)->cp_stat[i] == LOCKED) {
	 HD(j)->cp_stat[i] = FREE;
	 printk("%s: reset, locked mbox %d forced free.\n", BN(j), i);
	 continue;
	 }

      if (!(SCpnt = HD(j)->cp[i].SCpnt))
	 panic("%s: reset, mbox %d, SCpnt == NULL.\n", BN(j), i);

      if (HD(j)->cp_stat[i] == READY || HD(j)->cp_stat[i] == ABORTING) {
         HD(j)->cp_stat[i] = ABORTING;
         printk("%s: reset, mbox %d aborting, pid %ld.\n",
                BN(j), i, SCpnt->pid);
         }

      else {
         HD(j)->cp_stat[i] = IN_RESET;
         printk("%s: reset, mbox %d in reset, pid %ld.\n",
                BN(j), i, SCpnt->pid);
         }

      if (SCpnt->host_scribble == NULL)
	 panic("%s: reset, mbox %d, garbled SCpnt.\n", BN(j), i);

      if (*(unsigned int *)SCpnt->host_scribble != i) 
	 panic("%s: reset, mbox %d, index mismatch.\n", BN(j), i);

      if (SCpnt->scsi_done == NULL) 
	 panic("%s: reset, mbox %d, SCpnt->scsi_done == NULL.\n", BN(j), i);

      if (SCpnt == SCarg) arg_done = TRUE;
      }

   if (wait_on_busy(sh[j]->io_port)) {
      printk("%s: reset, cannot reset, timeout error.\n", BN(j));
      restore_flags(flags);
      return SCSI_RESET_ERROR;
      }

   outb(CMD_RESET, sh[j]->io_port + REG_LCL_INTR);
   printk("%s: reset, board reset done, enabling interrupts.\n", BN(j));

#if defined (DEBUG_RESET)
   do_trace = TRUE;
#endif

   HD(j)->in_reset = TRUE;
   sti();
   time = jiffies;
   while ((jiffies - time) < HZ && limit++ < 100000000);
   cli();
   printk("%s: reset, interrupts disabled, loops %d.\n", BN(j), limit);

   for (i = 0; i < sh[j]->can_queue; i++) {

      if (HD(j)->cp_stat[i] == IN_RESET) {
         SCpnt = HD(j)->cp[i].SCpnt;
         SCpnt->result = DID_RESET << 16;
         SCpnt->host_scribble = NULL;

         /* This mailbox is still waiting for its interrupt */
         HD(j)->cp_stat[i] = LOCKED;

         printk("%s, reset, mbox %d locked, DID_RESET, pid %ld done.\n",
	        BN(j), i, SCpnt->pid);
         }

      else if (HD(j)->cp_stat[i] == ABORTING) {
         SCpnt = HD(j)->cp[i].SCpnt;
         SCpnt->result = DID_RESET << 16;
         SCpnt->host_scribble = NULL;

         /* This mailbox was never queued to the adapter */
         HD(j)->cp_stat[i] = FREE;

         printk("%s, reset, mbox %d aborting, DID_RESET, pid %ld done.\n",
	        BN(j), i, SCpnt->pid);
         }

      else

         /* Any other mailbox has already been set free by interrupt */
         continue;

      restore_flags(flags);
      SCpnt->scsi_done(SCpnt);
      cli();
      }

   HD(j)->in_reset = FALSE;
   do_trace = FALSE;
   restore_flags(flags);

   if (arg_done) {
      printk("%s: reset, exit, success.\n", BN(j));
      return SCSI_RESET_SUCCESS;
      }
   else {
      printk("%s: reset, exit, wakeup.\n", BN(j));
      return SCSI_RESET_PUNT;
      }
}

int u14_34f_biosparam(Disk *disk, kdev_t dev, int *dkinfo) {
   unsigned int j = 0;
   int size = disk->capacity;

   dkinfo[0] = HD(j)->heads;
   dkinfo[1] = HD(j)->sectors;
   dkinfo[2] = size / (HD(j)->heads * HD(j)->sectors);
   return FALSE;
}

static void sort(unsigned long sk[], unsigned int da[], unsigned int n,
                 unsigned int rev) {
   unsigned int i, j, k, y;
   unsigned long x;

   for (i = 0; i < n - 1; i++) {
      k = i;

      for (j = k + 1; j < n; j++) 
         if (rev) {
            if (sk[j] > sk[k]) k = j;
            }
         else {
            if (sk[j] < sk[k]) k = j;
            }

      if (k != i) {
         x = sk[k]; sk[k] = sk[i]; sk[i] = x;
         y = da[k]; da[k] = da[i]; da[i] = y;
         }
      }

   return;
   }

static inline void reorder(unsigned int j, unsigned long cursec,
                 unsigned int ihdlr, unsigned int il[], unsigned int n_ready) {
   Scsi_Cmnd *SCpnt;
   struct mscp *cpp;
   unsigned int k, n;
   unsigned int rev = FALSE, s = TRUE, r = TRUE;
   unsigned int input_only = TRUE, overlap = FALSE;
   unsigned long sl[n_ready], pl[n_ready], ll[n_ready];
   unsigned long maxsec = 0, minsec = ULONG_MAX, seek = 0;

   static unsigned int flushcount = 0, batchcount = 0, sortcount = 0;
   static unsigned int readycount = 0, ovlcount = 0, inputcount = 0;
   static unsigned int readysorted = 0, revcount = 0;
   static unsigned long seeksorted = 0, seeknosort = 0;

   if (link_statistics && !(++flushcount % link_statistics))
      printk("fc %d bc %d ic %d oc %d rc %d rs %d sc %d re %d"\
             " av %ldK as %ldK.\n", flushcount, batchcount, inputcount,
             ovlcount, readycount, readysorted, sortcount, revcount,
             seeknosort / (readycount - batchcount + 1), 
             seeksorted / (readycount - batchcount + 1));

   if (n_ready <= 1) return;

   for (n = 0; n < n_ready; n++) {
      k = il[n]; cpp = &HD(j)->cp[k]; SCpnt = cpp->SCpnt;

      if (!(cpp->xdir == DTD_IN)) input_only = FALSE;

      if (SCpnt->request.sector < minsec) minsec = SCpnt->request.sector;
      if (SCpnt->request.sector > maxsec) maxsec = SCpnt->request.sector;

      sl[n] = SCpnt->request.sector;

      if (!n) continue;

      if (sl[n] < sl[n - 1]) s = FALSE;
      if (sl[n] > sl[n - 1]) r = FALSE;
   
      if (link_statistics) {
         if (sl[n] > sl[n - 1])
            seek += sl[n] - sl[n - 1];
         else
            seek += sl[n - 1] - sl[n];
         }

      }

   if (cursec > ((maxsec + minsec) / 2)) rev = TRUE;

   if (!((rev && r) || (!rev && s))) sort(sl, il, n_ready, rev);

   if (!input_only) for (n = 0; n < n_ready; n++) {
      k = il[n]; cpp = &HD(j)->cp[k]; SCpnt = cpp->SCpnt;
      ll[n] = SCpnt->request.nr_sectors; pl[n] = SCpnt->pid;

      if (!n) continue;
 
      if ((sl[n] == sl[n - 1]) || (!rev && ((sl[n - 1] + ll[n - 1]) > sl[n]))
          || (rev && ((sl[n] + ll[n]) > sl[n - 1]))) overlap = TRUE;
      }

   if (overlap) sort(pl, il, n_ready, FALSE);

   if (link_statistics) {
      batchcount++; readycount += n_ready, seeknosort += seek / 1024; 
      if (input_only) inputcount++;
      if (overlap) { ovlcount++; seeksorted += seek / 1024; }
      else seeksorted += (maxsec - minsec) / 1024;
      if (rev && !r)     {  revcount++; readysorted += n_ready; }
      if (!rev && !s)    { sortcount++; readysorted += n_ready; }
      }

#if defined (DEBUG_LINKED_COMMANDS)
   if (link_statistics && (overlap || !(flushcount % link_statistics)))
      for (n = 0; n < n_ready; n++) {
         k = il[n]; cpp = &HD(j)->cp[k]; SCpnt = cpp->SCpnt;
         printk("%s %d.%d:%d pid %ld mb %d fc %d nr %d sec %ld ns %ld"\
                " cur %ld s:%c r:%c rev:%c in:%c ov:%c xd %d.\n",
                (ihdlr ? "ihdlr" : "qcomm"), SCpnt->channel, SCpnt->target,
                SCpnt->lun, SCpnt->pid, k, flushcount, n_ready,
                SCpnt->request.sector, SCpnt->request.nr_sectors, cursec,
                YESNO(s), YESNO(r), YESNO(rev), YESNO(input_only), 
                YESNO(overlap), cpp->xdir);
         }
#endif
}

static void flush_dev(Scsi_Device *dev, unsigned long cursec, unsigned int j,
                      unsigned int ihdlr) {
   Scsi_Cmnd *SCpnt;
   struct mscp *cpp;
   unsigned int k, n, n_ready = 0, il[MAX_MAILBOXES];

   for (k = 0; k < sh[j]->can_queue; k++) {

      if (HD(j)->cp_stat[k] != READY && HD(j)->cp_stat[k] != IN_USE) continue;

      cpp = &HD(j)->cp[k]; SCpnt = cpp->SCpnt;

      if (SCpnt->device != dev) continue;

      if (HD(j)->cp_stat[k] == IN_USE) return;

      il[n_ready++] = k;
      }

   reorder(j, cursec, ihdlr, il, n_ready);
  
   for (n = 0; n < n_ready; n++) {
      k = il[n]; cpp = &HD(j)->cp[k]; SCpnt = cpp->SCpnt;

      if (wait_on_busy(sh[j]->io_port)) {
         printk("%s: %s, target %d.%d:%d, pid %ld, Mbox %d, adapter"\
                " busy, will abort.\n", BN(j), (ihdlr ? "ihdlr" : "qcomm"),
                SCpnt->channel, SCpnt->target, SCpnt->lun, SCpnt->pid, k);
         HD(j)->cp_stat[k] = ABORTING;
         continue;
         }

      outl(V2DEV(cpp), sh[j]->io_port + REG_OGM);
      outb(CMD_OGM_INTR, sh[j]->io_port + REG_LCL_INTR);
      HD(j)->cp_stat[k] = IN_USE;
      }
}

static void u14_34f_interrupt_handler(int irq, void *dev_id,
                                      struct pt_regs *regs) {
   Scsi_Cmnd *SCpnt;
   unsigned long flags;
   unsigned int i, j, k, c, status, tstatus, loops, total_loops = 0, reg, ret;
   struct mscp *spp;

   save_flags(flags);
   cli();

   if (!irqlist[irq]) {
      printk("%s, ihdlr, irq %d, unexpected interrupt.\n", driver_name, irq);
      restore_flags(flags);
      return;
      }

   if (do_trace) printk("%s: ihdlr, enter, irq %d, calls %d.\n", 
			driver_name, irq, calls[irq]);

   /* Service all the boards configured on this irq */
   for (j = 0; sh[j] != NULL; j++) {

      if (sh[j]->irq != irq) continue;

      loops = 0;

      /* Loop until all interrupts for a board are serviced */
      while ((reg = inb(sh[j]->io_port + REG_SYS_INTR)) & IRQ_ASSERTED) {
	 total_loops++;
	 loops++;

	 if (do_trace) printk("%s: ihdlr, start service, count %d.\n",
			      BN(j), HD(j)->iocount);

	 spp = (struct mscp *)DEV2V(ret = inl(sh[j]->io_port + REG_ICM));

	 /* Clear interrupt pending flag */
	 outb(CMD_CLR_INTR, sh[j]->io_port + REG_SYS_INTR);

	 i = spp - HD(j)->cp;

	 if (spp < HD(j)->cp || spp >= HD(j)->cp + sh[j]->can_queue
                                            || i >= sh[j]->can_queue)
	    panic("%s: ihdlr, invalid mscp bus address %p, cp0 %p.\n",
                  BN(j), (void *)ret, HD(j)->cp);

	 if (HD(j)->cp_stat[i] == IGNORE) {
	    HD(j)->cp_stat[i] = FREE;
	    continue;
	    }
	 else if (HD(j)->cp_stat[i] == LOCKED) {
	    HD(j)->cp_stat[i] = FREE;
	    printk("%s: ihdlr, mbox %d unlocked, count %d.\n",
		   BN(j), i, HD(j)->iocount);
	    continue;
	    }
	 else if (HD(j)->cp_stat[i] == FREE) {
	    printk("%s: ihdlr, mbox %d is free, count %d.\n", 
		   BN(j), i, HD(j)->iocount);
	    continue;
	    }
	 else if (HD(j)->cp_stat[i] == IN_RESET)
	    printk("%s: ihdlr, mbox %d is in reset.\n", BN(j), i);
	 else if (HD(j)->cp_stat[i] != IN_USE) 
	    panic("%s: ihdlr, mbox %d, invalid cp_stat.\n", BN(j), i);

	 HD(j)->cp_stat[i] = FREE;
	 SCpnt = spp->SCpnt;

	 if (SCpnt == NULL) 
	    panic("%s: ihdlr, mbox %d, SCpnt == NULL.\n", BN(j), i);

	 if (SCpnt->host_scribble == NULL) 
	    panic("%s: ihdlr, mbox %d, pid %ld, SCpnt %p garbled.\n",
		  BN(j), i, SCpnt->pid, SCpnt);

	 if (*(unsigned int *)SCpnt->host_scribble != i) 
	    panic("%s: ihdlr, mbox %d, pid %ld, index mismatch %d,"\
		  " irq %d.\n", BN(j), i, SCpnt->pid, 
		  *(unsigned int *)SCpnt->host_scribble, irq);

         if (linked_comm && SCpnt->device->queue_depth > 2
                                           && TLDEV(SCpnt->device->type))
            flush_dev(SCpnt->device, SCpnt->request.sector, j, TRUE);

	 tstatus = status_byte(spp->target_status);

	 switch (spp->adapter_status) {
	    case ASOK:     /* status OK */

	       /* Forces a reset if a disk drive keeps returning BUSY */
	       if (tstatus == BUSY && SCpnt->device->type != TYPE_TAPE) 
		  status = DID_ERROR << 16;

	       /* If there was a bus reset, redo operation on each target */
	       else if (tstatus != GOOD && SCpnt->device->type == TYPE_DISK
		        && HD(j)->target_redo[SCpnt->target][SCpnt->channel])
		  status = DID_BUS_BUSY << 16;

	       /* Works around a flaw in scsi.c */
	       else if (tstatus == CHECK_CONDITION
			&& SCpnt->device->type == TYPE_DISK
			&& (SCpnt->sense_buffer[2] & 0xf) == RECOVERED_ERROR)
		  status = DID_BUS_BUSY << 16;

	       else
		  status = DID_OK << 16;

	       if (tstatus == GOOD)
		  HD(j)->target_redo[SCpnt->target][SCpnt->channel] = FALSE;

	       if (spp->target_status && SCpnt->device->type == TYPE_DISK)
		  printk("%s: ihdlr, target %d.%d:%d, pid %ld, "\
                         "target_status 0x%x, sense key 0x%x.\n", BN(j), 
			 SCpnt->channel, SCpnt->target, SCpnt->lun,
                         SCpnt->pid, spp->target_status,
                         SCpnt->sense_buffer[2]);

	       HD(j)->target_to[SCpnt->target][SCpnt->channel] = 0;

               if (HD(j)->last_retried_pid == SCpnt->pid) HD(j)->retries = 0;

	       break;
	    case ASST:     /* Selection Time Out */

	       if (HD(j)->target_to[SCpnt->target][SCpnt->channel] > 1)
		  status = DID_ERROR << 16;
	       else {
		  status = DID_TIME_OUT << 16;
		  HD(j)->target_to[SCpnt->target][SCpnt->channel]++;
		  }

	       break;

            /* Perform a limited number of internal retries */
	    case 0x93:     /* Unexpected bus free */
	    case 0x94:     /* Target bus phase sequence failure */
	    case 0x96:     /* Illegal SCSI command */
	    case 0xa3:     /* SCSI bus reset error */

	       for (c = 0; c <= sh[j]->max_channel; c++) 
	          for (k = 0; k < sh[j]->max_id; k++) 
	             HD(j)->target_redo[k][c] = TRUE;
   

	    case 0x92:     /* Data over/under-run */

	       if (SCpnt->device->type != TYPE_TAPE
                   && HD(j)->retries < MAX_INTERNAL_RETRIES) {
		  status = DID_BUS_BUSY << 16;
		  HD(j)->retries++;
                  HD(j)->last_retried_pid = SCpnt->pid;
                  }
	       else 
		  status = DID_ERROR << 16;

	       break;
	    case 0x01:     /* Invalid command */
	    case 0x02:     /* Invalid parameters */
	    case 0x03:     /* Invalid data list */
	    case 0x84:     /* SCSI bus abort error */
	    case 0x9b:     /* Auto request sense error */
	    case 0x9f:     /* Unexpected command complete message error */
	    case 0xff:     /* Invalid parameter in the S/G list */
	    default:
	       status = DID_ERROR << 16;
	       break;
	    }

	 SCpnt->result = status | spp->target_status;
	 HD(j)->iocount++;

	 if (loops > 1) HD(j)->multicount++;

#if defined (DEBUG_INTERRUPT)
	 if (SCpnt->result || do_trace) 
#else
	 if ((spp->adapter_status != ASOK && HD(j)->iocount >  1000) ||
	     (spp->adapter_status != ASOK && 
	      spp->adapter_status != ASST && HD(j)->iocount <= 1000) ||
              do_trace || msg_byte(spp->target_status))
#endif
	    printk("%s: ihdlr, mbox %2d, err 0x%x:%x,"\
		   " target %d.%d:%d, pid %ld, reg 0x%x, count %d.\n",
		   BN(j), i, spp->adapter_status, spp->target_status,
		   SCpnt->channel, SCpnt->target, SCpnt->lun, SCpnt->pid,
                   reg, HD(j)->iocount);

	 /* Set the command state to inactive */
	 SCpnt->host_scribble = NULL;

	 restore_flags(flags);
	 SCpnt->scsi_done(SCpnt);
	 cli();

	 }   /* Multiple command loop */

      }   /* Boards loop */

   calls[irq]++;

   if (total_loops == 0) 
     printk("%s: ihdlr, irq %d, no command completed, calls %d.\n",
	    driver_name, irq, calls[irq]);

   if (do_trace) printk("%s: ihdlr, exit, irq %d, calls %d.\n",
			driver_name, irq, calls[irq]);

#if defined (DEBUG_STATISTICS)
   if ((calls[irq] % 100000) == 10000)
      for (j = 0; sh[j] != NULL; j++)
	 printk("%s: ihdlr, calls %d, count %d, multi %d.\n", BN(j),
		calls[(sh[j]->irq)], HD(j)->iocount, HD(j)->multicount);
#endif

   restore_flags(flags);
   return;
}

int u14_34f_release(struct Scsi_Host *shpnt) {
   unsigned long flags;
   unsigned int i, j;

   save_flags(flags);
   cli();

   for (j = 0; sh[j] != NULL && sh[j] != shpnt; j++);
    
   if (sh[j] == NULL) panic("%s: release, invalid Scsi_Host pointer.\n",
                            driver_name);

   for (i = 0; i < sh[j]->can_queue; i++) 
      if ((&HD(j)->cp[i])->sglist) kfree((&HD(j)->cp[i])->sglist);

   if (! --irqlist[sh[j]->irq]) free_irq(sh[j]->irq, NULL);

   if (sh[j]->dma_channel != NO_DMA) free_dma(sh[j]->dma_channel);

   release_region(sh[j]->io_port, sh[j]->n_io_port);
   scsi_unregister(sh[j]);
   restore_flags(flags);
   return FALSE;
}

#if defined(MODULE)
Scsi_Host_Template driver_template = ULTRASTOR_14_34F;

#include "scsi_module.c"
#endif
