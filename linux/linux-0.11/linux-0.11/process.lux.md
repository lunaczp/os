# 进程（Process）

## 基本概念
操作系统上层的所有任务，都以进程的形式存在。进程是用户业务逻辑/用户使用计算机资源的基本单元/ID。进程是参与系统使用的参与方。


## 实现
每个进程都对应一个进程体，包含了进程相关的所有信息。
- 基本信息
    - 可执行文件
    - 根目录
    - 当前目录
    - 代码段信息
    - 数据段信息
- 运行信息
    - 运行状态
    - 时间片
    - 信号
    - 退出码
    - pid
    - 父进程
    - 打开文件数组
- 其他状态信息
    - ldt
    - tss

[include/sched.h](include/sched.h)
```
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
	long pid,father,pgrp,session,leader;
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
	unsigned long close_on_exec;
	struct file * filp[NR_OPEN];//lux 打开文件句柄，保存的是指向系统打开文件表的指针
/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
	struct desc_struct ldt[3];
/* tss for this task */
	struct tss_struct tss;
};
```

另外，全局维护一个`current`指针，指向当前运行中的进程结构体。


# 进程切换

## 理论

### 上帝视角
上帝面前有一个盒子，盒子里可以运行一个任务，盒子有5个抽屉，可以存储任务相关的东西。旁边排队等了很多任务。
每一次，上帝要切换任务的时候
- 停止当前任务，把任务和抽屉里的东西标记并打包，放到一边。
- 把想要运行的任务放到盒子里，解包出打包的东西，放到对应的5个盒子里。
- 运行

整个系统就是任务不断切换执行的过程。盒子和抽屉就是CPU和各种寄存器资源。

### 任务视角
- __而对于每个任务而言，就好像从没停止过一样。__一直在线性运行下去。
- 但由于内存和外设（硬盘等）是共享的，所以进程有必要处理资源竞争问题。也就是说，任务在运行的时候可以认为是独占cpu的，但是涉及到对内存和外设的访问，应该假设访问不是独占的，从而处理好资源竞争问题。

### 其他
- 一个进程有内核态和用户态（当陷入内核的时候，是内核态），注意，这个切换本身跟进程调度是无关的。
- 其实内核代码和用户代码作为一个整体被进程调度。
- 需要注意的是，由于Linux0.11是不可抢占内核，在时钟中断处理程序中，会判断如果处于内核态`cpl=0`，则不进行进程调度，直接返回；否则才有可能触发进程调度。
```
|process    0   |process    1   |process    2   |
|user   code    |user   code    |user   code    |
|kernel code    |kernel code    |kernel code    |
----------------|---------------|---------------|
|switch unit    |switch unit    |switch unit    |
```


## 实现

### 基本说明
Linux的进程切换是通过CPU的`Task Switch`实现的。  

Linux多任务情况下，所有的代码执行都是以任务为单位/纬度的，都处于某个任务下。多任务切换通过`Task Switch`切换。
- 每个任务都有自己的tss。tss用于保存和加载进程的上下文，如`EFLAGS,AX,BX,CX,DX,cs,ds,es,fs,gs,esp,eip,esi,edi...`，tss保留任务执行所需要的所有数据。保证进程能够被停止,以及被重新唤起并从断点继续执行。
- 每个任务都有自己的内核栈和用户栈。内核栈是进程在内核态执行时使用的堆栈；用户栈是进程在用户态执行时使用的堆栈。

[include/sched.h](include/sched.h)
```
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
```
如上，`ljmp`实现了task switch。旧的上下文被保留到旧的tss；新的是上下文从新的tss中加载。另外全局`current`变量，也更新为新的进程。

### 切换入口
进程切换通过调用`schedule`函数实现，那么都有那么地方会触发`schedule`呢。

#### 时钟中断
定时触发的时钟中断方便进行进程调度。每当时钟中断触发：
- `--current->counter` 每个进程都有一个时间片数量，每次时钟中断，当前进程时间片减一。
- 当`counter`为0，调用`schedule`。

#### 系统调用，从内核态返回用户态
当系统调用完毕，准备返回用户态之前，系统会检查
- 当前进程是否`runnable`，不是则调用`schedule`重新调度
- 当前进程`counter`是否为0，是则调用`schedule`重新调度
- 否则正常返回用户态

#### 进程退出
一个进程退出的时候，会调用`do_exit`，而它最终会调用`schedule`触发进程调度

#### 等待资源
当等待某个资源可用的时候，可以调用`sleep_on`，它内存会调用`schedule`。（这个就是一个阻塞调用的场景。当资源可用的时候，操作系统会唤起进程，在此之前，进程一直阻塞。）

- 缓存系统，等待一个block可用的情况
- 文件系统，等待一个inode节点可用的情况

# 定时器
[sched.c](kernel/sched.c)中还实现了一个定时器功能，用来定时触发一些任务。目前主要是floppy驱动在用。

## 实现
- 系统维护一个定时器列表`timer_list[64]`。
- 其他程序可以添加自己的任务进来`void add_timer(long jiffies, void (*fn)(void))`
	- `add_timer`会维护`timer_list`，保证单链表有序，时间最短的在前面。
- 每次时钟中断，都会调用`do_timer`，内部会检查定时器列表：
	- 减小列表内任务的等待时间
	- 当发现到期的任务，则调用回调。

更多细节，参考[sched.c](kernel/sched.c)


# 其他

## 会话 Session
Session 一般是一个shell session。因为一般登陆系统后，shell是第一个要启动的程序，是和操作系统交互的窗口。所以，
- 一次登陆，开启一个shell，就开启了一个session，其标记就是shell进程的pid
- 在shell下执行的所有操作，都属于同一个session。
- 一个session，有一个session leader，一般地，就是最开始启动的那个shell。
- 当session leader退出后，所有同一个session的进程都会收到SIGHUP信号，通知其退出。
	- 这也是为什么，当ssh登陆到系统后，执行一下耗时的命令。当ssh退出的时候，未执行完的命令可能就直接退出了。

Session的设计初衷其实很简单，不用过分解读。

另外，`setsid`系统调用，可以启动一个新session，脱离旧的session环境。

### 实现
`task_struct`内标记了相关字段
```
struct task_struct {
	...
	long pid,father,pgrp,session,leader;//lux session（session id，一般=session leader的pid）, leader（是否是session leader）, pgrp（进程组组长的pid）
```

`setsid`的实现
[kernel/sys.c](kernel/sys.c)
```
int sys_setsid(void)
{
	if (current->leader && !suser())
		return -EPERM;
	current->leader = 1;
	current->session = current->pgrp = current->pid;
	current->tty = -1;
	return current->pgrp;
}
```

## 进程组 Process Group
进程组的概念方便对一组进程进行控制。
- 一般地，新启动的一个进程，会单独分配一个进程组，其标记就是进程的pid，进程组的组长也就是该进程。
- 当进程内通过`fork`启动子进程的时候，其属于父进程的进程组，组长是父进程。
- 如此，进程自己启动的进程属于同一个进程组，方便对进程控制。
- 当给一个进程组组长发信号的时候，组内所有进程都会收到。(when kill father's pid, all children receive kill signal)

## Quote
> In a POSIX-conformant operating system, a process group denotes a collection of one or more processes. Among other things, a process group is used to control the distribution of a signal; when a signal is directed to a process group, the signal is delivered to each process that is a member of the group.

> Similarly, a session denotes a collection of one or more process groups. A process group is not permitted to migrate from one session to another, and a process may not create a process group that belongs to another session; furthermore, a process is not permitted to join a process group that is a member of another session—that is, a process is not permitted to migrate from one session to another.

> When a process replaces its image with a new image (by calling one of the exec functions), the new image is subjected to the same process group (and thus session) membership as the old image.


## 可执行文件的加载
系统启动后，所有的可执行文件加载都是通过`execve`系统调用为入口。举例：
```
//main.c
...
static char * argv_rc[] = { "/bin/sh", NULL };
static char * envp_rc[] = { "HOME=/", NULL };
...
		execve("/bin/sh",argv_rc,envp_rc);//lux 执行shell

```
`execve`系统调用，指定三个参数
- 可执行文件路径
- argv参数数组
- envp环境变量数组

### `execve`的实现
- 读取文件
	- 如果发现是脚本文件，解析之，重新构造请求。
- 判断文件是可执行文件（a.out格式）
- 分配参数堆栈空间(32Page=128kb)
- 复制参数和环境变量到参数空间
- 清理
```
	if (current->executable)
		iput(current->executable);//lux 移除当前的可执行文件
	current->executable = inode;//lux 新的可执行文件
	for (i=0 ; i<32 ; i++)
		current->sigaction[i].sa_handler = NULL;//lux reset sig
	for (i=0 ; i<NR_OPEN ; i++)
		if ((current->close_on_exec>>i)&1)//lux 需要关闭
			sys_close(i);//lux close file
	current->close_on_exec = 0;//lux reset默认值。
	free_page_tables(get_base(current->ldt[1]),get_limit(0x0f));//lux free原有内存映射。这样当执行新的代码时，就会触发缺页中断，然后由中断程序去加载可执行文件，参考mm/memory.c
	free_page_tables(get_base(current->ldt[2]),get_limit(0x17));
	if (last_task_used_math == current)
		last_task_used_math = NULL;
	current->used_math = 0;
```
- 构造调用栈
```
	p = (unsigned long) create_tables((char *)p,argc,envc);//lux 构造参数堆栈，具体内存布局见create_tables
```
- 设定其他选项
- 设定代码入口和堆栈，返回
```
	eip[0] = ex.a_entry;		/* eip, magic happens :-) */ //lux 修改堆栈内的eip。
	eip[3] = p;			/* stack pointer */ //lux 修改堆栈指针
	return 0;
```

更多细节，参考[fs/exec.c](fs/exec.c)代码及注释。

