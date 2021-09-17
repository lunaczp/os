#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64 /*lux 目前系统最多支持64个任务/进程*/
#define HZ 100

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <signal.h>

#if (NR_OPEN > 32)
#error "Currently the close-on-exec-flags are in one word, max 32 files/proc"
#endif

#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_ZOMBIE		3
#define TASK_STOPPED		4

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern int copy_page_tables(unsigned long from, unsigned long to, long size);
extern int free_page_tables(unsigned long from, unsigned long size);

extern void sched_init(void);
extern void schedule(void);
extern void trap_init(void);
extern void panic(const char * str);
extern int tty_write(unsigned minor,char * buf,int count);

typedef int (*fn_ptr)();

struct i387_struct {
	long	cwd;
	long	swd;
	long	twd;
	long	fip;
	long	fcs;
	long	foo;
	long	fos;
	long	st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */
};
/*lux 进程上下文结构体，当进程发生切换时，上下文信息被cpu自动存储到这里/或从这里读取，由intel定义
 * see Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide ch7.2 Task Management Data Structures
 **/
struct tss_struct {
	long	back_link;	/* 16 high bits zero */
	long	esp0;
	long	ss0;		/* 16 high bits zero */
	long	esp1;
	long	ss1;		/* 16 high bits zero */
	long	esp2;
	long	ss2;		/* 16 high bits zero */
	long	cr3;/*lux 进程的页目录表入口*/
	long	eip;
	long	eflags;
	long	eax,ecx,edx,ebx;
	long	esp;
	long	ebp;
	long	esi;
	long	edi;
	long	es;		/* 16 high bits zero */
	long	cs;		/* 16 high bits zero */
	long	ss;		/* 16 high bits zero */
	long	ds;		/* 16 high bits zero */
	long	fs;		/* 16 high bits zero */
	long	gs;		/* 16 high bits zero */
	long	ldt;		/* 16 high bits zero */
	long	trace_bitmap;	/* bits: trace 0, bitmap 16-31 */
	struct i387_struct i387;
};
/*lux 进程结构体*/
struct task_struct {
/* these are hardcoded - don't touch */
	long state;	/* -1 unrunnable, 0 runnable, >0 stopped */
	long counter;/*lux 进程时间片*/
	long priority;
	long signal;
	struct sigaction sigaction[32];
	long blocked;	/* bitmap of masked signals */
/* various fields */
	int exit_code;
	unsigned long start_code,end_code,end_data,brk,start_stack;
	long pid,father,pgrp,session,leader;//lux session（session id，一般=session leader的pid）, leader（是否session leader）, pgrp（进程组组长pid）
	unsigned short uid,euid,suid;
	unsigned short gid,egid,sgid;
	long alarm;
	long utime,stime,cutime,cstime,start_time;
	unsigned short used_math;
/* file system info */
	int tty;		/* -1 if no tty, so it must be signed */ //lux index in tty_table,[0-2],see tty_io.c:56
	unsigned short umask;
	struct m_inode * pwd;
	struct m_inode * root;
	struct m_inode * executable;
	unsigned long close_on_exec;//lux 针对filp[] 的bitmap，在调用execve的时候(执行新的二进制文件)，是否关闭对应的打开文件。默认不关闭。
	struct file * filp[NR_OPEN];//lux 打开文件句柄，保存的是指向系统打开文件表的指针
/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
	struct desc_struct ldt[3];
/* tss for this task */
	struct tss_struct tss;
};

/**lux
 * ldt
 * ldt[0] cs, 0x9f,0xc0fa00 => 0x00c0 fa00 0000 009f
 * 00000000 11000000 11111010 00000000 00000000 00000000 00000000 10011111
 * G = 1
 * limit = 10011111,size = (limit +1)*4k = (2^7+2^5)*4k = 5*2^7k = 640k
 * init task的代码段基址是0x0，段长是640k
 * ldt[1] ds, 0x9f,0xc0f200 类似。
 * 
 * 注，结合head.s设置的内核代码段对比可知：
 * 内核代码段/数据段，基址0x0，段长16MB
 * task0（init task）的代码段/数据段，基址是0x0，段长640KB
 * //内核肯定是需要访问所有物理内存的，所以必须是16MB。task0应该无所谓，都可以，这里限制到了640kb的范围，更安全。
 * 
 * tss: 内核态堆栈：
 * tss.esp0 = PAGE_SIZE + (long)&init_task init_task是个union结构，底部是个堆栈。这个取法，拿到的是init_task的底部，即栈顶，see sched.c:53
 * tss.ess0 = 0x10 gdt 第3个条目，内核data段。
 * 
 * tss: 页表
 * cr3 = &pg_dir 使用的是head.s设置的页表(所有进程共享一个页表)
 * 
 * tss: 寄存器
 * eip eflags = 0
 * eax,ecx,edx,ebx = 0
 * esp ebp esi edi = 0
 * es cs ss ds fs gs = 0x17 ldt内第1个条目
 * 注意，tss内只是初始值，没作用。当首次进程切换（task0被切换掉）的时候，真正的值会被cpu压入tss。
 */
/*
 *  INIT_TASK is used to set up the first task table, touch at
 * your own risk!. Base=0, limit=0x9ffff (=640kB)
 */
#define INIT_TASK \
/* state etc */	{ 0,15,15, \
/* signals */	0,{{},},0, \
/* ec,brk... */	0,0,0,0,0,0, \
/* pid etc.. */	0,-1,0,0,0, \
/* uid etc */	0,0,0,0,0,0, \
/* alarm */	0,0,0,0,0,0, \
/* math */	0, \
/* fs info */	-1,0022,NULL,NULL,NULL,0, \
/* filp */	{NULL,}, \
	{ \
		{0,0}, \
/*lux cs*//* ldt */	{0x9f,0xc0fa00}, \
/*lux ds&&ss*/		{0x9f,0xc0f200}, \
	}, \
/*tss*/	{0,PAGE_SIZE+(long)&init_task,0x10,0,0,0,0,(long)&pg_dir,\
	 0,0,0,0,0,0,0,0, \
	 0,0,0x17,0x17,0x17,0x17,0x17,0x17, \
	 _LDT(0),0x80000000, \
		{} \
	}, \
}/*lux tss.cr3=&pg_dir*/

extern struct task_struct *task[NR_TASKS];
extern struct task_struct *last_task_used_math;
extern struct task_struct *current;
extern long volatile jiffies;
extern long startup_time;

#define CURRENT_TIME (startup_time+jiffies/HZ)

extern void add_timer(long jiffies, void (*fn)(void));
extern void sleep_on(struct task_struct ** p);
extern void interruptible_sleep_on(struct task_struct ** p);
extern void wake_up(struct task_struct ** p);

/*
 * Entry into gdt where to find first TSS. 0-nul, 1-cs, 2-ds, 3-syscall
 * 4-TSS0, 5-LDT0, 6-TSS1 etc ...
 */
/*
 *lux lldt 需要的操作数是segment selector，其3-15bit是是gdt内的条目偏移。
 * 所以要<<3,低三位是TI、RPL,保持为0
 * 而<<4是因为，gdt是ldt0,tss0,ldt1,tss1。第n个任务的ldtn前面有n个ldt，还有n个tss，所以要乘以2.最终是<<4
 */
#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))
#define ltr(n) __asm__("ltr %%ax"::"a" (_TSS(n)))
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))
#define str(n) \
__asm__("str %%ax\n\t" \
	"subl %2,%%eax\n\t" \
	"shrl $4,%%eax" \
	:"=a" (n) \
	:"a" (0),"i" (FIRST_TSS_ENTRY<<3))
/*
 *	switch_to(n) should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it does nothing.
 * This also clears the TS-flag if the task we switched to has used
 * tha math co-processor latest.
 */
/**lux 首先要修改_current，指向新的进程
 * 然后，最关键的：ljmp %0 会切换到tmp包含的tss对应的进程处。这是intel 8086的task switch机制
 * 切换会先把旧的上下文(最初由ltr加载）保存到旧进程的tss中，然后根据新的tss加载新的进程上下文。
 * >Saves the state of the current (old) task in the current task’s TSS. The processor finds the base address of the current TSS in the task register and then 
 * copies the states of the following registers into the current TSS: all the general-purpose registers, segment selectors from the segment registers, 
 * the temporarily saved image of the EFLAGS register, and the instruction pointer register (EIP).
 * see:
 * Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide chp7.3 Task Switching
 * 
 * 由上可知，`ljmp %0`下面的一条命令，是所有被切换进程的切口。也就是所有被切换掉的进程重新运行时候，eip的指向，其实是这里。`cmpl ...`
 */
#define switch_to(n) {\
struct {long a,b;} __tmp; \
__asm__("cmpl %%ecx,_current\n\t" \
	"je 1f\n\t" \
	"movw %%dx,%1\n\t" \
	"xchgl %%ecx,_current\n\t" \
	"ljmp %0\n\t" \
	"cmpl %%ecx,_last_task_used_math\n\t" \
	"jne 1f\n\t" \
	"clts\n" \
	"1:" \
	::"m" (*&__tmp.a),"m" (*&__tmp.b), \
	"d" (_TSS(n)),"c" ((long) task[n])); \
}

#define PAGE_ALIGN(n) (((n)+0xfff)&0xfffff000)

#define _set_base(addr,base) \
__asm__("movw %%dx,%0\n\t" \
	"rorl $16,%%edx\n\t" \
	"movb %%dl,%1\n\t" \
	"movb %%dh,%2" \
	::"m" (*((addr)+2)), \
	  "m" (*((addr)+4)), \
	  "m" (*((addr)+7)), \
	  "d" (base) \
	:"dx")

#define _set_limit(addr,limit) \
__asm__("movw %%dx,%0\n\t" \
	"rorl $16,%%edx\n\t" \
	"movb %1,%%dh\n\t" \
	"andb $0xf0,%%dh\n\t" \
	"orb %%dh,%%dl\n\t" \
	"movb %%dl,%1" \
	::"m" (*(addr)), \
	  "m" (*((addr)+6)), \
	  "d" (limit) \
	:"dx")

#define set_base(ldt,base) _set_base( ((char *)&(ldt)) , base )
#define set_limit(ldt,limit) _set_limit( ((char *)&(ldt)) , (limit-1)>>12 )

#define _get_base(addr) ({\
unsigned long __base; \
__asm__("movb %3,%%dh\n\t" \
	"movb %2,%%dl\n\t" \
	"shll $16,%%edx\n\t" \
	"movw %1,%%dx" \
	:"=d" (__base) \
	:"m" (*((addr)+2)), \
	 "m" (*((addr)+4)), \
	 "m" (*((addr)+7))); \
__base;})

#define get_base(ldt) _get_base( ((char *)&(ldt)) )
/**lux 获取指定段limit  
 * +1是因为gdt存的是边界(limit)，而不是size。我们期望返回的其实是size
 * 如[0,0xfff],则limit=0xfff,size=limit+1. see head.s:292
 * 另，lsll返回的是解析过的值，以byte为单位（segment descriptor内可以设置以4k或byte为单位。内核段和task0都是以4k为单位）
 */
#define get_limit(segment) ({ \
unsigned long __limit; \
__asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment)); \
__limit;})

#endif
