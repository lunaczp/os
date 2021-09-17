#ifndef _BLK_H
#define _BLK_H
/**
 * lux see device.lux.md
 * 硬盘主设备号是3，see ll_rw_blk.c:32
 * 逻辑设备号	设备文件	说明
 * 0x300	/dev/hd0	第1硬盘
 * 0x301	/dev/hd1	第1硬盘，第1分区
 * 0x302	/dev/hd2	第1硬盘，第2分区
 * 0x303	/dev/hd3	第1硬盘，第3分区
 * 0x304	/dev/hd4	第1硬盘，第4分区
 * 0x305	/dev/hd5	第2硬盘
 * ...
 * 软盘主设备号是2，see ll_rw_blk.c:32
 * 逻辑设备号	设备文件	说明
 * 0x208	/dev/at0	1.2MB A驱动器
 * 0x209	/dev/at1	1.2MB B驱动器
 * 0x21c	/dev/fd0	1.44MB A驱动器
 * 0x21d	/dev/fd1	1.44MB B驱动器
 * 
 * 关于读写
 * do_hd_request hd_fd_request 是硬盘、软盘 读写函数/入口
 * do_hd do_floppy 是硬盘、软盘 回调函数。因为磁盘IO是异步的。一般流程是：
 * 	- 程序向磁盘发送命令，返回继续执行（不等待磁盘）
 *  - 磁盘IO完成，调用程序设置的回调。
 * 而不同命令需要的回调不一样，所以会在代码内手动设置do_hd,do_floppy，更多参考hd.c floppy.c
 */
#define NR_BLK_DEV	7 //lux 块设备数量
/*
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 *
 * 32 seems to be a reasonable number: enough to get some benefit
 * from the elevator-mechanism, but not so much as to lock a lot of
 * buffers when they are in the queue. 64 seems to be too many (easily
 * long pauses in reading when heavy writing/syncing is going on)
 */
#define NR_REQUEST	32

/*
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and 'waiting' is used to wait for
 * read/write completion.
 */
struct request {
	int dev;		/* -1 if no request */
	int cmd;		/* READ or WRITE */
	int errors;
	unsigned long sector;
	unsigned long nr_sectors;
	char * buffer;
	struct task_struct * waiting;
	struct buffer_head * bh;
	struct request * next;
};

/*
 * This is used in the elevator algorithm: Note that
 * reads always go before writes. This is natural: reads
 * are much more time-critical than writes.
 */
/*lux：有序的规则：
 * 1. cmd: 先读后写
 * 2. cmd一样: dev小的先
 * 3. cmd一样，dev一样：sector小的先
 * 保证了1.读优先；2.顺序读
 */
#define IN_ORDER(s1,s2) \
((s1)->cmd<(s2)->cmd || (s1)->cmd==(s2)->cmd && \
((s1)->dev < (s2)->dev || ((s1)->dev == (s2)->dev && \
(s1)->sector < (s2)->sector)))
/*lux blk 块设备结构体，每个块设备一个*/
struct blk_dev_struct {
	void (*request_fn)(void);//lux 该块设备的请求处理函数
	struct request * current_request;//lux 该块设备的当前请求
};

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];//lux 每个块设备占一项，注意，这里是声明，真正的定义在ll_rw_blk.c:32
extern struct request request[NR_REQUEST];
extern struct task_struct * wait_for_request;
/*lux major device可以是floppy、hard disk等，但对应的操作也不一样，需要设置*/
#ifdef MAJOR_NR

/*
 * Add entries as needed. Currently the only block devices
 * supported are hard-disks and floppies.
 */

#if (MAJOR_NR == 1)
/* ram disk */
#define DEVICE_NAME "ramdisk"
#define DEVICE_REQUEST do_rd_request
#define DEVICE_NR(device) ((device) & 7)
#define DEVICE_ON(device) 
#define DEVICE_OFF(device)

#elif (MAJOR_NR == 2)
/* floppy */
#define DEVICE_NAME "floppy"
#define DEVICE_INTR do_floppy //lux 软盘中断回调，see system_call.s
#define DEVICE_REQUEST do_fd_request
#define DEVICE_NR(device) ((device) & 3)
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))

#elif (MAJOR_NR == 3)
/* harddisk */
#define DEVICE_NAME "harddisk"
#define DEVICE_INTR do_hd //lux 硬盘中断回调，see system_call.s
#define DEVICE_REQUEST do_hd_request
#define DEVICE_NR(device) (MINOR(device)/5)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#elif
/* unknown blk device */
#error "unknown blk device"

#endif

#define CURRENT (blk_dev[MAJOR_NR].current_request)
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)

#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL; //lux 磁盘中断回调(磁盘控制器完成读写操作后，回调告知系统)。硬盘对应do_hd，软盘对应do_floppy,见上面
#endif
static void (DEVICE_REQUEST)(void);//lux 磁盘读写入口。硬盘对应do_hd_request，软盘对应do_floppy_request，见上面

extern inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)
		printk(DEVICE_NAME ": free buffer being unlocked\n");
	bh->b_lock=0;
	wake_up(&bh->b_wait);
}

extern inline void end_request(int uptodate)
{//lux 当前请求完成，解锁buffer，唤醒等待进程（等待该buffer的，等待空闲队列项的），继续列表内下一个请求
	DEVICE_OFF(CURRENT->dev);
	if (CURRENT->bh) {
		CURRENT->bh->b_uptodate = uptodate;
		unlock_buffer(CURRENT->bh);//lux 读写成功，解锁buffer
	}
	if (!uptodate) {
		printk(DEVICE_NAME " I/O error\n\r");
		printk("dev %04x, block %d\n\r",CURRENT->dev,
			CURRENT->bh->b_blocknr);
	}
	wake_up(&CURRENT->waiting);//lux 唤醒等待buffer的进程
	wake_up(&wait_for_request);//lux 唤醒等待入队列的进程
	CURRENT->dev = -1;//lux 标记该条目空闲
	CURRENT = CURRENT->next;//lux next one
}

#define INIT_REQUEST \
repeat: \
	if (!CURRENT) \
		return; \
	if (MAJOR(CURRENT->dev) != MAJOR_NR) \
		panic(DEVICE_NAME ": request list destroyed"); \
	if (CURRENT->bh) { \
		if (!CURRENT->bh->b_lock) \
			panic(DEVICE_NAME ": block not locked"); \
	}

#endif

#endif
