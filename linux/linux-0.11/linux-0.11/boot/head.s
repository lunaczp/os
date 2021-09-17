/*
 *  linux/boot/head.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  head.s contains the 32-bit startup code.
 *
 * NOTE!!! Startup happens at absolute address 0x00000000, which is also where
 * the page directory will exist. The startup code will be overwritten by
 * the page directory.
 */
 /*lux 注意head.s使用段汇编和setup和bootsect不一样
  * setup.s bootsect.s是传统intel8086汇编，使用as86,ld86,且操作是从右到左，如 mov eax 0x10
  * head.s使用gnu as，采用at&t语法，操作是从左到右，如 mov $0x10, %eax
  */
  /*lux head.o(内的.text)是system的.text的第一部分（由ld连接程序确定）。而system在setup内被移动到0x0。所以0x0处的代码对应head.s的代码。（可以对ndisasm system确认）
   * 从而当在setup内jmp 0x8:0的时候（cs:offset => 0x0），就到达了这里。
   * 参考build.c
   */
  /*lux 32位初始化代码:
   * 1. 设置idt
   * 2. 设置gdt
   * 3. 其他检查、设置
   * 4. 设置分页
   * 5. 进入kernel main
   */
.text
.globl _idt,_gdt,_pg_dir,_tmp_floppy_area
_pg_dir:
startup_32:
	movl $0x10,%eax/*lux 用0x10(对应gdt中之前定义的data段) 初始化各个段寄存器。*/
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs
	lss _stack_start,%esp /*lux 使用sched.c:67定义的堆栈段*/
	call setup_idt
	call setup_gdt
	movl $0x10,%eax		# reload all the segment registers
	mov %ax,%ds		# after changing gdt. CS was already
	mov %ax,%es		# reloaded in 'setup_gdt'
	mov %ax,%fs
	mov %ax,%gs
	lss _stack_start,%esp
	xorl %eax,%eax
1:	incl %eax		# check that A20 really IS enabled
	movl %eax,0x000000	# loop forever if it isn't
	cmpl %eax,0x100000
	je 1b
/*
 * NOTE! 486 should set bit 16, to check for write-protect in supervisor
 * mode. Then it would be unnecessary with the "verify_area()"-calls.
 * 486 users probably want to set the NE (#5) bit also, so as to use
 * int 16 for math errors.
 */
	movl %cr0,%eax		# check math chip
	andl $0x80000011,%eax	# Save PG,PE,ET
/* "orl $0x10020,%eax" here for 486 might be good */
	orl $2,%eax		# set MP
	movl %eax,%cr0
	call check_x87
	jmp after_page_tables

/*
 * We depend on ET to be correct. This checks for 287/387.
 */
check_x87:
	fninit
	fstsw %ax
	cmpb $0,%al
	je 1f			/* no coprocessor: have to set bits */
	movl %cr0,%eax
	xorl $6,%eax		/* reset MP, set EM */
	movl %eax,%cr0
	ret
.align 2
1:	.byte 0xDB,0xE4		/* fsetpm for 287, ignored by 387 */
	ret

/*
 *  setup_idt
 *
 *  sets up a idt with 256 entries pointing to
 *  ignore_int, interrupt gates. It then loads
 *  idt. Everything that wants to install itself
 *  in the idt-table may do so themselves. Interrupts
 *  are enabled elsewhere, when we can be relatively
 *  sure everything is ok. This routine will be over-
 *  written by the page tables.
 */
setup_idt:
	lea ignore_int,%edx
	movl $0x00080000,%eax
	movw %dx,%ax		/* selector = 0x0008 = cs */
	movw $0x8E00,%dx	/* interrupt gate - dpl=0, present */

	lea _idt,%edi /*lux idt 入口地址*/
	mov $256,%ecx /*lux loop 256*/
rp_sidt:
	movl %eax,(%edi)
	movl %edx,4(%edi)
	addl $8,%edi /*lux 下一个idt条目*/
	dec %ecx
	jne rp_sidt
	lidt idt_descr /*lux:load idt */
	ret

/*
 *  setup_gdt
 *
 *  This routines sets up a new gdt and loads it.
 *  Only two entries are currently built, the same
 *  ones that were built in init.s. The routine
 *  is VERY complicated at two whole lines, so this
 *  rather long comment is certainly needed :-).
 *  This routine will beoverwritten by the page tables.
 */
setup_gdt:
	lgdt gdt_descr
	ret

/*
 * I put the kernel page tables right after the page directory,
 * using 4 of them to span 16 Mb of physical memory. People with
 * more than 16MB will have to expand this.
 */
 /*
  *linux 0.1的页表结构
  * 从0x0000开始，
  * 0x0000 - 0x1000 page directory表，共1024个表项，这里只初始化了四个表项，每个表项（pde）对应下面4个page table
  * 0x1000 - 0x2000 第1个page table，包含1024个pte，每个pte对应一个4k页，共4M
  * 0x2000 - 0x3000 第2个page table，包含1024个pte，每个pte对应一个4k页，共4M
  * 0x3000 - 0x4000 第3个page table，包含1024个pte，每个pte对应一个4k页，共4M
  * 0x4000 - 0x5000 第4个page table，包含1024个pte，每个pte对应一个4k页，共4M
  */
.org 0x1000
pg0:

.org 0x2000
pg1:

.org 0x3000
pg2:

.org 0x4000
pg3:

.org 0x5000
/*
 * tmp_floppy_area is used by the floppy-driver when DMA cannot
 * reach to a buffer-block. It needs to be aligned, so that it isn't
 * on a 64kB border.
 */
_tmp_floppy_area:
	.fill 1024,1,0

after_page_tables:
	pushl $0		# These are the parameters to main :-)
	pushl $0
	pushl $0
	pushl $L6		# return address for main, if it decides to.
	pushl $_main
# lux:
# 正常的汇编函数调用是call+ret。调用方通过call调用。被调用方调用ret返回。call会将参数和返回地址入栈，ret会从栈内取到返回地址并跳转过去。
# 这里通过这个push、jmp操作手动模拟了从main调用setup_paging的过程,
# 1. 手动构造call的入栈函数，(模拟入栈参数，和返回地址：main函数)
# 2. jmp到setup_paging, jmp是简单的段内跳转，不涉及堆栈变化
# 3. setup_paging调用ret，ret会从堆栈pop出返回地址，并跳转到返回地址（main）
# now let's fly to kernel main
	jmp setup_paging
L6:
	jmp L6			# main should never return here, but
				# just in case, we know what happens.

/* This is the default interrupt "handler" :-) */
int_msg:
	.asciz "Unknown interrupt\n\r"
.align 2
ignore_int:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	pushl $int_msg
	call _printk
	popl %eax
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret


/*
 * Setup_paging
 *
 * This routine sets up paging by setting the page bit
 * in cr0. The page tables are set up, identity-mapping
 * the first 16MB. The pager assumes that no illegal
 * addresses are produced (ie >4Mb on a 4Mb machine).
 *
 * NOTE! Although all physical memory should be identity
 * mapped by this routine, only the kernel page functions
 * use the >1Mb addresses directly. All "normal" functions
 * use just the lower 1Mb, or the local data space, which
 * will be mapped to some other place - mm keeps track of
 * that.
 *
 * For those with more memory than 16 Mb - tough luck. I've
 * not got it, why should you :-) The source is here. Change
 * it. (Seriously - it shouldn't be too difficult. Mostly
 * change some constants etc. I left it at 16Mb, as my machine
 * even cannot be extended past that (ok, but it was cheap :-)
 * I've tried to show which constants to change by having
 * some kind of marker at them (search for "16Mb"), but I
 * won't guarantee that's all :-( )
 */
.align 2
setup_paging:
	movl $1024*5,%ecx		/* 5 pages - pg_dir+4 page tables */
	xorl %eax,%eax
	xorl %edi,%edi			/* pg_dir is at 0x000 */
	/*
	 *lux cld;rep;stosl 三个指令，清空从0x00000开始的5k区域；用于下面作为page table。
	 * cld:
	 * Clears the DF flag in the EFLAGS register. When the DF flag is set to 0, string operations increment the index regis- ters (ESI and/or EDI). 
	 * stosl:
	 * In non-64-bit and default 64-bit mode; stores a byte, word, or doubleword from the AL, AX, or EAX register (respectively) into the destination operand. 
	 * The destination operand is a memory location, the address of which is read from either the ES:EDI or ES:DI register (depending on the address-size attribute of the instruction and the mode of operation). 
	 * The ES segment cannot be overridden with a segment override prefix.
	 */
	cld;rep;stosl
	/*
	 *lux 在page_dir中，填充4个pde表项，并设置 present user r/w三个标志位 
	 */
	movl $pg0+7,_pg_dir		/* set present bit/user r/w */
	movl $pg1+7,_pg_dir+4		/*  --------- " " --------- */
	movl $pg2+7,_pg_dir+8		/*  --------- " " --------- */
	movl $pg3+7,_pg_dir+12		/*  --------- " " --------- */
	/*
	 *lux 倒序，填充4个page table的pte，共4096个。从最后一个开始
	 * 4092 对应0xfff007, 16Mb-4096 + 7，即16Mb物理内存的最后一个4k段
	 * 4088 对应...
	 * ...
	 * 0 对应0x000007,对应16Mb的第一个4k端
	 * 代码实现：
	 * eax每次--4k，代表每个pte对应的4k页段的物理地址的开始偏移
	 * stosl每次把eax写入从es:edi，即pte表项，记录对应的物理页开始偏移
	 * jge通过判断subl产生的eflags变化(sf,of)来控制循环，当subl 0x1000,%eax为负，则循环结束
	 */
	movl $pg3+4092,%edi
	movl $0xfff007,%eax		/*  16Mb - 4096 + 7 (r/w user,p) */
	std
1:	stosl			/* fill pages backwards - more efficient :-) */
	subl $0x1000,%eax
	jge 1b
	/*lux 设置cr3*/
	xorl %eax,%eax		/* pg_dir is at 0x0000 */
	movl %eax,%cr3		/* cr3 - page directory start */
	/*lux 设置cr0.PG*/
	movl %cr0,%eax
	orl $0x80000000,%eax
	movl %eax,%cr0		/* set paging (PG) bit */
	ret			/* this also flushes prefetch-queue */

.align 2
.word 0
idt_descr:
	.word 256*8-1		# idt contains 256 entries
	.long _idt
.align 2
.word 0
gdt_descr:
	.word 256*8-1		# so does gdt (not that that's any
	.long _gdt		# magic number, but it works for me :^)

	.align 8
_idt:	.fill 256,8,0		# idt is uninitialized #lux: will be initialized in head.s:78

_gdt:	.quad 0x0000000000000000	/* NULL descriptor */
	.quad 0x00c09a0000000fff	/* 16Mb */ /*lux 内核代码段*/
	.quad 0x00c0920000000fff	/* 16Mb */ /*lux 内核数据段*/
	.quad 0x0000000000000000	/* TEMPORARY - don't use */
	.fill 252,8,0			/* space for LDT's and TSS's etc */

/*lux 一个gdt entry 8字节：

高4字节：
|31|30|29|28|27|26|25|24|23|22 |21|20 |19|18|17|16    |15|14|13|12|11|10|9|8|7|6|5|4|3|2|1|0|
|Base 31:24             |G |D/B|L |AVL|SegLimit 19:16 |P |DPL  |S |TYPE     |Base 23:16     |

低4字节：
|31|30|29|28|27|26|25|24|23|22 |21|20 |19|18|17|16    |15|14|13|12|11|10|9|8|7|6|5|4|3|2|1|0|
|Base 15:00                                    |SegLimit 15:00                              |

例：
0x00c09a0000000fff 16MB代码段
0    0    c    0    9    a    0    0    0    0    0    0    0    f    f    f 
0000 0000 1100 0000 1001 1010 0000 0000 0000 0000 0000 0000 0000 1111 1111 1111
31.................................031........................................0	

G(23): 1,limit以4kb为单位
Type(11...8):11是1，则是code段
limit:0xfff,(0xfff+1) * 4kb = 2^12 * 4k = 16mb

关于limit：为什么是是0xfff，而不是0x10000，因为cpu的判断方式：
The processor uses the segment limit in two different ways, depending on whether the segment is an expand-up or an expand-down segment. ...
For expand-up segments, the offset in a logical address can range from 0 to the segment limit.
所以严格讲limit是指边界，而不是size（大小）。指定的范围是[0,0xfff]，大小是16mb。
如果要得到size，则limit+1 (see sched.h:245)

See：“Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide” chp3.4.5
*/