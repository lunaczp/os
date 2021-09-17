# Socket
## TODO
- tcp实现
- 高性能网络配置、IO
- 网络驱动开发


## 相关代码

### [socket.h](include/linux/socket.h)

定义了socket的基础常量和数据结构，如Address Family,`sockaddr`
```c
struct sockaddr { /*lux 每一个socket都需要sockadr来指定地址*/
	sa_family_t	sa_family;	/* address family, AF_xxx	*/
	char		sa_data[14];	/* 14 bytes of protocol address	*/
};
```

### [net.h](include/linux/net.h)
socket的网络层定义，包含了`socket`结构体定义，`proto_ops`操作类定义等
```c
/**
 *  struct socket - general BSD socket
 *  @state: socket state (%SS_CONNECTED, etc)
 *  @type: socket type (%SOCK_STREAM, etc)
 *  @flags: socket flags (%SOCK_ASYNC_NOSPACE, etc)
 *  @ops: protocol specific socket operations
 *  @fasync_list: Asynchronous wake up list
 *  @file: File back pointer for gc
 *  @sk: internal networking protocol agnostic socket representation
 *  @wait: wait queue for several uses
 */
struct socket {
	socket_state		state;

	kmemcheck_bitfield_begin(type);
	short			type;
	kmemcheck_bitfield_end(type);

	unsigned long		flags;
	/*
	 * Please keep fasync_list & wait fields in the same cache line
	 */
	struct fasync_struct	*fasync_list;
	wait_queue_head_t	wait;

	struct file		*file;
	struct sock		*sk;
	const struct proto_ops	*ops;
};
```c
struct proto_ops {//lux 网络协议proto操作类
	int		family;
	struct module	*owner;
	int		(*release)   (struct socket *sock);
	int		(*bind)	     (struct socket *sock,
				      struct sockaddr *myaddr,
				      int sockaddr_len);
	int		(*connect)   (struct socket *sock,
				      struct sockaddr *vaddr,
				      int sockaddr_len, int flags);
	int		(*socketpair)(struct socket *sock1,
				      struct socket *sock2);
	int		(*accept)    (struct socket *sock,
				      struct socket *newsock, int flags);
	int		(*getname)   (struct socket *sock,
				      struct sockaddr *addr,
				      int *sockaddr_len, int peer);
	unsigned int	(*poll)	     (struct file *file, struct socket *sock,
				      struct poll_table_struct *wait);
	int		(*ioctl)     (struct socket *sock, unsigned int cmd,
				      unsigned long arg);
	int	 	(*compat_ioctl) (struct socket *sock, unsigned int cmd,
				      unsigned long arg);
	int		(*listen)    (struct socket *sock, int len);
	int		(*shutdown)  (struct socket *sock, int flags);
	int		(*setsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, unsigned int optlen);
	int		(*getsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, int __user *optlen);
	int		(*compat_setsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, unsigned int optlen);
	int		(*compat_getsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, int __user *optlen);
	int		(*sendmsg)   (struct kiocb *iocb, struct socket *sock,
				      struct msghdr *m, size_t total_len);
	int		(*recvmsg)   (struct kiocb *iocb, struct socket *sock,
				      struct msghdr *m, size_t total_len,
				      int flags);
	int		(*mmap)	     (struct file *file, struct socket *sock,
				      struct vm_area_struct * vma);
	ssize_t		(*sendpage)  (struct socket *sock, struct page *page,
				      int offset, size_t size, int flags);
	ssize_t 	(*splice_read)(struct socket *sock,  loff_t *ppos,
				       struct pipe_inode_info *pipe, size_t len, unsigned int flags);
};
```

其中`socket`结构又关联了一个`sock`，在`net/sock.h`定义

### [net/sock.h](include/net/sock.h)
`sock`实现`socket`的杂活累活

## 关键变量
```c
//net/core/sock.c
static LIST_HEAD(proto_list);//lux 所有的协议/protocol都注册在这里(thr proto_register)
```

```c
//net/socket.c
static const struct net_proto_family *net_families[NPROTO] __read_mostly;//lux 所有的协议簇都注册在这里。
```

## 初始化
### INET协议簇的初始化
```c
//net/ipv4/af_inet.c
fs_initcall(inet_init);//lux 注册INET协议簇的初始化函数
```

`inet_init`:
```c
//net/ipv4/af_inet.c
/**
 * lux INET协议簇的初始化，包括
 * 1. proto注册到全局proto_list @see core/sock.c
 * 2. INET注册到全局协议簇net_families @see socket.c
 * 3. proto注册到INET自身的hash表inet_protos
 * 4. 初始化各个协议，TCP、UDP、ICMP、etc
 */
static int __init inet_init(void
```

#### `fs_initcall`实现
[Ref](https://cstriker1407.info/blog/linux-kernel-initcall/)

`fs_initcall`使用的是`__define_initcall`,该宏用来定义一系列在内核启动时要加载的代码
```c
//include/linux/init.h

/* initcalls are now grouped by functionality into separate 
 * subsections. Ordering inside the subsections is determined
 * by link order. 
 * For backwards compatibility, initcall() puts the call in 
 * the device init subsection.
 *
 * The `id' arg to __define_initcall() is needed so that multiple initcalls
 * can point at the same handler without causing duplicate-symbol build errors.
 */

#define __define_initcall(level,fn,id) \
	static initcall_t __initcall_##fn##id __used \
	__attribute__((__section__(".initcall" level ".init"))) = fn

/*
 * Early initcalls run before initializing SMP.
 *
 * Only for built-in code, not modules.
 */
#define early_initcall(fn)		__define_initcall("early",fn,early)

/*
 * A "pure" initcall has no dependencies on anything else, and purely
 * initializes variables that couldn't be statically initialized.
 *
 * This only exists for built-in code, not for modules.
 */
#define pure_initcall(fn)		__define_initcall("0",fn,0)

#define core_initcall(fn)		__define_initcall("1",fn,1)
#define core_initcall_sync(fn)		__define_initcall("1s",fn,1s)
#define postcore_initcall(fn)		__define_initcall("2",fn,2)
#define postcore_initcall_sync(fn)	__define_initcall("2s",fn,2s)
#define arch_initcall(fn)		__define_initcall("3",fn,3)
#define arch_initcall_sync(fn)		__define_initcall("3s",fn,3s)
#define subsys_initcall(fn)		__define_initcall("4",fn,4)
#define subsys_initcall_sync(fn)	__define_initcall("4s",fn,4s)
#define fs_initcall(fn)			__define_initcall("5",fn,5)
#define fs_initcall_sync(fn)		__define_initcall("5s",fn,5s)
#define rootfs_initcall(fn)		__define_initcall("rootfs",fn,rootfs)
#define device_initcall(fn)		__define_initcall("6",fn,6)
#define device_initcall_sync(fn)	__define_initcall("6s",fn,6s)
#define late_initcall(fn)		__define_initcall("7",fn,7)
#define late_initcall_sync(fn)		__define_initcall("7s",fn,7s)

#define __initcall(fn) device_initcall(fn)
```

如上，这部分初始化代码最终被放到了指定的section段，`.initcall" level ".init"`
该代码段在`include/asm-generic/vmlinux.lds.h`
```asm
#define INITCALLS							\
	*(.initcallearly.init)						\
	VMLINUX_SYMBOL(__early_initcall_end) = .;			\
  	*(.initcall0.init)						\
  	*(.initcall0s.init)						\
  	*(.initcall1.init)						\
  	*(.initcall1s.init)						\
  	*(.initcall2.init)						\
  	*(.initcall2s.init)						\
  	*(.initcall3.init)						\
  	*(.initcall3s.init)						\
  	*(.initcall4.init)						\
  	*(.initcall4s.init)						\
  	*(.initcall5.init)						\
  	*(.initcall5s.init)						\
	*(.initcallrootfs.init)						\
  	*(.initcall6.init)						\
  	*(.initcall6s.init)						\
  	*(.initcall7.init)						\
  	*(.initcall7s.init)

#define INIT_CALLS							\
		VMLINUX_SYMBOL(__initcall_start) = .;			\
		INITCALLS						\
		VMLINUX_SYMBOL(__initcall_end) = .;

...

#define INIT_DATA_SECTION(initsetup_align)				\
	.init.data : AT(ADDR(.init.data) - LOAD_OFFSET) {		\
		INIT_DATA						\
		INIT_SETUP(initsetup_align)				\
		INIT_CALLS						\
		CON_INITCALL						\
		SECURITY_INITCALL					\
		INIT_RAM_FS						\
	}
```

内核启动的时候，会调用这部分函数
##### 启动路径
```c
//main.c
/*lux 启动内核
 * 对于i386，从head32.c进入
 */
asmlinkage void __init start_kernel(void

...

	/* Do the rest non-__init'ed, we're now alive */
	rest_init();//lux 其他的初始化(kernel_thread, initcall（网络协议等）），进入shell,etc
}
```

```c
//main.c
static noinline void __init_refok rest_init(void)
	__releases(kernel_lock)
{
	int pid;

	rcu_scheduler_starting();
	kernel_thread(kernel_init, NULL, CLONE_FS | CLONE_SIGHAND);//lux kernel_init, pid=1
	numa_default_policy()
```

```c
//lux kernel_init, pid=1
static int __init kernel_init(void * unused)
{

...
	do_basic_setup();//lux driver, initcall(网络等) etc


```c
//lux init(pid=1)进程负责的初始化工作
/*
 * Ok, the machine is now initialized. None of the devices
 * have been touched yet, but the CPU subsystem is up and
 * running, and memory and process management works.
 *
 * Now we can finally start doing some real work..
 */
static void __init do_basic_setup(void)
{
	init_workqueues();
	cpuset_init_smp();
	usermodehelper_init();
	init_tmpfs();
	driver_init();//lux 驱动
	init_irq_proc();
	do_ctors();
	do_initcalls();//lux 通过__define_initcall注册的初始化函数
}
```

```c
/**
 * lux 通过__define_initcall注册的初始化函数，
 * 如INET协议簇的初始化，@see net/ipv4/af_inet.c
 */
static void __init do_initcalls(void)
{
	initcall_t *call;

	for (call = __early_initcall_end; call < __initcall_end; call++)
		do_one_initcall(*call);

	/* Make sure there is no pending stuff from the initcall sequence */
	flush_scheduled_work();
}

```c
/**
 * lux 调用一个通过_define_initcall定义的初始化函数
 */
int do_one_initcall(initcall_t fn)
{
	int count = preempt_count();
	ktime_t calltime, delta, rettime;

	if (initcall_debug) {
		call.caller = task_pid_nr(current);
		printk("calling  %pF @ %i\n", fn, call.caller);
		calltime = ktime_get();
		trace_boot_call(&call, fn);
		enable_boot_trace();
	}

	ret.result = fn();
```


## 层次
1. Socket
2. Tcp
3. Ip
4. Dev
    1. Dev interface
    2. Hardware

1. Socket
    1. sendmsg
    2. sock_sendmsg
    3. __sock_sendmsg
        1. sock->ops->sendmsg(iocb, sock, msg, size);
2. TCP
    1. sendmsg = tcp_sendmsg
    2. __tcp_push_pending_frames
    3. tcp_write_xmit
    4. tcp_transmit_skb
        1.  icsk->icsk_af_ops->queue_xmit(skb, 0);
3. IP
    1. ip_queue_xmit
        1. 找到rt
            1. ip_route_output_flow
            2. __ip_route_output_key
            3. ip_route_output_slow
                1. 找到dev
            4. ip_mkroute_output
                1. rt_intern_hash
                    1. 指定neighbor
    2. ip_local_out
    3. dst_output
        1. skb_dst(skb)->output(skb);
    4. ip_finish_output
    5. ip_finish_output2
        1. dst->neighbour->output
4. Dev Interface
    1. output=neigh_resolve_output
        1.  neigh->ops->queue_xmit(skb); //arp.c
    2. dev_queue_xmit
    3. __dev_xmit_skb
    4. sch_direct_xmit
    5. dev_hard_start_xmit
        1. rc = ops->ndo_start_xmit(nskb, dev)
5. Hardware
    1. 如mac80211
        1. ndo_start_xmit=ieee80211_subif_start_xmit
        2. ieee80211_xmit
        3. ieee80211_tx
        4. __ieee80211_tx
        5. drv_tx
            1. 	return local->ops->tx(&local->hw, skb);//lux 发包
        6. Local 比如：rtl8187
            1. tx=rtl8187_tx



## TCP的相关算法
### 流量控制
滑动窗口
### 拥塞避免
- 慢开始
	- 慢开始门限(ssthresh)
	- 拥塞避免窗口(cwnd)
	- 发送窗口(swnd)
- 拥塞避免
- 快重传
- 快恢复

