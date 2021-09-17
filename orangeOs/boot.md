# boot

## 计算机启动／重启流程
- 主板加电（开机按钮）／重启（reset按钮）[Ref](http://www.drdobbs.com/parallel/booting-an-intel-architecture-system-par/232300699?pgno=1)
- 主板电压初始化
- cpu初始化
    - hardware initialization
    - BIST(built-in self test) （可选）
    - See:_Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 9 Processor Management and Initialization_
- cpu获取当前es:eip值指向的地址空间存储的指令，并执行
    - 第一条指令的地址对于一个cpu是固定的，比如80x86是`cs:0xffff`,`ip:0x0000`，那么指令所在地址则是`0xffff0`，参考
        - [80x86 reset](https://en.wikipedia.org/wiki/Hardware_reset)
        - [x86 Reset vector](https://en.wikipedia.org/wiki/Reset_vector)
        - _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Section 9.1.4 First Instruction Executed_
- 该地址被主板的北桥、南桥芯片解释。[Northbridge](https://en.wikipedia.org/wiki/Northbridge_(computing))
    - 如果发现是热启动`warm boot`/软重启`soft reset`，则将该地址定位到已经加载到RAM的BIOS代码开始处。
    - 如果发现是冷启动`cold boot`/硬重启`hard reset`，则将该地址定位到BIOS所在固件的地址。
- cpu执行得到的BIOS指令 [BIOS](https://en.wikipedia.org/wiki/BIOS#System_startup)
    - BIOS判断是否是冷启动，不是则跳过POST，否则执行[POST](https://en.wikipedia.org/wiki/Power-on_self-test)：
        - 检查cpu
        - 检查BIOS自己
        - 检查RAM
        - 检查其他
    - BIOS执行加载程序，按照配置的顺序，查找可启动的分区（软盘，硬盘、网络）
        - 看第一个扇区是否读取正常并标记为可启动(512字节的最后两个是`0x55 0xAA`)
        - 找到，则加载该扇区到RAM的`0x0000:0x7c00`处，
        - 跳转到此处，BIOS done。
        - 开始引导启动。

## 其他
因为第一条指令被加载到了`0x0000:7c00`处（1kib below  32kib)，那么也就要求系统的RAM至少是32K。

## Ref
[CPU RESET](https://jin-yang.github.io/reference/linux/kernel/CPU_Reset.pdf)