/*
 *  linux/kernel/hd.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This is the low-level hd interrupt support. It traverses the
 * request-list, using interrupts to jump between functions. As
 * all the functions are called within interrupts, we may not
 * sleep. Special care is recommended.
 * 
 *  modified by Drew Eckhardt to check nr of hd's from the CMOS.
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/hdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

#define MAJOR_NR 3
#include "blk.h"

#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

/* Max read/write errors/sector */
#define MAX_ERRORS	7
#define MAX_HD		2

static void recal_intr(void);

static int recalibrate = 1;
static int reset = 1;

/*
 *  This struct defines the HD's and their types.
 */
struct hd_i_struct {
	int head,sect,cyl,wpcom,lzone,ctl;
	};
#ifdef HD_TYPE
struct hd_i_struct hd_info[] = { HD_TYPE };
#define NR_HD ((sizeof (hd_info))/(sizeof (struct hd_i_struct)))
#else
struct hd_i_struct hd_info[] = { {0,0,0,0,0,0},{0,0,0,0,0,0} };/*lux 硬盘信息，由sys_setup设置 see 本文:71*/
static int NR_HD = 0;
#endif
/*lux 硬盘逻辑分区信息：开始扇区，扇区数*/
static struct hd_struct {
	long start_sect;
	long nr_sects;
} hd[5*MAX_HD]={{0,0},};

#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr):"cx","di")

#define port_write(port,buf,nr) \
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr):"cx","si")

extern void hd_interrupt(void);
extern void rd_load(void);

/* This may be used only once, enforced by 'static int callable' */
int sys_setup(void * BIOS)
{/*lux 读取硬盘信息，设置根文件系统*/
	static int callable = 1;
	int i,drive;
	unsigned char cmos_disks;
	struct partition *p;
	struct buffer_head * bh;

	if (!callable)
		return -1;
	callable = 0;
#ifndef HD_TYPE /*lux 没有手动设置HD_TYPE(默认)，直接从BIOS获取（在setup.s中已经读取，从main.c传入）*/
	for (drive=0 ; drive<2 ; drive++) {//lux 填充硬盘信息
		hd_info[drive].cyl = *(unsigned short *) BIOS;
		hd_info[drive].head = *(unsigned char *) (2+BIOS);
		hd_info[drive].wpcom = *(unsigned short *) (5+BIOS);
		hd_info[drive].ctl = *(unsigned char *) (8+BIOS);
		hd_info[drive].lzone = *(unsigned short *) (12+BIOS);
		hd_info[drive].sect = *(unsigned char *) (14+BIOS);
		BIOS += 16;
	}
	if (hd_info[1].cyl)//lux 硬盘数量
		NR_HD=2;
	else
		NR_HD=1;
#endif
	for (i=0 ; i<NR_HD ; i++) {
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = hd_info[i].head*
				hd_info[i].sect*hd_info[i].cyl;
	}

	/*
		We querry CMOS about hard disks : it could be that 
		we have a SCSI/ESDI/etc controller that is BIOS
		compatable with ST-506, and thus showing up in our
		BIOS table, but not register compatable, and therefore
		not present in CMOS.

		Furthurmore, we will assume that our ST-506 drives
		<if any> are the primary drives in the system, and 
		the ones reflected as drive 1 or 2.

		The first drive is stored in the high nibble of CMOS
		byte 0x12, the second in the low nibble.  This will be
		either a 4 bit drive type or 0xf indicating use byte 0x19 
		for an 8 bit type, drive 1, 0x1a for drive 2 in CMOS.

		Needless to say, a non-zero value means we have 
		an AT controller hard disk for that drive.

		
	*/

	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)//lux 如果cmos中读取不到，则不使用
		if (cmos_disks & 0x0f)
			NR_HD = 2;
		else
			NR_HD = 1;
	else
		NR_HD = 0;
	for (i = NR_HD ; i < 2 ; i++) {//lux 重置hd
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = 0;
	}
	for (drive=0 ; drive<NR_HD ; drive++) {
		if (!(bh = bread(0x300 + drive*5,0))) {//lux 逻辑设备号命令，see blk.h:3; 0代表第0个block
			printk("Unable to read partition table of drive %d\n\r",
				drive);
			panic("");
		}
		if (bh->b_data[510] != 0x55 || (unsigned char)//lux 0x55aa is mbr signature
		    bh->b_data[511] != 0xAA) {
			printk("Bad partition table on drive %d\n\r",drive);
			panic("");
		}
		/**lux
		 * see: https://en.wikipedia.org/wiki/Master_boot_record
		 * Structure of a classical generic MBR
		 * hex	dec	description
		 * +000	+0	Bootstrap code area	446
		 * +1BE	+446	Partition entry 1	Partition table
		 * +1CE	+462	Partition entry 2	16
		 * +1DE	+478	Partition entry 3	16
		 * +1EE	+494	Partition entry 4	16
		 * +1FE	+510	55hex	Boot signature[a]	2
		 * 
		 * Partition Entry see hdreg.h:52
		 * 下面读取分区表,其实主要就关心两个字段：start_sect, nr_sects
		 */
		p = 0x1BE + (void *)bh->b_data;
		for (i=1;i<5;i++,p++) {
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
		}
		brelse(bh);//读取完毕，可以释放该buffer了
	}
	if (NR_HD)
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");
	rd_load();/*lux 尝试加载ramdisk*/
	mount_root();/*lux 加载根文件系统*/
	return (0);
}

static int controller_ready(void)
{
	int retries=10000;

	while (--retries && (inb_p(HD_STATUS)&0xc0)!=0x40);
	return (retries);
}

static int win_result(void)
{
	int i=inb_p(HD_STATUS);

	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
		== (READY_STAT | SEEK_STAT))
		return(0); /* ok */
	if (i&1) i=inb(HD_ERROR);
	return (1);
}
/**lux 向硬盘发送命令，并设置回调:intr_addr，硬盘IO完毕会调用回调函数。
 */ 
static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
{
	register int port asm("dx");

	if (drive>1 || head>15)
		panic("Trying to write bad sector");
	if (!controller_ready())
		panic("HD controller not ready");
	do_hd = intr_addr;//lux 设置当前回调函数 磁盘中断会调用do_hd(see system_call.s:241),do_hd指向对应的处理函数
	outb_p(hd_info[drive].ctl,HD_CMD);
	port=HD_DATA;
	outb_p(hd_info[drive].wpcom>>2,++port);
	outb_p(nsect,++port);
	outb_p(sect,++port);
	outb_p(cyl,++port);
	outb_p(cyl>>8,++port);
	outb_p(0xA0|(drive<<4)|head,++port);
	outb(cmd,++port);
}

static int drive_busy(void)
{
	unsigned int i;

	for (i = 0; i < 10000; i++)
		if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))
			break;
	i = inb(HD_STATUS);
	i &= BUSY_STAT | READY_STAT | SEEK_STAT;
	if (i == READY_STAT | SEEK_STAT)
		return(0);
	printk("HD controller times out\n\r");
	return(1);
}

static void reset_controller(void)
{
	int	i;

	outb(4,HD_CMD);
	for(i = 0; i < 100; i++) nop();
	outb(hd_info[0].ctl & 0x0f ,HD_CMD);
	if (drive_busy())
		printk("HD-controller still busy\n\r");
	if ((i = inb(HD_ERROR)) != 1)
		printk("HD-controller reset failed: %02x\n\r",i);
}

static void reset_hd(int nr)
{//lux 重置硬盘
	reset_controller();
	hd_out(nr,hd_info[nr].sect,hd_info[nr].sect,hd_info[nr].head-1,
		hd_info[nr].cyl,WIN_SPECIFY,&recal_intr);
}

void unexpected_hd_interrupt(void)
{//lux 无效的do_hd see system_call.s:261
	printk("Unexpected HD interrupt\n\r");
}

static void bad_rw_intr(void)
{//lux 允许多次尝试；都失败后，结束请求，并标记失败
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);//lux 结束请求，标记失败
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;//lux 失败过半，重置harddisk，继续尝试
}
//lux 读操作回调（硬盘控制器已经读取完毕）
static void read_intr(void)
{
	if (win_result()) {
		bad_rw_intr();
		do_hd_request();//lux 继续发送请求
		return;
	}
	port_read(HD_DATA,CURRENT->buffer,256);
	CURRENT->errors = 0;
	CURRENT->buffer += 512;
	CURRENT->sector++;
	if (--CURRENT->nr_sectors) {//lux 还没读完，返回，等待下一次中断通知
		do_hd = &read_intr;
		return;
	}
	end_request(1);//lux 当前请求处理完了
	do_hd_request();//lux 继续处理剩余请求
}
//lux 写操作回调（硬盘控制器已经写完毕)
static void write_intr(void)
{
	if (win_result()) {
		bad_rw_intr();
		do_hd_request();
		return;
	}
	if (--CURRENT->nr_sectors) {//lux 还有，继续写
		CURRENT->sector++;
		CURRENT->buffer += 512;
		do_hd = &write_intr;
		port_write(HD_DATA,CURRENT->buffer,256);
		return;
	}
	end_request(1);//lux 当前处理完毕
	do_hd_request();//lux 继续处理剩余请求
}
//lux 重置硬盘的回调函数
static void recal_intr(void)
{
	if (win_result())
		bad_rw_intr();
	do_hd_request();
}
/*lux 硬盘读写入口
读写是立即返回的，而不等待磁盘结果（由上次调用方自行控制sleep）
磁盘通过异步中断通知进程，读写完成，进程判断所需要的读写都完成后，唤醒等待进程。 see end_request
*/
void do_hd_request(void)
{
	int i,r;
	unsigned int block,dev;
	unsigned int sec,head,cyl;
	unsigned int nsect;
//lux 下面是循环读取，直至request列表为空
	INIT_REQUEST;//lux 这个宏里定义了repeat
	dev = MINOR(CURRENT->dev);
	block = CURRENT->sector;
	if (dev >= 5*NR_HD || block+2 > hd[dev].nr_sects) {//lux 非法请求
		end_request(0);
		goto repeat;//lux 在INIT_REQUEST里定义
	}
	/** lux 下面换算出CHS(cylinder,header,sector)
	 * 假设一个磁盘 12个cylinder，2个header，spt = 512 (sector per track)
	 * 现在要读取整个磁盘第1025个sector，算出其CHS:
	 * 1025 / 512 = 2 余 1,1就是sectorNum，但由于sectorNum是从1计数的，所有这个sectorNum = 1+1 = 2
	 * 2 / 2 = 1 余 0， 0是headerNum，1是cylinderNum
	 * 即，第1025个sector，在第1个cylinder，第0个header，第2个sector。
	 */
	block += hd[dev].start_sect;//lux得出要读取的sector
	dev /= 5;//lux 计算出第几个硬盘
	__asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
		"r" (hd_info[dev].sect));
	__asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
		"r" (hd_info[dev].head));
	sec++;//lux  因为sector是从1开始计数的。
	nsect = CURRENT->nr_sectors;
	if (reset) {
		reset = 0;
		recalibrate = 1;
		reset_hd(CURRENT_DEV);//lux 重置harddisk
		return;
	}
	if (recalibrate) {//lux 重新校准
		recalibrate = 0;
		hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,
			WIN_RESTORE,&recal_intr);
		return;
	}	
	if (CURRENT->cmd == WRITE) {//lux 写操作
		hd_out(dev,nsect,sec,head,cyl,WIN_WRITE,&write_intr);//发送写指令，并设置回调write_intr，在回调内判断是否写入完成，是否继续处理下一个请求。
		for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
			/* nothing */ ;
		if (!r) {
			bad_rw_intr();//写失败
			goto repeat;
		}
		port_write(HD_DATA,CURRENT->buffer,256);
	} else if (CURRENT->cmd == READ) {//lux 读操作
		hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);//发送读指令，并设置回调read_intr
	} else
		panic("unknown hd-command");
}

void hd_init(void)
{
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	set_intr_gate(0x2E,&hd_interrupt);/*Lux 注册硬盘中断*/
	outb_p(inb_p(0x21)&0xfb,0x21);/*lux irq2,8259A主从连接打开*/
	outb(inb_p(0xA1)&0xbf,0xA1);/*lux 8259A从盘，irq14，硬盘中断打开*/
}
