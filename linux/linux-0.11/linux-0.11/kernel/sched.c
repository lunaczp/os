/*
 *  linux/kernel/sched.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'sched.c' is the main kernel file. It contains scheduling primitives
 * (sleep_on, wakeup, schedule etc) as well as a number of simple system
 * call functions (type getpid(), which just extracts a field from
 * current-task
 */
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/fdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

#include <signal.h>

#define _S(nr) (1<<((nr)-1))
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

void show_task(int nr,struct task_struct * p)
{
	int i,j = 4096-sizeof(struct task_struct);

	printk("%d: pid=%d, state=%d, ",nr,p->pid,p->state);
	i=0;
	while (i<j && !((char *)(p+1))[i])
		i++;
	printk("%d (of %d) chars free in kernel stack\n\r",i,j);
}

void show_stat(void)
{
	int i;

	for (i=0;i<NR_TASKS;i++)
		if (task[i])
			show_task(i,task[i]);
}

#define LATCH (1193180/HZ) //lux 8253芯片的输入频率是1193180，系统希望的频率是HZ（100），这里计算出比例，用来设置8253芯片

extern void mem_use(void);

extern int timer_interrupt(void);
extern int system_call(void);

union task_union {
	struct task_struct task;
	char stack[PAGE_SIZE];//lux 利用union结构，构造了一个PAGE_SIZE(4k)大小的空间，上面作为task存储，下面作为堆栈。在task_union的底部作为栈顶，提供一个向下的堆栈，供init_task在内核态使用,see sched.h INIT_TASK tss.esp0
};

static union task_union init_task = {INIT_TASK,};

long volatile jiffies=0;
long startup_time=0;//lux 系统启动时间，由main.c初始化
struct task_struct *current = &(init_task.task);
struct task_struct *last_task_used_math = NULL;
/*lux 进程表task，第一个进程为INIT_TASK*/
struct task_struct * task[NR_TASKS] = {&(init_task.task), };

long user_stack [ PAGE_SIZE>>2 ] ;/*lux 一个4k大小的堆栈(1024个条目，每个4byte)，在系统初始化时候及后来的任务0、1中用作堆栈*/

struct {
	long * a;
	short b;
	} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };/*lux stack_start结构便于使用lss命令来设置堆栈，栈顶是user_stack[1024]，向下生长，0x10对应gdt中第3个表项（data段），参见head.s:23*/
/*
 *  'math_state_restore()' saves the current math information in the
 * old math state array, and gets the new ones from the current task
 */
void math_state_restore()
{
	if (last_task_used_math == current)
		return;
	__asm__("fwait");
	if (last_task_used_math) {
		__asm__("fnsave %0"::"m" (last_task_used_math->tss.i387));
	}
	last_task_used_math=current;
	if (current->used_math) {
		__asm__("frstor %0"::"m" (current->tss.i387));
	} else {
		__asm__("fninit"::);
		current->used_math=1;
	}
}
/*lux schedule 进程调度器*/
/*
 *  'schedule()' is the scheduler function. This is GOOD CODE! There
 * probably won't be any reason to change this, as it should work well
 * in all circumstances (ie gives IO-bound processes good response etc).
 * The one thing you might take a look at is the signal-handler code here.
 *
 *   NOTE!!  Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'
 * information in task[0] is never used.
 */
void schedule(void)
{
	int i,next,c;
	struct task_struct ** p;

/* check alarm, wake up any interruptible tasks that have got a signal */

	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
		if (*p) {
			if ((*p)->alarm && (*p)->alarm < jiffies) {
					(*p)->signal |= (1<<(SIGALRM-1));
					(*p)->alarm = 0;
				}
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
			(*p)->state==TASK_INTERRUPTIBLE)
				(*p)->state=TASK_RUNNING;
		}

/* this is the scheduler proper: */

	while (1) {
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
		while (--i) {
			if (!*--p)
				continue;
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
				c = (*p)->counter, next = i;
		}
		if (c) break;
		for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) +
						(*p)->priority;
	}
	switch_to(next);/*lux 最终的真正的切换在这里 see sched.h:176*/
}
/*Lux task0 idle进程循环调用pause，pause调用schedule，唤醒可用进程*/
int sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}
/*
lux：等待某个资源可用
注意，这里形成了一个等待队列的效果，但并没有使用队列。

*p指向当前等待某一资源的进程。如果有新进程sleep on，那么*p就会变成新进程。如：
A sleep on ResourceX
tmp = *p
*p = A
A sleep

B sleep on Resourcex
tmp = *p (这时候*p=A)
*p = B
B sleep

当最终ResourceX可用时，B被唤醒（state=0,runnable，从schedule的断点继续执行），B会把tmp（A）唤醒（state=0)。
如此，等待ResourceX的进程被一个个唤醒。
这里并没有实现一个请求队列，而是依赖进程的请求顺序，和一个临时变量tmp，天然地把请求连接在了一起。
*/
void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;//lux 没有等待进程，无须排队，则直接返回
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;//lux 将当前进程设为等待资源 *p
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();//lux 睡眠当前进程
	/* lux 下面：
	 * 当该进程被再次唤醒时，该进程会把在这个进程之前就已经等待的进程也唤醒`tmp->state=0;`。
	 * 逐级唤起，那么有个可能的效率问题是：这个资源被第一个唤起的进程候锁住了，那么后面唤起的进程还要继续进入sleep，就没什么意义了。类似“惊群”。
	 * 比如：5个人等待一个资源R。R可用，则5个人都被唤起，第一个人拿到资源，并再次锁住R，其他4个人醒来后发现资源不可用，继续睡眠。如此反复5次直到所有人都拿到R一次。但其实这5次每次都唤醒所有进程是没有必要的，因为其实也那得不到。
	 * 当然这种情况的其他是，一个进程被唤起拿到资源后会马上锁住。如果不锁的（共享，读），其实那么5个都被唤起是没问题也是应该的。所以这个设计只是在特定场景下会有一些性能问题，正常情况没问题。
	 */
	if (tmp)
		tmp->state=0;//lux runnable,ready to run
}//lux update 2018.6.22 下面的问题(old)不会发生，因为每次wakeup，都会把全局的**p（内核数据段，所有进程共享）都重置。所以Msleep的时候 tmp=NULL，M是第一个。所以后面当再次wakeup，M就会被唤起。
//另外，sleep和wakeup是线性的。有可能的情况是 
//time:----------------------------------------------->>>------------------------
//process: As,Bs(tmp=A),Cs(tmp=B) wakeup Cw Bw Aw
//process:									Ms(tmp=NULL) Xs(tmp=M) Ys(tmp=X) wakeup Yw Xw Mw
//
//也就是，进程的切换和事件的发生最终都是线形的（因为cpu线性的，而进程是按照cpu的时间片进行的），从而保证最总如上的状态。所有的wakeup都会唤醒之前的进程。而所有的sleep都会自动成链。
//（但如果是多核cpu，出现真正的并发，就需要更严格的控制。比如B C同时sleep（tmp=A)，我们需要对资源进行更多访问控制）
//
//ps.代码执行是以进程为单位的。进程共享内核代码和数据。但每个进程都自己独立的堆栈（包括用户态和内核态）。其实这段代码内的tmp就是在各自进程的内核态堆内维护的。
//
//old:
//这个设计可能不够严谨:sleep和wakeup交替发生，wakeup唤醒之前sleep的对象。谁来保证sleep之后一定有wakeup
//time:		-------------------->>>----------------------
//process:	Asleep	Bsleep(tmp=A)	Csleep(tmp=B)	Cwakeup(be wakeup)	Bwakeup(by C)	Awakeup(by B) 
//event:										wakeup								...(will there be a wakeup for M)
//process:												Msleep(tmp=C M sleep because of C)			

//lux 类似sleep_on,但允许中断
void interruptible_sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp=*p;
	*p=current;
repeat:	current->state = TASK_INTERRUPTIBLE;
	schedule();
	if (*p && *p != current) {
		(**p).state=0;
		goto repeat;
	}
	*p=NULL;
	if (tmp)
		tmp->state=0;
}
/*lux 唤醒等待指定资源的进程，通过直接设置其task结构体的state字段来实现（直接设置为runnable，则在下次schedule的时候就可以被调度了）*/
void wake_up(struct task_struct **p)
{
	if (p && *p) {
		(**p).state=0;//lux 0 runnable
		*p=NULL;//lux 重置全局变量
	}
}

/*
 * OK, here are some floppy things that shouldn't be in the kernel
 * proper. They are here because the floppy needs a timer, and this
 * was the easiest way of doing it.
 */
static struct task_struct * wait_motor[4] = {NULL,NULL,NULL,NULL};
static int  mon_timer[4]={0,0,0,0};//lux 启动耗时
static int moff_timer[4]={0,0,0,0};//lux 关闭等待耗时
/**lux 软驱寄存器
 * 7-4	4个驱动器D-A 马达的驱动：1启动 0关闭
 * 3	1 允许DMA和中断 0 不允许
 * 2	1 启动软盘控制器 0 复位软盘控制器
 * 1-0	选择4个控制器A-D
 * 如此，通过设置0-1，选定软驱；设定7-4，开关马达。
 */
unsigned char current_DOR = 0x0C;//lux 0000 1100:A盘，允许DMA，启动软控

//lux 返回等待floppy启动需要的tick
int ticks_to_floppy_on(unsigned int nr)
{
	extern unsigned char selected;
	unsigned char mask = 0x10 << nr;

	if (nr>3)
		panic("floppy_on: nr>3");
	moff_timer[nr]=10000;		/* 100 s = very big :-) */ //lux 停转等待时间
	cli();				/* use floppy_off to turn it off */
	mask |= current_DOR;//lux 设定7-4，选定要选的。
	if (!selected) {//lux 当前没有选中的软盘，选中需要的那个
		mask &= 0xFC;//lux reset
		mask |= nr;//lux 设定1-0
	}
	if (mask != current_DOR) {//lux 需要设置新的mask
		outb(mask,FD_DOR);
		if ((mask ^ current_DOR) & 0xf0)//lux 选中的那个马达还没启动
			mon_timer[nr] = HZ/2;//lux 等待时间较长，50个tick
		else if (mon_timer[nr] < 2)//lux 等待时间短，设置为2就行
			mon_timer[nr] = 2;
		current_DOR = mask;
	}
	sti();
	return mon_timer[nr];//lux 返回等待的tick数。
}
//lux 等待马达开启
void floppy_on(unsigned int nr)
{
	cli();
	while (ticks_to_floppy_on(nr))
		sleep_on(nr+wait_motor);
	sti();
}
//lux 关闭马达（3s后）
void floppy_off(unsigned int nr)
{
	moff_timer[nr]=3*HZ;
}
//lux floppy时钟中断（由系统时钟中断调用）
void do_floppy_timer(void)
{
	int i;
	unsigned char mask = 0x10;

	for (i=0 ; i<4 ; i++,mask <<= 1) {
		if (!(mask & current_DOR))
			continue;
		if (mon_timer[i]) {
			if (!--mon_timer[i])
				wake_up(i+wait_motor);//lux 启动时间到到，唤起。
		} else if (!moff_timer[i]) {//lux 停转时间到，复位
			current_DOR &= ~mask;
			outb(current_DOR,FD_DOR);
		} else
			moff_timer[i]--;//lux 停转计时
	}
}

#define TIME_REQUESTS 64

static struct timer_list {
	long jiffies;
	void (*fn)();
	struct timer_list * next;
} timer_list[TIME_REQUESTS], * next_timer = NULL;

void add_timer(long jiffies, void (*fn)(void))
{
	struct timer_list * p;

	if (!fn)
		return;
	cli();
	if (jiffies <= 0)
		(fn)();
	else {
		for (p = timer_list ; p < timer_list + TIME_REQUESTS ; p++)//lux 先找到空闲项
			if (!p->fn)
				break;
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time requests free");
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;
		next_timer = p;
		while (p->next && p->next->jiffies < p->jiffies) {//lux 有序插入。jiffies采用增量存储。比如本身是1 3 7 15，那么存储的是：1 2 4 8。
			p->jiffies -= p->next->jiffies;
			fn = p->fn;
			p->fn = p->next->fn;
			p->next->fn = fn;
			jiffies = p->jiffies;
			p->jiffies = p->next->jiffies;
			p->next->jiffies = jiffies;
			p = p->next;
		}
	}
	sti();
}
/**
 * lux 时钟中断处理程序
 */
void do_timer(long cpl)
{
	extern int beepcount;
	extern void sysbeepstop(void);

	if (beepcount)
		if (!--beepcount)
			sysbeepstop();

	if (cpl)
		current->utime++;//user mode
	else
		current->stime++;//kernel mode

	if (next_timer) {//lux 定时器逻辑处理。执行到期的timer，并移动到下一个
		next_timer->jiffies--;
		while (next_timer && next_timer->jiffies <= 0) {
			void (*fn)(void);
			
			fn = next_timer->fn;
			next_timer->fn = NULL;
			next_timer = next_timer->next;
			(fn)();
		}
	}
	if (current_DOR & 0xf0)//lux 软盘马达开着则处理floppy中断
		do_floppy_timer();
	if ((--current->counter)>0) return;//lux 当前进程时间片还有剩余，不切换，直接返回。
	current->counter=0;
	if (!cpl) return;//lux 内核模式，不可抢占/调度。
	schedule();//lux 调度
}

int sys_alarm(long seconds)
{
	int old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;
	return (old);
}

int sys_getpid(void)
{
	return current->pid;
}

int sys_getppid(void)
{
	return current->father;
}

int sys_getuid(void)
{
	return current->uid;
}

int sys_geteuid(void)
{
	return current->euid;
}

int sys_getgid(void)
{
	return current->gid;
}

int sys_getegid(void)
{
	return current->egid;
}

int sys_nice(long increment)
{
	if (current->priority-increment>0)
		current->priority -= increment;
	return 0;
}

void sched_init(void)
{
	int i;
	struct desc_struct * p;

	if (sizeof(struct sigaction) != 16)
		panic("Struct sigaction MUST be 16 bytes");
	set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));/*lux 填充任务0的tss和ldt*/
	set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));
	p = gdt+2+FIRST_TSS_ENTRY;
	for(i=1;i<NR_TASKS;i++) {/*Lux 初始化task列表，并填充gdt（初始化所有task条目在gdt内的ldt和tss，注意当前其实没有那么多task，只不过先在gdt中占位，清理）*/
		task[i] = NULL;/*i=0为INIT TASK，保持不变*/
		p->a=p->b=0;
		p++;
		p->a=p->b=0;
		p++;
	}
/* Clear NT, so that we won't have troubles with that later on */
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");/*lux 清除EFLAFS的NT（Nested Task）位。NT控制递归调用*/
	ltr(0);/*Lux 加载第0个任务的tss和ldt*/
	lldt(0);
	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */ /*lux LATCH用来设置8253芯片，LATCH=1193180/HZ，最终实现的效果是8253的时钟中断频率是HZ（=100，即每10ms一次中断。）*/
	outb(LATCH >> 8 , 0x40);	/* MSB */
	set_intr_gate(0x20,&timer_interrupt);/*lux 注册时钟中断*/
	outb(inb_p(0x21)&~0x01,0x21);/*lux 打开时钟中断引脚*/
	set_system_gate(0x80,&system_call);/*lux 注册系统调用_system_call到80中断 */
}
