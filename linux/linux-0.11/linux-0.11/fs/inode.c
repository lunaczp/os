/**lux inode 管理
 * - iget 获取一个inode
 * - iput 释放一个inode
 * 
 * 系统在内存维护一个inode_table，支持32个条目。
 * 对于iget：
 * 	1. 如果inode_table中已经有，则更新，然后返回该inode
 *  2. 否则，申请inode_table内到空闲项m_inode，读取磁盘inode信息，填充到m_inode 并返回。
 */
/*
 *  linux/fs/inode.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <string.h>
#include <sys/stat.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/system.h>

struct m_inode inode_table[NR_INODE]={{0,},};//lux 全局inode table

static void read_inode(struct m_inode * inode);
static void write_inode(struct m_inode * inode);

static inline void wait_on_inode(struct m_inode * inode)
{
	cli();
	while (inode->i_lock)
		sleep_on(&inode->i_wait);
	sti();
}

static inline void lock_inode(struct m_inode * inode)
{
	cli();
	while (inode->i_lock)
		sleep_on(&inode->i_wait);
	inode->i_lock=1;
	sti();
}

static inline void unlock_inode(struct m_inode * inode)
{
	inode->i_lock=0;
	wake_up(&inode->i_wait);
}

void invalidate_inodes(int dev)
{
	int i;
	struct m_inode * inode;

	inode = 0+inode_table;
	for(i=0 ; i<NR_INODE ; i++,inode++) {
		wait_on_inode(inode);
		if (inode->i_dev == dev) {
			if (inode->i_count)
				printk("inode in use on removed disk\n\r");
			inode->i_dev = inode->i_dirt = 0;
		}
	}
}
/*lux inodes 刷盘(只是更新到了系统buffer)*/
void sync_inodes(void)
{
	int i;
	struct m_inode * inode;

	inode = 0+inode_table;
	for(i=0 ; i<NR_INODE ; i++,inode++) {
		wait_on_inode(inode);
		if (inode->i_dirt && !inode->i_pipe)
			write_inode(inode);
	}
}
/**lux 拿到指定inode的数据区的第block个数据块对应的逻辑块编号
 * 注意i_zone内存储的都是物理块的逻辑块号。具体物理位置由文件系统和底层驱动负责换算。
 * 由于inode->i_zone前7个是直接块号，第8个是间接块号(间接块内有512个块号)，第9个是二级间接块号，应分开处理。 
 * 
 * 比如拿到inode x的第0个数据块所在的逻辑块，那么就是 inodex->i_zone[0]
 * 第9个，则
 * 1. 先获取 datablock = inodex->i_zone[7],
 * 2. 读取 datablock，里面第2个存储的就是要找的逻辑块号。
 * 	content = read(datablock), ret = content[2]
 * 
 */
static int _bmap(struct m_inode * inode,int block,int create)
{
	struct buffer_head * bh;
	int i;

	if (block<0)
		panic("_bmap: block<0");
	if (block >= 7+512+512*512)
		panic("_bmap: block>big");
	if (block<7) {
		if (create && !inode->i_zone[block])//lux 如果对应zone为空，且指定允许创建，则创建一个新块，提供给该zone
			if (inode->i_zone[block]=new_block(inode->i_dev)) {//lux 分配一个新块，存储的是逻辑块号
				inode->i_ctime=CURRENT_TIME;
				inode->i_dirt=1;
			}
		return inode->i_zone[block];
	}
	block -= 7;
	if (block<512) {
		if (create && !inode->i_zone[7])
			if (inode->i_zone[7]=new_block(inode->i_dev)) {
				inode->i_dirt=1;
				inode->i_ctime=CURRENT_TIME;
			}
		if (!inode->i_zone[7])
			return 0;
		if (!(bh = bread(inode->i_dev,inode->i_zone[7])))
			return 0;
		i = ((unsigned short *) (bh->b_data))[block];
		if (create && !i)
			if (i=new_block(inode->i_dev)) {
				((unsigned short *) (bh->b_data))[block]=i;
				bh->b_dirt=1;
			}
		brelse(bh);
		return i;
	}
	block -= 512;
	if (create && !inode->i_zone[8])
		if (inode->i_zone[8]=new_block(inode->i_dev)) {
			inode->i_dirt=1;
			inode->i_ctime=CURRENT_TIME;
		}
	if (!inode->i_zone[8])
		return 0;
	if (!(bh=bread(inode->i_dev,inode->i_zone[8])))
		return 0;
	i = ((unsigned short *)bh->b_data)[block>>9];
	if (create && !i)
		if (i=new_block(inode->i_dev)) {
			((unsigned short *) (bh->b_data))[block>>9]=i;
			bh->b_dirt=1;
		}
	brelse(bh);
	if (!i)
		return 0;
	if (!(bh=bread(inode->i_dev,i)))
		return 0;
	i = ((unsigned short *)bh->b_data)[block&511];
	if (create && !i)
		if (i=new_block(inode->i_dev)) {
			((unsigned short *) (bh->b_data))[block&511]=i;
			bh->b_dirt=1;
		}
	brelse(bh);
	return i;
}

int bmap(struct m_inode * inode,int block)
{
	return _bmap(inode,block,0);
}
//lux 获取指定的数据块
int create_block(struct m_inode * inode, int block)
{
	return _bmap(inode,block,1);
}
		
/**lux 释放一个inode对应的资源
 * 1. 如果是pipe
 * 	1. 释放相关资源（内存页）
 *  2. 更新inode，返回
 * 2. 如果是block
 * 	1. 刷盘
 *  2. 如果count>1，则-1返回。
 *  3. 否则，如果没有软链，则清空inode对应的所有资源（数据块），且释放inode自身
 *  4. 返回
 */ 
void iput(struct m_inode * inode)
{
	if (!inode)
		return;
	wait_on_inode(inode);
	if (!inode->i_count)
		panic("iput: trying to free free inode");
	if (inode->i_pipe) {
		wake_up(&inode->i_wait);
		if (--inode->i_count)
			return;
		free_page(inode->i_size);
		inode->i_count=0;
		inode->i_dirt=0;
		inode->i_pipe=0;
		return;
	}
	if (!inode->i_dev) {
		inode->i_count--;
		return;
	}
	if (S_ISBLK(inode->i_mode)) {
		sync_dev(inode->i_zone[0]);
		wait_on_inode(inode);
	}
repeat:
	if (inode->i_count>1) {
		inode->i_count--;
		return;
	}//lux 以下，开始清理
	if (!inode->i_nlinks) {//lux 没有链接，则释放掉inode占用的所有数据块，及inode本身(一个文件被删除的情况)
		truncate(inode);
		free_inode(inode);
		return;
	}
	if (inode->i_dirt) {
		write_inode(inode);	/* we can sleep - so do again */
		wait_on_inode(inode);
		goto repeat;
	}
	inode->i_count--;
	return;
}
/**lux 获取inode_table里一个空闲的inode条目
 * 1. 遍历inode_table，找到空闲项
 * 2. 返回空闲项，否则panic
 * 
 * luxtodo 如何保证一定找到？
 * 规则：每次使用后及时释放。当然这样也无法百分之百保证，极低的概率会无法找到？
 */
struct m_inode * get_empty_inode(void)
{
	struct m_inode * inode;
	static struct m_inode * last_inode = inode_table;
	int i;

	do {
		inode = NULL;
		for (i = NR_INODE; i ; i--) {
			if (++last_inode >= inode_table + NR_INODE)
				last_inode = inode_table;
			if (!last_inode->i_count) {
				inode = last_inode;
				if (!inode->i_dirt && !inode->i_lock)//lux bingo 找到
					break;
			}
		}
		if (!inode) {//lux 没找到
			for (i=0 ; i<NR_INODE ; i++)
				printk("%04x: %6d\t",inode_table[i].i_dev,
					inode_table[i].i_num);
			panic("No free inodes in mem");
		}
		wait_on_inode(inode);//lux 等待inode可用
		while (inode->i_dirt) {//lux 回写
			write_inode(inode);
			wait_on_inode(inode);
		}
	} while (inode->i_count);//lux 直到找到一个indoe，且i_count=0
	memset(inode,0,sizeof(*inode));//lux 初始化，系统加载的时候没有初始化过，这里要初始化。
	inode->i_count = 1;//lux 使用，+1
	return inode;
}
/**lux 分配一个pipe数据页，返回inode
 */
struct m_inode * get_pipe_inode(void)
{
	struct m_inode * inode;

	if (!(inode = get_empty_inode()))//lux 申请一个空闲inode
		return NULL;
	if (!(inode->i_size=get_free_page())) {//lux 分配空闲页，返回物理地址，保存在i_size
		inode->i_count = 0;
		return NULL;
	}
	inode->i_count = 2;	/* sum of readers/writers *///lux 初始为2
	PIPE_HEAD(*inode) = PIPE_TAIL(*inode) = 0;//lux 初始化读写指针
	inode->i_pipe = 1;//lux 标记inode类型为pipe
	return inode;
}
/**lux 根据inode号返回指定的inode。
 * 1. 在inode_table里，则更新后返回；
 * 2. 否则
 * 	1. 分配inode_table新节点
 *  2. 从磁盘加载该inode到内存，初始化
 *  3. 返回。
 */
struct m_inode * iget(int dev,int nr)
{
	struct m_inode * inode, * empty;

	if (!dev)
		panic("iget with dev==0");
	empty = get_empty_inode();//lux 在inode_table里找到一个空闲的inode
	inode = inode_table;
	while (inode < NR_INODE+inode_table) {//lux 如果已经存在于inode_table，则更新信息，并返回
		if (inode->i_dev != dev || inode->i_num != nr) {
			inode++;
			continue;
		}
		wait_on_inode(inode);
		if (inode->i_dev != dev || inode->i_num != nr) {
			inode = inode_table;
			continue;
		}
		inode->i_count++;//lux 引用++
		if (inode->i_mount) {//luxtodo i_mount
			int i;

			for (i = 0 ; i<NR_SUPER ; i++)
				if (super_block[i].s_imount==inode)
					break;
			if (i >= NR_SUPER) {
				printk("Mounted inode hasn't got sb\n");
				if (empty)
					iput(empty);
				return inode;
			}
			iput(inode);
			dev = super_block[i].s_dev;
			nr = ROOT_INO;
			inode = inode_table;
			continue;
		}
		if (empty)
			iput(empty);//lux 释放分配的节点
		return inode;
	}
	if (!empty)//lux 否则，inode_table里没有指定(dev,nr)的inode，且没有分配到空闲indoe，返回NULL
		return (NULL);
	inode=empty;//lux 分配成功了，初始化，并返回。注意，这个empty本身就是inode_table的空闲项，已经在inode_table里了，只需要初始化即可。
	inode->i_dev = dev;
	inode->i_num = nr;
	read_inode(inode);//lux 加载磁盘上的inode信息(通过dev,nr指定）到inode
	return inode;
}
//lux 读取指定inode的磁盘数据信息，填充到inode自身(内存数据结构)
static void read_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	lock_inode(inode);
	if (!(sb=get_super(inode->i_dev)))
		panic("trying to read inode without dev");
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num-1)/INODES_PER_BLOCK;//lux 计算inode所在逻辑块
	if (!(bh=bread(inode->i_dev,block)))//lux 读取块
		panic("unable to read i-node block");
	*(struct d_inode *)inode =
		((struct d_inode *)bh->b_data)
			[(inode->i_num-1)%INODES_PER_BLOCK];//lux 赋值给inode。注意inode在块内的偏移(一个块内有多个inode 1024/32=32个)
			//lux 注意这里巧妙的利用两点完成赋值
			//1. m_inode和d_inode的前部分的一致性。
			//2. 指振赋值 *a = *b
			//完成了从bh->b_data[x]拷贝指定长度sizeof(d_inode)到inode的操作
	brelse(bh);
	unlock_inode(inode);
}
//lux 写入inode（只要写入到磁盘缓存（bh）就认为成功。不关心底层的缓存管理）
static void write_inode(struct m_inode * inode)
{
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	lock_inode(inode);
	if (!inode->i_dirt || !inode->i_dev) {
		unlock_inode(inode);
		return;
	}
	if (!(sb=get_super(inode->i_dev)))
		panic("trying to write inode without device");
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num-1)/INODES_PER_BLOCK;
	if (!(bh=bread(inode->i_dev,block)))
		panic("unable to read i-node block");
	((struct d_inode *)bh->b_data)
		[(inode->i_num-1)%INODES_PER_BLOCK] =
			*(struct d_inode *)inode;
	bh->b_dirt=1;
	inode->i_dirt=0;
	brelse(bh);
	unlock_inode(inode);
}
