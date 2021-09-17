# Make

## 生成系统镜像/启动盘
Linux镜像由三部分
- bootsect
- setup
- system

三个文件由`tool/build`检查后，按序写入一个镜像/磁盘，作为启动磁盘。

## 启动
- 把上面的磁盘设置为启动盘。
- 由于bootsect部分512字节，且标记为可启动，机器加电后，BIOS经过一系列初始化，然后会把这512字节代码加载到`0x7c00`处，并跳转过去，开始执行`bootsect`代码

### bootsect
- 先把自己从`0x7c00`移动到`0x90000`(bootsect总共512byte)
- 再把setup读取到内存地址`0x90200`
- 再把system读取到内存地址`0x10000`(2^16byte=64k处)
- 跳转到setup `0x90200`

### setup
- 读取硬件信息，并存储到 0x90000-0x901FF
- 移动内核(system)，对齐到0x00000
- 打开a20地址线
- 初始化idt
- 初始化gdt(保护模式需要至少定义code segment，data segment两个段)
    - 定义了8Mb的Code段
    - 定义了8Mb的Data段
- 标记cr0的PE=1，切换到保护模式
- 初始化8259A
    - BIOS设置的8259A的中断号没有采用Intel的规范，覆盖了Intel保留的中断号，这里调整回来，采用Intel的规定。
- 跳转到CS:0，即代码段内偏移0处，也即决定内存地址0x00000处，也就是system被加载的位置，进入head.s部分代码（head.s是system的第一个区块）
- head.s是32位初始化代码
    - 初始化IDT
    - 初始化GDT
    - 初始化分页(页表位于0x0，覆盖占用了原来head.s的位置)
    - 手动构造调用栈，返回，进入main函数
- kernel world