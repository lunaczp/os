/*
 *  linux/kernel/system_call.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  system_call.s  contains the system-call low-level handling routines.
 * This also contains the timer-interrupt handler, as some of the code is
 * the same. The hd- and flopppy-interrupts are also here.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call. Ordinary interrupts
 * don't handle signal-recognition, as that would clutter them up totally
 * unnecessarily.
 *
 * Stack layout in 'ret_from_system_call':
 *
 *	 0(%esp) - %eax
 *	 4(%esp) - %ebx
 *	 8(%esp) - %ecx
 *	 C(%esp) - %edx
 *	10(%esp) - %fs
 *	14(%esp) - %es
 *	18(%esp) - %ds
 *	1C(%esp) - %eip
 *	20(%esp) - %cs
 *	24(%esp) - %eflags
 *	28(%esp) - %oldesp
 *	2C(%esp) - %oldss
 */
 /*lux
  * system call 是通过int 80中断实现的。int中断的实现类似于far call，不同的是，int中断会把EFLAGS入栈（在返回地址前），而far call不会
  * far call stack:
  * 0 ip
  * 1 cs
  * 2 esp
  * 3 ss
  * int中断 stack:
  * 0 ip
  * 1 cs
  * 2 EFLAGS
  * 3 esp
  * 4 ss
  * see: Intel 64 and IA-32 Architectures Software Developer's Manual Volume 2-Instruction Set Reference int
  * The action of the INT n instruction (including the INTO and INT 3 instructions) is similar to that of a far call made with the CALL instruction. 
  * The primary difference is that with the INT n instruction, the EFLAGS register is pushed onto the stack before the return address.
  * (The return address is a far address consisting of the current values of the CS and EIP registers.) Returns from interrupt procedures
  * are handled with the IRET instruction, which pops the EFLAGS information and return address from the stack.
  * 从而 ret_from_system_call的堆栈如最上面的注释所说。
  */

SIG_CHLD	= 17

EAX		= 0x00
EBX		= 0x04
ECX		= 0x08
EDX		= 0x0C
FS		= 0x10
ES		= 0x14
DS		= 0x18
EIP		= 0x1C
CS		= 0x20
EFLAGS		= 0x24
OLDESP		= 0x28
OLDSS		= 0x2C

state	= 0		# these are offsets into the task-struct.
counter	= 4
priority = 8
signal	= 12
sigaction = 16		# MUST be 16 (=len of sigaction)
blocked = (33*16)

# offsets within sigaction
sa_handler = 0
sa_mask = 4
sa_flags = 8
sa_restorer = 12

nr_system_calls = 72 /*lux system call, 72个*/

/*
 * Ok, I get parallel printer interrupts while using the floppy for some
 * strange reason. Urgel. Now I just ignore them.
 */
.globl _system_call,_sys_fork,_timer_interrupt,_sys_execve
.globl _hd_interrupt,_floppy_interrupt,_parallel_interrupt
.globl _device_not_available, _coprocessor_error

.align 2
bad_sys_call:
	movl $-1,%eax
	iret
.align 2
reschedule:/*lux 进程切换的一个入口*/
	pushl $ret_from_sys_call /*lux 入栈返回地址，那么在jmp到的代码执行完，调用ret，就可以返回到指定的返回地址了*/
	jmp _schedule /*lux 重新运行调度程序 see sched.c:104*/
.align 2
_system_call:/*lux int 80 system_call here*/
	cmpl $nr_system_calls-1,%eax
	ja bad_sys_call
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx		# push %ebx,%ecx,%edx as parameters
	pushl %ebx		# to the system call
	movl $0x10,%edx		# set up ds,es to kernel space # lux setup segment
	mov %dx,%ds
	mov %dx,%es
	movl $0x17,%edx		# fs points to local data space #lux 指向用户数据段，方便kernel代码通过fs对用户态的数据（如传入的参数）进行寻址。参见get_fs_byte()
	mov %dx,%fs
	call _sys_call_table(,%eax,4) # lux 调用注册过的系统函数 see sys.h:75
	pushl %eax
	movl _current,%eax # lux current赋值给eax，current指向的是当前进程的 task_struct
	cmpl $0,state(%eax)		# state # lux 获取 task_struct->state 
	jne reschedule # lux 如果status!=0 则重新调度 status=0 runnable, see sched.h:78
	cmpl $0,counter(%eax)		# counter # lux task_struct->counter, 如果counter=0，即时间片用完，则重新调度
	je reschedule
ret_from_sys_call: # lux 返回前，处理信号,不管是否reschedule了，最终都会会走到这里。
	movl _current,%eax		# task[0] cannot have signals
	cmpl _task,%eax
	je 3f
	cmpw $0x0f,CS(%esp)		# was old code segment supervisor ?
	jne 3f
	cmpw $0x17,OLDSS(%esp)		# was stack segment = 0x17 ?
	jne 3f
	movl signal(%eax),%ebx # lux 处理信号
	movl blocked(%eax),%ecx
	notl %ecx
	andl %ebx,%ecx
	bsfl %ecx,%ecx
	je 3f
	btrl %ecx,%ebx
	movl %ebx,signal(%eax)
	incl %ecx
	pushl %ecx
	call _do_signal # lux 调用信号处理函数
	popl %eax
3:	popl %eax # lux 之前手动入栈的，都再手动出栈
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	iret # interrupt gate 和 trap gate 都是用iret来返回，与ret不同的是，会设置EFLAGS

.align 2
_coprocessor_error:
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	pushl $ret_from_sys_call
	jmp _math_error

.align 2
_device_not_available:
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	pushl $ret_from_sys_call
	clts				# clear TS so that we can use math
	movl %cr0,%eax
	testl $0x4,%eax			# EM (math emulation bit)
	je _math_state_restore
	pushl %ebp
	pushl %esi
	pushl %edi
	call _math_emulate
	popl %edi
	popl %esi
	popl %ebp
	ret

.align 2
_timer_interrupt: # lux 时钟中断
	push %ds		# save ds,es and put kernel data space
	push %es		# into them. %fs is used by _system_call
	push %fs
	pushl %edx		# we save %eax,%ecx,%edx as gcc doesn't
	pushl %ecx		# save those across function calls. %ebx
	pushl %ebx		# is saved as we use that in ret_sys_call
	pushl %eax
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	incl _jiffies
	movb $0x20,%al		# EOI to interrupt controller #1
	outb %al,$0x20
	movl CS(%esp),%eax
	andl $3,%eax		# %eax is CPL (0 or 3, 0=supervisor)
	pushl %eax
	call _do_timer		# 'do_timer(long CPL)' does everything from
	addl $4,%esp		# task switching to accounting ...
	jmp ret_from_sys_call

.align 2
_sys_execve: # lux execve 系统调用
	lea EIP(%esp),%eax # lux 获取堆栈内eip的值，赋给eax
	pushl %eax # lux 上面获取的eip入栈
	call _do_execve
	addl $4,%esp # lux 丢弃上面手动入栈的eip
	ret

.align 2
_sys_fork: # lux fork 系统调用实现
	call _find_empty_process # lux see fork.c:134
	testl %eax,%eax # lux 测试返回值是否是负数 test: temp->src1 AND src2; SF<-MSB(TEMP)
	js 1f # lux js: jump near sign(sf=1)
	push %gs
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %eax
	call _copy_process
	addl $20,%esp # lux push的丢掉即可
1:	ret

_hd_interrupt: # lux 硬盘中断
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	movb $0x20,%al
	outb %al,$0xA0		# EOI to interrupt controller #1
	jmp 1f			# give port chance to breathe
1:	jmp 1f
1:	xorl %edx,%edx
	xchgl _do_hd,%edx # lux 硬盘中断回调
	testl %edx,%edx
	jne 1f
	movl $_unexpected_hd_interrupt,%edx # lux _do_hd为空，非法
1:	outb %al,$0x20
	call *%edx		# "interesting" way of handling intr.
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

_floppy_interrupt: # lux 软盘中断
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	movl $0x17,%eax
	mov %ax,%fs
	movb $0x20,%al
	outb %al,$0x20		# EOI to interrupt controller #1
	xorl %eax,%eax
	xchgl _do_floppy,%eax # lux 软盘中断回调
	testl %eax,%eax
	jne 1f
	movl $_unexpected_floppy_interrupt,%eax
1:	call *%eax		# "interesting" way of handling intr.
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

_parallel_interrupt:
	pushl %eax
	movb $0x20,%al
	outb %al,$0x20
	popl %eax
	iret
