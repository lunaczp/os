/**lux 字符设备操作集合
 * tty ram mem...
 * 
 */
/*
 *  linux/fs/char_dev.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <sys/types.h>

#include <linux/sched.h>
#include <linux/kernel.h>

#include <asm/segment.h>
#include <asm/io.h>

extern int tty_read(unsigned minor,char * buf,int count);
extern int tty_write(unsigned minor,char * buf,int count);

typedef (*crw_ptr)(int rw,unsigned minor,char * buf,int count,off_t * pos);
//lux ttyx 读写
static int rw_ttyx(int rw,unsigned minor,char * buf,int count,off_t * pos)
{
	return ((rw==READ)?tty_read(minor,buf,count):
		tty_write(minor,buf,count));
}
//lux tty 读写
static int rw_tty(int rw,unsigned minor,char * buf,int count, off_t * pos)
{//lux rw_tty始终向当前进程的tty写入，入参minor无效。区别于rw_ttyx
	if (current->tty<0)
		return -EPERM;
	return rw_ttyx(rw,current->tty,buf,count,pos);//lux 向当前进程的tty写入。
}
//lux ram 读写
static int rw_ram(int rw,char * buf, int count, off_t *pos)
{
	return -EIO;
}
//lux mem读写
static int rw_mem(int rw,char * buf, int count, off_t * pos)
{
	return -EIO;
}

static int rw_kmem(int rw,char * buf, int count, off_t * pos)
{
	return -EIO;
}

static int rw_port(int rw,char * buf, int count, off_t * pos)
{
	int i=*pos;

	while (count-->0 && i<65536) {
		if (rw==READ)
			put_fs_byte(inb(i),buf++);
		else
			outb(get_fs_byte(buf++),i);
		i++;
	}
	i -= *pos;
	*pos += i;
	return i;
}

static int rw_memory(int rw, unsigned minor, char * buf, int count, off_t * pos)
{
	switch(minor) {
		case 0:
			return rw_ram(rw,buf,count,pos);
		case 1:
			return rw_mem(rw,buf,count,pos);
		case 2:
			return rw_kmem(rw,buf,count,pos);
		case 3:
			return (rw==READ)?0:count;	/* rw_null */
		case 4:
			return rw_port(rw,buf,count,pos);
		default:
			return -EIO;
	}
}

#define NRDEVS ((sizeof (crw_table))/(sizeof (crw_ptr)))
/**lux 每个设备类型都有一个回调函数，这里只是定义了字符设备的回调，其他设备由对应的驱动自己定义。
 * 注意数组的顺序是不能修改的，和ll_rw_blk.c:35 blk_dev[]的顺序是一致的。
 * 比如fd是2，hd是3，即硬盘的主设备号是3,这些是全局定好的。所以在crw_table里要保持一致。
 * 
 * 这里等于是每个设备类型都定义了一个回调数组，数组包含所有设备，且顺序是全局定好的，然后每个设备只填充自己的表项，其余留空。因为设备驱动自己只会调用相关的表项。
 * 其实更好的处理方法感觉应该是全局只使用一个数组，然后每个设备驱动单独填充，这样更清晰，也更方便管理。
 */ 
static crw_ptr crw_table[]={
	NULL,		/* nodev */
	rw_memory,	/* /dev/mem etc */
	NULL,		/* /dev/fd */
	NULL,		/* /dev/hd */
	rw_ttyx,	/* /dev/ttyx */
	rw_tty,		/* /dev/tty */
	NULL,		/* /dev/lp */
	NULL};		/* unnamed pipes */

int rw_char(int rw,int dev, char * buf, int count, off_t * pos)
{
	crw_ptr call_addr;

	if (MAJOR(dev)>=NRDEVS)//lux 主设备号
		return -ENODEV;
	if (!(call_addr=crw_table[MAJOR(dev)]))//lux 根据主设备号，获取对应读回调函数
		return -ENODEV;
	return call_addr(rw,MINOR(dev),buf,count,pos);//lux 调用回调函数
}
