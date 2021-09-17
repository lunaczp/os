# File System


## Linux0.1 / Minix 1.0 文件系统
Linux0.1 FS使用了Minix1.0的FS，而Minix1.0的FS基本类似于UNIX的FS。下面以Minix1.0的FS为例说明。

filesystem layout:
```
|-------------------------------------------------------------------------------|
|mbr    |super block    |inode bitmap   |block bitmap   |inode area |data area  |
|-------------------------------------------------------------------------------|
```

注意，上面是一个FS，而不是一整个硬盘。一个硬盘可能有多个分区，对应多个FS，上面是一个FS的布局(不包括mbr，mbr一个硬盘只有一个)。

### data strucure
基本数据结构定义，参考[fs.h](include/linux/fs.h)

### mbr
主引导记录，在第一个扇区`sector`，一共512byte，以`0x55AA`作为标记`signature`

mbr中包含了硬盘的分区信息，详细信息见[MBR wikipedia](https://en.wikipedia.org/wiki/Master_boot_record)

```
//hd.c:147

		/**lux
		 * see: https://en.wikipedia.org/wiki/Master_boot_record
		 * Structure of a classical generic MBR
		 * hex	dec	description
		 * +000	+0	Bootstrap code area	446
		 * +1BE	+446	Partition entry 1	Partition table
		 * +1CE	+462	Partition entry 2	16
		 * +1DE	+478	Partition entry 3	16
		 * +1EE	+494	Partition entry 4	16
		 * +1FE	+510	55hex	Boot signature[a]	2
		 * 
		 * Partition Entry see hdreg.h:52
		 * 下面读取分区表,其实主要就关心两个字段：start_sect, nr_sects
		 */
```


### super block
super block包含了该文件系统的基本信息

|name   |length |desc
---     |---    |---
|s_ninodes  |short  |inode节点数
|s_nzones  |short   |逻辑块数
|s_imap_blocks  |short  |inode bitmap 占用块数
|s_zmap_blocks  |short  |block bitmap 占用块数
|s_firstdatazone    |short  |数据区第一个逻辑块块号
|s_log_zone_size    |short  |log2(磁盘块数/逻辑块数)
|s_max_size |long   |最大文件长度
|s_magic    |short  |文件系统魔数(0x137f)


### inode bitmap
inode bitmap是inode area内inode节点(32byte)使用情况的bitmap映射。bitmap的第一个bit，对应了inode area内的第一个inode使用情况，以此类推。

__注意,map[0]不用，从map[1]开始，"/"对应的是第一个inode，也即对应map[1]__

### block bitmap
block bitmap是数据区域`data area`逻辑块(1kb)使用情况的bitmap映射。bitmap的第1个bit，对应了数据区域第一个logic block的使用情况，以此类推。

__注意，类似inode map，block map的第0位为也不用，map[0]始终为1。从map[1]开始__ 

### inode area
inode area是一个个的inode节点。一个inode结构如下：

|name   |length |desc
---     |---    |---
|i_mode  |short  |文件类型和属性（rwx）
|i_uid  |short  |文件宿主的用户ID
|i_size  |long  |文件长度（字节）
|i_mtime  |long  |修改时间（从1970.1.1 0时起）
|i_gid  |char  |文件宿主的组ID
|i_nlinks  |char  |链接数
|i_zone[9]  |short  |文件所占逻辑块号数组，这里共9个。unix是13个。存储的是逻辑块号。

__注意，每个inode按照你顺序索引，即1，2...以此编号来标识每个inode。__ 一般地，根目录("/")的编号是1。

### data area
真正的数据区。数据块每个1k，按照顺序编号，1，2，3...。这些编号就是逻辑块的编号，以此来引用逻辑块。

逻辑块与文件/inode的关联，通过inode的i_zone映射。
- `i_zone[0]至i_zone[6]`是直接寻址，保存的是对应的逻辑块号。
- `i_zone[7]`一次间接寻址，保存的是下一级寻址表所在逻辑块号（设为表L1），表L1的结构是`L1[512]`，有512个表项，每个表项指向一个逻辑数据块号。
- `i_zone[8]`二次间接寻址，类似上一个，只不过是两层。

所有的数据寻址都通过`i_zone`来实现。


## 实现

### 文件系统的分层和模块
```
|------------------------------------------------------------------------------------
|	open.c: open create close chmod		| 高级API层
|------------------------------------------------------------------------------------
|	namei.c namei open_namei dir_namei	| 文件名和inode的映射，为上层提供基于文件名的操作
|------------------------------------------------------------------------------------
|	inode.c iget iput			| 对上层提供inode服务
|------------------------------------------------------------------------------------
|	bitmap.c new_inode new_block		| inode自身的bitmap管理
|------------------------------------------------------------------------------------
```

#### 高级API层
高级API，就是对外的API了，我们常见的`open`,`close`等函数的实现就在这里

#### 映射层
提供文件名和inode的映射。比如
- namei 根据文件名，获取其inode
- open_namei 根据文件名打开文件
- dir_namei 根据文件名，获取目录及文件

#### inode层
inode对上层提供inode服务
- `iget(int dev, int nr)` 根据inode number 获取指定的indoe
- `iput(struct m_inode * inode)` 释放掉一个inode对应的资源。注意需要和`iget`配合使用，否则会造成内存泄漏

关于inode的管理，系统会维护一个`inode_table[32]`数组。当调用`iget`的时候
- 先从`inode_table`找，找到则返回。没有则继续
- 从`inode_table`分配一个空闲项
	- 初始化
	- 从磁盘加载该inode信息
- 返回

注意，当读写inode的时候，会调用`lock_inode`和`unlock_inode`来上锁和解锁，保证并发一致性。

#### inode bitmap管理
这一层主要是inode的辅助函数
- `new_inode` 从`inode_table`获取一个新的空闲项并初始化
- `free_inode` 释放inode数据结构
- `new_block` 新增一个数据块
- `free_block` 释放一个数据块

### 目录、文件、inode
对用户而言，看到的文件系统就是一个个的目录和文件。那么这些是怎么组织起来的呢。
- 目录、文件本身都对应一个inode。
```
struct d_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned long i_size;
	unsigned long i_time;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];
};
```
- 文件的`i_zone`内存储的是数据块的逻辑号，目录的`i_zone`一样，但是其数据块内的数据是一个个的`dir_entry`。`dir_entry`内的`inode`指向了真正的文件。
```
struct dir_entry {
	unsigned short inode;
	char name[NAME_LEN];//lux 文件名
};
```
如上，一个文件是一个inode节点，但可以有多个文件名。也就是所谓“软链”，（这样看，文件名是属于目录结构的。）


### 文件系统的加载
- kernel main -> INIT_TASK(pid=1) 调用 `setup`
- `setup`读取硬盘信息，设置根文件系统
	- 解析从`setup.s`读取的硬盘信息（经由main.c传入），对每块硬盘
		- 读取mbr
		- 填充全局`hd`硬盘分区信息
	- 调用`mount_root`加载根文件系统
- `mount_root`
	- 根据全局设置的`ROOT_DEV`（根文件系统所在磁盘分区。在`bootsect.s`中指定。)，读取其`superblock`
		- 注意，这里指向的磁盘分区应该是格式化好并装载了Minix1.0 FS的磁盘。也就是说，这个磁盘是一个分好区的，且指定分区是格式化好了Minix1.0文件系统的。 
		- 如果是一个坏的文件系统，或者不是Minix1.0，下面也就无法继续了，系统开机启动失败（加载文件系统失败）。
	- 解析`superblock`
	- 获取“/”根目录所对应的inode，设为`mi`（已知1.所在磁盘分区，2.inodeNum（“/”为1），自然可以最终计算出“/”对应的inode在磁盘上的位置，读取之。）
		- 已知inodeNum，可以计算出该inode节点在在该磁盘分区的具体位置。
		- 已知磁盘分区的基本信息（开始位置，大小），可以最终得到该inode在物理磁盘上的位置（这些由硬盘驱动实现`do_hd_request`，上层无需关心）
	- 更新当前进程的`pwd`和`root`为`mi`
	```
	current->pwd = mi;//lux 当前进程的pwd和root
	current->root = mi;
	```
	- 计算并打印出可用block为多少
	- 计算并打印出可以inode为多少
	- 至此，根文件系统加载完毕

如上，其实根文件系统的加载，主要就是读取`superblock`，然后更新下当前进程的根目录和工作目录。

### 文件的创建/打开
- （创建）`sys_creat`调用`sys_open`
- （打开）`sys_open`
	- 在进程的打开文件数组找到空闲项`current->filp[]`
	- 在系统的打开文件数组找到空闲项`file_table[]`，并赋值到第一步找到的空闲项，记为`f`
	- 调用`open_namei`打开该文件，取到`inode`
	- 更新`f`
	```
	f->f_mode = inode->i_mode;//lux 初始化f，并返回
	f->f_flags = flag;
	f->f_count = 1;
	f->f_inode = inode;//lux 记录文件的inode
	f->f_pos = 0;//lux 文件偏移
	```
	- 返回`f`

关于`open_namei`：
- 根据给定的文件路径获取到所在目录`dir`
- 判断目录下：已经存在对应文件
	- 权限校验
	- 读取inode
	- 更新访问时间
	- 返回
- 判断目录下：不存在对应文件
	- 新建inode
	- 初始化信息
	- 加入到目录
	- 返回

### 文件的写入
- 先打开`sys_open`，获取句柄（即`flip[]`的下标）
- 写入`sys_write`
	- `file_write`(普通文件，其他情况参考[fs/read_write.c](fs/read_write.c))
		- 定位写入点pos(追加模式还是覆盖模式)
			- 追加模式取`pos = inode->i_size;`
			- 覆盖模式取`pos = filp->f_pos;`
		- 根据pos换算block
			- zone[9]，前7个为直接数据块，后面为一级、二级间接数据块。所以要根据pos的大小，找到它对应的位置，（如果需要新分块则新分块）。
		- 读取对应的block
		- 定位到开始位置，写入。（这里写的其实是bh（block head，缓存），系统会负责定时刷盘）
		- 更新inode信息
		- 返回

