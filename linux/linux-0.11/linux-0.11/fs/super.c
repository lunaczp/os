/**lux super 超级块管理
 * 系统维护一个super_block数组，最多支持8个条目，也就是说最多支持8个文件系统/分区。
 * 
 * mount_root: 加载根文件系统，一个Linux系统有一个根文件系统。
 * mount_root会读取并初始化superblock块，当设置了superblock，就可以认为文件系统已经加载了(所有需要得信息都在superblock)。
 */
/*
 *  linux/fs/super.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * super.c contains code to handle the super-block tables.
 */
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>

#include <errno.h>
#include <sys/stat.h>

int sync_dev(int dev);
void wait_for_keypress(void);
/**lux
 * bt	base bitoffset	store selected bit in CF flag
 * setb	dest			set byte if below(CF=1)
 * 最终set_bit(bitnr,addr) 返回0（对应bit为0），1（对应bit为1）
 * 注意，{}定义了一个block，block内最后一个表达式的值，就是整个{}的值。所以是__res see:C语言{}用法
 */
/* set_bit uses setb, as gas doesn't recognize setc */
#define set_bit(bitnr,addr) ({ \
register int __res __asm__("ax"); \
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })

struct super_block super_block[NR_SUPER];
/* this is initialized in init/main.c */
int ROOT_DEV = 0;

static void lock_super(struct super_block * sb)
{
	cli();
	while (sb->s_lock)
		sleep_on(&(sb->s_wait));
	sb->s_lock = 1;
	sti();
}

static void free_super(struct super_block * sb)
{//lux 解锁，唤醒等待进程
	cli();
	sb->s_lock = 0;
	wake_up(&(sb->s_wait));
	sti();
}

static void wait_on_super(struct super_block * sb)
{
	cli();
	while (sb->s_lock)
		sleep_on(&(sb->s_wait));
	sti();
}

struct super_block * get_super(int dev)
{//lux 从super_block列表中找到指定设备对应的super block
	struct super_block * s;

	if (!dev)
		return NULL;
	s = 0+super_block;
	while (s < NR_SUPER+super_block)
		if (s->s_dev == dev) {
			wait_on_super(s);
			if (s->s_dev == dev)//lux double check after wakeup
				return s;
			s = 0+super_block;
		} else
			s++;
	return NULL;
}

void put_super(int dev)
{
	struct super_block * sb;
	struct m_inode * inode;
	int i;

	if (dev == ROOT_DEV) {
		printk("root diskette changed: prepare for armageddon\n\r");
		return;
	}
	if (!(sb = get_super(dev)))
		return;
	if (sb->s_imount) {
		printk("Mounted disk changed - tssk, tssk\n\r");
		return;
	}
	lock_super(sb);
	sb->s_dev = 0;
	for(i=0;i<I_MAP_SLOTS;i++)
		brelse(sb->s_imap[i]);
	for(i=0;i<Z_MAP_SLOTS;i++)
		brelse(sb->s_zmap[i]);
	free_super(sb);
	return;
}
/**lux 读取指定设备的super block。如果存在于superblock list则直接返回该条目；否则说明是第一次，加载之。
 * 加载完superblock，初始化superblock的参数（imap,zmap,,,,）基本就可以认为文件系统加载成功了。
 */
static struct super_block * read_super(int dev)
{
	struct super_block * s;
	struct buffer_head * bh;
	int i,block;

	if (!dev)
		return NULL;
	check_disk_change(dev);
	if (s = get_super(dev))//lux 在super block 列表中已经存在，返回即可
		return s;
	//lux 不存在，下面开始读取该设备的superblock，初始化，并添加到super block list中。
	for (s = 0+super_block ;; s++) {//lux 在super block列表中找到一个空闲项
		if (s >= NR_SUPER+super_block)
			return NULL;
		if (!s->s_dev)
			break;
	}
	s->s_dev = dev;//lux 初始化
	s->s_isup = NULL;
	s->s_imount = NULL;
	s->s_time = 0;
	s->s_rd_only = 0;
	s->s_dirt = 0;
	lock_super(s);//lux 上锁
	if (!(bh = bread(dev,1))) {//lux 读取该设备第一个block；super block在这里
		s->s_dev=0;//lux 失败则清理后返回NULL
		free_super(s);
		return NULL;
	}
	*((struct d_super_block *) s) =
		*((struct d_super_block *) bh->b_data);//lux 赋值给s
	brelse(bh);//lux 用完释放bh
	if (s->s_magic != SUPER_MAGIC) {//lux signature对不上，失败
		s->s_dev = 0;
		free_super(s);
		return NULL;
	}
	for (i=0;i<I_MAP_SLOTS;i++)//lux 初始化imap
		s->s_imap[i] = NULL;
	for (i=0;i<Z_MAP_SLOTS;i++)//lux 初始化zmap
		s->s_zmap[i] = NULL;
	block=2;//lux inode bitmap从第二个逻辑块开始
	for (i=0 ; i < s->s_imap_blocks ; i++)//lux 初始化imap
		if (s->s_imap[i]=bread(dev,block))
			block++;
		else
			break;
	for (i=0 ; i < s->s_zmap_blocks ; i++)//lux 初始化zmap
		if (s->s_zmap[i]=bread(dev,block))
			block++;
		else
			break;
	if (block != 2+s->s_imap_blocks+s->s_zmap_blocks) {//如果上面读取inode bitmap和block bitmap出错，则释放所有资源，返回NULL
		for(i=0;i<I_MAP_SLOTS;i++)
			brelse(s->s_imap[i]);
		for(i=0;i<Z_MAP_SLOTS;i++)
			brelse(s->s_zmap[i]);
		s->s_dev=0;
		free_super(s);
		return NULL;
	}
	s->s_imap[0]->b_data[0] |= 1;//lux 第一个bit设置为1，标记占用（即默认不用）
	s->s_zmap[0]->b_data[0] |= 1;//lux 第一个bit设置为1，标记占用（即默认不用）
	free_super(s);//lux 解锁
	return s;
}

int sys_umount(char * dev_name)
{
	struct m_inode * inode;
	struct super_block * sb;
	int dev;

	if (!(inode=namei(dev_name)))
		return -ENOENT;
	dev = inode->i_zone[0];
	if (!S_ISBLK(inode->i_mode)) {
		iput(inode);
		return -ENOTBLK;
	}
	iput(inode);
	if (dev==ROOT_DEV)
		return -EBUSY;
	if (!(sb=get_super(dev)) || !(sb->s_imount))
		return -ENOENT;
	if (!sb->s_imount->i_mount)
		printk("Mounted inode has i_mount=0\n");
	for (inode=inode_table+0 ; inode<inode_table+NR_INODE ; inode++)
		if (inode->i_dev==dev && inode->i_count)
				return -EBUSY;
	sb->s_imount->i_mount=0;
	iput(sb->s_imount);
	sb->s_imount = NULL;
	iput(sb->s_isup);
	sb->s_isup = NULL;
	put_super(dev);
	sync_dev(dev);
	return 0;
}

int sys_mount(char * dev_name, char * dir_name, int rw_flag)
{
	struct m_inode * dev_i, * dir_i;
	struct super_block * sb;
	int dev;

	if (!(dev_i=namei(dev_name)))
		return -ENOENT;
	dev = dev_i->i_zone[0];
	if (!S_ISBLK(dev_i->i_mode)) {
		iput(dev_i);
		return -EPERM;
	}
	iput(dev_i);
	if (!(dir_i=namei(dir_name)))
		return -ENOENT;
	if (dir_i->i_count != 1 || dir_i->i_num == ROOT_INO) {
		iput(dir_i);
		return -EBUSY;
	}
	if (!S_ISDIR(dir_i->i_mode)) {
		iput(dir_i);
		return -EPERM;
	}
	if (!(sb=read_super(dev))) {
		iput(dir_i);
		return -EBUSY;
	}
	if (sb->s_imount) {
		iput(dir_i);
		return -EBUSY;
	}
	if (dir_i->i_mount) {
		iput(dir_i);
		return -EPERM;
	}
	sb->s_imount=dir_i;
	dir_i->i_mount=1;
	dir_i->i_dirt=1;		/* NOTE! we don't iput(dir_i) */
	return 0;			/* we do that in umount */
}
/*lux 加载根文件系统
 * 1. 加载对应的superblock
 * 2. 加载根目录"/"对应的inode
 * everytinng is done
 * 之后所有对该文件系统的请求(文件查找，编辑，目录操作...)，都能够通过superblock和inode推演计算出来。
 */
void mount_root(void)
{
	int i,free;
	struct super_block * p;
	struct m_inode * mi;

	if (32 != sizeof (struct d_inode))
		panic("bad i-node size");
	for(i=0;i<NR_FILE;i++)
		file_table[i].f_count=0;//lux 文件表初始化
	if (MAJOR(ROOT_DEV) == 2) {
		printk("Insert root floppy and press ENTER");
		wait_for_keypress();
	}
	for(p = &super_block[0] ; p < &super_block[NR_SUPER] ; p++) {//lux super block表初始化
		p->s_dev = 0;
		p->s_lock = 0;
		p->s_wait = NULL;
	}
	if (!(p=read_super(ROOT_DEV)))//lux 读取跟文件设备的superblock。 加载/初始化完成了superblock，对应的文件系统基本可以认为就加载成功了。
		panic("Unable to mount root");
	if (!(mi=iget(ROOT_DEV,ROOT_INO)))//lux 读取“/”的inode(不存在会加载并初始化)
		panic("Unable to read root i-node");
	mi->i_count += 3 ;	/* NOTE! it is logically used 4 times, not 1 */
	p->s_isup = p->s_imount = mi;//lux 更新superblock的根目录节点和安装节点
	current->pwd = mi;//lux 当前进程的pwd和root
	current->root = mi;
	free=0;
	i=p->s_nzones;
	while (-- i >= 0)
		//lux set_bit(nr,addr) 判断addr中，第nr个bit是否为0
		// 8191 = 1024*8 -1 = 0x1fff
		// 2^13 = 8192 一个区块包含的bit数
		//i&8191 = 第i个区块对应的bit偏移。如i=10，对应第10个bit
		if (!set_bit(i&8191,p->s_zmap[i>>13]->b_data))//lux 根据zmap统计可用blocks，set_bit只测试，不修改
			free++;
	printk("%d/%d free blocks\n\r",free,p->s_nzones);
	free=0;
	i=p->s_ninodes+1;
	while (-- i >= 0)
		if (!set_bit(i&8191,p->s_imap[i>>13]->b_data))//lux 根据imap统计可用inodes
			free++;
	printk("%d/%d free inodes\n\r",free,p->s_ninodes);
}
