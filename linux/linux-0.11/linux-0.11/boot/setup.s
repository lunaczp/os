!
!	setup.s		(C) 1991 Linus Torvalds
!
! setup.s is responsible for getting the system data from the BIOS,
! and putting them into the appropriate places in system memory.
! both setup.s and system has been loaded by the bootblock.
!
! This code asks the bios for memory/disk/other parameters, and
! puts them in a "safe" place: 0x90000-0x901FF, ie where the
! boot-block used to be. It is then up to the protected mode
! system to read them from there before the area is overwritten
! for buffer-blocks.
!
! lux
! 1. 读取硬件信息，并存储到 0x90000-0x901FF
! 2. 移动内核，对齐到0x00000
! 3. 标记cr0，初始化gdt，idt，打开a20地址线，进入保护模式

! NOTE! These had better be the same as in bootsect.s!

INITSEG  = 0x9000	! we move boot here - out of the way
SYSSEG   = 0x1000	! system loaded at 0x10000 (65536).
SETUPSEG = 0x9020	! this is the current segment

.globl begtext, begdata, begbss, endtext, enddata, endbss
.text
begtext:
.data
begdata:
.bss
begbss:
.text

entry start
start:

! ok, the read went well so we get current cursor position and save it for
! posterity.

	mov	ax,#INITSEG	! this is done in bootsect already, but...
	mov	ds,ax
	mov	ah,#0x03	! read cursor pos
	xor	bh,bh
	int	0x10		! save it in known place, con_init fetches
	mov	[0],dx		! it from 0x90000.
	! lux: https://en.wikipedia.org/wiki/INT_10H
	! [0] 代表当前段的第一个段内地址，即0x9000:0，即0x90000
	! 注意，dx占用两个字节，[0],[1]

! Get memory size (extended mem, kB)
! lux 0x00002
! lux  returns a single value for contiguous memory above 1 MB; 16-bit value in KB,max representation 64MB
	mov	ah,#0x88
	int	0x15
	mov	[2],ax

! Get video-card data:

	mov	ah,#0x0f
	int	0x10
	mov	[4],bx		! bh = display page
	mov	[6],ax		! al = video mode, ah = window width

! check for EGA/VGA and some config parameters

	mov	ah,#0x12
	mov	bl,#0x10
	int	0x10
	mov	[8],ax
	mov	[10],bx
	mov	[12],cx

! Get hd0 data
! lux: get 0x10 bytes from [4*0x41] to 0x00080
	mov	ax,#0x0000
	mov	ds,ax
	lds	si,[4*0x41]
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0080
	mov	cx,#0x10
	rep
	movsb

! Get hd1 data
! lux: get 0x10 bytes from [4*0x46] to 0x00090
! lux todo bios data
	mov	ax,#0x0000
	mov	ds,ax
	lds	si,[4*0x46] ! lux: LDS r16,m16:16, load DS:r16 with pointer from memory
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0090
	mov	cx,#0x10 ! lux: repet 0x10 times
	rep
	movsb ! lux Move byte DS:[SI] to ES:[DI] 即，0x00090

! Check that there IS a hd1 :-)

	mov	ax,#0x01500
	mov	dl,#0x81
	int	0x13
	jc	no_disk1
	cmp	ah,#3
	je	is_disk1
no_disk1:
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x0090
	mov	cx,#0x10
	mov	ax,#0x00 ! lux 重置为0
	rep
	stosb ! lux: Store AL in byte ES:[(E)DI], update (E)DI
is_disk1:

! now we want to move to protected mode ...

	cli			! no interrupts allowed !

! first we move the system to it's rightful place
! lux 把0x10000 - 0x90000 移动到 0x00000 - 0x80000

! lux:
! Ref: Intel 64 and IA-32 Architectures Software Developer's Manual Volume 1-Basic Architecture.pdf 3.4.3.2
!   The direction flag (DF, located in bit 10 of the EFLAGS register) controls string instructions (MOVS, CMPS, SCAS, LODS, and STOS). 
! Setting the DF flag causes the string instructions to auto-decrement (to process strings from high addresses to low addresses). 
! Clearing the DF flag causes the string instructions to auto-increment (process strings from low addresses to high addresses).
! The STD and CLD instructions set and clear the DF flag, respectively.

	mov	ax,#0x0000
	cld			! 'direction'=0, movs moves forward
do_move:
	mov	es,ax		! destination segment
	add	ax,#0x1000
	cmp	ax,#0x9000
	jz	end_move
	mov	ds,ax		! source segment
	sub	di,di
	sub	si,si
	mov 	cx,#0x8000
	rep
	movsw ! lux: Move word DS:[(E)SI] to ES:[(E)DI]
	jmp	do_move

! then we load the segment descriptors

end_move:
	mov	ax,#SETUPSEG	! right, forgot this at first. didn't work :-)
	mov	ds,ax
	lidt	idt_48		! load idt with 0,0
	lgdt	gdt_48		! load gdt with whatever appropriate

! that was painless, now we enable A20

	call	empty_8042
	mov	al,#0xD1		! command write
	out	#0x64,al
	call	empty_8042
	mov	al,#0xDF		! A20 on
	out	#0x60,al
	call	empty_8042

! well, that went ok, I hope. Now we have to reprogram the interrupts :-(
! we put them right after the intel-reserved hardware interrupts, at
! int 0x20-0x2F. There they won't mess up anything. Sadly IBM really
! messed this up with the original PC, and they haven't been able to
! rectify it afterwards. Thus the bios puts interrupts at 0x08-0x0f,
! which is used for the internal hardware interrupts as well. We just
! have to reprogram the 8259's, and it isn't fun.

! lux:
! Ref: https://en.wikipedia.org/wiki/Intel_8259
! On the PC, the BIOS (and thus also DOS) traditionally maps the master 8259 interrupt requests (IRQ0-IRQ7) to interrupt vector offset 8 (INT08-INT0F) and 
! the slave 8259 (in PC/AT and later) interrupt requests (IRQ8-IRQ15) to interrupt vector offset 112 (INT70-INT77). This was done despite the first 32 (INT00-INT1F) interrupt vectors
! being reserved by the processor for internal exceptions (this was ignored for the design of the PC for some reason). Because of the reserved vectors for exceptions most other operating 
! systems map (at least the master) 8259 IRQs (if used on a platform) to another interrupt vector base offset.
!
! BIOS将8259A的IRQ0-IRQ7 对应的中断号设置成了0x8-0xF, 而中断号0x8-0xF是Intel保留的，所以我们重新编程8259A，将16个中断重新分号。
! IRQ0-IRQ7 -> 0x20 - 0x27
! IRQ8-IRQ15 -> 0x28 - 0x2F
! 注意，8259A的针脚中断含义是确定的，比如IRQ0是时钟中断，但中断号是可以编程分配的，而中断号是cpu去idt里调用对应的处理程序对应的依据。
! 最终idt里0x0-0x1F是intel约定的中断对应的处理程序，0x20-0x2F是8259A IRQ0-IRQ15对应的16个中断，后面可以继续加其他用户中断，总共255个。
!
! Intel Reserved,See Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide.pdf Ch6 INTERRUPT AND EXCEPTION HANDLING


	mov	al,#0x11		! initialization sequence
	out	#0x20,al		! send it to 8259A-1
	.word	0x00eb,0x00eb		! jmp $+2, jmp $+2
	out	#0xA0,al		! and to 8259A-2
	.word	0x00eb,0x00eb
	mov	al,#0x20		! start of hardware int's (0x20)
	out	#0x21,al
	.word	0x00eb,0x00eb
	mov	al,#0x28		! start of hardware int's 2 (0x28)
	out	#0xA1,al
	.word	0x00eb,0x00eb
	mov	al,#0x04		! 8259-1 is master
	out	#0x21,al
	.word	0x00eb,0x00eb
	mov	al,#0x02		! 8259-2 is slave
	out	#0xA1,al
	.word	0x00eb,0x00eb
	mov	al,#0x01		! 8086 mode for both
	out	#0x21,al
	.word	0x00eb,0x00eb
	out	#0xA1,al
	.word	0x00eb,0x00eb
	mov	al,#0xFF		! mask off all interrupts for now
	out	#0x21,al
	.word	0x00eb,0x00eb
	out	#0xA1,al

! well, that certainly wasn't fun :-(. Hopefully it works, and we don't
! need no steenking BIOS anyway (except for the initial loading :-).
! The BIOS-routine wants lots of unnecessary data, and it's less
! "interesting" anyway. This is how REAL programmers do it.
!
! Well, now's the time to actually move into protected mode. To make
! things as simple as possible, we do no register set-up or anything,
! we let the gnu-compiled 32-bit programs do that. We just jump to
! absolute address 0x00000, in 32-bit protected mode.

	! lux: lmsw ax: load ax to CR0, cause CR0.PE=1 , switch to protected mode!
	mov	ax,#0x0001	! protected mode (PE) bit
	lmsw	ax		! This is it!
	jmpi	0,8		! jmp offset 0 of segment 8 (cs)
	! lux: fly to kernel main now.

! This routine checks that the keyboard command queue is empty
! No timeout is used - if this hangs there is something wrong with
! the machine, and we probably couldn't proceed anyway.
empty_8042:
	.word	0x00eb,0x00eb
	in	al,#0x64	! 8042 status port
	test	al,#2		! is input buffer full?
	jnz	empty_8042	! yes - loop
	ret

gdt:
	.word	0,0,0,0		! dummy

	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0
	.word	0x9A00		! code read/exec
	.word	0x00C0		! granularity=4096, 386

	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0
	.word	0x9200		! data read/write
	.word	0x00C0		! granularity=4096, 386

idt_48:
	.word	0			! idt limit=0
	.word	0,0			! idt base=0L

gdt_48:
	.word	0x800		! gdt limit=2048, 256 GDT entries
	.word	512+gdt,0x9	! gdt base = 0X9xxxx !lux 512是bootsect大小，也就是setup的起始偏移，gdt是上面定义的gdt的偏移。最终代表的绝对地址是0x9xxxx
	
.text
endtext:
.data
enddata:
.bss
endbss:
