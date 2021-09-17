# C Basic

##  Arrays of Length Zero
```c
//fs/select.c
struct poll_list {
	struct poll_list *next;
	int len;
	struct pollfd entries[0];
    };
```

Ref: https://gcc.gnu.org/onlinedocs/gcc/Zero-Length.html
> Declaring zero-length arrays is allowed in GNU C as an extension. A zero-length array can be useful as the last element of a structure that is really a header for a variable-length object:
```c
struct line {
  int length;
  char contents[0];
};

struct line *thisline = (struct line *)
  malloc (sizeof (struct line) + this_length);
thisline->length = this_length;
```

## 变量作用域与内存分布
```c
#include <stdio.h>

const int A = 10;
int a = 20;
static int b = 30;
int c;

int main(void)
{
	static int a = 40;
	char b[] = "Hello world";
	register int c = 50;

	printf("Hello world %d\n", c);

	return 0;
}

```
- 文件作用域/全局作用域
  - `const int A` 在`.rodata section`，生成可执行文件后，和`.text section`合并在可加载、可执行、只读段（`load，RE segment`）。
  - `int a`在`.data section`，并最终在可加载，可读写段（`load RW segment`）
  - `static int b`，类似`int a`，不同的是，它的作用域是`LOCAL`
  - `int c`,没有初始化的变量，在`.bss`，并最终在可加载，可执行段（`load RW segment`）。（`.bss`和`.data`最终在一个段，一般地，`.bss`在内存中紧跟在`.data`之后）。
如上，在文件内，且不在任意函数内的变量，是全局作用域，即最终所有文件的所有代码都能访问。例外是加了`static`关键词的，此时只在该文件内可见，对其他文件不可见。

- 函数作用域
  - `static int a`，定义在`main`函数内，`a`真是位置在`.data section`
  - `char b[]`，在堆栈上，`main`的stack frame内
    - 这里的'Hello world'本身不会保存在`.data`区域，而是直接在代码执行的时候直接复制给`b`的，直接填充到`b`在栈上到空间。`Hello world`本身就在代码内存储。 
       ```
       (gdb) info f
       Stack level 0, frame at 0x7fffffffe4a0:
        rip = 0x4004e7 in main (v.c:14); saved rip 0x392601ed20
        source language c.
        Arglist at 0x7fffffffe490, args:
        Locals at 0x7fffffffe490, Previous frame's sp is 0x7fffffffe4a0
        Saved registers:
         rbx at 0x7fffffffe488, rbp at 0x7fffffffe490, rip at 0x7fffffffe498

       (gdb) x/10xg 0x7fffffffe460
       0x7fffffffe460:	0x000000392600fba0	0x0000000000400520
       0x7fffffffe470:	0x6f77206f6c6c6548	0x0000000000646c72
       0x7fffffffe480:	0x00007fffffffe570	0x0000000000000000
       0x7fffffffe490:	0x0000000000000000	0x000000392601ed20
       0x7fffffffe4a0:	0x0000000000000000	0x00007fffffffe578

       (gdb) p &b
       $37 = (char (*)[12]) 0x7fffffffe470
       (gdb) x/12c 0x7fffffffe470
        0x7fffffffe470:	72 'H'	101 'e'	108 'l'	108 'l'	111 'o'	32 ' '	119 'w'	111 'o'
        0x7fffffffe478:	114 'r'	108 'l'	100 'd'	0 '\000'

       Dump of assembler code for function main:
          0x00000000004004c4 <+0>:	push   %rbp
          0x00000000004004c5 <+1>:	mov    %rsp,%rbp
          0x00000000004004c8 <+4>:	push   %rbx
          0x00000000004004c9 <+5>:	sub    $0x18,%rsp
          0x00000000004004cd <+9>:	movl   $0x6c6c6548,-0x20(%rbp) #lux 6c6c6548 (lleh)
          0x00000000004004d4 <+16>:	movl   $0x6f77206f,-0x1c(%rbp) #lux 6f77206f(orw o)
          0x00000000004004db <+23>:	movl   $0x646c72,-0x18(%rbp) #lux 646c72(dlr)
          0x00000000004004e2 <+30>:	mov    $0x32,%ebx
       => 0x00000000004004e7 <+35>:	mov    $0x40060c,%eax

      当前的stack frame开始于0x7fffffffe4a0,
      |----------------------------|
      |                            |
      |----------------------------| 4a0
      | 0x000000392601ed20         | (RIP)
      |----------------------------| 498
      | 0x0000000000000000         | (push rbp)
      |----------------------------| 490 (<<================= rbp)
      | 0x0000000000000000         | (push rbx)
      |----------------------------| 488
      | 0x00007fffffffe570         | (todo)
      |----------------------------| 480
      |                    dlr     | (movl $0x646c72, -0x1c(%rbp))
      |----------------------------| 478
      |              ow olleH      | (movl $0x6c6c6548, -0x20(%rbp) movl $0x6f77206f, -0x1c(%rbp)
      |----------------------------| 470 (<<================ rsp after sub 0x18,%rsp)
      |                            |
      |----------------------------| 468
      |                            |
      |----------------------------| 460
       ```
  - `register int c`，在register上，不在任何物理内存上。
如上，在函数内的变量，默认是函数作用域，只对函数内代码可见。一般地，函数的变量被分配在栈上（stack frame），这样，每次调用一个函数，都是新的stack frame，相当于重新进入，变量都会重新初始化，因而是无状态的。例外的情况是：
    - 用`static`关键词声明的，还是函数作用域，但是不会每次都初始化。它的位置是在`.data`，不是在栈上，从而避免了每次的重新加载。
    - 用`register`关键词声明的，它的值在某个寄存器上，不在物理内存上。
