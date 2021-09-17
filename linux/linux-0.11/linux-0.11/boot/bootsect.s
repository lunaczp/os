!
! SYS_SIZE is the number of clicks (16 bytes) to be loaded.
! 0x3000 is 0x30000 bytes = 196kB, more than enough for current
! versions of linux
!
SYSSIZE = 0x3000 !lux: 192kB
!
!	bootsect.s		(C) 1991 Linus Torvalds
!
! bootsect.s is loaded at 0x7c00 by the bios-startup routines, and moves
! iself out of the way to address 0x90000, and jumps there.
!
! It then loads 'setup' directly after itself (0x90200), and the system
! at 0x10000, using BIOS interrupts. 
!
! NOTE! currently system is at most 8*65536 bytes long. This should be no
! problem, even in the future. I want to keep it simple. This 512 kB
! kernel size should be enough, especially as this doesn't contain the
! buffer cache as in minix
!
! The loader has been made as simple as possible, and continuos
! read errors will result in a unbreakable loop. Reboot by hand. It
! loads pretty fast by getting whole sectors at a time whenever possible.

! lux 该程序用as86编译，参见： 
! http://wiki.osdev.org/AS86 
! https://www.systutorials.com/docs/linux/man/1-as86/

.globl begtext, begdata, begbss, endtext, enddata, endbss
.text
begtext:
.data
begdata:
.bss
begbss:
.text

SETUPLEN = 4				! nr of setup-sectors
BOOTSEG  = 0x07c0			! original address of boot-sector
INITSEG  = 0x9000			! we move boot here - out of the way
SETUPSEG = 0x9020			! setup starts here
SYSSEG   = 0x1000			! system loaded at 0x10000 (65536).
ENDSEG   = SYSSEG + SYSSIZE		! where to stop loading

! ROOT_DEV:	0x000 - same type of floppy as boot.
!		0x301 - first partition on first drive etc
ROOT_DEV = 0x306

entry start
start: ! lux 将自己移动到0x90000处
	mov	ax,#BOOTSEG
	mov	ds,ax
	mov	ax,#INITSEG
	mov	es,ax
	mov	cx,#256
	sub	si,si
	sub	di,di
	rep
	movw ! lux: 每次一个字（2byte），一共256次循环，从BOOTSEG拷贝512字节到INITSEG
	jmpi	go,INITSEG ! lux: 跳转到INITSEG(0x90000)的go标记的指令处
go:	mov	ax,cs
	mov	ds,ax
	mov	es,ax
! put stack at 0x9ff00.
	mov	ss,ax
	mov	sp,#0xFF00		! arbitrary value >>512

! load the setup-sectors directly after the bootblock.
! Note that 'es' is already set up.

load_setup: ! lux 读入setup，setup程序在image磁盘的第0柱面，0磁道，2扇区（第1扇区是512字节的bootsect，即本程序。）参见build.c
	mov	dx,#0x0000		! drive 0, head 0
	mov	cx,#0x0002		! sector 2, track 0
	mov	bx,#0x0200		! address = 512, in INITSEG
	mov	ax,#0x0200+SETUPLEN	! service 2, nr of sectors
	int	0x13			! read it
	jnc	ok_load_setup		! ok - continue
	mov	dx,#0x0000
	mov	ax,#0x0000		! reset the diskette
	int	0x13
	j	load_setup

ok_load_setup:

! Get disk drive parameters, specifically nr of sectors/track

! lux:
! int 13h https://en.wikipedia.org/wiki/INT_13H#INT_13h_AH=08h:_Read_Drive_Parameters
!
! when ah=8 ,get drive parameters
! return:
! CF: set on error, clear if no error
! AH: return code 
! DL: number of hard disk drives
! DH: ogical last index of heads = number_of - 1 (because index starts with 0)
! CX:
! 	[7:6] [15:8][7] logical last index of cylinders = number_of - 1 (because index starts with 0)
! 	[5:0][7] logical last index of sectors per track = number_of (because index starts with 1)
! BL: drive type (only AT/PS2 floppies)
! ES:DI pointer to drive parameter table (only for floppies)

	mov	dl,#0x00 !lux: 0x00 floppy, 0x80 1st HDD
	mov	ax,#0x0800		! AH=8 is get drive parameters
	int	0x13
	mov	ch,#0x00
	seg cs
	mov	sectors,cx !lux: sector num per track
	mov	ax,#INITSEG
	mov	es,ax

! Print some inane message

	mov	ah,#0x03		! read cursor pos
	xor	bh,bh
	int	0x10
	
	mov	cx,#24
	mov	bx,#0x0007		! page 0, attribute 7 (normal)
	mov	bp,#msg1
	mov	ax,#0x1301		! write string, move cursor
	int	0x10

! ok, we've written the message, now
! we want to load the system (at 0x10000)

	mov	ax,#SYSSEG
	mov	es,ax		! segment of 0x010000
	call	read_it
	call	kill_motor

! After that we check which root-device to use. If the device is
! defined (!= 0), nothing is done and the given device is used.
! Otherwise, either /dev/PS0 (2,28) or /dev/at0 (2,8), depending
! on the number of sectors that the BIOS reports currently.

	seg cs
	mov	ax,root_dev
	cmp	ax,#0
	jne	root_defined
	seg cs
	mov	bx,sectors
	mov	ax,#0x0208		! /dev/ps0 - 1.2Mb
	cmp	bx,#15
	je	root_defined
	mov	ax,#0x021c		! /dev/PS0 - 1.44Mb
	cmp	bx,#18
	je	root_defined
undef_root:
	jmp undef_root
root_defined:
	seg cs
	mov	root_dev,ax

! after that (everyting loaded), we jump to
! the setup-routine loaded directly after
! the bootblock:

	jmpi	0,SETUPSEG ! lux 进入setup

! This routine loads the system at address 0x10000, making sure
! no 64kB boundaries are crossed. We try to load it as fast as
! possible, loading whole tracks whenever we can.
!
! in:	es - starting address segment (normally 0x1000)
!
sread:	.word 1+SETUPLEN	! sectors read of current track
head:	.word 0			! current head
track:	.word 0			! current track

! lux:
! CHS 磁盘地址命名 https://en.wikipedia.org/wiki/Cylinder-head-sector
! 一般来讲，cylinder和track可以混用，但对于双面的磁盘，严格地，
! cylinder 指代柱面
! track 指代一个同心圆
! header 指代一个读取磁头。
! sector 指代一个同心圆的一部分
! sectors一般存储512byte，一个cylinder由headerNum个track组成，一个track由sectorPerTrack个sector组成

! lux:
! 要做的事：读取192kb sysmap数据到0x10000开始的内存
! 要注意的：
! 由于intel的分段机制，一个段最大2^16=64kB，也就是我们在读取sysmap写入内存的时候，中间至少要切换两次段。具体地,
! 每次当前段写满，就要重新加载段基址，进入下一个段基，在这里，实际上就是段0x1000,段0x2000,段0x3000
! 
! 另外，我们的读取策略是尽量每次读取一个track，而不是一个sector一个sector去读，这样更快。
! 每个track读完，track+1,每个head读完，head+1，每个cylinder读完，cylinder+1，直到192kb数据读完
! 
! 具体地：
! loop:判断是否读完，是则返回，否则继续
! （track）判断读取剩下的sector (sectorNum)是否会触发越界（超出当前段）
!	是
!		计算在不越界的情况下，能够读取几个sector，设置sectorNum
!	不是
!		取sectorNum即可
!	读取sectorNum个sector
!	
!	当前track的sector是否读完
!		读完
!			head=0则head=1，读floppy背面
!			head=1则head=0，且track+1，该track的两面都读完了，读下一个track
!		没读完
!			判断是否是由于越界而只读取了部分，是则切换到下一个段
!		从头loop

read_it:
	mov ax,es
	test ax,#0x0fff
die:	jne die			! es must be at 64kB boundary
	xor bx,bx		! bx is starting address within segment
rp_read:
	mov ax,es
	cmp ax,#ENDSEG		! have we loaded all yet?
	jb ok1_read
	ret
ok1_read:
	seg cs
	mov ax,sectors
	sub ax,sread
	mov cx,ax
	shl cx,#9 !lux 一个sector 512byte(2^9)，这里计算得到的是已经读取的总byte数
	add cx,bx
	! lux 这里的读取策略是，尽量一次读取一个track的全部sector；但由于segment 的限制，es:bx所能表示的段内偏移是bx的最大值，也就是FFFF。一个段64kb
	! 因而，每次尝试读取一批sector的时候，都要先判断读取这些sector写入内存时是否会超时es:bx表示的地址范围（是否在该段内）
	! 如果没超出，则继续读取
	! 否则，应该只读取该段内剩余能表示的空间所对应的sector数量，也就是(0x10000-doneSectorNum*2^9)/2^9
	! 实际采用的计算方法是 x = 0 - doneSectorNum, rst = x >> 9 
	! 这么做是因为
	! 2^n = x + wanted
	! 2^n = x + Revert(x) + 1, 对x取反，+1，再加上x，就等于2^n,(假设x <2^n)
	! 	那么 wanted = Revert(x) + 1 = unsigned(2'sCompliment(-x)) = -x的二进制补码取正整数
	! 也就是会说，我们要的剩余字节数，就是unsigned(-x)
	!
	! 然后，我们再shr ax, #9 也就是 unsigned(-x) >> 9，（除512，一个sector的大小）就得到了sectorNum。
	! 因为shr是unsiged divide，操作会把左边填充为0,这样我们才能取到unsigned(-x)
	! shr: https://lunaczp.github.io/learning_note/os/doc/x86_instruction/

	jnc ok2_read
	je ok2_read
	xor ax,ax
	sub ax,bx
	shr ax,#9
ok2_read:
	call read_track ! lux: 读取 min(这个track剩余所有的sectors,segment剩下能填充的sector数量), 此时al里保存了要读取的sector数量
	mov cx,ax ! lux: 暂存ax，下面判断用
	add ax,sread
	seg cs
	cmp ax,sectors ! lux: 这个track的sector是否读完，只要没有到segment边界，我们总能一次读完这个track。但如果到了边界，我们必须切分开，先读一部分sector；切换segment，再读剩下的部分。
	jne ok3_read ! lux: 还没读完，跳到ok3_read，继续读
	mov ax,#1 
	sub ax,head ! lux: 这个track的sector读完了
	jne ok4_read ! lux: head=0，则跳转到ok4_read
	inc track ! lux: head=1,则说明该面读完了，该调整track了，track+1(sector先变，head再变，track最后变，所谓CHS)
ok4_read:
	mov head,ax ! lux: 当head=0，此时ax=1，操作为set head=1,读floppy背面；当head=1，此时ax=0，操作为set head=0，因为已经跳转到下一个track，head从0开始
	xor ax,ax ! lux重置ax，因为进入到新的track,下面会赋值sread=ax=0
ok3_read:
	mov sread,ax ! lux 更新sread为已经读取的量
	shl cx,#9
	add bx,cx ! lux 利用上面暂存的cx，判断是否到了边界
	jnc rp_read ! lux: 如果没有超过segment边界，继续读
	mov ax,es ! lux: 否则，进入下一个段：更新es到下一个段基址,并重置bx, es:bx代表下一个64kb段
	add ax,#0x1000
	mov es,ax
	xor bx,bx
	jmp rp_read

read_track:
	push ax
	push bx
	push cx
	push dx
	mov dx,track
	mov cx,sread
	inc cx ! lux: read next sector
	mov ch,dl ! lux: read this track
	mov dx,head ! lux:read this head
	mov dh,dl
	mov dl,#0
	and dx,#0x0100
	mov ah,#2 ! lux: 读扇区 ch cylinder, cl sector, dh head, dl drive, es:bx 目标地址 al里保存的是要读取的sector数量，（被设置为本track剩余的sector数量，也就是一次要读取剩下的整个track）
	int 0x13
	jc bad_rt
	pop dx
	pop cx
	pop bx
	pop ax
	ret
bad_rt:	mov ax,#0
	mov dx,#0
	int 0x13 ! lux: rest disk 0, ah:reset, dh:drive num
	pop dx
	pop cx
	pop bx
	pop ax
	jmp read_track

/*
 * This procedure turns off the floppy drive motor, so
 * that we enter the kernel in a known state, and
 * don't have to worry about it later.
 */
kill_motor:
	push dx
	mov dx,#0x3f2
	mov al,#0
	outb
	pop dx
	ret

sectors:
	.word 0

msg1:
	.byte 13,10
	.ascii "Loading system ..."
	.byte 13,10,13,10

.org 508
root_dev:
	.word ROOT_DEV
boot_flag:
	.word 0xAA55

.text
endtext:
.data
enddata:
.bss
endbss:
