# Socket

## Select
### 实现
`select`系统调用是最基本的系统IO服务接口。
```
int select(int n, fd_set *inp, fd_set *outp, fd_set *exp, struct timeval *tvp)
```

它的实现流程如下：
- `sys_select`
```
//fs/select.c
asmlinkage int sys_select(int n, fd_set *inp, fd_set *outp, fd_set *exp, struct timeval *tvp)
{
    ...
    error = do_select(n,
	(fd_set *) &in,
	(fd_set *) &out,
	(fd_set *) &ex,
	(fd_set *) &res_in,
	(fd_set *) &res_out,
	(fd_set *) &res_ex);//lux 当返回的时候，要么是监听到事件了，要么是出错了。
```
- `do_select`
```
//fs/select.c
static int do_select(int n, fd_set *in, fd_set *out, fd_set *ex,
	fd_set *res_in, fd_set *res_out, fd_set *res_ex)
{
    ...
   	if (FD_ISSET(i,in) && check(SEL_IN,wait,file)) {//lux 是否可读
		FD_SET(i, res_in);
		count++;
		wait = NULL;
	}
```
- `check`调用底层对应的文件类型的select，以unix domain socket为例
    - `sock_select`
    - `unix_select`
```
//fs/select.c
static int check(int flag, select_table * wait, struct file * file)
{
	struct inode * inode;
	struct file_operations *fops;
	int (*select) (struct inode *, struct file *, int, select_table *);//lux 声明了一个函数指针变量

	inode = file->f_inode;
	if ((fops = file->f_op) && (select = fops->select))
		return select(inode, file, flag, wait)
		    || (wait && select(inode, file, flag, NULL));
	if (flag != SEL_EX)//lux 否则，如果没有加互斥锁，则返回正常。
		return 1;
	return 0;//lux 否则失败。
```

```
//net/socket.c
static int sock_select(struct inode *inode, struct file *file, int sel_type, select_table * wait)
{
	struct socket *sock;

	sock = socki_lookup(inode);//lux 找到inode对应的socket

	/*
	 *	We can't return errors to select, so it's either yes or no. 
	 */

	if (sock->ops->select)
		return(sock->ops->select(sock, sel_type, wait));//lux 调用协议本身的select回调，比如对于unix domain socket，调用unix_select
	return(0);
}
```

```
//net/unix/af_unix.c
static int unix_select(struct socket *sock,  int sel_type, select_table *wait)
{
	return datagram_select(sock->data,sel_type,wait);
}

//net/core/datagram.c
int datagram_select(struct sock *sk, int sel_type, select_table *wait)
{
	select_wait(sk->sleep, wait);//lux 插入等待队列
	switch(sel_type)
	{
		case SEL_IN://lux 读
			if (sk->err)
				return 1;
			if (sk->shutdown & RCV_SHUTDOWN)
				return 1;
			if (connection_based(sk) && sk->state==TCP_CLOSE)
			{
				/* Connection closed: Wake up */
				return(1);
			}
			if (skb_peek(&sk->receive_queue) != NULL)
			{	/* This appears to be consistent
				   with other stacks */
				return(1);
			}
			return(0);

//linux/include/linux/skbuff.h
extern __inline__ struct sk_buff *skb_peek(struct sk_buff_head *list_)
{
	struct sk_buff *list = ((struct sk_buff *)list_)->next;
	if (list == (struct sk_buff *)list_)//lux 到头了
		list = NULL;
	return list;
}
```

注意
- 所有的socket层数据包，都是通过`skb_*`操作类来完成。如
    - `sbk_peek` 检查是否有数据（可读）
    - `skb_dequeue` 读取一个数据包

最终，`check`返回0（no），1（yes），`socket`返回正数（监听到的事件数），其他（失败）。

### 说明
`select`的工作模式，是这样，
- 用户调用`select`，设置超时t
- 系统修改调整当前进程的时间片为t1（由t换算出来），调用`do_select`
- `do_select`进入循环调度
```
repeat:
	current->state = TASK_INTERRUPTIBLE;
	for (i = 0 ; i < n ; i++) {
    	struct file * file = current->files->fd[i];

        ...

	/* lux 同时满足以下情况，则重新调度（下次继续检测）
	 * 1. 没有检测到事件
	 * 2. 还有时间片
	 * 3. 没有信号
	 * 否则，任何其他情况，都不再处理，返回count。（比如检测到事件了，或者时间片用完了，或者有信号（需要在返回用户态之前由内核调用用户的回调））
	 */
	if (!count && current->timeout && !(current->signal & ~current->blocked)) {
		schedule();
		goto repeat;
	}
	free_wait(&wait_table);
	free_page((unsigned long) entry);
	current->state = TASK_RUNNING;
	return count;
```
如代码，`do_select`每次调度一次，都会遍历一次当前进程的打开文件数组`fd`，依次去检查是否有符合条件的。如果此时 1.没有检测到。2.还有时间片。3.没有信号，那么会调用`schedule`交出cpu重新调度。当被重新调用，代码还是回到循环，重新检查一遍。直到条件发生变化（有事件、或者时间片用完了、或者有信号），此时返回。

- 返回到`sys_select`，它
    - 整理资源、更新状态
    - 设置返回值，返回到用户。

至此，我们知道，当调用`select`，该进程不能做其他事情，会阻塞到`select`，每次进程调度到该进程，它都是在内核态遍历文件，检查状态。直到检测到事件发生、超时、检测到信号，才会返回。

### 缺点
- 个数限制，局限于打开文件的数量，1024
- 每次内核都要重新遍历（当然不会全部遍历，因为指定了最大值），效率低
- 进程不会休眠，在时间片消耗完之前（超时之前）会重复运行（每次唤起，检查一次，然后schedule out；再次唤起。。。直到超时/找到事件/有信号）
	- 而2.6.32中`poll`的实现，是检查一次，如果没有，休眠直到超时，检查，返回最终结果。参见:`../linux2.6.32`文档
- 每次用户拿到数据后，也要遍历，效率低
- 每次使用之后，都要重新设置要检测的fd，麻烦，效率低。