/*
 *  linux/mm/memory.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * demand-loading started 01.12.91 - seems it is high on the list of
 * things wanted, and it should be easy to implement. - Linus
 */

/*
 * Ok, demand-loading was easy, shared pages a little bit tricker. Shared
 * pages started 02.12.91, seems to work. - Linus.
 *
 * Tested sharing by executing about 30 /bin/sh: under the old kernel it
 * would have taken more than the 6M I have free, but it worked well as
 * far as I could see.
 *
 * Also corrected some "invalidate()"s - I wasn't doing enough of them.
 */

#include <signal.h>

#include <asm/system.h>

#include <linux/sched.h>
#include <linux/head.h>
#include <linux/kernel.h>

volatile void do_exit(long code);
//lux 内存不够用，退出
static inline volatile void oom(void)
{
	printk("out of memory\n\r");
	do_exit(SIGSEGV);
}
//lux 重新加载cr3,可以刷新页表的高速缓存。在修改了页表后使用，达到使修改立即生效的目的。(页目录表基址是0x0)
#define invalidate() \
__asm__("movl %%eax,%%cr3"::"a" (0))

/* these are not to be changed without changing head.s etc */
#define LOW_MEM 0x100000 /*lux 1mb以上可供程序使用，因为640k-1MB为显卡和ROM BIOS所占用。*/
#define PAGING_MEMORY (15*1024*1024)/*lux 系统最大支持16mb内存。除去1mb上面被占用的，剩余15mb*/
#define PAGING_PAGES (PAGING_MEMORY>>12) /*lux 每页4K，共15*256=3840页*/
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)/*lux 逻辑地址到mem_map表内偏移的映射。第一个页开始于LOW_MEM,为mem_map[0]*/
#define USED 100

#define CODE_SPACE(addr) ((((addr)+4095)&~4095) < \
current->start_code + current->end_code)

static long HIGH_MEMORY = 0;
//lux 拷贝一页数据(参数为物理地址)
#define copy_page(from,to) \
__asm__("cld ; rep ; movsl"::"S" (from),"D" (to),"c" (1024):"cx","di","si")

static unsigned char mem_map [ PAGING_PAGES ] = {0,};/*lux 内存管理表(按页管理)*/

/*
 * Get physical address of first (actually last :-) free page, and mark it
 * used. If no free pages left, return 0.
 */
unsigned long get_free_page(void)
{
register unsigned long __res asm("ax");
/**lux 获取一个可用的物理页
 * 实现：从mem_map数组，倒序找到第一个空闲页面。并清空对应的物理页，返回物理页开始地址。
 * eax			__res
 * low_mem		%2
 * PAGING_PAGES	ecx
 * mem_map+PAGING_PAGES-1	edi ;mem_map[PAGING_PAGES-1]最后一页
 */

__asm__("std ; repne ; scasb\n\t" //lux 从mem_map[last]开始和al(0)比较，直到相等（找到空闲页），或者循环结束
	"jne 1f\n\t" //如果没找到，则返回
	"movb $1,1(%%edi)\n\t" //mem_map[x] = 1
	"sall $12,%%ecx\n\t" // phy_offset = page_offset * 4k
	"addl %2,%%ecx\n\t" // phy_address = phy_offset + low_mem
	"movl %%ecx,%%edx\n\t" // edx = phy_address
	"movl $1024,%%ecx\n\t"
	"leal 4092(%%edx),%%edi\n\t" // edi = 4092 + edx (该页末)
	"rep ; stosl\n\t" //清空该物理页，设置0
	"movl %%edx,%%eax\n"//eax = phy_address
	"1:"
	:"=a" (__res)
	:"0" (0),"i" (LOW_MEM),"c" (PAGING_PAGES),
	"D" (mem_map+PAGING_PAGES-1)
	:"di","cx","dx");
return __res;
}

/*
 * Free a page of memory at physical address 'addr'. Used by
 * 'free_page_tables()'
 */
void free_page(unsigned long addr)
{
	if (addr < LOW_MEM) return;
	if (addr >= HIGH_MEMORY)
		panic("trying to free nonexistent page");
	addr -= LOW_MEM;
	addr >>= 12;
	if (mem_map[addr]--) return;//lux >=1的情况下，-1后返回。 注意，x--是先使用后-1. 所以 if(1--) 为真。
	mem_map[addr]=0;//lux 如果到这里，说明出现异常，上一步的mem_map[addr] =0，此时mem_map[addr] = -1，说明是'free a free page'。先重置为0，并死机。
	panic("trying to free free page");
}
/**lux 清空指定逻辑内存区间对应的页表。用于清理一个进程的页表/内存页（当exit的时候）
 */
/*
 * This function frees a continuos block of page tables, as needed
 * by 'exit()'. As does copy_page_tables(), this handles only 4Mb blocks.
 */
int free_page_tables(unsigned long from,unsigned long size)
{
	unsigned long *pg_table;
	unsigned long * dir, nr;

	if (from & 0x3fffff)
		panic("free_page_tables called with wrong alignment");
	if (!from)
		panic("Trying to free up swapper memory space");
	size = (size + 0x3fffff) >> 22;//lux 页目录表项个数
	dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */ //lux 要free的页目录表项指针
	for ( ; size-->0 ; dir++) {
		if (!(1 & *dir))//lux 无效的不需要处理 P=0
			continue;
		pg_table = (unsigned long *) (0xfffff000 & *dir);//lux 页表指针
		for (nr=0 ; nr<1024 ; nr++) {//lux free 所有页表项对应的物理页，并清空表项
			if (1 & *pg_table)
				free_page(0xfffff000 & *pg_table);//lux free 物理页
			*pg_table = 0;//lux 清空表项
			pg_table++;
		}
		free_page(0xfffff000 & *dir);//lux free 页表所在物理页
		*dir = 0;//lux 清空页目录项
	}
	invalidate();//lux 更新页表缓存
	return 0;
}
/**lux 内存拷贝，采用Copy on Write策略：
 * 只拷贝页表，然后让对应的物理页引用++
 * 如此可以让进程共用一份物理页，当随后需要写的时候，再单独分开。
 * 如此减少/推迟了不必要的内存分配操作，且节省了内存空间
 * from： 旧基址
 * to：新基址
 * size：大小
 * 
 * 第一次copy_page_tables发生在第一次fork，即task0 => task1
 * task0 和内核段其实是重叠的。
 * 内核段(C&D) 基址 0x0，段大小 64MB
 * task0(C&D) 基址 0x0，段大小 640KB
 * task1(C&D) 基址 64MB，段大小 640KB
 * 
 * 页目录表:
 * 0-3，是内核代码和task0使用的，映射了16MB内存
 * 3-15 没用(内核和task0只用了0-3，而task1是从6mb开始的，所以中间这段其实是空着的。)
 * 16-31 task1
 * 32-47 task2
 * ...
 * 注意，逻辑地址和页表的映射是自动完成的。4G的线性空间，映射到1024个页目录表和1024*1024个页表项目。(物理地址0x-0x1000共4k，每个dir entry 4byte，共1024个。see head.s _pg_dir)
 * 又由于每个进程使用4G线性地址空间的64MB（通过segment控制），task1:64mb-127mb task2 128-191mb...
 * 所以进程和页表的映射关系是确定的，比如task1的地址空间是64mb-127mb，那么映射到的页目录表项就是第16-31。
 * 
 * 另外，task1虽然有64mb寻址空间，但是其segmdent限制是640k。所以在拷贝页表时，其实只是拷贝了部分。64mb剩余空间的页目录表和页表都没有使用。
 * 
 * 另外，所有没有设置的页目录表和页表，都可以在随后的缺页中断中自动补全。
*/
/*
 *  Well, here is one of the most complicated functions in mm. It
 * copies a range of linerar addresses by copying only the pages.
 * Let's hope this is bug-free, 'cause this one I don't want to debug :-)
 *
 * Note! We don't copy just any chunks of memory - addresses have to
 * be divisible by 4Mb (one page-directory entry), as this makes the
 * function easier. It's used only by fork anyway.
 *
 * NOTE 2!! When from==0 we are copying kernel space for the first
 * fork(). Then we DONT want to copy a full page-directory entry, as
 * that would lead to some serious memory waste - we just copy the
 * first 160 pages - 640kB. Even that is more than we need, but it
 * doesn't take any more memory - we don't copy-on-write in the low
 * 1 Mb-range, so the pages can be shared with the kernel. Thus the
 * special case for nr=xxxx.
 */
int copy_page_tables(unsigned long from,unsigned long to,long size)
{
	unsigned long * from_page_table;
	unsigned long * to_page_table;
	unsigned long this_page;
	unsigned long * from_dir, * to_dir;
	unsigned long nr;

	if ((from&0x3fffff) || (to&0x3fffff))
		panic("copy_page_tables called with wrong alignment");

	/**lux 根据线性地址，换算出其所属于的页目录表的入口指针
	 * page_table是从物理地址0开始的，每个表项4字节，映射4MB的内存空间。
	 * 第一步，我们需要计算出，from对应的页目录表偏移，就知道其物理地址是多少了。
	 * 比如
	 * 		from是3MB，则显然位于第0个条目，条目物理地址是0x0
	 * 		from是5MB，则显然位于第1个条目，条目物理地址是0x4（每个条目4字节）
	 * 
	 * 第二步，我们怎么让cpu寻址这个物理地址呢，对于不开启分页的时候，直接使用0x4就可以访问到物理地址0x4，但是当我们开启了分页，就要考虑转换的问题。我们需要构造一个虚拟地址来映射到物理地址0x4。
	 * 这里，我们其实面临的情况很简单，因为根据之前设置的页表结构，低16MB的虚拟地址和物理地址是直接映射的。所以，
	 * 我们可以直接使用0x4作为虚拟地址，来映射物理地址0x4。
	 * 
	 * 实现，
	 * dirNum = from >> 22 得到页目录表偏移
	 * addr = dirNum * 4 = dirNum << 2 （每个条目占4个字节）
	 * long * from_dir = (long *) addr
	 * 如此，我们就得到了指向from所在目录表表项入口的指针。
	 * (from>>20) & 0xffc 其实等于 (from>>22)<< 2 也等于 （from>>22)*4
	 */ 
	from_dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */ //lux 页表项指针
	to_dir = (unsigned long *) ((to>>20) & 0xffc); //lux 待填充页表项指针
	size = ((unsigned) (size+0x3fffff)) >> 22;//lux 0x3fffff = 4M-1 这里是要换算出页表项个数。 task0 大小 640k，则size= (640k+4m-1)>>22 = 1
	for( ; size-->0 ; from_dir++,to_dir++) {
		if (1 & *to_dir)
			panic("copy_page_tables: already exist");
		if (!(1 & *from_dir))//lux P=0 无效
			continue;
		from_page_table = (unsigned long *) (0xfffff000 & *from_dir);//lux 页表入口地址
		if (!(to_page_table = (unsigned long *) get_free_page()))//lux 新分配一页，给新程序的页表使用。
			return -1;	/* Out of memory, see freeing */

	 	/**lux page (table) entry
		  *	|31					12	|11	10	9	|8	|7	|6	|5	|4	|3	|2	|1	|0|
		  *	|Page Frame Address		|AVL		|0	|D  |A  |0			|U/S|R/W|P|
		  */
		*to_dir = ((unsigned long) to_page_table) | 7;//lux 设置新的页目录表项，7代表111，代表P=1 R/W=1 U/S=1
		nr = (from==0)?0xA0:1024;//lux 为0的情况下，因为进程0只用了640k，只需要拷贝640k/4k=160=0xA0个表项就可以了。
		for ( ; nr-- > 0 ; from_page_table++,to_page_table++) {
			this_page = *from_page_table;//lux 当前表项
			if (!(1 & this_page))//lux 无效表项
				continue;
			this_page &= ~2;//lux 设置只读，当后面发生写的时候，产生异常，系统捕捉到异常，再进行物理内存的分配。(CopyOnWrite)
			*to_page_table = this_page;//lux 赋值给新进程的页表项
			if (this_page > LOW_MEM) {//lux 对于高端物理内存，记录引用。//todo
				*from_page_table = this_page;//lux 修改当前进程的表项(上面设置了只读)。
				this_page -= LOW_MEM;
				this_page >>= 12;
				mem_map[this_page]++;//lux 物理页引用++
			}
		}
	}
	invalidate();
	return 0;
}
/**lux 将一个物理页(page)绑定/映射到指定的逻辑地址(address)
 */ 
/*
 * This function puts a page in memory at the wanted address.
 * It returns the physical address of the page gotten, 0 if
 * out of memory (either when trying to access page-table or
 * page.)
 */
unsigned long put_page(unsigned long page,unsigned long address)
{
	unsigned long tmp, *page_table;

/* NOTE !!! This uses the fact that _pg_dir=0 */

	if (page < LOW_MEM || page >= HIGH_MEMORY)
		printk("Trying to put page %p at %p\n",page,address);
	if (mem_map[(page-LOW_MEM)>>12] != 1)//lux 因为理论上是先从 mem_map中分配出的p，所以应该已经标记为占用。否则就是见鬼了。
		printk("mem_map disagrees with %p at %p\n",page,address);
	page_table = (unsigned long *) ((address>>20) & 0xffc);//lux 该逻辑地址对应的页目录表项的指针。更准确的命名是page_dir
	if ((*page_table)&1)//lux 表项已经存在，可用 P=1
		page_table = (unsigned long *) (0xfffff000 & *page_table);//lux 这时候的page_table才是指向页表的指针。之前的page_table应该叫做page_dir
	else {//lux 表项无效，P=0，则分配新页表
		if (!(tmp=get_free_page()))
			return 0;
		*page_table = tmp|7;//lux 设置页目录表项，指向刚分配的二级页表，U/S=1 R/W=1 P=1
		page_table = (unsigned long *) tmp;//lux 赋值page_table,这时候page_table指向的是二级页表
	}
	page_table[(address>>12) & 0x3ff] = page | 7;//lux address在页表内的偏移项，赋值为物理页地址（page），并设置U/s R/W P
/* no need for invalidate */
	return page;//lux 返回物理页地址
}

/**lux 取消页面写保护，由do_wp_page调用，处理写保护异常。
 * 逻辑：
 * 1. 如果不是共享，则返回
 * 2. 否则，新申请物理内存页，拷贝数据，更新页表。
 * 3. done
 */
void un_wp_page(unsigned long * table_entry)
{
	unsigned long old_page,new_page;

	old_page = 0xfffff000 & *table_entry;
	if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)]==1) {//lux ==1 说明只有一个进程在使用。直接取消写保护，返回即可。
		*table_entry |= 2;//lux R/W=1 允许读写
		invalidate();//lux 更新页表缓存
		return;
	}
	if (!(new_page=get_free_page()))//lux 获取新物理页
		oom();
	if (old_page >= LOW_MEM)
		mem_map[MAP_NR(old_page)]--;//lux 原有物理页的引用可以--了。
	*table_entry = new_page | 7;//lux 表项更新为新的物理页，并设置 U/S=1 R/W=1 P=1
	invalidate();//lux 更新页表缓存
	copy_page(old_page,new_page);//lux 数据拷贝
}	
/**lux 页面写保护异常中断处理 (wp:write protect)，由page.s调用
 * 当fork的时候，父子进程是共享物理内存的，相应页表被设置为只读。
 * 当父子某一个进程写入的时候，触发写保护异常中断，来到这里。
 * 调用un_wp_page来处理。
 */ 
/*
 * This routine handles present pages, when users try to write
 * to a shared page. It is done by copying the page to a new address
 * and decrementing the shared-page counter for the old page.
 *
 * If it's in code space we exit with a segment error.
 */
void do_wp_page(unsigned long error_code,unsigned long address)
{
#if 0
/* we cannot do this yet: the estdio library writes to code space */
/* stupid, stupid. I really want the libc.a from GNU */
	if (CODE_SPACE(address))
		do_exit(SIGSEGV);
#endif
	un_wp_page((unsigned long *)
		(((address>>10) & 0xffc) + (0xfffff000 &
		*((unsigned long *) ((address>>20) &0xffc)))));//lux 获取到页表项指针： 页表入口+表内偏移

}

void write_verify(unsigned long address)
{
	unsigned long page;

	if (!( (page = *((unsigned long *) ((address>>20) & 0xffc)) )&1))
		return;
	page &= 0xfffff000;
	page += ((address>>10) & 0xffc);
	if ((3 & *(unsigned long *) page) == 1)  /* non-writeable, present */
		un_wp_page((unsigned long *) page);
	return;
}
//lux 分配物理内存到指定逻辑地址
void get_empty_page(unsigned long address)
{
	unsigned long tmp;

	if (!(tmp=get_free_page()) || !put_page(tmp,address)) {//lux 获取一个新物理页，并分配给指定的逻辑地址
		free_page(tmp);		/* 0 is ok - ignored */
		oom();
	}
}
/**lux 尝试共享一个页面：当前进程尝试和进程p共享address所指向的物理地址。
 * 如果对应的物理地址存在且干净（D=0），则尝试共享。
 */
/*
 * try_to_share() checks the page at address "address" in the task "p",
 * to see if it exists, and if it is clean. If so, share it with the current
 * task.
 *
 * NOTE! This assumes we have checked that p != current, and that they
 * share the same executable.
 */
static int try_to_share(unsigned long address, struct task_struct * p)
{
	unsigned long from;
	unsigned long to;
	unsigned long from_page;
	unsigned long to_page;
	unsigned long phys_addr;

	from_page = to_page = ((address>>20) & 0xffc);
	from_page += ((p->start_code>>20) & 0xffc);
	to_page += ((current->start_code>>20) & 0xffc);
/* is there a page-directory at from? */
	from = *(unsigned long *) from_page;
	if (!(from & 1))
		return 0;
	from &= 0xfffff000;
	from_page = from + ((address>>10) & 0xffc);//lux 这里得到了真正的页表项指针
	phys_addr = *(unsigned long *) from_page;//lux 页表项内保存的物理地址
/* is the page clean and present? */
	if ((phys_addr & 0x41) != 0x01)//lux 物理地址干净才可以继续，否则不能共享
		return 0;
	phys_addr &= 0xfffff000;
	if (phys_addr >= HIGH_MEMORY || phys_addr < LOW_MEM)
		return 0;
	to = *(unsigned long *) to_page;
	if (!(to & 1))
		if (to = get_free_page())//lux 如果对应的页表不存在，则新分配，并初始化 U/S=1 R/W=1 P=1
			*(unsigned long *) to_page = to | 7;
		else
			oom();
	to &= 0xfffff000;
	to_page = to + ((address>>10) & 0xffc);//lux 这里得到真正的页表项指针
	if (1 & *(unsigned long *) to_page)//lux 已经存在了，异常。
		panic("try_to_share: to_page already exists");
/* share them: write-protect */
	*(unsigned long *) from_page &= ~2;//lux R/W=0 只读或执行，不允许写
	*(unsigned long *) to_page = *(unsigned long *) from_page;//lux 指向同一个物理页（共享）
	invalidate();//lux 更新页表缓存
	phys_addr -= LOW_MEM;
	phys_addr >>= 12;
	mem_map[phys_addr]++;//lux 物理页引用++
	return 1;
}
/**lux 尝试共享一个页面
 * 实现：
 * 1. 在全局进程表内，找到和当前进程executable的m_inode一样的（同一份程序镜像）
 * 2. 调用try_to_share尝试共享指定页面，返回成功失败。
 */
/*
 * share_page() tries to find a process that could share a page with
 * the current one. Address is the address of the wanted page relative
 * to the current data space.
 *
 * We first check if it is at all feasible by checking executable->i_count.
 * It should be >1 if there are other tasks sharing this inode.
 */
static int share_page(unsigned long address)
{
	struct task_struct ** p;

	if (!current->executable)
		return 0;
	if (current->executable->i_count < 2)
		return 0;
	for (p = &LAST_TASK ; p > &FIRST_TASK ; --p) {
		if (!*p)
			continue;
		if (current == *p)
			continue;
		if ((*p)->executable != current->executable)
			continue;
		if (try_to_share(address,*p))
			return 1;
	}
	return 0;
}
/**lux 缺页中断处理函数 由page.s调用
 * case 1: 比如当execute某个程序的时候，不会真正从磁盘加载数据，而是延迟到真正执行的时候，由cpu触发缺页中断，通过缺页程序来加载可执行文件（即此函数）。
 */ 
void do_no_page(unsigned long error_code,unsigned long address)
{
	int nr[4];
	unsigned long tmp;
	unsigned long page;
	int block,i;

	address &= 0xfffff000;//lux 4k页对齐，因为要以页为单位分配。
	tmp = address - current->start_code;
	/**lux 
	 * 如果executable为空，说明不是为了加载代码产生的中断（这种情况，需要分配物理页，并从磁盘加载代码，这个在后面处理）
	 * 如果tmp>=current->end_data 说明需要新的数据存储空间
	 * 以上两种情况，都可以直接分配一个新物理页并返回即可（不需要加载数据）
	 */
	if (!current->executable || tmp >= current->end_data) {
		get_empty_page(address);//lux 分配物理内存到指定逻辑地址。
		return;
	}

	/**lux 下面是executable存在，且tmp<current->end_data的情况
	 * 必须有相应的物理页和数据
	 * 1. 尝试分享
	 * 2. 否则分配物理页，并从磁盘加载数据
	 */
	if (share_page(tmp))//lux 如果能共享则共享（代码段、数据段都可以）
		return;

	//lux 下面只能分配物理页，并加载数据了。
	if (!(page = get_free_page()))
		oom();
/* remember that 1 block is used for header */
	/**lux 
	 * 1. 计算需要加载数据的起始block。比如code段已经加载了一部分了，那么计算当前这个缺页所在的block，继续加载
	 * 2. 连续加载4个block，并读取block数据到page（物理内存地址）
	 * 3. 映射物理内存地址到逻辑地址（address）
	 * 4. address加载完毕（分配物理地址，从磁盘加载数据，绑定到页表）
	 * 实际的场景可能是：
	 * 1. 进程execute一个新程序
	 * 2. fork出进程，并设定页表
	 * 3. 进程开始执行，当从逻辑地址读取代码的时候，页表内P=0，缺页中断
	 * 4. 缺页中断处理程序（本程序）加载指定偏移对应的磁盘数据到内存
	 * 	4.1 输入：一个逻辑地址address
	 * 	4.2 address可以计算出其离start_code的偏移，也就是对应磁盘文件代码段的偏移，从而可以从executable的inode读取磁盘数据，分配物理页，并加载数据到该物理页，更新页表，映射到address
	 * 	4.2.1 新堆栈申请，和共享的情况见下面代码。
	 * 	4.3 如此，完成address缺页的处理
	 * 5. 每次第一次读取一段代码，就是重复3.4.的过程：读取代码，产生缺页中断，加载，继续执行。如此多随着进程的执行，多次之后，该程序的所有代码都会被加载到内存空间。
	 * 	5.1 在内存较小，程序较大的情况下，之前分配的物理页面可以被交换出去。从而实现了小内存执行大程序的功能（按需加载）。“只用一截火车轨道，就可以将火车开到全世界”，如是。
	 */ 
	block = 1 + tmp/BLOCK_SIZE;
	for (i=0 ; i<4 ; block++,i++)
		nr[i] = bmap(current->executable,block);
	bread_page(page,current->executable->i_dev,nr);//lux 读取一页磁盘数据到指定物理内存
	i = tmp + 4096 - current->end_data;//lux 下面是将超出部分清零
	tmp = page + 4096;
	while (i-- > 0) {
		tmp--;
		*(char *)tmp = 0;
	}
	if (put_page(page,address))//lux 更新页表
		return;
	free_page(page);
	oom();
}
/*Lux 初始化可用内存空间*/
void mem_init(long start_mem, long end_mem)
{
	int i;

	HIGH_MEMORY = end_mem;
	for (i=0 ; i<PAGING_PAGES ; i++)
		mem_map[i] = USED;/*lux 所有内存页先标记为占用*/
	i = MAP_NR(start_mem);/*lux 将可用内存页设为0，（可用）*/
	end_mem -= start_mem;
	end_mem >>= 12;
	while (end_mem-->0)
		mem_map[i++]=0;
}

void calc_mem(void)
{
	int i,j,k,free=0;
	long * pg_tbl;

	for(i=0 ; i<PAGING_PAGES ; i++)
		if (!mem_map[i]) free++;
	printk("%d pages free (of %d)\n\r",free,PAGING_PAGES);
	for(i=2 ; i<1024 ; i++) {
		if (1&pg_dir[i]) {
			pg_tbl=(long *) (0xfffff000 & pg_dir[i]);
			for(j=k=0 ; j<1024 ; j++)
				if (pg_tbl[j]&1)
					k++;
			printk("Pg-dir[%d] uses %d pages\n",i,k);
		}
	}
}
