# segment

分段是8086开始有的，最开始的目的是为了提供超过64k的cpu寻址空间。因为当时地址总线是20位，可以支持1M地址空间，但是CPU的寄存器是16位的，只支持64k的寻址空间。为了能够匹配上1M的地址空间。Intel的寻址协议做了调整，利用16位的寄存器，实现了1M寻址空间。做法是：
- 把寻址空间分段，对一个地址的定位是段地址+段内地址
- 用16位寄存器保存段基址，最终的地址定位是(segBase * 2^4) + segOffset
    - 之所以可以这么做，要求段至少是2^4字节对齐的。这样就可以用16位寄存器，存储20位的地址长度了，（低4位为0，存储的时候拿掉，计算的时候加上）。
如上，在8086实现了1M虚拟寻址空间的寻址。

从80286开始，Intel采用了新的分段协议，之前的分段模式被称为`实模式`，新的被称为`保护模式`。在新的模式下，Intel提供了更多保护机制，包括特权级、分页等。简单来讲
- 实模式的初衷是为了兼容更大寻址空间
- 保护模式的初衷就是提供了更多安全机制

从x86-64开始，分段是禁用的。（理解是因为地址空间足够大以至于不需要复杂的机制来处理，直接使用就行）

## ia-32模式下的内存模型
- Basic Flat Model 平坦模型，所有内存都在一个地址空间。使用者只需要指定两个segment descriptor（CS，DS），一个给代码，一个给数据。
- Protected Flat Model 在Flat Model的基础上，加上校验和分页机制，提供了页级的访问控制。结合起来可以做进程间的访问控制
- Multi-Segment Model 每个进程有单独的segment，从而提供硬件级别的权限控制。

Ref: Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 3 Using Segments

### 实际使用
实际上，Linux系统并没有采用x86提供的方案。

> Linux generally uses neither. On x86, Linux has separate page tables for userspace processes and the kernel. The userspace page tables do not contain mappings to kernel memory, which makes it impossible for user-space processes to access kernel memory directly.

> Technically, "virtual addresses" on x86 pass through segmentation first (and are converted from logical addresses to linear addresses) before being remapped from linear addresses to physical addresses through the page tables. Except in unusual cases, segmentation won't change the resulting physical address in 64 bit mode (segmentation is just used to store traits like the current privilege level, and enforce features like SMEP).

> One well known "unusual case" is the implementation of Thread Local Storage by most compilers on x86, which uses the FS and GS segments to define per logical processor offsets into the address space. Other segments can not have non-zero bases, and therefore cannot shift addresses through segmentation.

See:[StackOverflow](https://stackoverflow.com/questions/37559282/which-type-of-memory-model-i-e-flat-segmentation-is-used-by-linux-kernel)


### 备注
* 在16bit模式下，segment是可选的。
* 在32bit模式下，segment是必须开启的。
* 在64bit模式下，segment是禁用的。

## Read More
> x86 memory segmentation refers to the implementation of memory segmentation in the Intel x86 computer instruction set architecture. Segmentation was introduced on the Intel 8086 in 1978 as a way to allow programs to address more than 64 KB (65,536 bytes) of memory. The Intel 80286 introduced a second version of segmentation in 1982 that added support for virtual memory and memory protection. At this point the original model was renamed real mode, and the new version was named protected mode. The x86-64 architecture, introduced in 2003, has largely dropped support for segmentation in 64-bit mode.

> In both real and protected modes, the system uses 16-bit segment registers to derive the actual memory address. In real mode, the registers CS, DS, SS, and ES point to the currently used program code segment (CS), the current data segment (DS), the current stack segment (SS), and one extra segment determined by the programmer (ES). The Intel 80386, introduced in 1985, adds two additional segment registers, FS and GS, with no specific uses defined by the hardware. The way in which the segment registers are used differs between the two modes.[1]

> The choice of segment is normally defaulted by the processor according to the function being executed. Instructions are always fetched from the code segment. Any stack push or pop or any data reference referring to the stack uses the stack segment. All other references to data use the data segment. The extra segment is the default destination for string operations (for example MOVS or CMPS). FS and GS have no hardware-assigned uses. The instruction format allows an optional segment prefix byte which can be used to override the default segment for selected instructions if desired.

## Ref
- [X86 Memory Segment:wikipedia](https://en.wikipedia.org/wiki/X86_memory_segmentation)
- _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 3 PROTECTED-MODE MEMORY MANAGEMENT_