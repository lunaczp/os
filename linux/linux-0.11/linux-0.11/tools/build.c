/*
 *  linux/tools/build.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * This file builds a disk-image from three different files:
 *
 * - bootsect: max 510 bytes of 8086 machine code, loads the rest
 * - setup: max 4 sectors of 8086 machine code, sets up system parm
 * - system: 80386 code for actual system
 *
 * It does some checking that all files are of the correct type, and
 * just writes the result to stdout, removing headers and padding to
 * the right amount. It also writes some system data to stderr.
 */

/*
 * Changes by tytso to allow root device specification
 */

/**
 *lux
 * 这个代码的运行环境应该是在minix上，也就是，linus T编程的环境是一台i386机器，+minix操作系统(with gcc)
 * 所以这些c代码都是用gcc编译的，里面用到的open read函数，也是gcc提供的标准C库实现。
 * update 2018.2.5
 * Linux内核完全注释上提到（chp.17.9），Linus最开始开发Linux的环境是：
 * 1. 硬件：386机器
 * 2. 操作系统：（Bruce Evans patched Minix 1.5.10） Minix-i386
 * 3. linus移植到Minix-i386上的gnu工具集（ gcc gld emacs bash ）
 * Linus在这个环境上，开发并交叉编译了最初几个版本的Linux（0.01，0.03，0.11） 
 */

#include <stdio.h>	/* fprintf */
#include <string.h>
#include <stdlib.h>	/* contains exit */
#include <sys/types.h>	/* unistd.h needs this */
#include <sys/stat.h>
#include <linux/fs.h>
#include <unistd.h>	/* contains read/write */
#include <fcntl.h>

#define MINIX_HEADER 32
#define GCC_HEADER 1024

#define SYS_SIZE 0x2000
/*lux: major root, minor root, what's this?*/
#define DEFAULT_MAJOR_ROOT 3
#define DEFAULT_MINOR_ROOT 6

/* max nr of sectors of setup: don't change unless you also change
 * bootsect etc */
#define SETUP_SECTS 4

#define STRINGIFY(x) #x

void die(char * str)
{
	fprintf(stderr,"%s\n",str);
	exit(1);
}

void usage(void)
{
	die("Usage: build bootsect setup system [rootdev] [> image]");
}

int main(int argc, char ** argv)
{
	int i,c,id;
	char buf[1024];
	char major_root, minor_root;
	struct stat sb;

	if ((argc != 4) && (argc != 5))
		usage();
	if (argc == 5) {
		if (strcmp(argv[4], "FLOPPY")) {
			if (stat(argv[4], &sb)) {
				perror(argv[4]);
				die("Couldn't stat root device.");
			}
			major_root = MAJOR(sb.st_rdev);
			minor_root = MINOR(sb.st_rdev);
		} else {
			major_root = 0;
			minor_root = 0;
		}
	} else {
		major_root = DEFAULT_MAJOR_ROOT;
		minor_root = DEFAULT_MINOR_ROOT;
	}
	fprintf(stderr, "Root device is (%d, %d)\n", major_root, minor_root);
	if ((major_root != 2) && (major_root != 3) &&
	    (major_root != 0)) {
		fprintf(stderr, "Illegal root device (major = %d)\n",
			major_root);
		die("Bad root device --- major #");
	}
	for (i=0;i<sizeof buf; i++) buf[i]=0;
	if ((id=open(argv[1],O_RDONLY,0))<0)
		die("Unable to open 'boot'");
	if (read(id,buf,MINIX_HEADER) != MINIX_HEADER)
		die("Unable to read header of 'boot'");
	if (((long *) buf)[0]!=0x04100301)
		die("Non-Minix header of 'boot'");
	if (((long *) buf)[1]!=MINIX_HEADER)
		die("Non-Minix header of 'boot'");
	if (((long *) buf)[3]!=0)
		die("Illegal data segment in 'boot'");
	if (((long *) buf)[4]!=0)
		die("Illegal bss in 'boot'");
	if (((long *) buf)[5] != 0)
		die("Non-Minix header of 'boot'");
	if (((long *) buf)[7] != 0)
		die("Illegal symbol table in 'boot'");
	i=read(id,buf,sizeof buf);
	fprintf(stderr,"Boot sector %d bytes.\n",i);
	if (i != 512)
		die("Boot block must be exactly 512 bytes");
	if ((*(unsigned short *)(buf+510)) != 0xAA55)
		die("Boot block hasn't got boot flag (0xAA55)");
	buf[508] = (char) minor_root;
	buf[509] = (char) major_root;	
	i=write(1,buf,512);
	if (i!=512)
		die("Write call failed");
	close (id);
	
	if ((id=open(argv[2],O_RDONLY,0))<0)
		die("Unable to open 'setup'");
	if (read(id,buf,MINIX_HEADER) != MINIX_HEADER)
		die("Unable to read header of 'setup'");
	if (((long *) buf)[0]!=0x04100301)
		die("Non-Minix header of 'setup'");
	if (((long *) buf)[1]!=MINIX_HEADER)
		die("Non-Minix header of 'setup'");
	if (((long *) buf)[3]!=0)
		die("Illegal data segment in 'setup'");
	if (((long *) buf)[4]!=0)
		die("Illegal bss in 'setup'");
	if (((long *) buf)[5] != 0)
		die("Non-Minix header of 'setup'");
	if (((long *) buf)[7] != 0)
		die("Illegal symbol table in 'setup'");
	for (i=0 ; (c=read(id,buf,sizeof buf))>0 ; i+=c )
		if (write(1,buf,c)!=c)
			die("Write call failed");
	close (id);
	if (i > SETUP_SECTS*512)
		die("Setup exceeds " STRINGIFY(SETUP_SECTS)
			" sectors - rewrite build/boot/setup");
	fprintf(stderr,"Setup is %d bytes.\n",i);
	for (c=0 ; c<sizeof(buf) ; c++)
		buf[c] = '\0';
	while (i<SETUP_SECTS*512) {
		c = SETUP_SECTS*512-i;
		if (c > sizeof(buf))
			c = sizeof(buf);
		if (write(1,buf,c) != c)
			die("Write call failed");
		i += c;
	}
	
	if ((id=open(argv[3],O_RDONLY,0))<0)
		die("Unable to open 'system'");
	if (read(id,buf,GCC_HEADER) != GCC_HEADER)
		die("Unable to read header of 'system'");
	if (((long *) buf)[5] != 0)
		die("Non-GCC header of 'system'");
	/*lux 注意，这里其实是把gcc头部剔除了。最终生成的Image不包含system的gcc head，而直接就是.text段. 
	 * 参考：https://github.com/lunaczp/Linux-0.11/blob/zhp/Makefile
	 * Image布局是
	 * |bootsect	|setup	|system(.text(setup.o,main.o...) .data)
	 * 
	 * 一个elf文件的布局是这样的
	 * |elfHeader	|progHeader	|.text	|.data	|...
	 * 在最简单的情况下，直接把elf从.text开始的部分，放入内存0x0处，然后从指定的entry处，就可以正常开始执行了。
	 * 这是因为elf文件中代码寻址是间接寻址，放到0x0处，那么所有的间接地址就刚好对应到相应的逻辑地址（物理地址）。
	 * 
	 * 这种简单的情况，也正是kernel加载的方式。
	 * 	1. 这里把system的header去掉，只保留.text,.data...等放到Image内
	 * 	2. bootsect内，会加载Image的setup，system
	 * 	3. setup内，会把system移动到0x0，并设置cs,ds
	 * 	4. 当跳转到cs:0，其实就是进入到system的.text段，也就是head.s对应的代码（head.s是system的.text段第一段代码，由ld链接时候确定）
	 * 		4.1 正常情况，elf提供了entry入口，对应的是main函数。kernel这里的情况不同，
	 * 		4.2 这里直接进入head.s，进行初始化操作；然后手动构造调用栈，（通过ret)进入main函数。
	 * 
	 * 另，
	 * 1. 其实跳转到cs:0的时候，就是进入了gcc生成的elf代码内了。
	 * 2. ld就是把多个object放到一起，解析符号依赖，然后组成一个更大的object。
	 * 	而head.s其实就是一个ld到system的一个object而已，把它放到system的头部，用来做初始化操作，完成后，再真正进入main函数。
	 * 
	 * 另，通常情况下，elf文件的加载，是需要先读取头部，然后分别设定代码段和数据段的。
	 * 因为加载到的逻辑地址通常并不是0x0，所以需要通过设定CS、DS，来进行地址转换。同时也做边界校验。
	 * 比如，system(|.text|.data|...)被加载到 0x1000。那么CS = selector(0x1000),DS = selector(0x1000)。
	 * 	注：linux内一个进程的CS和DS指向同一段逻辑地址，内存内的效果是:
	 * |------------------------------------------------------------|
	 * |----code-----|-----data-----|...............................|
	 * X															Y
	 * CS和DS都指向X。Limit都为Y-X
	 * 
	 * 当代码内有一个函数A，地址是0x105的时候，
	 * 	1. 代表elf文件内，A函数对应.text的偏移是0x105。
	 * 	2. 代表内存内，A距离code base 0x105，即内存内的地址应该是codeBase + offset = 0x1000 + 0x105 = 0x1105
	 *  3. 当cpu对0x105寻址的时候，使用 CS:OFFSET，得到的地址也刚好是0x1105，刚好对应到内存内A被加载到的位置。
	 */
	for (i=0 ; (c=read(id,buf,sizeof buf))>0 ; i+=c ) 
		if (write(1,buf,c)!=c)
			die("Write call failed");
	close(id);
	fprintf(stderr,"System is %d bytes.\n",i);
	if (i > SYS_SIZE*16)
		die("System is too big");
	return(0);
}
