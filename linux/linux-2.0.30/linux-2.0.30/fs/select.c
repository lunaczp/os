/*
 * This file contains the procedures for the handling of select
 *
 * Created for Linux based loosely upon Mathius Lattner's minix
 * patches by Peter MacDonald. Heavily edited by Linus.
 *
 *  4 February 1994
 *     COFF/ELF binary emulation. If the process has the STICKY_TIMEOUTS
 *     flag set in its personality we do *not* modify the given timeout
 *     parameter to reflect time remaining.
 */

#include <linux/types.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/personality.h>
#include <linux/mm.h>

#include <asm/segment.h>
#include <asm/system.h>

#define ROUND_UP(x,y) (((x)+(y)-1)/(y))

/*
 * Ok, Peter made a complicated, but straightforward multiple_wait() function.
 * I have rewritten this, taking some shortcuts: This code may not be easy to
 * follow, but it should be free of race-conditions, and it's practical. If you
 * understand what I'm doing here, then you understand how the linux
 * sleep/wakeup mechanism works.
 *
 * Two very simple procedures, select_wait() and free_wait() make all the work.
 * select_wait() is a inline-function defined in <linux/sched.h>, as all select
 * functions have to call it to add an entry to the select table.
 */

/*
 * I rewrote this again to make the select_table size variable, take some
 * more shortcuts, improve responsiveness, and remove another race that
 * Linus noticed.  -- jrs
 */

static void free_wait(select_table * p)
{
	struct select_table_entry * entry = p->entry + p->nr;

	while (p->nr > 0) {
		p->nr--;
		entry--;
		remove_wait_queue(entry->wait_address,&entry->wait);
	}
}

/*
 * The check function checks the ready status of a file using the vfs layer.
 *
 * If the file was not ready we were added to its wait queue.  But in
 * case it became ready just after the check and just before it called
 * select_wait, we call it again, knowing we are already on its
 * wait queue this time.  The second call is not necessary if the
 * select_table is NULL indicating an earlier file check was ready
 * and we aren't going to sleep on the select_table.  -- jrs
 */

static int check(int flag, select_table * wait, struct file * file)
{
	struct inode * inode;
	struct file_operations *fops;
	int (*select) (struct inode *, struct file *, int, select_table *);//lux 声明了一个函数指针变量

	inode = file->f_inode;
	if ((fops = file->f_op) && (select = fops->select))//lux 如果存在select函数，则调用。因为是vfs，底层可能是普通文件（没有select回调 see minix/file.c:minix_file_operations），socket等，对应的回调不同。
		return select(inode, file, flag, wait)
		    || (wait && select(inode, file, flag, NULL));
	if (flag != SEL_EX)//lux 否则，如果没有加互斥锁，则返回正常。
		return 1;
	return 0;//lux 否则失败。
}
/* lux 真正执行select的函数
 * *in, *out, *ex 输入
 * *res_in, *res_out, *res_ex 输出/返回
 *
 * 这里，我们就看到用户调用select的时候，为什么要指定n（要监听的fd的最大值）。
 * 如果不指定，这里的for循环就要遍历该进程所有的fd[]（打开文件），效率很低。如果指定了最大值，那么只需要遍历到该值即可。
 *
 * ps.当然这并不是最好的设计，最好的设计不应该让用户去控制，系统应该自己能完成。
 */
static int do_select(int n, fd_set *in, fd_set *out, fd_set *ex,
	fd_set *res_in, fd_set *res_out, fd_set *res_ex)
{
	int count;
	select_table wait_table, *wait;
	struct select_table_entry *entry;
	unsigned long set;
	int i,j;
	int max = -1;

	j = 0;
	for (;;) {
		i = j * __NFDBITS;
		if (i >= n)//lux 超过最大值，结束
			break;
		set = in->fds_bits[j] | out->fds_bits[j] | ex->fds_bits[j];
		j++;
		for ( ; set ; i++,set >>= 1) {
			if (i >= n)
				goto end_check;
			if (!(set & 1))//lux 没设置，不用监听
				continue;
			if (!current->files->fd[i])//lux 不存在，异常
				return -EBADF;
			if (!current->files->fd[i]->f_inode)//lux 不存在， 异常
				return -EBADF;
			max = i;
		}
	}
end_check:
	n = max + 1;
	/*lux 下面先分配一个页来存储entry。并使用wait_table来管理监听数量（nr）和各个条目（entry）
	 * 在后面的for循环当中，对每个要监听的(fd,event)对，都会调用check，并最终调用fd对应的select回调，在回调里：
	 * 	1. 会从entry页里分配一次，初始化，并nr++
	 * 	2. 把entry.wait加入到fd对应的等待队列(fd.sleep)
	 * 如上，每次for循环，都可能会消耗调entryPage内对几条entry。如果超过（see wait.h:__MAX_SELECT_TABLE_ENTRIES）则check返回失败
	 * 
	 * 这里wait_table的作用就是管理select监听的条目(fd,event)对。一个页面可以存放_MAX_SELECT_TABLE_ENTRIES个，也就是要监听的“读”、“写”、“异常”数组的总数最多是这个值。
	 * see:af_unix.c:unix_select
	 */
	if(!(entry = (struct select_table_entry*) __get_free_page(GFP_KERNEL)))
		return -ENOMEM;//lux 内存不够
	count = 0;
	wait_table.nr = 0;
	wait_table.entry = entry;
	wait = &wait_table;
repeat:
	current->state = TASK_INTERRUPTIBLE;
	for (i = 0 ; i < n ; i++) {
		struct file * file = current->files->fd[i];
		if (!file)
			continue;
		if (FD_ISSET(i,in) && check(SEL_IN,wait,file)) {//lux 是否可读
			FD_SET(i, res_in);
			count++;
			wait = NULL;
		}
		if (FD_ISSET(i,out) && check(SEL_OUT,wait,file)) {//lux 是否可写
			FD_SET(i, res_out);
			count++;
			wait = NULL;
		}
		if (FD_ISSET(i,ex) && check(SEL_EX,wait,file)) {//lux 是否有异常
			FD_SET(i, res_ex);
			count++;
			wait = NULL;
		}
	}
	wait = NULL;
	/* lux 同时满足以下情况，则重新调度（下次继续检测）
	 * 1. 没有检测到事件
	 * 2. 还有时间片
	 * 3. 没有信号
	 * 否则，任何其他情况，都不再处理，返回count。（比如检测到事件了，或者时间片用完了，或者有信号（需要在返回用户态之前由内核调用用户的回调））
	 */
	if (!count && current->timeout && !(current->signal & ~current->blocked)) {
		schedule();
		goto repeat;
	}
	free_wait(&wait_table);
	free_page((unsigned long) entry);
	current->state = TASK_RUNNING;
	return count;
}

/*
 * We do a VERIFY_WRITE here even though we are only reading this time:
 * we'll write to it eventually..
 *
 * Use "int" accesses to let user-mode fd_set's be int-aligned.
 */
static int __get_fd_set(unsigned long nr, int * fs_pointer, int * fdset)
{
	/* round up nr to nearest "int" */
	nr = (nr + 8*sizeof(int)-1) / (8*sizeof(int));
	if (fs_pointer) {
		int error = verify_area(VERIFY_WRITE,fs_pointer,nr*sizeof(int));
		if (!error) {
			while (nr) {
				*fdset = get_user(fs_pointer);//lux 读取一个元素
				nr--;
				fs_pointer++;
				fdset++;
			}
		}
		return error;
	}
	while (nr) {
		*fdset = 0;
		nr--;
		fdset++;
	}
	return 0;
}

static void __set_fd_set(long nr, int * fs_pointer, int * fdset)
{
	if (!fs_pointer)
		return;
	while (nr >= 0) {
		put_user(*fdset, fs_pointer);
		nr -= 8 * sizeof(int);
		fdset++;
		fs_pointer++;
	}
}

/* We can do long accesses here, kernel fdsets are always long-aligned */
static inline void __zero_fd_set(long nr, unsigned long * fdset)
{
	while (nr >= 0) {
		*fdset = 0;
		nr -= 8 * sizeof(unsigned long);
		fdset++;
	}
}		

/*
 * Due to kernel stack usage, we use a _limited_ fd_set type here, and once
 * we really start supporting >256 file descriptors we'll probably have to
 * allocate the kernel fd_set copies dynamically.. (The kernel select routines
 * are careful to touch only the defined low bits of any fd_set pointer, this
 * is important for performance too).
 *
 * Note a few subtleties: we use "long" for the dummy, not int, and we do a
 * subtract by 1 on the nr of file descriptors. The former is better for
 * machines with long > int, and the latter allows us to test the bit count
 * against "zero or positive", which can mostly be just a sign bit test..
 */
typedef struct {
	unsigned long dummy[NR_OPEN/(8*(sizeof(unsigned long)))];
} limited_fd_set;

#define get_fd_set(nr,fsp,fdp) \
__get_fd_set(nr, (int *) (fsp), (int *) (fdp))

#define set_fd_set(nr,fsp,fdp) \
__set_fd_set((nr)-1, (int *) (fsp), (int *) (fdp))

#define zero_fd_set(nr,fdp) \
__zero_fd_set((nr)-1, (unsigned long *) (fdp))

/*lux select系统调用
 */
/*
 * We can actually return ERESTARTSYS instead of EINTR, but I'd
 * like to be certain this leads to no problems. So I return
 * EINTR just for safety.
 *
 * Update: ERESTARTSYS breaks at least the xview clock binary, so
 * I'm trying ERESTARTNOHAND which restart only when you want to.
 */
asmlinkage int sys_select(int n, fd_set *inp, fd_set *outp, fd_set *exp, struct timeval *tvp)
{
	int error;
	limited_fd_set res_in, in;
	limited_fd_set res_out, out;
	limited_fd_set res_ex, ex;
	unsigned long timeout;

	error = -EINVAL;
	if (n < 0)
		goto out;
	if (n > NR_OPEN)/*lux 超过了最大数*/
		n = NR_OPEN;
	if ((error = get_fd_set(n, inp, &in)) ||
	    (error = get_fd_set(n, outp, &out)) ||
	    (error = get_fd_set(n, exp, &ex))) goto out;//lux 拷贝用户参数到内核
	timeout = ~0UL;
	if (tvp) {
		error = verify_area(VERIFY_WRITE, tvp, sizeof(*tvp));
		if (error)
			goto out;
		timeout = ROUND_UP(get_user(&tvp->tv_usec),(1000000/HZ));
		timeout += get_user(&tvp->tv_sec) * (unsigned long) HZ;
		if (timeout)
			timeout += jiffies + 1;
	}
	zero_fd_set(n, &res_in);
	zero_fd_set(n, &res_out);
	zero_fd_set(n, &res_ex);
	current->timeout = timeout;//lux 设定当前进程的timeout为用户设置的超时
	error = do_select(n,
		(fd_set *) &in,
		(fd_set *) &out,
		(fd_set *) &ex,
		(fd_set *) &res_in,
		(fd_set *) &res_out,
		(fd_set *) &res_ex);//lux 当返回的时候，要么是监听到事件了，要么是出错了。
	timeout = current->timeout - jiffies - 1;
	current->timeout = 0;
	if ((long) timeout < 0)
		timeout = 0;
	if (tvp && !(current->personality & STICKY_TIMEOUTS)) {
		put_user(timeout/HZ, &tvp->tv_sec);
		timeout %= HZ;
		timeout *= (1000000/HZ);
		put_user(timeout, &tvp->tv_usec);
	}
	if (error < 0)
		goto out;
	if (!error) {
		error = -ERESTARTNOHAND;
		if (current->signal & ~current->blocked)//lux 有信号
			goto out;
		error = 0;
	}
	set_fd_set(n, inp, &res_in);//lux 设置返回数据，回写到用户数据段
	set_fd_set(n, outp, &res_out);
	set_fd_set(n, exp, &res_ex);
out:
	return error;
}
