# Block Device
块设备是可以进行寻址的设备。常用来做计算机的存储设备，如硬盘、软盘等。

## 命名
```
//ll_rw_blk.c:35
struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
	{ NULL, NULL },		/* no_dev */
	{ NULL, NULL },		/* dev mem */
	{ NULL, NULL },		/* dev fd */	/*lux 软盘，index=2 */
	{ NULL, NULL },		/* dev hd */	/*lux 硬盘，index=3 */
	{ NULL, NULL },		/* dev ttyx */	/*lux ttyx 虚拟终端 4 */
	{ NULL, NULL },		/* dev tty */	/*lux tty 5 */
	{ NULL, NULL }		/* dev lp */	/*lux lp 打印机 6 */
};
```
Linux下，设备号分为主（大/Major）设备号码、次（小/Minor）设备号。`blk_dev`的数组下标就代表主设备号，比如floppy是2，hard disk是3。

__下面是Linus Torvalds提交linux0.11的commit note：__

硬盘：
>In order to use the harddisk as root, this value has to be changed to
point to the correct device. Harddisks have a major number of 3 under
linux, and the minor nr is the same as the number X in /dev/hdX. The
complete device number is then calculated with
>
>DEV_NO = (major<<8)+minor
>
>or alternatively major*256+minor. Thus /dev/hd1 is (3<<8)+1 = 0x301,
/dev/hd6 = 0x0306 etc. Assuming the partition you made into the new root
was /dev/hd2, you will have to write 0x0302 into the boot-image. That
is, you should change the 508th byte in the image to 0x02, and the 509th
byte to 0x03. There is a sample program for this in some of the older
INSTALL-notes, if you don't understand what it's all about.

对于floppy：
> As with harddisk, floppies have device numbers, but this time major = 2
instead of 3. The minor number is not as easy: it's a composite that
tells which drive (A, B, C or D) and what type of drive (360kB, 1.2M,
1.44M etc). The formula is 'minor = type*4+nr', where nr is 0-3 for A-D,
and type is 2 for 1.2M disks, and 7 for 1.44M disks. There are other
types, but these should suffice for now.
>
> Thus if you have a 1.2M A-drive, and want to call it "floppy0", you have
to tell linux so. This is done with the "mknod" command. mknod takes 4
paramters: the unix name of the device, a "b" or a "c" depending on
whether it's a Block of Character device, and the major and minor
numbers. Thus to make "floppy0" a 1.2M A-drive, you write:
>
> mknod /dev/floppy0 b 2 8
>
> b is for Block-device, the 2 is for floppy, and the 8 is 4*2+0, where
the 2 is 1.2M-drive and the 0 is drive A. Likewise to make a "floppy1"
device that is a 1.44M drive in B, you write:
>
> mknod /dev/floppy1 b 2 29
>
> where 29 = 4*7 + 1. There are a couple of standard names, for users
that are used to minix (major, minor in parentheses): /dev/PS0 is a
1.44M in A (2,28), /dev/PS1 a 1.44M in B (2,29), /dev/at0 is a 1.2M in A
(2,8), /dev/at1 is a 1.2M in B (2,9). Use mknod to make those that fit
your computer.


综上，
- 硬盘harddisk
```
DEV_NO = (major<<8)+minor
marjor = 3
minor = X (where X is as in /dev/hdX)
```

- 软盘floppy
```
DEV_NO = (major<<8)+minor
marjor = 2
minor = type*4+nr
    type: 2 for 1.2M-drive, 7 for 1.44M-drive
    nr: 0-A 1-B 2-C 3-D
```

另关于硬盘分区，参考[blk.h:5](kernel/blk_drv/blk.h)的注释。

## 模型
```
|--------------------------------------------
|   file system
|--------------------------------------------
|   ll_rw_blk.c:ll_rw_block
|--------------------------------------------
|   hd.c floppy.c
|--------------------------------------------
```
如上，`ll_rw_blk`对上层提供块设备的读写服务，内部封装了不同设备的读写。
- `ll_rw_blk`实现来一个请求队列`request[]`
- 所有的磁盘读写，入口都是`ll_rw_block`
    - `ll_rw_block`调用`make_quest`初始化request
    - `make_request`调用`add_request`加入到请求队列

```
//blk.h
struct blk_dev_struct {
	void (*request_fn)(void);//lux 该块设备的请求处理函数
	struct request * current_request;//lux 该块设备的当前请求
};

//ll_rw_block.c
struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
	{ NULL, NULL },		/* no_dev */
	{ NULL, NULL },		/* dev mem */
	{ NULL, NULL },		/* dev fd */	/*lux 软盘，index=2 */
	{ NULL, NULL },		/* dev hd */	/*lux 硬盘，index=3 */
	{ NULL, NULL },		/* dev ttyx */	/*lux ttyx 虚拟终端 4 */
	{ NULL, NULL },		/* dev tty */	/*lux tty 5 */
	{ NULL, NULL }		/* dev lp */	/*lux lp 打印机 6 */
};
```
每个类型的块设备都注册在`blk_dev`内，且都有各自等读写回调`request_fn`。

当`add_request`加入请求队列时候，
- 如果当前没有要正处理的请求，则直接处理加入的这个请求。调用`request_fn`
    - 对于硬盘，`request_fn`对应`do_hd_request`
        - `do_hd_request`回发送磁盘读写命令，并设置回调，回调负责收集数据，通知调用方数据ready。
- 如果当前有处理的请求，则加入队列等待。
    - `request_fn`是一个循环处理逻辑，只要列表不为空，就会循环处理，从而保证只要有请求就能被处理。

如上，就实现了磁盘读写。

## 硬盘
- 硬盘的读写入口是：`do_hd_request`，提供给`ll_rw_block`使用。
- 硬盘的回调是：`do_hd`
    - 因为磁盘IO是异步的，当程序发命令给磁盘读写后，程序就继续运行了，不会等待磁盘IO。
    - 当磁盘IO完成后，会调用设置好的回调，通知程序数据ready。
    - 不同的命令，对应的回调不一样，所以`do_hd`是动态设置的。可能的回调有
        - `read_intr` 读回调
        - `write_intr` 写回调

## 软盘
软盘类似硬盘
- 读写入口`do_fd_request`
- 回调`do_floppy`，可能的值
    - `rw_interrupt` 读写回调
    - `recal_interrupt` 重新校正
    - `reset_interrupt` 重置

软盘较为复杂的一点是：当不工作的时候，马达是关闭的。
 - 所以有数据需要读写的时候需要开启
 - 读写完毕需要关闭。

 如此
 - 当有请求的时候，会先打开马达。然后等待启动完毕再执行读写操作。
 - 当请求完毕，过一段时间自动关闭马达。

 这里的实现，使用了定时器来控制时间。具体请参考[kernel/blk_drv/floppy.c](kernel/blk_drv/floppy.c)
