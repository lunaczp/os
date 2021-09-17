/**lux fcntl 系统调用，包括fcntl，dup...
 */
/*
 *  linux/fs/fcntl.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <string.h>
#include <errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <fcntl.h>
#include <sys/stat.h>

extern int sys_close(int fd);
/**lux 拷贝打开文件句柄，返回新句柄（表内偏移）
 * 复制当前进程打开文件表内的指定句柄。
 * 输入：
 * 	fd 需要复制的句柄
 * 	arg 开始偏移
 * 输出：
 * 	新句柄(arg)
 */
static int dupfd(unsigned int fd, unsigned int arg)
{
	if (fd >= NR_OPEN || !current->filp[fd])
		return -EBADF;
	if (arg >= NR_OPEN)
		return -EINVAL;
	while (arg < NR_OPEN)//lux 找到空闲项
		if (current->filp[arg])
			arg++;
		else
			break;
	if (arg >= NR_OPEN)
		return -EMFILE;
	current->close_on_exec &= ~(1<<arg);
	(current->filp[arg] = current->filp[fd])->f_count++;//lux 复制到新表项
	return arg;
}

int sys_dup2(unsigned int oldfd, unsigned int newfd)
{
	sys_close(newfd);
	return dupfd(oldfd,newfd);
}
//lux sys_dup
int sys_dup(unsigned int fildes)
{
	return dupfd(fildes,0);
}

int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg)
{	
	struct file * filp;

	if (fd >= NR_OPEN || !(filp = current->filp[fd]))
		return -EBADF;
	switch (cmd) {
		case F_DUPFD:
			return dupfd(fd,arg);
		case F_GETFD:
			return (current->close_on_exec>>fd)&1;
		case F_SETFD:
			if (arg&1)
				current->close_on_exec |= (1<<fd);
			else
				current->close_on_exec &= ~(1<<fd);
			return 0;
		case F_GETFL:
			return filp->f_flags;
		case F_SETFL:
			filp->f_flags &= ~(O_APPEND | O_NONBLOCK);
			filp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
			return 0;
		case F_GETLK:	case F_SETLK:	case F_SETLKW:
			return -1;
		default:
			return -1;
	}
}
