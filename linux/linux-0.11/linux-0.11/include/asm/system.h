/**
 *lux 
 * 这里的$0x17是堆栈段选择子(segment selector)
 * 0000 0000 0001 0111
 * 3-15位是gdt或ldt内段描述符的偏移，即第1个条目
 * 2位是TI，为1，代表使用ldt。之前在sched.c中已经加载好了task 0的ldt
 * 0-1位是RPL，为3，即特权级是Level 3，用户态
 * 由于之前在sched.c中已经加载好了任务0的tss和ldt，这里iret之后，就是进入Level 3，任务0所设置的context
 * 
 *	"pushl $0x17\n\t" \ //lux 堆栈段选择符入栈，其实就是task0的ldt[1]数据段 level 3
 *	"pushl %%eax\n\t" \ //lux esp入栈
 *	"pushfl\n\t" \ //lux eflags入栈
 *	"pushl $0x0f\n\t" \  //lux task0 代码段选择子入栈 其实就是task0的ldt[0]代码段 level 3
 *	"pushl $1f\n\t" \  //lux 标号1地址入栈。注意$1f是标号1的地址，而不是常数1 ($0x1)
 *	"iret\n" \ //lux iret后，pop出cs:eip,即下面标号1，继续执行
 *	"1:\tmovl $0x17,%%eax\n\t" \ //lux 用数据段选择子初始化各个段
 *	"movw %%ax,%%ds\n\t" \
 *	"movw %%ax,%%es\n\t" \
 *	"movw %%ax,%%fs\n\t" \
 *	"movw %%ax,%%gs" \
 */
#define move_to_user_mode() \
__asm__ ("movl %%esp,%%eax\n\t" \
	"pushl $0x17\n\t" \
	"pushl %%eax\n\t" \
	"pushfl\n\t" \
	"pushl $0x0f\n\t" \ 
	"pushl $1f\n\t" \
	"iret\n" \
	"1:\tmovl $0x17,%%eax\n\t" \
	"movw %%ax,%%ds\n\t" \
	"movw %%ax,%%es\n\t" \
	"movw %%ax,%%fs\n\t" \
	"movw %%ax,%%gs" \
	:::"ax")

#define sti() __asm__ ("sti"::)
#define cli() __asm__ ("cli"::)
#define nop() __asm__ ("nop"::)

#define iret() __asm__ ("iret"::)

#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \
	"movw %0,%%dx\n\t" \
	"movl %%eax,%1\n\t" \
	"movl %%edx,%2" \
	: \
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4+(char *) (gate_addr))), \
	"d" ((char *) (addr)),"a" (0x00080000))

#define set_intr_gate(n,addr) \
	_set_gate(&idt[n],14,0,addr) //lux interrupt gate 14

#define set_trap_gate(n,addr) \
	_set_gate(&idt[n],15,0,addr) //lux trap gate 15 dpl = 0  只限系统使用，比如中断、错误等

#define set_system_gate(n,addr) \
	_set_gate(&idt[n],15,3,addr) //lux trap gate 15 dpl = 3 允许用户调用,system_call 走这个

#define _set_seg_desc(gate_addr,type,dpl,base,limit) {\
	*(gate_addr) = ((base) & 0xff000000) | \
		(((base) & 0x00ff0000)>>16) | \
		((limit) & 0xf0000) | \
		((dpl)<<13) | \
		(0x00408000) | \
		((type)<<8); \
	*((gate_addr)+1) = (((base) & 0x0000ffff)<<16) | \
		((limit) & 0x0ffff); }

#define _set_tssldt_desc(n,addr,type) \
__asm__ ("movw $104,%1\n\t" \
	"movw %%ax,%2\n\t" \
	"rorl $16,%%eax\n\t" \
	"movb %%al,%3\n\t" \
	"movb $" type ",%4\n\t" \
	"movb $0x00,%5\n\t" \
	"movb %%ah,%6\n\t" \
	"rorl $16,%%eax" \
	::"a" (addr), "m" (*(n)), "m" (*(n+2)), "m" (*(n+4)), \
	 "m" (*(n+5)), "m" (*(n+6)), "m" (*(n+7)) \
	)

#define set_tss_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x89")
#define set_ldt_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x82")
