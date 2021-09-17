# Buffer 缓存系统
由于磁盘IO相对于CPU太慢，过多的磁盘IO会严重降低系统性能。Linux采用了缓存系统来解决这个问题。  
简单的讲，系统在内存开辟一段空间，将磁盘IO的结果存储到这里。读写都优先走缓存(内存访问速度快)，以此来降低磁盘IO频率，提升系统速率。

这里以磁盘代表普通块设备（block device）

## 简介
[fs/buffer.c](fs/buffer.c)
- 系统开辟出一个缓存区，从end（内核段/system末尾，由ld链接时候设置），到buffer_end(由main程序计算指定)
- 在缓存区高端地址，是1kb单位的数据block，
- 在缓存区的低端地址，是对应数据block的block header
- 所有block header连接成一个free_list 双向链表
- 另外，系统维护一个hashtable，用于通过(dev,blockNum)快速查询block header

上层和磁盘交互，通过buffer系统。
- 读
    - 当所需要的block存在于缓存中（hashtable），则直接返回其bh（block header）
    - 否则，从磁盘读取该block，分配到一个新的bh，并返回bh。
- 写
    - 直接写bh。buffer系统会负责具体的刷盘操作。


## 具体实现

### 数据结构
```
//fs/buffer.c
struct buffer_head * hash_table[NR_HASH];
static struct buffer_head * free_list;

//include/linux/fs.h
struct buffer_head {
	char * b_data;			/* pointer to data block (1024 bytes) */
	unsigned long b_blocknr;	/* block number */
	unsigned short b_dev;		/* device (0 = free) */
	unsigned char b_uptodate;
	unsigned char b_dirt;		/* 0-clean,1-dirty */
	unsigned char b_count;		/* users using this block */
	unsigned char b_lock;		/* 0 - ok, 1 -locked */
	struct task_struct * b_wait;
	struct buffer_head * b_prev;
	struct buffer_head * b_next;
	struct buffer_head * b_prev_free;
	struct buffer_head * b_next_free;
};
```

### 初始化
- `kernel main`调用`buffer_init`
    - 从`end`到`buffer_end`，低端做`buffer_head`结构体，高端做数据存储（1 block =1 1k）
    - 将所有`buffer_head`初始化，构造`free_list`，初始化`hast_table`。

### 对外API
buffer系统提供的是一个中间层，内部封装自己的逻辑，对外提供磁盘读取接口。这里的接口主要有：
- `bread` 读取一个设备块
- `bread_page` 读取一页
- `breada` 读取并预读多块
- `brelse` 释放一个块。所有通过`bread`获取的`buffer_head`都要通过`brelse`来释放(调用方负责)，否则会造成内存泄漏。
    - 只有释放的块，才能在`free_list`中被重新使用。

主要是`bread`，其核心逻辑：
- 尝试从`hash_table`获取，存在则返回，否则继续。
- 尝试从`free_list`找到一个空闲项，不存在则sleep(`sleep_on(&buffer_wait)`，加入等待队列)，循环直到获取到一个，记为`bh`
- 处理`bh`
    - 如果数据脏，则刷盘
- 验证`bh`可用，继续
- 更新`bh`
    ```
	bh->b_count=1;//lux 1代表有人用，0为空闲。
	bh->b_dirt=0;//lux 0 没有写入，所以不脏。 1 有写入且没有刷盘
	bh->b_uptodate=0;//lux 0 不是最新数据（不可用来读），1 最新数据(可读)
    ```
- 从`free_list`和`hash_table`移除并重新插入`bh`
- 返回`bh`

注意，在确定`bh`最终可用前，进行了很多次重复校验。因为期间包含了多次`sleep`，而每次`sleep`唤起后，都有可能有其他进程修改了`bh`，所以每次都要重新校验。
[fs/buffer.c](fs/buffer.c)
```
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
```
如上，每次调用`wait_on_buffer`进程都有可能sleep。而当被再次唤醒，`bh`有可能被其他进程使用了。所以，每次调用完`wait_on_buffer`，都要重新验证下`bh->b_count`来保证没有其他进程使用。  
另外，在这种确定使用`bh`之前，通过`find_buffer(dev,block)`再次校验，因为有可能在sleep的时候，其他进程也需要该block，且已经成功读取。那么此时不再继续，返回。

ps. 对操作系统而言，这种多用户模式，需要对资源的竞争做处理。

### 资源竞争、锁、队列
在上面我们看到，对一个`buffer_head`的访问作了很多判断，原因就是资源的竞争。由于多个进程可能对同一个物理块有并发的读写，那么如何保证不出问题呢。这里的解决方案就是 __锁__，和 __排队__。

__关于锁__:  
访问一个资源，判断是否被锁，如果已经被锁，则加入等待列表，sleep直到解除。  
以`buffer_head`为例：
```
//bread
    getblk
    ...
	wait_on_buffer(bh);
    ...

//wait_on_buffer
static inline void wait_on_buffer(struct buffer_head * bh)
{/*lux sleep直到block可用。通过锁机制，解决buffer的资源竞争问题*/
	cli();
	while (bh->b_lock)//lux 如果block被锁，则加入等待队列，直至被重新唤起(bh可用的时候)。
		sleep_on(&bh->b_wait);
	sti();//lux 开中断
```
如上，每个`buffer_head`都有一个全局锁`bh->b_lock`，使用前都要验证`bh`没有被锁。  

那么什么时候会加锁，什么时候释放呢：
```
/*lux buffer加锁*/
static inline void lock_buffer(struct buffer_head * bh)
{
	cli();
	while (bh->b_lock)
		sleep_on(&bh->b_wait);
	bh->b_lock=1;//lux: 加lock
	sti();
}
/*lux buffer解锁*/
static inline void unlock_buffer(struct buffer_head * bh)
{
	if (!bh->b_lock)
		printk("ll_rw_block.c: buffer not locked\n\r");
	bh->b_lock = 0;
	wake_up(&bh->b_wait);//lux 唤醒等待进程
}

//上锁
//ll_rw_block.c
//add_request:`
    ...
	lock_buffer(bh);//lux lock


//解锁
//blk.h
//end_request:

extern inline void end_request(int uptodate)
{//lux 当前请求完成，解锁buffer，唤醒等待进程（等待该buffer的，等待空闲队列项的），继续列表内下一个请求
	DEVICE_OFF(CURRENT->dev);
	if (CURRENT->bh) {
		CURRENT->bh->b_uptodate = uptodate;
		unlock_buffer(CURRENT->bh);//lux 读写成功，解锁buffer
	}
	if (!uptodate) {
		printk(DEVICE_NAME " I/O error\n\r");
		printk("dev %04x, block %d\n\r",CURRENT->dev,
			CURRENT->bh->b_blocknr);
	}
	wake_up(&CURRENT->waiting);//lux 唤醒等待buffer的进程
	wake_up(&wait_for_request);//lux 唤醒等待入队列的进程
	CURRENT->dev = -1;//lux 标记该条目空闲
	CURRENT = CURRENT->next;//lux next one
}
```
如上，磁盘驱动读写数据块前加锁，完毕后解锁。


另外，我们也看到，当被锁的情况，当前进程会把自己加入`bh->b_wait`。注意，这里就是一个排队的实现。所有对`bh`的访问，依靠`b_wait`来实现了排队访问。

__关于排队__:
上面我们其实已经看到了排队的使用，主要是通过`sleep_on`：

`sleep_on`和`wake_up`通过对一个全局变量的监测，实现了资源请求的排队。
- 所有调用`sleep_on`的函数都会保留上一个调用者的task指针，如此通过指针串联成一个反向列表`A<-B<-C`。
- 而当调用`wake_up`，会通过反向列表逐级把进程的运行态设置为`runnable`。从而使C唤起B，B唤起A。

如上实现了排队。更多详细分析，参见[kernel/sched.c: sleep_on](kernel/sched.c)
```
//sleep_on
void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;//lux 没有等待进程，无须排队，则直接返回
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;//lux 将当前进程设为等待资源 *p
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();//lux 睡眠当前进程
	if (tmp)
		tmp->state=0;//lux runnable,ready to run
}

//wake_up
void wake_up(struct task_struct **p)
{
	if (p && *p) {
		(**p).state=0;//lux 0 runnable
		*p=NULL;//lux 重置全局变量
	}
}

```

还是以`buffer_head`的读写为例。
- 磁盘驱动读写`bh`前，会上锁。
- 其他程序在使用`bh`前，会调用`wait_on_buffer`，检查锁，有锁的情况，会调用`sleep_on`把自己加入请求队列。
- 磁盘驱动完成读写后，会关闭锁，同时调用`wake_up`唤起那些`sleep_on`的进程。

如上，通过 __锁__ 和 __排队__ 解决了资源竞争问题。

### 刷盘
Linux上层对磁盘的使用，都是通过buffer这一层来做的（`bread`作为入口）。比如`superblock`信息，文件系统的信息（inode）、文件的读写
其底层的交互都是通过`buffer_head`，也就是说，比如上层应用对文件的写入，其实只是写入了`buffer_head`而已（内存）。所以buffer系统有另外一个职责即使要及时把这些信息落地到磁盘。

涉及到的函数
- `sys_sync(void)` 系统调用，由上层服务调用。可以显式把buffer数据写入磁盘
- `sync_dev(int dev)`内部函数。将指定文件系统/分区的buffer数据写入磁盘。

其主要功能一样，都是把缓存区内的所有数据，落到对应的磁盘物理块。
其区别主要是一个是磁盘全部数据，一个是针对某个分区的数据。

```
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
```
`sys_sync`由用户程序触发，无需多说。`sync_dev`则有系统本身择机触发，那么什么情况下会触发`sync_dev`呢。
- 需要使用某个bh，但检测到数据脏时，需要落盘。
```
//bread
	while (bh->b_dirt) {
		sync_dev(bh->b_dev);//刷盘
```
- 释放某个inode节点时，需要把inode信息落盘。
```
//iput
	if (S_ISBLK(inode->i_mode)) {
		sync_dev(inode->i_zone[0]);
		wait_on_inode(inode);
```

- 卸载文件系统的时候，需要落盘。
```
//sys_umount
	put_super(dev);
	sync_dev(dev);
	return 0;
```