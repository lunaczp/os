# Socket Init

- `main.c: start_kernel`
```c
//main.c
asmlinkage void start_kernel(void)
{
    ...
	mem_init(memory_start,memory_end);
	buffer_init();
	sock_init();//lux 网络组件初始化 see net/socket.c
```
- `socket.c: sock_init, proto_init`
```c
//net/socket.c
void sock_init(void)
{
    ...
	for (i = 0; i < NPROTO; ++i) pops[i] = NULL;
    ...
#ifdef CONFIG_FIREWALL	 
	fwchain_init();
#endif

	/*
	 *	Initialize the protocols module. 
	 */

	proto_init();//lux 各类协议初始化 unix domain, tcp/ip...

//net/socket.c
//lux 各类协议初始化（协议定义在net/protocols.c）
void proto_init(void)
{
	extern struct net_proto protocols[];	/* Network protocols */ //lux see net/protocols.c
	struct net_proto *pro;

	/* Kick all configured protocols. */
	pro = protocols;
	while (pro->name != NULL) 
	{
		(*pro->init_func)(pro);//lux 调用初始化函数
		pro++;
	}
	/* We're all done... */
}

//net/protocols.c
struct net_proto protocols[] = {
#ifdef	CONFIG_UNIX
  { "UNIX",	unix_proto_init	},			/* Unix domain socket family 	*/
#endif
#if defined(CONFIG_IPX)   || defined(CONFIG_IPX_MODULE) || \
    defined(CONFIG_ATALK) || defined(CONFIG_ATALK_MODULE)
  { "802.2",	p8022_proto_init },			/* 802.2 demultiplexor		*/
  { "802.2TR",	p8022tr_proto_init },			/* 802.2 demultiplexor		*/
  { "SNAP",	snap_proto_init },			/* SNAP demultiplexor		*/
#endif
#ifdef CONFIG_TR
  { "RIF",	rif_init },				/* RIF for Token ring		*/
#endif  
#ifdef CONFIG_AX25  
  { "AX.25",	ax25_proto_init },
#ifdef CONFIG_NETROM
  { "NET/ROM",	nr_proto_init },
#endif
#endif  
#ifdef	CONFIG_INET
  { "INET",	inet_proto_init	},			/* TCP/IP			*/
#endif
#ifdef  CONFIG_IPX
  { "IPX",	ipx_proto_init },			/* IPX				*/
#endif
#ifdef CONFIG_ATALK
  { "DDP",	atalk_proto_init },			/* Netatalk Appletalk driver	*/
#endif
  { NULL,	NULL		}			/* End marker			*/
};

```
- 以`unix domain socket`为例
```c
//net/unix/af_unix.c
//lux unix proto注册
void unix_proto_init(struct net_proto *pro)
{
	printk(KERN_INFO "NET3: Unix domain sockets 0.13 for Linux NET3.035.\n");
	sock_register(unix_proto_ops.family, &unix_proto_ops);
#ifdef CONFIG_PROC_FS
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_UNIX,  4, "unix",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		unix_get_info
	});
#endif
}
```

如上，就是socket协议初始化的过程。最终，所有的协议都通过`sock_register`注册到了全局的协议数组`pops`，提供了其回调函数。
- socket操作类对上层提供统一的服务接口`socket`,`bind`,`connect`,..
- socket操作类内部调用各个协议注册的回调函数