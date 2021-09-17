/*
 *  linux/init/main.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>
#include <time.h>

/*
 * we need this inline - forking from kernel space will result
 * in NO COPY ON WRITE (!!!), until an execve is executed. This
 * is no problem, but for the stack. This is handled by not letting
 * main() use the stack at all after fork(). Thus, no function
 * calls - which means inline code for fork too, as otherwise we
 * would use the stack upon exit from 'fork()'.
 *
 * Actually only pause and fork are needed inline, so that there
 * won't be any messing with the stack from main(), but we define
 * some others too.
 */
static inline _syscall0(int,fork)
static inline _syscall0(int,pause)
static inline _syscall1(int,setup,void *,BIOS)
static inline _syscall0(int,sync)

#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <asm/system.h>
#include <asm/io.h>

#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <linux/fs.h>

static char printbuf[1024];

extern int vsprintf();
extern void init(void);
extern void blk_dev_init(void);
extern void chr_dev_init(void);
extern void hd_init(void);
extern void floppy_init(void);
extern void mem_init(long start, long end);
extern long rd_init(long mem_start, int length);
extern long kernel_mktime(struct tm * tm);
extern long startup_time;

/*
 * This is set up by the setup-routine at boot-time
 */
#define EXT_MEM_K (*(unsigned short *)0x90002) /*lux see setup.s:51*/
#define DRIVE_INFO (*(struct drive_info *)0x90080)/*lux see setup.s:73*/
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC) /*lux 0x1FC = 508, see bootsect.s:319*/

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */
/*lux cmos https://wiki.osdev.org/CMOS
 Register  Contents
 0x00      Seconds
 0x02      Minutes
 0x04      Hours
 0x06      Weekday
 0x07      Day of Month
 0x08      Month
 0x09      Year
 0x32      Century (maybe)
 0x0A      Status Register A
 0x0B      Status Register B
*/
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

static void time_init(void)
{
	struct tm time;

	do {
		time.tm_sec = CMOS_READ(0);
		time.tm_min = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon = CMOS_READ(8);
		time.tm_year = CMOS_READ(9);
	} while (time.tm_sec != CMOS_READ(0));
	BCD_TO_BIN(time.tm_sec);
	BCD_TO_BIN(time.tm_min);
	BCD_TO_BIN(time.tm_hour);
	BCD_TO_BIN(time.tm_mday);
	BCD_TO_BIN(time.tm_mon);
	BCD_TO_BIN(time.tm_year);
	time.tm_mon--;
	startup_time = kernel_mktime(&time);
}

static long memory_end = 0;
static long buffer_memory_end = 0;
static long main_memory_start = 0;

struct drive_info { char dummy[32]; } drive_info;

void main(void)		/* This really IS void, no error here. */
{			/* The startup routine assumes (well, ...) this */
/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */

 	ROOT_DEV = ORIG_ROOT_DEV;
 	drive_info = DRIVE_INFO;//lux 磁盘信息
	memory_end = (1<<20) + (EXT_MEM_K<<10);/*lux EXT_MEM_K占用2byte，以kb为单位，所以要转换成bit*/
	memory_end &= 0xfffff000;/*lux 裁成4kb页对齐，再比较*/
	if (memory_end > 16*1024*1024)/*lux 超过16MB的内存，忽略不用*/
		memory_end = 16*1024*1024;
	if (memory_end > 12*1024*1024)/*lux 根据总内存的大小，分配缓存区。超过12，用4；超过6，用2；否则1 */
		buffer_memory_end = 4*1024*1024;
	else if (memory_end > 6*1024*1024)
		buffer_memory_end = 2*1024*1024;
	else
		buffer_memory_end = 1*1024*1024;
	main_memory_start = buffer_memory_end;/*lux 剩余真正用户程序可用的主内存*/
#ifdef RAMDISK
	main_memory_start += rd_init(main_memory_start, RAMDISK*1024);/*lux 还要给ramdisk映射一部分内存*/
#endif
	mem_init(main_memory_start,memory_end);/*lux 内存初始化*/
	trap_init();/*lux 初始化调用门*/
	blk_dev_init();/*lux 块设备初始化*/
	chr_dev_init();/*lux 字符设备初始化，nothing for now*/
	tty_init();/*lux 终端(串口，显示器)初始化*/
	time_init();/*lux 系统时间初始化*/
	sched_init();/*lux 调度器初始化*/
	buffer_init(buffer_memory_end);/*lux Linux系统缓存初始化*/
	hd_init();/*lux 硬盘初始化*/
	floppy_init();/*lux 软盘初始化*/
	sti();/*lux 开启中断*/
	move_to_user_mode();/*lux 进入用户模式，Level3，且进入task0的上下文*/
	if (!fork()) {		/* we count on this going ok */
		init();/*lux 子进程，task 1*/
	}
/*
 *   NOTE!!   For any other task 'pause()' would mean we have to get a
 * signal to awaken, but task0 is the sole exception (see 'schedule()')
 * as task 0 gets activated at every idle moment (when no other tasks
 * can run). For task0 'pause()' just means we go check if some other
 * task can run, and if not we return here.
 */
	for(;;) pause();//lux task 0 
}

static int printf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));
	va_end(args);
	return i;
}

static char * argv_rc[] = { "/bin/sh", NULL };
static char * envp_rc[] = { "HOME=/", NULL };

static char * argv[] = { "-/bin/sh",NULL };
static char * envp[] = { "HOME=/usr/root", NULL };
/*lux init进程，pid=1*/
void init(void)
{
	int pid,i;

	setup((void *) &drive_info);//lux setup system:1.hd info, 2.root filesystem...
	(void) open("/dev/tty0",O_RDWR,0);
	(void) dup(0);
	(void) dup(0);
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);
	printf("Free mem: %d bytes\n\r",memory_end-main_memory_start);
	if (!(pid=fork())) {//lux 子进程
		/**
		 * 下面
		 * 1. 关闭fd[0]，
		 * 2. 打开/etc/rc，此时fd[0]=/etc/rc，即标准输入是来自/etc/rc。
		 * 3. 执行/bin/sh。其stdin来自/etc/rc
		 * 如上，实现了执行/etc/rc文件中包含的命令的目的。
		 * 
		 * 注，/etc/rc包含了系统启动时候需要执行的命令（即，开机启动脚本，rc=run command） 
		 * 	see https://unix.stackexchange.com/questions/111611/what-does-the-rc-stand-for-in-etc-rc-d
		 */
		close(0);
		if (open("/etc/rc",O_RDONLY,0))
			_exit(1);
		execve("/bin/sh",argv_rc,envp_rc);//lux 执行shell
		_exit(2);
	}
	//lux 以下父进程
	if (pid>0)
		while (pid != wait(&i))//lux 父进程，等待上面的rc初始化脚本完成
			/* nothing */;
	/**lux 下面开始一个简单的shell交互
	 */
	while (1) {
		if ((pid=fork())<0) {
			printf("Fork failed in init\r\n");
			continue;
		}
		if (!pid) {//lux 子进程,进入shell
			close(0);close(1);close(2);
			setsid();
			(void) open("/dev/tty0",O_RDWR,0);
			(void) dup(0);//lux 0,1,2	stdin,stdout,stderr 都指向/dev/tty0
			(void) dup(0);
			_exit(execve("/bin/sh",argv,envp));
		}
		while (1)//lux 父进程
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();
	}
	_exit(0);	/* NOTE! _exit, not exit() */
}
