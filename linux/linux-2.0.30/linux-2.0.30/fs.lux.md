# File System

## 文件操作的抽象：
文件系统本身是一种抽象，对上层提供统一的服务接口，如`open`,`create`,`close`。而内部则封装了不同设备的处理。因为不同设备不同文件类型，其处理函数是不一样的， 那么，这些都需要注册才能使用。以`open`为例：

- `open`先获取一个`fd`，然后调用`do_open`
- `do_open`
	- 获取一个系统打开文件表的表项`f`
	- 调用`open_namei`：根据文件名，找到文件的inode
	- 调用该文件对应的`open`函数。最终返回。
```
	if (inode->i_op)
		f->f_op = inode->i_op->default_file_ops;
	if (f->f_op && f->f_op->open) {
		error = f->f_op->open(inode,f);
		if (error)
			goto cleanup_all;
	}
	f->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);

	current->files->fd[fd] = f;
	return 0;
```

这里，`file`的`open`函数，来源于`inode`，而`inode`的`open`函数，底层不同的文件类型对应的回调也不同。以minix文件系统为例，其初始化流程是：

- 系统启动，`main.c:init`调用`setup`，它调用`sys_setup`
- `sys_setup`调用`init_minix_fs`，它注册minix文件系统：
```
//filesystems.c
#ifdef CONFIG_MINIX_FS
	init_minix_fs();//lux 注册minix文件系统
#endif


//minix/inode.c
//注册自己
	int init_minix_fs(void)
{
		return register_filesystem(&minix_fs_type);
}


//minix/inode.c
//这里是minix_fs_type定义：
static struct file_system_type minix_fs_type = {
minix_read_super, "minix", 1, NULL
};


//minix/inodec.
//这里是minix_read_super函数
struct super_block *minix_read_super(struct super_block *s,void *data, 
				 int silent)
{
...
s->s_dev = dev;
s->s_op = &minix_sops;
```
- 调用`mount_root`加载根文件系统
```
	//filesystems.c
	mount_root();//lux 加载根文件系统
	return 0;
```
- `sys_setup`调用`mount_root`
```
//filesystems.c
	mount_root();//lux 加载根文件系统
	return 0;
}


//fs/super.c
void mount_root(void)
{
	memset(super_blocks, 0, sizeof(super_blocks));
	do_mount_root();
}


//fs/super.c
static void do_mount_root(void)
{
...
//对每个注册的文件系统，调用read_super，读取超级块
	else for (fs_type = file_systems ; fs_type ; fs_type = fs_type->next) {
  		if (!fs_type->requires_dev)
  			continue;
  		sb = read_super(ROOT_DEV,fs_type->name,root_mountflags,NULL,1);
		if (sb) {
			inode = sb->s_mounted;


//fs/super.c
static struct super_block * read_super(kdev_t dev,const char *name,int flags,
				       void *data, int silent)
{
...
//调用注册的函数，对于minix而言，是在init_minix_fs中设置的minix_read_super
	if (!type->read_super(s,data, silent)) {
		s->s_dev = 0;
		return NULL;
	}
	

//minix/inode.c
	struct super_block *minix_read_super(struct super_block *s,void *data, 
				     int silent)
{
...
//调用iget
	s->s_dev = dev;
	s->s_op = &minix_sops;//lux 设定饿各种inode操作类，下面的调用要用（其实所有的inode操作都要经过s->s_op）
	s->s_mounted = iget(s,MINIX_ROOT_INO);//lux 读取根目录"/"

	
//include/linux/fs.h
extern inline struct inode * iget(struct super_block * sb,int nr)
{
	return __iget(sb, nr, 1);
}

//fs/inode.c
//lux iget会调用__get
struct inode *__iget(struct super_block * sb, int nr, int crossmntp)
{
...
	read_inode(inode);//lux 读取一个inode

//fs/inode.c
//lux 调用不同文件系统自身的read_inode函数，对minix而言，调用的是minix_read_inode
static inline void read_inode(struct inode * inode)
{
	lock_inode(inode);
	if (inode->i_sb && inode->i_sb->s_op && inode->i_sb->s_op->read_inode)
		inode->i_sb->s_op->read_inode(inode);
	unlock_inode(inode);
}


//minix/inode.c
//这里是minix_read_inode定义，当需要读取一个inode的时候，会调用
void minix_read_inode(struct inode * inode)
{
	if (INODE_VERSION(inode) == MINIX_V1)
		V1_minix_read_inode(inode);
	else
		V2_minix_read_inode(inode);
}
...

//lux 下面会根据文件类型的不同，注册不同的回调
static void V1_minix_read_inode(struct inode * inode)
{
...
	if (S_ISREG(inode->i_mode))
	inode->i_op = &minix_file_inode_operations;
else if (S_ISDIR(inode->i_mode))
	inode->i_op = &minix_dir_inode_operations;
else if (S_ISLNK(inode->i_mode))
	inode->i_op = &minix_symlink_inode_operations;
else if (S_ISCHR(inode->i_mode))
	inode->i_op = &chrdev_inode_operations;
else if (S_ISBLK(inode->i_mode))
	inode->i_op = &blkdev_inode_operations;
else if (S_ISFIFO(inode->i_mode))
	init_fifo(inode);

//minix/file.c
//以上面的普通文件为例，看下minix_file_inode_operations是啥：
static struct file_operations minix_file_operations = {
	NULL,			/* lseek - default */
	generic_file_read,	/* read */
	minix_file_write,	/* write */
	NULL,			/* readdir - bad */
	NULL,			/* select - default */
	NULL,			/* ioctl - default */
	generic_file_mmap,	/* mmap */
	NULL,			/* no special open is needed */
	NULL,			/* release */
	minix_sync_file		/* fsync */
};

struct inode_operations minix_file_inode_operations = {
	&minix_file_operations,	/* default file operations */
	NULL,			/* create */
	NULL,			/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	generic_readpage,	/* readpage */
	NULL,			/* writepage */
	minix_bmap,		/* bmap */
	minix_truncate,		/* truncate */
	NULL			/* permission */
};
```

如上，
- `init_minix_fs`将自己注册到全局文件系统。
- `mount_root`调用minix注册设置的函数`minix_read_super`，读取其superblock，并加载`/`根目录（inode已经初始化）。那么后面所有的文件查看创建删除，都可以从根目录来进行定位了。
- 其他文件系统的注册类似。

回到`open`的例子：
```
//fs/open.c
...
	error = open_namei(filename,flag,mode,&inode,NULL);
...
if (inode->i_op)
		f->f_op = inode->i_op->default_file_ops;
	if (f->f_op && f->f_op->open) {
		error = f->f_op->open(inode,f);
		if (error)
			goto cleanup_all;
	}
```
- `open_namei`
	- 调用`dir_namei`找到最低一级目录的`inode`（这里会从根目录开始，由于根目录的`inode`已经设置好了(回调函数设置了)，那么逐级往下，就可以定位目录及文件）
		- 调用`lookup`，最终到`inode->lookup`，到`minix_lookup`
		- `minix_lookup`调用`iget`，最终调用`__iget`
		- `__iget`调用`read_inode`，最终调用`minix_read_inode`
		- `minix_read_inode`，调用`V1_minix_read_inode`：
		```
			static void V1_minix_read_inode(struct inode * inode)
		{
		...
			if (S_ISREG(inode->i_mode))
			inode->i_op = &minix_file_inode_operations;
		else if (S_ISDIR(inode->i_mode))
			inode->i_op = &minix_dir_inode_operations;
		else if (S_ISLNK(inode->i_mode))
			inode->i_op = &minix_symlink_inode_operations;
		else if (S_ISCHR(inode->i_mode))
			inode->i_op = &chrdev_inode_operations;
		else if (S_ISBLK(inode->i_mode))
			inode->i_op = &blkdev_inode_operations;
		else if (S_ISFIFO(inode->i_mode))
			init_fifo(inode);
	
		```
		根据不同类型，设定不同的操作函数。
	- 调用`lookup`找到对应文件，流程同上面，返回`inode`
	- `open_namei`设置好了`inode`，返回成功，此时`inode`已经初始化各种回调函数。
- 有了`inode`，就可以操作该文件了。
	
其他文件系统的注册即使用过程类型。整体来讲就是，
- 先注册，再使用。
- 使用过程中围绕`inode`，`inode`封装好了底层的操作，对上层提供统一接口。

### 特殊设备的初始化
```
	static void V1_minix_read_inode(struct inode * inode)
{
...
	if (S_ISREG(inode->i_mode))
	inode->i_op = &minix_file_inode_operations;
else if (S_ISDIR(inode->i_mode))
	inode->i_op = &minix_dir_inode_operations;
else if (S_ISLNK(inode->i_mode))
	inode->i_op = &minix_symlink_inode_operations;
else if (S_ISCHR(inode->i_mode))
	inode->i_op = &chrdev_inode_operations;
else if (S_ISBLK(inode->i_mode))
	inode->i_op = &blkdev_inode_operations;
else if (S_ISFIFO(inode->i_mode))
	init_fifo(inode);

```
在上面我们看到，特殊设备的回调是不一样的，比如
- 块设备`blkdev_inode_operations`
- 字符设备`chrdev_inode_operations`


这些不依赖于特定的文件系统，而是全局统一的。那么这些回调是怎么注册的呢：
- 首先，`fs/devices.c`定义了全局的设备数组，和注册函数：
```
struct device_struct {
	const char * name;
	struct file_operations * fops;
};

static struct device_struct chrdevs[MAX_CHRDEV] = {
	{ NULL, NULL },
};//lux 全局字符设备数组

static struct device_struct blkdevs[MAX_BLKDEV] = {
	{ NULL, NULL },
};//lux 全局块设备数组
```

```
int register_chrdev(unsigned int major, const char * name, struct file_operations *fops)
int register_blkdev(unsigned int major, const char * name, struct file_operations *fops)
int unregister_chrdev(unsigned int major, const char * name)
int unregister_blkdev(unsigned int major, const char * name)
```

- 通过注册函数，设备可以把自身的`设备号`，`名字`，`回调`注册到全局数组，那么上层应用就可以通过其设备号获取到。
```
struct file_operations * get_blkfops(unsigned int major)
{
	return get_fops (major,0,MAX_BLKDEV,"block-major-%d",blkdevs);
}

struct file_operations * get_chrfops(unsigned int major, unsigned int minor)
{
	return get_fops (major,minor,MAX_CHRDEV,"char-major-%d",chrdevs);
}
```

- 如此，就可以操作各类设备了。


那么，这些设备是这么初始化的呢：
```
//main.c
static int init(void * unused)
{
	...
	setup();//lux 设备初始化，注册文件系统，加载根文件系统

//filesystems.c
asmlinkage int sys_setup(void)
{
	...
	device_setup();//lux 设备初始化

//drivers/block/genhd.c
void device_setup(void)
{
	extern void console_map_init(void);
	struct gendisk *p;
	int nr=0;

	chr_dev_init();//lux 字符设备初始化
	blk_dev_init();//lux 块设备初始化
	sti();

//下面是chr_dev_init相关的
//drivers/char/mem.c
int chr_dev_init(void)
{
	if (register_chrdev(MEM_MAJOR,"mem",&memory_fops))
		printk("unable to get major %d for memory devs\n", MEM_MAJOR);
	rand_initialize();
	tty_init();
#ifdef CONFIG_PRINTER
	lp_init();//lux 打印机初始化
#endif
...
#ifdef CONFIG_SOUND
	soundcard_init();//lux 声卡初始化
#endif

//drivers/char/lp.c
int lp_init(void)
{
	...
	if (register_chrdev(LP_MAJOR,"lp",&lp_fops)) {
		printk("lp: unable to get major %d\n", LP_MAJOR);
		return -EIO;
	}

//drivers/char/lp.c
static struct file_operations lp_fops = {
	lp_lseek,
	NULL,		/* lp_read */
	lp_write,
	NULL,		/* lp_readdir */
	NULL,		/* lp_select */
	lp_ioctl,
	NULL,		/* lp_mmap */
	lp_open,
	lp_release
};

//如上，我们看到了字符设备初始化的流程，最终都是通过register_chrdev注册到了全局的字符设备数组chrdevs了。

//下面是块设备相关blk_dev_init
//drivers/block/ll_rw_blk.c
int blk_dev_init(void)
{
	...
#ifdef CONFIG_BLK_DEV_RAM
	rd_init();//lux ramdisk初始化
#endif
#ifdef CONFIG_BLK_DEV_LOOP
	loop_init();//lux loop设备初始化
#endif
#ifdef CONFIG_CDI_INIT
	cdi_init();		/* this MUST precede ide_init */
#endif CONFIG_CDI_INIT
#ifdef CONFIG_BLK_DEV_IDE
	ide_init();		/* this MUST precede hd_init */
#endif
#ifdef CONFIG_BLK_DEV_HD
	hd_init();//lux hd初始化
#endif
#ifdef CONFIG_BLK_DEV_XD
	xd_init();
#endif
#ifdef CONFIG_BLK_DEV_FD
	floppy_init();//lux 软盘初始化

//drivers/block/hd.c
int hd_init(void)
{
	if (register_blkdev(MAJOR_NR,"hd",&hd_fops)) {
		printk("hd: unable to get major %d for harddisk\n",MAJOR_NR);
		return -1;
	}
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	read_ahead[MAJOR_NR] = 8;		/* 8 sector (4kB) read-ahead */
	hd_gendisk.next = gendisk_head;
	gendisk_head = &hd_gendisk;
	timer_table[HD_TIMER].fn = hd_times_out;
	return 0;
}

//drivers/block/hd.c
static struct file_operations hd_fops = {
	NULL,			/* lseek - default */
	block_read,		/* read - general block-dev read */
	block_write,		/* write - general block-dev write */
	NULL,			/* readdir - bad */
	NULL,			/* select */
	hd_ioctl,		/* ioctl */
	NULL,			/* mmap */
	hd_open,		/* open */
	hd_release,		/* release */
	block_fsync		/* fsync */
};
//如上，是块设备初始化过程，最终都是通过register_blkdev注册到了全局的块设备数组blkdevs了。
```

如上，就是字符设备和块设备的初始化过程，其提供给文件系统的回调函数(`read`,`write`,`select`,`open`,`lseek`...)都在整个过程中注册。


### socket
`socket`相关的操作流程不太一样，通常的使用方法是：
```
...
int fd = socket(...) //lux 通过socket系统调用获取一个句柄。
...
```
如此，`fd`的回调函数是在`socket`调用中设置好的，而不是类似于普通文件或其他设备，通过`open`，到`inode`，在`inode`中设置的。

对于`socket`是：
```
//net/socket.c
asmlinkage int sys_socket(int family, int type, int protocol)
{
	...
	if (!(sock = sock_alloc())) 
	{
		printk(KERN_WARNING "socket: no more sockets\n");
		return(-ENOSR);	/* Was: EAGAIN, but we are out of
				   system resources! */
	}

	sock->type = type;
	sock->ops = ops;
	if ((i = sock->ops->create(sock, protocol)) < 0) 
	{
		sock_release(sock);
		return(i);
	}

	if ((fd = get_fd(SOCK_INODE(sock))) < 0) //lux 获取一个fd
	{
		sock_release(sock);
		return(-EINVAL);
	}

//net/socket.c
struct socket *sock_alloc(void)
{
	struct inode * inode;
	struct socket * sock;

	inode = get_empty_inode();//lux 获取一个inode
	if (!inode)
		return NULL;

	inode->i_mode = S_IFSOCK;//lux inode类型为socket
	inode->i_sock = 1;
	inode->i_uid = current->uid;
	inode->i_gid = current->gid;

	sock = &inode->u.socket_i;
	sock->state = SS_UNCONNECTED;
	sock->flags = 0;
	sock->ops = NULL;
	sock->data = NULL;
	sock->conn = NULL;
	sock->iconn = NULL;
	sock->next = NULL;
	sock->file = NULL;
	sock->wait = &inode->i_wait;
	sock->inode = inode;		/* "backlink": we could use pointer arithmetic instead */
	sock->fasync_list = NULL;
	sockets_in_use++;
	return sock;
}

//net/socket.c
static int get_fd(struct inode *inode)
{
	...
	fd = get_unused_fd();
	if (fd >= 0) {
		struct file *file = get_empty_filp();

		if (!file) {
			put_unused_fd(fd);
			return -ENFILE;
		}

		current->files->fd[fd] = file;
		file->f_op = &socket_file_ops;
		file->f_mode = 3;
		file->f_flags = O_RDWR;
		file->f_count = 1;
		file->f_inode = inode;
		if (inode) 
			inode->i_count++;
		file->f_pos = 0;
	}
	return fd;
}
```
如上，`socket`类的`fd`和`inode`是自己单独处理的。