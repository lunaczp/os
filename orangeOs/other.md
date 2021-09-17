# other

## 直接写命令
处理器只是根据当前地址，尝试读取一个内存块并判断是否是有效命令，是则执行，不是则抛出异常。

```
; 64-ia-32-architectures-software-developer-manual-vol-3-System Programming chp9.10.2
; load GDTR and IDTR
MOV     EBX,RAM_START
        DB 66H
LGDT    APP_GDT_ram[EBX] ; execute a 32 bit LGDT
```

如上，就是直接用`DB 66H`，写入66，和后面的LGDT结合起来，就是一个32位的指令。你只需要这么写，处理器会负责解析，因为这个规范是处理器定的。(64-ia-32-architectures-software-developer-manual-vol-3-System Programming chp21 MIXING 16-BIT AND 32-BIT CODE)

甚至，你可以直接写01，要符合处理器的约定（是处理器可以识别的命令），就可以执行。当然，你需要一个入口让cpu来执行你的命令。比如你可以把写好的01指令放到软盘，并标记软盘可启动，然后设置BIOS开机从软盘启动。这样，开机后就会加载你的代码到内存0000:7c00处，然后开始执行你的代码。

当然，你可能会说，我直接写01，然后在我的Mac／Windows／Unix下能否执行呢，其实是不可以的。因为不同操作系统要求的可执行文件格式都不一样，并不是直接在文件里放执行代码就可以（当然你可以写个操作系统来支持这种格式）。

一般的操作系统下的可执行文件都要包含一些额外的信息，通过这些信息，操作系统就知道这个可执行程序的数据、代码以及其他一些属性，然后按照约定加载进内存，然后跳转到开始处，这时才真正执行你的代码。

如上，操作系统上的可执行代码，是程序和操作系统之间的约定，一切操作由操作系统代理和规定，你按照操作系统的规定，才能使用它。如果开机直接执行的代码，是程序和硬件（主要是处理器）的约定，你知道处理器的规定，就可以按照固定指挥它。

不同层层级的约定、封装、协作，anything is possible。

## 处理器初始化
Ref: _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 9 Processor Management and Initialization_

## 工具
* [在Mac上安装交叉编译工具](https://github.com/cfenollosa/os-tutorial/tree/master/11-kernel-crosscompiler)


## Ref
- Intel 64 and IA-32 Architectures Software Developer's Manual Volume 1-Basic Architecture
- Intel 64 and IA-32 Architectures Software Developer's Manual Volume 2-Instruction Set Reference
- Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide