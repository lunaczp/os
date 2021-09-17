/**lux execve系统调用的实现。加载可执行程序
 * argc: argument count
 * argv: argument vector
 */ 
/*
 *  linux/fs/exec.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * #!-checking implemented by tytso.
 */

/*
 * Demand-loading implemented 01.12.91 - no need to read anything but
 * the header into memory. The inode of the executable is put into
 * "current->executable", and page faults do the actual loading. Clean.
 *
 * Once more I can proudly say that linux stood up to being changed: it
 * was less than 2 hours work to get demand-loading completely implemented.
 */

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <a.out.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/segment.h>

extern int sys_exit(int exit_code);
extern int sys_close(int fd);

/*
 * MAX_ARG_PAGES defines the number of pages allocated for arguments
 * and envelope for the new program. 32 should suffice, this gives
 * a maximum env+arg of 128kB !
 */
#define MAX_ARG_PAGES 32
/**lux 为将要执行的被调用程序构造堆栈
 * 调用create_tables前，envp和argv已经填充到相应位置。这里要做的是，填充堆栈。
 * 最终效果如下
 * |------------------------------------------------------------------------------------------------------------------------------------------|
 * |--...---|argc |argv |envp |argv[0] |argv[1] |... |NULL |envp[0] |envp[1] |... |NULL |argvD[0] |...|argvD[Last] |envpD[0] |...|envpD[Last] |
 * |		sp(return value)																					64MB top
 * sp:构造完成后的栈顶，也是函数返回值。
 * 堆栈内，依次是argc,argv,envp...
 * 	argc	参数个数
 * 	argv	指向参数数组的指针，此处即argv[0]所在位置
 * 	argv[0]	包含第一个参数的指针，指向argvD[0]，也即在调用本函数前就填充好的。
 * 	envp	指向envp的指针，此处即envp[0]所在位置
 * 	envp[0]	包含真实envp数据的指针，即指向envpD[0]
 * 
 * 如上，我们就不难理解，为什么main函数是
 * 	int main(int argc, char *argv[]),或者
 * 	int main(int argc, char **argv)
 * 因为argv是一个包含指针的数组，或者是，指向指针的指针。envp也一样。
 */
/*
 * create_tables() parses the env- and arg-strings in new user
 * memory and creates the pointer tables from them, and puts their
 * addresses on the "stack", returning the new stack pointer value.
 */
static unsigned long * create_tables(char * p,int argc,int envc)
{
	unsigned long *argv,*envp;
	unsigned long * sp;

	sp = (unsigned long *) (0xfffffffc & (unsigned long) p);//lux 使p对齐到long。sp是long指针。 这也是为什么最开始定义p的时候-4。一个long占用4个字节。
	sp -= envc+1;
	envp = sp;
	sp -= argc+1;
	argv = sp;
	put_fs_long((unsigned long)envp,--sp);//lux 存放envp指针
	put_fs_long((unsigned long)argv,--sp);//lux 存放argv指针
	put_fs_long((unsigned long)argc,--sp);//lux 存放argc
	while (argc-->0) {
		put_fs_long((unsigned long) p,argv++);//lux 填充argv[]，值为p，p指向argvD
		while (get_fs_byte(p++)) /* nothing */ ;//lux 遇到"\0"停止，移动到下一个参数的开始位置。
	}
	put_fs_long(0,argv);//lux 添加argv结束标记 NULL
	while (envc-->0) {
		put_fs_long((unsigned long) p,envp++);
		while (get_fs_byte(p++)) /* nothing */ ;
	}
	put_fs_long(0,envp);//lux 添加envp结束标记 NULL
	return sp;
}

/*
 * count() counts the number of arguments/envelopes
 */
static int count(char ** argv)
{
	int i=0;
	char ** tmp;

	if (tmp = argv)
		while (get_fs_long((unsigned long *) (tmp++)))
			i++;

	return i;
}

/**lux
 * 关于from_kmem
 * 当从用户态通过系统中断进入内核态时，内核代码段(cs)和数据段(ds)会被设置，同时fs会设置称为0x17，即用户态数据段。(参见system_call.s)
 * 如此，方便内核代码通过fs直接对用户态数据（如传入的参数）进行寻址。
 * 通过使用的方法如：get_fs_byte(address),get_fs_long(address),都是利用“fs:address”从用户态指定的地址读取数据。(因为寻址是基于分段的，地址都是相对地址。一个用户态的地址，必须配合其段基址，才能计算出其绝对地址)
 * 
 * 正常情况下，这些就足够了。那如果一个指针，既有可能来自内核段，也有可能来自用户段，那就要分开处理。
 * 而下面的代码，传入的参数char ** argv 就是这种情况，
 * 1. 假如 *argv 这个数组来自内核段，那么对它使用get_fs_byte()寻址的时候，就需要设定fs为内核段地址。
 * 2. 假如 **argv 这个字符串来自内核段，那么对它使用get_fs_byte()寻址的时候，就需要设定fs为内核段地址。
 * 
 * 其实，来自内核段的时候，完全可以不用get_fs_byte()，单独使用一个函数，或者直接寻址就行（*argv，直接寻址，默认走ds）。
 * 之所以还是要用get_fs_byte()，是因为from_kmem是后来加的，为了保证代码兼容性，和修改起来好改。保留get_fs_byte()的使用，同时根据是来自于内核还是用户，来动态设定fs（set_fs），逻辑更清晰，修改的更简单。
 * 从而，产生了下面的代码。
 */
/*
 * 'copy_string()' copies argument/envelope strings from user
 * memory to free pages in kernel mem. These are in a format ready
 * to be put directly into the top of new user memory.
 *
 * Modified by TYT, 11/24/91 to add the from_kmem argument, which specifies
 * whether the string and the string array are from user or kernel segments:
 * 
 * from_kmem     argv *        argv **
 *    0          user space    user space
 *    1          kernel space  user space
 *    2          kernel space  kernel space
 * 
 * We do this by playing games with the fs segment register.  Since it
 * it is expensive to load a segment register, we try to avoid calling
 * set_fs() unless we absolutely have to.
 */
static unsigned long copy_strings(int argc,char ** argv,unsigned long *page,
		unsigned long p, int from_kmem)
{
	char *tmp, *pag;
	int len, offset = 0;
	unsigned long old_fs, new_fs;

	if (!p)
		return 0;	/* bullet-proofing */
	new_fs = get_ds();
	old_fs = get_fs();
	if (from_kmem==2)//lux **argv和*argv的寻址都走内核
		set_fs(new_fs);
	while (argc-- > 0) {//lux 一次处理一个参数
		if (from_kmem == 1)//lux *argv来自内核，在对其操作前，先修改fs
			set_fs(new_fs);
		/**lux 注意，argv是一个指向数组的指针，而数组本身的元素也是指针。
		 * 即，char ** argv
		 * argv[0] = p1; *p1="A=B\0"
		 */ 
		if (!(tmp = (char *)get_fs_long(((unsigned long *)argv)+argc)))//lux 获取第argc个参数的指针。
			panic("argc is wrong");
		if (from_kmem == 1)
			set_fs(old_fs);
		len=0;		/* remember zero-padding */
		do {
			len++;
		} while (get_fs_byte(tmp++));//lux 读取第argc个参数字符串。直到"\0"，如上面的"A=B\0"
		if (p-len < 0) {	/* this shouldn't happen - 128kB */ //lux 申请的参数页不够大，不应该出现。
			set_fs(old_fs);
			return 0;
		}
		while (len) {//lux 从后向前，逐个拷贝当前参数，如“A=B”，则 B、=、A
			--p; --tmp; --len;
			if (--offset < 0) {
				offset = p % PAGE_SIZE;//lux offset从每个页的最后一个long处开始。
				if (from_kmem==2)
					set_fs(old_fs);
				if (!(pag = (char *) page[p/PAGE_SIZE]) &&
				    !(pag = (char *) page[p/PAGE_SIZE] =
				      (unsigned long *) get_free_page())) //lux 分配页面填充page
					return 0;
				if (from_kmem==2)
					set_fs(new_fs);

			}
			/**lux
			 * 初始时，p指向最后一个页的页内最后一个long(4字节)偏移。
			 * 随着从后往前填充argv，p减少
			 * pag指向当前页
			 * offset指向当前业内偏移
			 */
			*(pag + offset) = get_fs_byte(tmp);//lux 逐直接拷贝到page对应的页内存里。
		}
	}
	if (from_kmem==2)
		set_fs(old_fs);
	return p;//lux 返回最新的p指针
}
/**lux 调整ldt
 * 以当前进程的code_base data_base为基址，
 * code_limit取决于text段大小。
 * data_limit始终为64MB
 *
 * 流程：
 * 1. 更新ldt的cs、ds
 * 2. 将之前已经分配的物理页（存放参数和环境变量）映射到该进程的线性地址空间（从64MB顶部开始，自上而下）
 */ 
static unsigned long change_ldt(unsigned long text_size,unsigned long * page)
{
	unsigned long code_limit,data_limit,code_base,data_base;
	int i;

	code_limit = text_size+PAGE_SIZE -1;
	code_limit &= 0xFFFFF000;
	data_limit = 0x4000000;//lux 64MB
	code_base = get_base(current->ldt[1]);
	data_base = code_base;
	set_base(current->ldt[1],code_base);
	set_limit(current->ldt[1],code_limit);
	set_base(current->ldt[2],data_base);
	set_limit(current->ldt[2],data_limit);
/* make sure fs points to the NEW data segment */
	__asm__("pushl $0x17\n\tpop %%fs"::);
	data_base += data_limit;//lux 从数据段顶部，映射已经分配好的参数页。
	for (i=MAX_ARG_PAGES-1 ; i>=0 ; i--) {
		data_base -= PAGE_SIZE;
		if (page[i])
			put_page(page[i],data_base);//lux 地址映射，已经分配的物理地址，映射到该进程的逻辑地址空间。
	}
	return data_limit;
}
/**lux execve系统调用的实现
 */ 
/*
 * 'do_execve()' executes a new program.
 */
int do_execve(unsigned long * eip,long tmp,char * filename,
	char ** argv, char ** envp)
{
	struct m_inode * inode;
	struct buffer_head * bh;
	struct exec ex;
	unsigned long page[MAX_ARG_PAGES];//lux 为参数和环境变量提供存储
	int i,argc,envc;
	int e_uid, e_gid;
	int retval;
	int sh_bang = 0;
	unsigned long p=PAGE_SIZE*MAX_ARG_PAGES-4;

	if ((0xffff & eip[1]) != 0x000f)//lux 只有用户态才能调用
		panic("execve called from supervisor mode");
	for (i=0 ; i<MAX_ARG_PAGES ; i++)	/* clear page-table */
		page[i]=0;
	if (!(inode=namei(filename)))		/* get executables inode */
		return -ENOENT;
	argc = count(argv);
	envc = count(envp);
	
restart_interp:
	if (!S_ISREG(inode->i_mode)) {	/* must be regular file */
		retval = -EACCES;
		goto exec_error2;
	}
	i = inode->i_mode;
	e_uid = (i & S_ISUID) ? inode->i_uid : current->euid;//lux 文件是否设置了setuid,是则取之为有效用户。效果：允许用户以其他用户的身份执行程序。一般地，允许普通用户执行属于root的程序
	e_gid = (i & S_ISGID) ? inode->i_gid : current->egid;//lux 文件是否设置了setgid,是则取之为有效组。
	if (current->euid == inode->i_uid)//lux 当前进程的有效用户是文件拥有者，则取拥有者的权限属性
		i >>= 6;
	else if (current->egid == inode->i_gid)//lux 取所属组的权限属性
		i >>= 3;
	if (!(i & 1) &&
	    !((inode->i_mode & 0111) && suser())) {//lux 没有执行权限
		retval = -ENOEXEC;
		goto exec_error2;
	}
	if (!(bh = bread(inode->i_dev,inode->i_zone[0]))) {
		retval = -EACCES;
		goto exec_error2;
	}
	ex = *((struct exec *) bh->b_data);	/* read exec-header */ //lux 可执行脚本的头部
	if ((bh->b_data[0] == '#') && (bh->b_data[1] == '!') && (!sh_bang)) {//lux 处理脚本文件
		/*
		 * This section does the #! interpretation.
		 * Sorta complicated, but hopefully it will work.  -TYT
		 */

		char buf[1023], *cp, *interp, *i_name, *i_arg;//lux 下面开始获取第一行的解释器文件和参数，如/usr/local/bin/php -l
		unsigned long old_fs;

		strncpy(buf, bh->b_data+2, 1022);//lux 读取整个逻辑块 的数据
		brelse(bh);
		iput(inode);
		buf[1022] = '\0';
		if (cp = strchr(buf, '\n')) {//lux 先定位到换行处
			*cp = '\0';//lux 换行处标记为结束
			for (cp = buf; (*cp == ' ') || (*cp == '\t'); cp++);//lux 跳过空格，从头移动到非空处。比如"# /bin/bash"，则移动到"/"
		}
		if (!cp || *cp == '\0') {//lux 没有找到换行符，或者没有找到有效字符串。（看来不支持1行超过1024byte的情况。因为只读取了一个zone，只有1024byte，找不到换行，就认为失败。如果第一行特别长，这个不支持。）
			retval = -ENOEXEC; /* No interpreter name found */
			goto exec_error1;
		}
		interp = i_name = cp;//lux 指向解析器的首字符，如，/usr/bin/php的第一个/
		i_arg = 0;
		for ( ; *cp && (*cp != ' ') && (*cp != '\t'); cp++) {
 			if (*cp == '/')
				i_name = cp+1;//lux i_name定位到最后一个“/”后面的字符，即文件名，如/usr/bin/php中的php
		}
		if (*cp) {//lux 后面还有（还没有碰到"\0"）
			*cp++ = '\0';//lux cp处记为"\0"，往前是文件名，往后是参数。记为“\0"，方便处理文件名。
			i_arg = cp;//lux i_arg指向参数部分，如/usr/bin/php -l中的-l。此时“php -l”中间的空格经由已经变成了“\0”
		}
		/*
		 * OK, we've parsed out the interpreter name and
		 * (optional) argument.
		 */
		if (sh_bang++ == 0) {//lux 第一次
			p = copy_strings(envc, envp, page, p, 0);//lux 复制envp 环境变量
			p = copy_strings(--argc, argv+1, page, p, 0);//lux 复制argv 环境变量 不要第一个参数(自身)
		}
		/**lux 下面是构造新的参数，是为找到的解析器构造新参数，然后调用解析器。
		 * 比如，我们要execve a.php，参数是x y。环境变量HOME=/home/nuc。文件内容是
		 * #!/bin/php
		 * echo $argv[0] + $argv[1];
		 * 
		 * 我们在命令行执行`./a.php x y`
		 * 那么，第一次解析，解析出
		 * 		解析器：/bin/php
		 * 		命令行参数：x,y
		 * 		环境变量HOME=/home/nuc
		 * 之后，我们就要载入二进制文件/bin/php，然后为它构造参数。最终的效果是
		 * /bin/php  a.php x y
		 * 		可执行文件： /bin/php
		 * 		命令行参数：a.php x y
		 * 		环境变量HOME=/home/nuc
		 * 如上，对脚本类型的文件，我们其实是转发了一次，重新封装参数后，最后真正调用的还是解释器二进制程序。
		 */
		/*
		 * Splice in (1) the interpreter's name for argv[0]
		 *           (2) (optional) argument to interpreter
		 *           (3) filename of shell script
		 *
		 * This is done in reverse order, because of how the
		 * user environment and arguments are stored.
		 */
		p = copy_strings(1, &filename, page, p, 1);//lux 文件名。&filename来自内核段，filename来自用户段
		argc++;
		if (i_arg) {
			p = copy_strings(1, &i_arg, page, p, 2);//lux 参数数量。
			argc++;
		}
		p = copy_strings(1, &i_name, page, p, 2);//lux 可执行文件的文件名
		argc++;
		if (!p) {
			retval = -ENOMEM;//lux 内存不够
			goto exec_error1;
		}//lux 上面完成了参数构造。下面开始正常的二进制文件解析
		/*
		 * OK, now restart the process with the interpreter's inode.
		 */
		old_fs = get_fs();
		set_fs(get_ds());//lux 由于下面使用的interp是来自内核态，需要临时设置fs为内核数据段地址。
		if (!(inode=namei(interp))) { /*lux get executables inode，拿到解释器文件*/
			set_fs(old_fs);
			retval = -ENOENT;
			goto exec_error1;
		}
		set_fs(old_fs);
		goto restart_interp;//lux 开始正常解析
	}
	brelse(bh);//lux 下面开始处理二进制可执行文件
	if (N_MAGIC(ex) != ZMAGIC || ex.a_trsize || ex.a_drsize ||
		ex.a_text+ex.a_data+ex.a_bss>0x3000000/*lux 16M*3*/ ||
		inode->i_size < ex.a_text+ex.a_data+ex.a_syms+N_TXTOFF(ex)) {//lux 基本参数校验
		retval = -ENOEXEC;
		goto exec_error2;
	}
	if (N_TXTOFF(ex) != BLOCK_SIZE) {
		printk("%s: N_TXTOFF != BLOCK_SIZE. See a.out.h.", filename);
		retval = -ENOEXEC;
		goto exec_error2;
	}
	if (!sh_bang) {//lux 如果是脚本文件，那么不用再处理，因为已经处理过了。
		p = copy_strings(envc,envp,page,p,0);//lux 复制参数和环境变量到新程序的堆栈，每次调用都会更新p到新的位置
		p = copy_strings(argc,argv,page,p,0);
		if (!p) {
			retval = -ENOMEM;
			goto exec_error2;
		}
	}
/* OK, This is the point of no return */
	if (current->executable)
		iput(current->executable);//lux 移除当前的可执行文件
	current->executable = inode;//lux 新的可执行文件
	for (i=0 ; i<32 ; i++)
		current->sigaction[i].sa_handler = NULL;//lux reset sig
	for (i=0 ; i<NR_OPEN ; i++)
		if ((current->close_on_exec>>i)&1)//lux 需要关闭
			sys_close(i);//lux close file
	current->close_on_exec = 0;//lux reset默认值。
	free_page_tables(get_base(current->ldt[1]),get_limit(0x0f));//lux free原有内存映射。这样当执行新的代码时，就会触发缺页中断，然后由中断程序去加载可执行文件，参考mm/memory.c
	free_page_tables(get_base(current->ldt[2]),get_limit(0x17));
	if (last_task_used_math == current)
		last_task_used_math = NULL;
	current->used_math = 0;
	/**lux p是一个数值。内存分布如下
	 * |-------------------------------------------------|
	 * |--------------------|---argv-----|---envp--------|
	 * 						p 
	 * 最开始，p=PAGE_SIZE*MAX_ARG_PAGES-4，随着envp和argv的拷贝，p逐渐减小。
	 * 当执行完下面的操作
	 * 	p = change_ldt(ex.a_text,page) - MAX_ARG_PAGES*PAGE_SIZE
	 * 得到的p对应的就是进程的64MB线性地址空间中的偏移，指向argv的位置。
	 * 注意，这里实现的效果是，在64MB数据段中，从顶部填充环境变量（envp）和参数（argv）。
	 * 
	 * 下面，当对p进行指针操作，(char *p)，那么得到的就是一个逻辑地址(数据段的偏移)，指向上图中的位置
	 */
	p += change_ldt(ex.a_text,page)-MAX_ARG_PAGES*PAGE_SIZE;
	p = (unsigned long) create_tables((char *)p,argc,envc);//lux 构造参数堆栈，具体内存布局见create_tables
	/**lux 可执行文件在内存的分布
	 * |---------------------------------------------------------------------------------|
	 * |----code-----|-----data------|------bss-----|......|---stack----|---argv,envp----|
	 * 0		end_code		end_data			brk	   start_stack
	 * 
	 * 注意，Linux本身是不支持separate I&D的，即代码和数据是在一起的（从codeBase==dataBase可知）
	 * 不过，代码段和数据段的大小不一样。
	 * 代码段：从0到a_text，即用text大小限制了代码段。
	 * 数据段：从0到64M，固定。
	 * 
	 * 另外，a_text,a_data,都是由可执行文件提供，从而计算出end_code,end_data（内存内各个模块的界限）
	 * 
	 * 注意，a.out的格式比较简单，不需要单独加载不同的段，而是简单的把文件整个线性映射到内存空间即可，其代码内部的地址都是从0开始的相对地址。
	 * 因此这里没有任务加载操作，而是稍后由缺页中断异步加载即可。
	 * 
	 * 如果是elf类型的文件，则需要先映射不同的段到不同的线性地址空间才可以。
	 */
	current->brk = ex.a_bss +
		(current->end_data = ex.a_data +
		(current->end_code = ex.a_text));
	current->start_stack = p & 0xfffff000;//lux p是堆栈指针
	current->euid = e_uid;
	current->egid = e_gid;
	i = ex.a_text+ex.a_data;
	while (i&0xfff)//lux 如果代码段和数据段小于1页，则把页内剩余部分清0 注： linux0.11实际不会触发，因为是页对齐的。这是之前版本的遗留代码
		put_fs_byte(0,(char *) (i++));
	eip[0] = ex.a_entry;		/* eip, magic happens :-) */ //lux 修改堆栈内的eip。
	eip[3] = p;			/* stack pointer */ //lux 修改堆栈指针
	return 0;
exec_error2:
	iput(inode);
exec_error1:
	for (i=0 ; i<MAX_ARG_PAGES ; i++)
		free_page(page[i]);
	return(retval);
}
