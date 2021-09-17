# The Design of the UNIX Operating System（1986）

# Preface
- UNIX 1974年首次被正式介绍。
    - The UNIX system was  first described in a 1974 paper in the Communications of the ACM [Thompson 74] by Ken Thompson and Dennis Ritchie.
- 在本书（1986）之前，已经有其他书籍介绍UNIX
    - 1984. The UNIX Programming Environment
    - 1985. Advanced UNIX Programming
    这些书主要介绍编程接口（Programming Interface），而本书则关注系统本身`The Kernel`。
- 本书的内容和素材主要来自于作者在1983-1984期间，在贝尔实验室的课程。
- 本书的介绍主要基于UNIX System V Release 2，以及一些Release 3的特性。另外会提及一些BSD的版本。


# General overview of the system
## History
- 1965 Bell联合GE和其他公司，合力打造一个新的操作系统`Multics`，目的是开发一个多任务的、有强大处理能力的、数据存储、数据共享的操作系统。
- 1969年，`Multics`得以运行在GE 645，但是预期目标没有达到，而且也没有明确的计划何时能够实现。逐渐，Bell实验室退出这个计划。
- 为了完善自己的工作环境`improve there programming environment，Ken Thompson，Dennis Ritchie和其他人起草了一个文件系统论文（之后成为早期的UNIX文件系统）。Thompson模拟测试了这个文件系统，甚至在GE 645上实现了一个简单的kernel。
- Thompson给GECOS系统用Fortran编写了一个游戏程序“Space Travel”，但是在GECOS使用起来很昂贵。随后Thompson找到一台没人用的PDP-7（`provided good graphic display and cheap executing power`），尝试在上面跑自己的程序。但此时需要在GECOS上交叉编译，再在PDP-7上运行。
    - (1969) 为了提升编程效率，Thompson和Ritchi开始在PDP-7上设计一个新系统。最终实现了一个早期的UNIX文件系统、进程子系统、工具集。这样，不再需要GECOS，直接在PDP-7上建立了编程环境，可以直接开发程序。这个系统就被成为UNIX。
- 1971年，为了给专利局提供一个文本处理程序，UNIX被移植到PDP-11。Thompson本来打算给UNIX提供一个Fortran编译器，结果实现了B语言，一个BCPL的变种。由于B是一个解释性语言，性能较低，之后Ritchie把它重新设计，成为C语言(`allowing generation of machine code, declaration of data types, definition of data structures`).
- 1973年，UNIX用C重写。
- 由于法律限制，AT&T不能商业化计算机产品，所以把UNIX免费提供给了需要它的大学。
- 1974年，Thompson and Ritchie published a paper describing the UNIX system in the Communications of the ACM
- 1977年，UNIX首次被移植到了非PDP机器。
- 随着UNIX的流行，很多公司都移植UNIX到自己的机器，并加强了很多功能，从而产生了很多变体。
    - 1977-1982，Bell实验室结合了多个版本的UNIX，提出了UNIX System V。1983年，AT&T宣布对System V提供官方支持。
    - 同时伯克利大学开发了自己的UNIX，最新的是（截止1986）`BSD 4.3 for VAX`。

_UNIX 流行的原因：_
- 高级语言编写，易读、易理解、易修改、易移植
- `simple use interface`
- 层级文件系统，易维护和实现。
- 多用户、多任务系统
- 隐藏硬件细节，易于编写可移植的程序。

## System structure
## User perspective
## Operating System services
## Assumption about hardware


# Introduction to the kernel
## Architecture of the UNIX operating system
Two major components of the kernel
- File system
- Process subsystem

> the UNIX system supports the illusions that the file system has "places" and that processes have "life."

## Introduction to system concepts
### File system
三个`table`
- `the user file discriptor table` 每个进程一个，包含该进程打开的所有文件
- `file table`全局一个，包含操作系统所有打开的文件
- `inode table`全局一个，包含文件系统内所有的文件

一个文件系统，包含如下组件
- `boot block`，一般是磁盘分区第一个块，包含启动代码
- `super block`，文件系统的描述
- `inode list`，每个代表一个文件条目
- `data blocks` 数据区块（文件的真实存储）

### Processes
三个`table`
- `process table`
- `per process region table`
- `region table`

#### 进程状态及转换

## Kernel data structures
## System Administration


# The buffer cache
Buffer 组件位于文件系统和磁盘系统之间，提供磁盘读写的缓存，用于提升文件系统的性能。典型地：
- 读数据前，先读缓存，没命中，再读磁盘。
- 写数据前，先写缓存，随后异步刷入磁盘。

## Buffer headers
Buffer header是buffer管理的基本单元。一个buffer header对应一个buffer，对应一个物理磁盘block。header内包含：
- 设备号
- 块号
- 指向数据块的指针
- 其他
## Structure of the buffer pool
系统维护两个数据结构
- `free list`， 空闲链表（双向链表），链表内的每一项是空闲的header数据结构。初始时候空闲链表为空。
- `buffer pool`，缓存池，维护一个哈希队列`hash queue`，按照设备号哈希。方便查找。

操作：
- 每次需要一个新的缓存块时候，从`free list`取出一个，初始化数据，并加入到哈希队列。

## Scenarios for retrieval of a buffer
## Reading and writing disk blocks
## Advantages and disadvantages of the buffer cache


# Internal representation of files
## Inodes
## Structures of a regular file
## Directories
## Conversion of a path name to an inode
## Super block
## Inode assignment to a new file
## Allocation of disk blocks
## Other file types

# System calls to the file system
## Open
## Read
## Write
## File and record locking
## Lseek
## Close
## File creation
## Creation of special files
## Change directory and change root
## Change owner and change mode
## Stat and fstat
## Pipes
## Dup
## Mounting and unmounting file systems
## Link
## Unlink
## File system abstractions
## File system maintenance


# The structure of processess
## Process states and transitions

## Layout of system memory
### Region
进程在内存内，是按Region分布的
- Text
- Data
- Stack
### Pages and page table
- kernel和process的page table独立。

## The context of a process
- 进程上下文
- 进程上下文切换

## Saving the context of a process
## Manipulating of the process address space
## Sleep

# Process control
## Process creation
## Signals
## Process termination
## Awaiting Process termination
## Invoking other process
## The user id of a process
## Changing the size of a process
## The shell
## System boot and the init process


# Process scheduling and time
## Process scheduling
## System calls for time
## Clock


# Memory management policies
## Swapping
## Demand paging
## A hybrid system of swapping and demand paging

# The I/O Subsystem
## Driver interfaces
## Disk drivers
## Terminal drivers
## Streams


# Interprocess communication
## Process tracing
## System V IPC
## Network communications
## Sockets


# Multiprocessor systems
## Problem of multiprocessor systems
## Solution with master and slave processors
## Solution with semaphores
## The tunis system
## Performance limitations


# Distributed unix systems
## Satellite processors
## The Newcastle connection
## Transparent distributed file systems
## A transparent distrubuted model without stub processes