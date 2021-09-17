/**lux block buffer 磁盘缓存实现
 * 系统开辟出一个缓存区，从end（内核段/system末尾，由ld链接时候设置），到buffer_end(由main程序计算指定)
 * 在缓存区高端地址，是1kb单位的数据block，
 * 在缓存区的低端地址，是对应数据block的block header
 * 所有block header连接成一个free_list
 * 另外，系统维护一个hashtable，用于通过(dev,blockNum)快速查询block header
 * 
 * 上层和磁盘交互，通过buffer管理系统。
 * 	1. 当所需要的block存在于缓存中（hashtable），则直接返回其bh（block header）
 * 	2. 否则，从磁盘读取该block，分配到一个新的bh，并返回bh。
 */
/*
 *  linux/fs/buffer.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'buffer.c' implements the buffer-cache functions. Race-conditions have
 * been avoided by NEVER letting a interrupt change a buffer (except for the
 * data, of course), but instead letting the caller do it. NOTE! As interrupts
 * can wake up a caller, some cli-sti sequences are needed to check for
 * sleep-on-calls. These should be extremely quick, though (I hope).
 */

/*
 * NOTE! There is one discordant note here: checking floppies for
 * disk change. This is where it fits best, I think, as it should
 * invalidate changed floppy-disk-caches.
 */

#include <stdarg.h>
 
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/io.h>

extern int end;
struct buffer_head * start_buffer = (struct buffer_head *) &end;/*lux:参考文件头部注释；参考Linux内核完全注释p539*/
struct buffer_head * hash_table[NR_HASH];
static struct buffer_head * free_list;
static struct task_struct * buffer_wait = NULL;//lux 全局的buffer_wait锁，等待free list有空闲项。
int NR_BUFFERS = 0;

static inline void wait_on_buffer(struct buffer_head * bh)
{/*lux sleep直到block可用。通过锁机制，解决buffer的资源竞争问题*/
	cli();//lux 关中断，注意，这个只对当前进程有效。当切换到新进程的时候，EFLAGS会重新加载，IF可能是设置的，那么就可以中断。
	//也就是说系统运行是以是以任务为单位的，所有的上下文也都是关联到任务的。
	//具体到这里，cli，sti只作用在当前进程(运行中的进程)。cli保证当该任务运行的时候，不接受中断。
	while (bh->b_lock)//lux 如果block被锁，则加入等待队列，直至被重新唤起(bh可用的时候)。
		sleep_on(&bh->b_wait);
	sti();//lux 开中断
}
/*lux sync系统调用，刷盘
 * 1. 先刷新inode到buffer
 * 2. 更新buffer到磁盘
 */
int sys_sync(void)
{
	int i;
	struct buffer_head * bh;

	sync_inodes();		/* write out inodes into buffers */
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		wait_on_buffer(bh);
		if (bh->b_dirt)
			ll_rw_block(WRITE,bh);//lux buffer 刷盘
	}
	return 0;
}
/*lux 对某个分区刷盘，内部使用。(功能类同sys_sync，但只针对某个分区/文件系统）*/
int sync_dev(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	sync_inodes();
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	return 0;
}

void inline invalidate_buffers(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev)
			bh->b_uptodate = bh->b_dirt = 0;
	}
}

/*
 * This routine checks whether a floppy has been changed, and
 * invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to loose :-)
 *
 * NOTE! Although currently this is only for floppies, the idea is
 * that any additional removable block-device will use this routine,
 * and that mount/open needn't know that floppies/whatever are
 * special.
 */
void check_disk_change(int dev)
{//lux 检测磁盘切换，当前只对floppy有效。
	int i;

	if (MAJOR(dev) != 2)
		return;
	if (!floppy_change(dev & 0x03))
		return;
	for (i=0 ; i<NR_SUPER ; i++)
		if (super_block[i].s_dev == dev)
			put_super(super_block[i].s_dev);
	invalidate_inodes(dev);//lux 设置无效
	invalidate_buffers(dev);//lux 设置无效
}

#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]
/*lux 从hashtable和freelist中移除*/
static inline void remove_from_queues(struct buffer_head * bh)
{
/* remove from hash-queue */
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	if (hash(bh->b_dev,bh->b_blocknr) == bh)//lux 刚好是头部，则去掉
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
/* remove from free list */
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
	if (free_list == bh)//lux 刚好是头部，则去掉
		free_list = bh->b_next_free;
}
/*lux 插入hashtable和freelist*/
static inline void insert_into_queues(struct buffer_head * bh)
{
/* put at end of free list */
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;
/* put the buffer in new hash-queue if it has a device */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!bh->b_dev)
		return;
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);
	hash(bh->b_dev,bh->b_blocknr) = bh;
	bh->b_next->b_prev = bh;
}
/*lux 在当前hashtable中是否已经存在要找的block header*/
static struct buffer_head * find_buffer(int dev, int block)
{		
	struct buffer_head * tmp;

	for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
		if (tmp->b_dev==dev && tmp->b_blocknr==block)
			return tmp;
	return NULL;
}

/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are readint them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 */
/**lux
 * 1. 因为没有锁，所以拿到bh后，要double check，所以代码是一个fork循环，直到成功返回。
 * 2. 之所以没有锁，是因为在读取一个确定的buffer的时候使用锁才是高效的。而这里是查找buffer，其实是一个遍历的过程。
 *    没读取一个先上锁，不是一个高效的做法。
 * 而且这种竞争出现的情况应该较小，所以综合来看，不上锁+double check的方式更合适。（乐观锁也是一样的道理）
 */
struct buffer_head * get_hash_table(int dev, int block)
{/*lux 从hashtable中查找块，不存在则返回NULL，存在则等待直至可用，返回*/
	struct buffer_head * bh;

	for (;;) {//find one avaiable or null or else loop
		if (!(bh=find_buffer(dev,block)))//find one
			return NULL;
		bh->b_count++;//lux 存在，++
		wait_on_buffer(bh);//lux sleep直到block可用
		if (bh->b_dev == dev && bh->b_blocknr == block)
			return bh;//lux 再次确认block没有被其他进程使用之后，再返回
		bh->b_count--;//lux 否则--;即没有成功，继续循环。
	}
}
/**lux 拿到指定block header，成功否则无限循环。先从hashtable找，有则返回；否则从从空闲列表分配，初始化后返回
 * 输入：
 * 	设备号	dev
 * 	逻辑块号	block
 * 输出：
 * 	该块的buffer_head
 * 
 * 流程：
 * 1. 尝试从hashtable拿，有则返回，无则继续。
 * 2. 尝试从freelist拿到一个空闲项，循环直至成功。
 * 	2.1 注意这一步会重复尝试直至成功，期间会多次sleep，等待资源可用。那么每次sleep唤起之后，都需要重新检查资源状态，因为有可能被其他进程修改了。
 *  2.2 检查的点在于 1. bh的(dev,block)是否正确；2. bh的b_count是否为0。3. bh旧数据已经落盘。这三点保证该bh可用。
 * 3. 更新bh信息
 * 4. 更新freelist和hashtable（先删除再插入）
 * 5. 返回bh(该bh被占用，b_count=1，只能由调用方通过brelse释放)
 * ps 所有通过getblk获取的bh，都应该由调用方通过brelse释放。否则会造成内存泄漏（该bh永远无法释放）
 */
/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algoritm is changed: hopefully better, and an elusive bug removed.
 */
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)
struct buffer_head * getblk(int dev,int block)
{
	struct buffer_head * tmp, * bh;

repeat:
	if (bh = get_hash_table(dev,block))
		return bh;//lux 如果缓存块已经存在，则返回
	tmp = free_list;//lux 否则分配一个
	do {
		if (tmp->b_count)//lux 被占用
			continue;
		if (!bh || BADNESS(tmp)<BADNESS(bh)) {
			bh = tmp;
			if (!BADNESS(tmp))
				break;
		}
/* and repeat until we find something good */
	} while ((tmp = tmp->b_next_free) != free_list);
	if (!bh) {
		sleep_on(&buffer_wait);/*lux 找不到freelist的空闲项，则加入等待队列，直至被唤醒（freelist有空闲项），然后继续find*/
		goto repeat;
	}//lux 成功获取到一个freelist的空闲block header
	wait_on_buffer(bh);//lux 资源竞争问题：等待该bh可用。因为使用了锁来解决资源竞争问题，该bh有可能被锁了，需要加入等待队列直至被唤醒。
	if (bh->b_count)
		goto repeat;//lux 再次确认，如果被使用，重新再找一个
	while (bh->b_dirt) {
		sync_dev(bh->b_dev);//刷盘
		wait_on_buffer(bh);
		if (bh->b_count)//lux 每次sleep后唤起，都要再次检查bh，因为有可能其他进程在这期间做了修改。
			goto repeat;
	}
/* NOTE!! While we slept waiting for this block, somebody else might */
/* already have added "this" block to the cache. check it */
	if (find_buffer(dev,block))//lux 其他进程有可能已经成功读取该block，不再处理。
		goto repeat;
/* OK, FINALLY we know that this buffer is the only one of it's kind, */
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
	bh->b_count=1;//lux 1代表有人用，0为空闲。
	bh->b_dirt=0;//lux 0 没有写入，所以不脏。 1 有写入且没有刷盘
	bh->b_uptodate=0;//lux 0 不是最新数据（不可用来读），1 最新数据(可读)
	remove_from_queues(bh);//lux 从freelist和hasttable移除
	bh->b_dev=dev;
	bh->b_blocknr=block;
	insert_into_queues(bh);//lux 重新插入freelist和hashtable
	return bh;
}
//lux 释放一个bh
void brelse(struct buffer_head * buf)
{
	if (!buf)
		return;
	wait_on_buffer(buf);
	if (!(buf->b_count--))
		panic("Trying to free free buffer");
	wake_up(&buffer_wait);//lux 如果有进程在等待freelist的空闲块，唤醒之（因为刚释放出来一个，可以用了）。
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
/** lux 块设备读高级接口/对外外层API
 * bread
 * 1. getblk 获取块(内存块)
 * 2. 数据是新的，返回，否则继续
 * 3. ll_rw_block 读取块(物理块)
 * 4. 等待bh可用（sleep on b_wait...磁盘驱动读取完毕后，更新b_wait,...schedule...唤起） see sched.c:sleep_on, sched.c:wake_up, hd.c:end_request
 * 5. bh有效则返回bh；否则释放bh，返回NULL。
 */
struct buffer_head * bread(int dev,int block)
{
	struct buffer_head * bh;

	if (!(bh=getblk(dev,block)))//lux 拿到block
		panic("bread: getblk returned NULL\n");
	if (bh->b_uptodate)//lux 缓存内包含最新数据，可以直接返回
		return bh;
	ll_rw_block(READ,bh);//lux 调用low level读取方法，真正向磁盘发送读取命令(ll_rw_block内部是异步读取，发送完命令就返回。由硬盘处理程序异步自行处理，然后填充到指定内存。)
	wait_on_buffer(bh);//lux sleep直到buffer可用/解锁。上面的ll_rw_block会锁定bh，直到bh被填充
	if (bh->b_uptodate)//lux always check after wake
		return bh;
	brelse(bh);//lux 失败，释放该块
	return NULL;
}
//lux 拷贝一个block大小的数据，从fromd到to
#define COPYBLK(from,to) \
__asm__("cld\n\t" \
	"rep\n\t" \
	"movsl\n\t" \
	::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) \
	:"cx","di","si")

/**lux 读取一页（4个block）数据，并复制到address。（加载磁盘数据到内存）
 * 1 block = 1k
 * 1 page = 4k = 4 block
 */
/*
 * bread_page reads four buffers into memory at the desired address. It's
 * a function of its own, as there is some speed to be got by reading them
 * all at the same time, not waiting for one to be read, and then another
 * etc.
 * //lux 上面说的是，为什么不直接调用bread4次，而是单独又写了一个函数。因为这样更快。
 */
void bread_page(unsigned long address,int dev,int b[4])
{
	struct buffer_head * bh[4];
	int i;

	for (i=0 ; i<4 ; i++)
		if (b[i]) {
			if (bh[i] = getblk(dev,b[i]))
				if (!bh[i]->b_uptodate)
					ll_rw_block(READ,bh[i]);
		} else
			bh[i] = NULL;
	for (i=0 ; i<4 ; i++,address += BLOCK_SIZE)
		if (bh[i]) {
			wait_on_buffer(bh[i]);
			if (bh[i]->b_uptodate)
				COPYBLK((unsigned long) bh[i]->b_data,address);
			brelse(bh[i]);//lux 记得要释放bh
		}
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */
struct buffer_head * breada(int dev,int first, ...)
{
	va_list args;
	struct buffer_head * bh, *tmp;

	va_start(args,first);
	if (!(bh=getblk(dev,first)))
		panic("bread: getblk returned NULL\n");
	if (!bh->b_uptodate)
		ll_rw_block(READ,bh);
	while ((first=va_arg(args,int))>=0) {
		tmp=getblk(dev,first);
		if (tmp) {
			if (!tmp->b_uptodate)
				ll_rw_block(READA,bh);
			tmp->b_count--;//lux 直接简单的-1来release，而不调用brelse做复杂调操作，因为这个bh没人用，其作用不过是要给ll_rw_block标记一次读取。
		}
	}
	va_end(args);
	wait_on_buffer(bh);
	if (bh->b_uptodate)
		return bh;
	brelse(bh);
	return (NULL);
}
//lux 缓存区初始化
void buffer_init(long buffer_end)
{
	struct buffer_head * h = start_buffer;
	void * b;
	int i;

	if (buffer_end == 1<<20)
		b = (void *) (640*1024);
	else
		b = (void *) buffer_end;
	/*
	 *lux 在start_buffer和buffer_end之间建立缓存区
	 * 低端存储双向链表，每个表项是一个缓冲头结构，代表一个缓冲区
	 * 高端是真正的缓存块，每个占用BLOCK_SIZE
	 * 代码：
	 * 从高端开始划分缓存块，每划分一个，同时写入低端一个链表项，直到无法划分（低端高端相遇或者剩余空间不够一个块大小）
	 * 其他：
	 * free_list 是空闲块双向链表
	 * hash_table用来通过设备号和逻辑号快速定位对应的缓冲块
	 */
	while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) {
		h->b_dev = 0;
		h->b_dirt = 0;
		h->b_count = 0;
		h->b_lock = 0;
		h->b_uptodate = 0;
		h->b_wait = NULL;
		h->b_next = NULL;
		h->b_prev = NULL;
		h->b_data = (char *) b;
		h->b_prev_free = h-1;
		h->b_next_free = h+1;
		h++;
		NR_BUFFERS++;//lux 记录缓冲块大小
		if (b == (void *) 0x100000)
			b = (void *) 0xA0000;
	}
	h--;
	free_list = start_buffer;
	free_list->b_prev_free = h;
	h->b_next_free = free_list;
	for (i=0;i<NR_HASH;i++)
		hash_table[i]=NULL;
}	
