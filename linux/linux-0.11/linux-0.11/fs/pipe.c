/**lux pipe
 * 操作函数：
 * read_pipe,write_pipe
 * sys_pipe系统调用
 * 
 * pipe 通过文件系统来实现。
 * 1. 分配一个物理页作为数据区（4K），用inode表示 inode->i_pipe=1， inode->i_size存储物理页指针。inode->i_zone[0]记录读偏移，inode->i_zone[1]记录写偏移
 * 2. 使用pipe时候，分配两个文件句柄指向inode。一个只读，一个只写，提供给上层程序。
 * 
 * 这里借助文件系统实现了pipe，其目的是用文件封装一切。但是用inode表示pipe，在使用上有些不舒服，也许inode可以设计的更合理。
 */
/*
 *  linux/fs/pipe.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <signal.h>

#include <linux/sched.h>
#include <linux/mm.h>	/* for get_free_page */
#include <asm/segment.h>

//lux pipe读
int read_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, read = 0;

	while (count>0) {
		while (!(size=PIPE_SIZE(*inode))) {//lux 没有要读的数据
			wake_up(&inode->i_wait);//lux 唤醒写进程
			if (inode->i_count != 2) /* are there any writers? */
				return read;
			sleep_on(&inode->i_wait);//lux sleep
		}
		chars = PAGE_SIZE-PIPE_TAIL(*inode);//lux 读取位置距页尾举例。保证一次最多读取到页尾。
		if (chars > count)//lux 取最小读取单位
			chars = count;
		if (chars > size)
			chars = size;
		count -= chars;
		read += chars;
		size = PIPE_TAIL(*inode);//lux 读指针
		PIPE_TAIL(*inode) += chars;
		PIPE_TAIL(*inode) &= (PAGE_SIZE-1);
		while (chars-->0)
			put_fs_byte(((char *)inode->i_size)[size++],buf++);//lux 真正读
	}
	wake_up(&inode->i_wait);//lux 唤醒进程(写)
	return read;//lux 返回读取大小
}
//lux pipe写	
int write_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, written = 0;

	while (count>0) {
		while (!(size=(PAGE_SIZE-1)-PIPE_SIZE(*inode))) {//lux 数据已满（写满一页）
			wake_up(&inode->i_wait);//lux 唤起等待进程(读进程)
			if (inode->i_count != 2) { /* no readers */ //lux 异常
				current->signal |= (1<<(SIGPIPE-1));
				return written?written:-1;
			}
			sleep_on(&inode->i_wait);//lux sleep
		}
		/**lux 注意chars和size不同
		 * |------------------------------------------|
		 * |---A---|------B-------|-----------C-------|
		 * |      read        write                   |
		 * A部分是写入且已读
		 * B部分是写入且未读
		 * C部分是空闲
		 * size记录的是A+C，所以可写区域
		 * chars记录的是C，页尾剩余可写区域
		 */
		chars = PAGE_SIZE-PIPE_HEAD(*inode);//lux 页尾剩余空间
		if (chars > count)//lux 取最小写入单位
			chars = count;
		if (chars > size)
			chars = size;
		count -= chars;//lux 更新剩余需要写入大小
		written += chars;//lux 记录写入大小
		size = PIPE_HEAD(*inode);//lux 写指针
		PIPE_HEAD(*inode) += chars;//lux 更新写入后的指针位置
		PIPE_HEAD(*inode) &= (PAGE_SIZE-1);//lux 处理loopback，循环写入
		while (chars-->0)
			((char *)inode->i_size)[size++]=get_fs_byte(buf++);//lux 真正写入
	}
	wake_up(&inode->i_wait);//lux 唤醒等待进程（读进程）
	return written;//lux 返回写入大小
}
//lux pipe系统调用，创建无名管道
int sys_pipe(unsigned long * fildes)
{
	struct m_inode * inode;
	struct file * f[2];
	int fd[2];
	int i,j;

	j=0;
	for(i=0;j<2 && i<NR_FILE;i++)//lux 在全局file_table中找到两个空闲项，找不到则失败。
		if (!file_table[i].f_count)
			(f[j++]=i+file_table)->f_count++;
	if (j==1)
		f[0]->f_count=0;//lux 重置
	if (j<2)
		return -1;//lux 失败
	j=0;
	for(i=0;j<2 && i<NR_OPEN;i++)//lux 在当前进程的打开文件表flip中找到两个空闲表项，找不到则失败。
		if (!current->filp[i]) {
			current->filp[ fd[j]=i ] = f[j];
			j++;
		}
	if (j==1)
		current->filp[fd[0]]=NULL;//lux 重置
	if (j<2) {//lux 失败
		f[0]->f_count=f[1]->f_count=0;
		return -1;
	}
	//lux 上面都成功分配好了，继续
	if (!(inode=get_pipe_inode())) {//lux 分配一个pipe数据页，返回inode
		current->filp[fd[0]] =
			current->filp[fd[1]] = NULL;
		f[0]->f_count = f[1]->f_count = 0;
		return -1;//lux 失败
	}
	f[0]->f_inode = f[1]->f_inode = inode;//lux 记录inode
	f[0]->f_pos = f[1]->f_pos = 0;
	f[0]->f_mode = 1;		/* read */
	f[1]->f_mode = 2;		/* write */
	put_fs_long(fd[0],0+fildes);//lux 读句柄
	put_fs_long(fd[1],1+fildes);//lux 写句柄
	return 0;
}
