# System Call
系统调用是怎么实现的呢。当你在C语言中调用`write`的时候，实际是发生了什么呢。

## `write`的例子

### 用户代码 =》标准库
C代码中调用`write`，使用的是C标准库中的`write`函数。具体实现因平台而已，在Linux上，主要是glibc实现。

### 标准库 =》 操作系统调用
标准库中会使用OS提供的系统调用

OS提供的`write`调用

```
//lib/write.c

#define __LIBRARY__
#include <unistd.h>

_syscall3(int,write,int,fd,const char *,buf,off_t,count)

```

### 操作系统处理

#### 触发80中断
```
//include/unistd.h

#define _syscall3(type,name,atype,a,btype,b,ctype,c) \
type name(atype a,btype b,ctype c) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((long)(a)),"c" ((long)(b)),"d" ((long)(c))); \
if (__res>=0) \
	return (type) __res; \
errno=-__res; \
return -1; \
}
```

其中`__NR_##name`定义了系统调用编号
```
//include/unistd.h
#define __NR_setup	0	/* used only by init, to get system going */
#define __NR_exit	1
#define __NR_fork	2
#define __NR_read	3
#define __NR_write	4
#define __NR_open	5
#define __NR_close	6
#define __NR_waitpid	7
#define __NR_creat	8
#define __NR_link	9
#define __NR_unlink	10
#define __NR_execve	11
#define __NR_chdir	12
#define __NR_time	13
#define __NR_mknod	14
#define __NR_chmod	15
#define __NR_chown	16
#define __NR_break	17
#define __NR_stat	18
#define __NR_lseek	19
#define __NR_getpid	20
#define __NR_mount	21
#define __NR_umount	22
#define __NR_setuid	23
#define __NR_getuid	24
#define __NR_stime	25
#define __NR_ptrace	26
#define __NR_alarm	27
#define __NR_fstat	28
#define __NR_pause	29
#define __NR_utime	30
#define __NR_stty	31
#define __NR_gtty	32
#define __NR_access	33
#define __NR_nice	34
#define __NR_ftime	35
#define __NR_sync	36
#define __NR_kill	37
#define __NR_rename	38
#define __NR_mkdir	39
#define __NR_rmdir	40
#define __NR_dup	41
#define __NR_pipe	42
#define __NR_times	43
#define __NR_prof	44
#define __NR_brk	45
#define __NR_setgid	46
#define __NR_getgid	47
#define __NR_signal	48
#define __NR_geteuid	49
#define __NR_getegid	50
#define __NR_acct	51
#define __NR_phys	52
#define __NR_lock	53
#define __NR_ioctl	54
#define __NR_fcntl	55
#define __NR_mpx	56
#define __NR_setpgid	57
#define __NR_ulimit	58
#define __NR_uname	59
#define __NR_umask	60
#define __NR_chroot	61
#define __NR_ustat	62
#define __NR_dup2	63
#define __NR_getppid	64
#define __NR_getpgrp	65
#define __NR_setsid	66
#define __NR_sigaction	67
#define __NR_sgetmask	68
#define __NR_ssetmask	69
#define __NR_setreuid	70
#define __NR_setregid	71
```

#### 中断处理

提前注册好的80中断处理例程
```
//sched.c
	set_system_gate(0x80,&system_call);/*lux 注册系统调用_system_call到80中断 */
```

从`_sys_call_table`中找到真正的处理函数。偏移量就是上面的`__NR`定义的。他们一一对应

```
//system_call.s

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

```

```
//sys.h
/*lux 系统函数表*/
fn_ptr sys_call_table[] = { sys_setup, sys_exit, sys_fork, sys_read,
sys_write, sys_open, sys_close, sys_waitpid, sys_creat, sys_link,
sys_unlink, sys_execve, sys_chdir, sys_time, sys_mknod, sys_chmod,
sys_chown, sys_break, sys_stat, sys_lseek, sys_getpid, sys_mount,
sys_umount, sys_setuid, sys_getuid, sys_stime, sys_ptrace, sys_alarm,
sys_fstat, sys_pause, sys_utime, sys_stty, sys_gtty, sys_access,
sys_nice, sys_ftime, sys_sync, sys_kill, sys_rename, sys_mkdir,
sys_rmdir, sys_dup, sys_pipe, sys_times, sys_prof, sys_brk, sys_setgid,
sys_getgid, sys_signal, sys_geteuid, sys_getegid, sys_acct, sys_phys,
sys_lock, sys_ioctl, sys_fcntl, sys_mpx, sys_setpgid, sys_ulimit,
sys_uname, sys_umask, sys_chroot, sys_ustat, sys_dup2, sys_getppid,
sys_getpgrp, sys_setsid, sys_sigaction, sys_sgetmask, sys_ssetmask,
sys_setreuid,sys_setregid }
```

#### 真正`write`处理

```
//read_write.c
int sys_write(unsigned int fd,char * buf,int count)
{
	struct file * file;
	struct m_inode * inode;
	
	if (fd>=NR_OPEN || count <0 || !(file=current->filp[fd]))
		return -EINVAL;
	if (!count)
		return 0;
	inode=file->f_inode;
	if (inode->i_pipe)//lux pipe
		return (file->f_mode&2)?write_pipe(inode,buf,count):-EIO;
	if (S_ISCHR(inode->i_mode))//lux char
		return rw_char(WRITE,inode->i_zone[0],buf,count,&file->f_pos);
	if (S_ISBLK(inode->i_mode))//lux blk
		return block_write(inode->i_zone[0],&file->f_pos,buf,count);
	if (S_ISREG(inode->i_mode))//lux 普通文件
		return file_write(inode,file,buf,count);
	printk("(Write)inode->i_mode=%06o\n\r",inode->i_mode);
	return -EINVAL;
}
```
