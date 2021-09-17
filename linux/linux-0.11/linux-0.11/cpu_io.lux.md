# CPU IO 

Ref: _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 1-Basic Architecture.pdf 17 Input/Output_

The processor permits applications to access I/O ports in either of two ways:
- Through a separate I/O address space
- Through memory-mapped I/O

## IO Port
Intel x96提供了2^16（64k）的IO地址空间。`0x0000-0xFFFF`，但是`0x00F8-0x00FF`是保留的。