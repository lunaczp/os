# IO Multiplexing

## poll

### 实现
- `sys_poll`
- `do_sys_poll`
- `do_poll`
    - loop，对每一个fd，调用`do_pollfd`
    - `do_pollfd`
        - 调用`mask = file->f_op->poll(file, pwait);`

### 分析
- 相对于`select`，没有了数量限制。
- 但内核需要遍历要监听的描述符。
- 返回的结果，依然需要遍历，效率低。

## epoll

### 实现
- `epoll_create`新增一个实例
- `epoll_ctl` 增删改要监听的句柄及事件
- `epoll_wait`获取结果
    - 在`rdlist`为空的时候，会休眠。一旦监听的资源事件发生，会调用回调函数`ep_poll_callback`，由它来唤起休眠的进程，继续处理rdlist

epoll对每一个实例（`epoll_create`创建的），
- 维护一个红黑树，元素是`epitem`，代表一个监听（`epoll_ctl`加入的）。
- 维护一个`rdlist`，代表已经ready的监听。

针对每一个文件的监听，都会向该文件注册回调函数，当事件发生，会触发回调函数，唤醒等待的进程队列（`epoll_wait`当没有事件的时候，会把自己加入等待列表）。

### 分析
相对于`select`和`poll`，效率更高。
- `select`和`poll`每次都会遍历监听列表，调用底层的`select`和`poll`来检查是否有事件产生。
    - `epoll`通过回调来主动通知事件的发生。`epoll_wait`只需要在被唤起的时候读取`rdlist`就行（肯定有数据），而不用盲目的轮训
- `select`和`poll`每次调用都需要重新从用户空间拷贝要监听的fd数组到内核空间。由于通常的用法都是
```
while(1) {
    select()
    poll()
    epoll_wait()
}
```
所以，当监听的文件句柄较多的时候，每次调用`select`和`poll`的开销是很大的。  
而`epoll`只需要提前注册一次，然后调用`epoll_wait`即可。


### 其他
> 如果没有大量的idle -connection或者dead-connection，epoll的效率并不会比select/poll高很多，但是当遇到大量的idle- connection，就会发现epoll的效率大大高于select/poll。
Ref:https://segmentfault.com/a/1190000003063859

因为epoll不需要遍历，而select 和poll都需要遍历全部监听的事件，假如监听的1000个链接当中只有10个活跃链接，显然select 和poll 会做很多无用功。