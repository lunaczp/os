# Todo
- 语言进化
    - Fortran中读写文件


- ~~BIOS stuff disk info in 0x475?~~
    - on bootup, BIOS will collect and check hardware info, where does it store?
- ~~权限控制RPL CPL DPL~~
- ~~tty~~
- ~~blk 锁机制与资源竞争~~
- ~~Linux Minix Unix总结整理~~
- ~~为什么system可以直接加载到内存，而不用解析（elf32）~~
    - 其实system是被处理过了，已经去掉了头部，参考`build.c`
- ~~Paging~~
- ~~段与寻址~~
    - 程序内的地址是相对地址，在寻址的时候，会配合cs/ds来实现。而cs/ds保存的是段基址。结合得到线性地址
    - 线性地址经过页表映射得到物理地址。
- ~~内存分页管理~~

- ~~ret,iret~~
    - iret用于interrupt gate和exception gate，影响EFLAGS
- ~~c函数调用，是否会处理堆栈~~
    - 调用者维护堆栈
