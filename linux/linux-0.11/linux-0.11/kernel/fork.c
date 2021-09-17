/*
 *  linux/kernel/fork.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also system_call.s), and some misc functions ('verify_area').
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
 */
#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

extern void write_verify(unsigned long address);

long last_pid=0;

void verify_area(void * addr,int size)
{
	unsigned long start;

	start = (unsigned long) addr;
	size += start & 0xfff;
	start &= 0xfffff000;
	start += get_base(current->ldt[2]);
	while (size>0) {
		size -= 4096;
		write_verify(start);
		start += 4096;
	}
}
/*lux:内存拷贝，实际上使用了CopyOnWrite，只拷贝了页表*/
int copy_mem(int nr,struct task_struct * p)
{
	unsigned long old_data_base,new_data_base,data_limit;
	unsigned long old_code_base,new_code_base,code_limit;

	code_limit=get_limit(0x0f);//当前进程的ldt[0]的limit
	data_limit=get_limit(0x17);//当前进程的ldt[1]的limit
	old_code_base = get_base(current->ldt[1]);
	old_data_base = get_base(current->ldt[2]);
	if (old_data_base != old_code_base)
		panic("We don't support separate I&D");
	if (data_limit < code_limit)
		panic("Bad data_limit");
	new_data_base = new_code_base = nr * 0x4000000;//lux 64mb 每个进程有64mb的逻辑地址空间
	p->start_code = new_code_base;
	set_base(p->ldt[1],new_code_base);//lux 设定新进程的code base
	set_base(p->ldt[2],new_data_base);//lux 设定新进程的data base
	if (copy_page_tables(old_data_base,new_data_base,data_limit)) {//lux 拷贝当前进程的页表到新进程
		free_page_tables(new_data_base,data_limit);
		return -ENOMEM;
	}
	return 0;
}
/*Lux copy process, fork会调用， see system_call:217*/
/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entirety.
 */
/*lux nr:之前生成找到的task空闲表项*/
int copy_process(int nr,long ebp,long edi,long esi,long gs,long none,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss)
{/*lux 这么多参数，在调用之前已经被显示或者自动push入栈。这里当c编译器转换成的汇编代码里，会直接调用堆栈内获取到所有参数。 注意入栈参数是从右到左。 see system_call.s:217*/
	struct task_struct *p;
	int i;
	struct file *f;

	p = (struct task_struct *) get_free_page();
	if (!p)
		return -EAGAIN;
	task[nr] = p;
	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */ //lux 内容拷贝
	p->state = TASK_UNINTERRUPTIBLE;
	p->pid = last_pid;
	p->father = current->pid;
	p->counter = p->priority;
	p->signal = 0;
	p->alarm = 0;
	p->leader = 0;		/* process leadership doesn't inherit */
	p->utime = p->stime = 0;
	p->cutime = p->cstime = 0;
	p->start_time = jiffies;
	p->tss.back_link = 0;
	p->tss.esp0 = PAGE_SIZE + (long) p;//lux 每个进程都有自己的内核栈，与p在同一个页，指向页末，向下生长。 注意这里保存的是物理地址，与ss0(内核数据段选择子，从0x0开始）组合后，其实地址不变。
	p->tss.ss0 = 0x10;//lux 内核data段选择子
	p->tss.eip = eip;
	p->tss.eflags = eflags;
	p->tss.eax = 0;//lux eax存放的是fork调用的返回值，对于子进程而言，应该是0
	p->tss.ecx = ecx;
	p->tss.edx = edx;
	p->tss.ebx = ebx;
	p->tss.esp = esp;
	p->tss.ebp = ebp;
	p->tss.esi = esi;
	p->tss.edi = edi;
	p->tss.es = es & 0xffff;
	p->tss.cs = cs & 0xffff;
	p->tss.ss = ss & 0xffff;
	p->tss.ds = ds & 0xffff;
	p->tss.fs = fs & 0xffff;
	p->tss.gs = gs & 0xffff;
	p->tss.ldt = _LDT(nr);
	p->tss.trace_bitmap = 0x80000000;
	if (last_task_used_math == current)
		__asm__("clts ; fnsave %0"::"m" (p->tss.i387));
	if (copy_mem(nr,p)) {//lux 拷贝当前进程的内存数据到p的内存空间（实际上只是做了页表映射，CopyOnWrite策略）
		task[nr] = NULL;//lux 失败，清空，返回
		free_page((long) p);
		return -EAGAIN;
	}
	for (i=0; i<NR_OPEN;i++)
		if (f=p->filp[i])
			f->f_count++;//lux 打开的文件索引++
	if (current->pwd)
		current->pwd->i_count++;
	if (current->root)
		current->root->i_count++;
	if (current->executable)
		current->executable->i_count++;
	set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));//lux 设定gdt内的该进程的tss表项
	set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));//lux 设定gdt内的该进程的ldt表项
	p->state = TASK_RUNNING;	/* do this last, just in case */
	return last_pid;
}
/*lux 找到一个可用的/空的task表项, fork会调用，see system_call.s:208*/
int find_empty_process(void)
{
	int i;

	repeat:
		if ((++last_pid)<0) last_pid=1;//lux ++last_pid,产生一个最新的pid
		for(i=0 ; i<NR_TASKS ; i++)//lux 遍历，如果这个新pid已经被占用，则重复，直到找到一个没有被占用的pid
			if (task[i] && task[i]->pid == last_pid) goto repeat;
	for(i=1 ; i<NR_TASKS ; i++)//lux 遍历，找到一个空闲的表项，返回表项索引
		if (!task[i])
			return i;
	return -EAGAIN;
}
