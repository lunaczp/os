/* ptrace.c */
/* By Ross Biro 1/23/92 */
/* edited by Linus Torvalds */
/* mangled further by Bob Manson (manson@santafe.edu) */
/* more mutilation by David Mosberger (davidm@azstarnet.com) */

#include <linux/head.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/ptrace.h>
#include <linux/user.h>
#include <linux/debugreg.h>

#include <asm/segment.h>
#include <asm/pgtable.h>
#include <asm/system.h>

#undef DEBUG

#ifdef DEBUG

  enum {
      DBG_MEM		= (1<<0),
      DBG_BPT		= (1<<1),
      DBG_MEM_ALL	= (1<<2)
  };

  int debug_mask = DBG_BPT;

# define DBG(fac,args)	{if ((fac) & debug_mask) printk args;}

#else
# define DBG(fac,args)
#endif

#define BREAKINST	0x00000080	/* call_pal bpt */

/*
 * does not yet catch signals sent when the child dies.
 * in exit.c or in signal.c.
 */

/*
 * Processes always block with the following stack-layout:
 *
 *  +================================+ --------------------------
 *  | PALcode saved frame (ps, pc,   | ^		      ^
 *  | gp, a0, a1, a2)		     | |		      |
 *  +================================+ | struct pt_regs	      |
 *  |	        		     | |		      |
 *  | frame generated by SAVE_ALL    | |		      |
 *  |	        		     | v		      | P
 *  +================================+			      |	A
 *  |	        		     | ^		      |	G
 *  | frame saved by do_switch_stack | | struct switch_stack  |	E
 *  |	        		     | v		      |	_
 *  +================================+			      |	S
 *  |	        		     |			      |	I
 *  |	        		     |			      |	Z
 *  /	        		     /			      |	E
 *  /	                             /			      |
 *  |				     |			      |
 *  |				     |			      |
 *  |				     |			      v
 *  +================================+ <-------------------------
 *					task->kernel_stack_page
 */
#define PT_REG(reg)	(PAGE_SIZE - sizeof(struct pt_regs)	\
			 + (long)&((struct pt_regs *)0)->reg)
#define SW_REG(reg)	(PAGE_SIZE - sizeof(struct pt_regs)	\
			 - sizeof(struct switch_stack)		\
			 + (long)&((struct switch_stack *)0)->reg)
/* 
 * The following table maps a register index into the stack offset at
 * which the register is saved.  Register indices are 0-31 for integer
 * regs, 32-63 for fp regs, and 64 for the pc.  Notice that sp and
 * zero have no stack-slot and need to be treated specially (see
 * get_reg/put_reg below).
 */
enum {
	REG_R0 = 0, REG_F0 = 32, REG_PC = 64
};

static unsigned short regoff[] = {
	PT_REG(	   r0), PT_REG(	   r1), PT_REG(	   r2), PT_REG(	  r3),
	PT_REG(	   r4), PT_REG(	   r5), PT_REG(	   r6), PT_REG(	  r7),
	PT_REG(	   r8), SW_REG(	   r9), SW_REG(	  r10), SW_REG(	 r11),
	SW_REG(	  r12), SW_REG(	  r13), SW_REG(	  r14), SW_REG(	 r15),
	PT_REG(	  r16), PT_REG(	  r17), PT_REG(	  r18), PT_REG(	 r19),
	PT_REG(	  r20), PT_REG(	  r21), PT_REG(	  r22), PT_REG(	 r23),
	PT_REG(	  r24), PT_REG(	  r25), PT_REG(	  r26), PT_REG(	 r27),
	PT_REG(	  r28), PT_REG(	   gp),		   -1,		   -1,
	SW_REG(fp[ 0]), SW_REG(fp[ 1]), SW_REG(fp[ 2]), SW_REG(fp[ 3]),
	SW_REG(fp[ 4]), SW_REG(fp[ 5]), SW_REG(fp[ 6]), SW_REG(fp[ 7]),
	SW_REG(fp[ 8]), SW_REG(fp[ 9]), SW_REG(fp[10]), SW_REG(fp[11]),
	SW_REG(fp[12]), SW_REG(fp[13]), SW_REG(fp[14]), SW_REG(fp[15]),
	SW_REG(fp[16]), SW_REG(fp[17]), SW_REG(fp[18]), SW_REG(fp[19]),
	SW_REG(fp[20]), SW_REG(fp[21]), SW_REG(fp[22]), SW_REG(fp[23]),
	SW_REG(fp[24]), SW_REG(fp[25]), SW_REG(fp[26]), SW_REG(fp[27]),
	SW_REG(fp[28]), SW_REG(fp[29]), SW_REG(fp[30]), SW_REG(fp[31]),
	PT_REG(	   pc)
};

static long zero;


/* change a pid into a task struct. */
static inline struct task_struct * get_task(int pid)
{
	int i;

	for (i = 1; i < NR_TASKS; i++) {
		if (task[i] != NULL && (task[i]->pid == pid))
			return task[i];
	}
	return NULL;
}

/*
 * Get contents of register REGNO in task TASK.
 */
static inline long get_reg(struct task_struct * task, long regno)
{
	long *addr;

	if (regno == 30) {
		addr = &task->tss.usp;
	} else if (regno == 31) {
		zero = 0;
		addr = &zero;
	} else {
		addr = (long *) (task->kernel_stack_page + regoff[regno]);
	}
	return *addr;
}

/*
 * Write contents of register REGNO in task TASK.
 */
static inline int put_reg(struct task_struct *task, long regno, long data)
{
	long *addr, zero;

	if (regno == 30) {
		addr = &task->tss.usp;
	} else if (regno == 31) {
		addr = &zero;
	} else {
		addr = (long *) (task->kernel_stack_page + regoff[regno]);
	}
	*addr = data;
	return 0;
}

/*
 * This routine gets a long from any process space by following the page
 * tables. NOTE! You should check that the long isn't on a page boundary,
 * and that it is in the task area before calling this: this routine does
 * no checking.
 */
static unsigned long get_long(struct task_struct * tsk,
	struct vm_area_struct * vma, unsigned long addr)
{
	pgd_t * pgdir;
	pmd_t * pgmiddle;
	pte_t * pgtable;
	unsigned long page;

	DBG(DBG_MEM_ALL, ("getting long at 0x%lx\n", addr));
repeat:
	pgdir = pgd_offset(vma->vm_mm, addr);
	if (pgd_none(*pgdir)) {
		do_no_page(tsk, vma, addr, 0);
		goto repeat;
	}
	if (pgd_bad(*pgdir)) {
		printk("ptrace: bad page directory %08lx\n", pgd_val(*pgdir));
		pgd_clear(pgdir);
		return 0;
	}
	pgmiddle = pmd_offset(pgdir, addr);
	if (pmd_none(*pgmiddle)) {
		do_no_page(tsk, vma, addr, 0);
		goto repeat;
	}
	if (pmd_bad(*pgmiddle)) {
		printk("ptrace: bad page middle %08lx\n", pmd_val(*pgmiddle));
		pmd_clear(pgmiddle);
		return 0;
	}
	pgtable = pte_offset(pgmiddle, addr);
	if (!pte_present(*pgtable)) {
		do_no_page(tsk, vma, addr, 0);
		goto repeat;
	}
	page = pte_page(*pgtable);
/* this is a hack for non-kernel-mapped video buffers and similar */
	if (page >= high_memory)
		return 0;
	page += addr & ~PAGE_MASK;
	return *(unsigned long *) page;
}

/*
 * This routine puts a long into any process space by following the page
 * tables. NOTE! You should check that the long isn't on a page boundary,
 * and that it is in the task area before calling this: this routine does
 * no checking.
 *
 * Now keeps R/W state of page so that a text page stays readonly
 * even if a debugger scribbles breakpoints into it.  -M.U-
 */
static void put_long(struct task_struct * tsk, struct vm_area_struct * vma,
	unsigned long addr, unsigned long data)
{
	pgd_t *pgdir;
	pmd_t *pgmiddle;
	pte_t *pgtable;
	unsigned long page;

repeat:
	pgdir = pgd_offset(vma->vm_mm, addr);
	if (!pgd_present(*pgdir)) {
		do_no_page(tsk, vma, addr, 1);
		goto repeat;
	}
	if (pgd_bad(*pgdir)) {
		printk("ptrace: bad page directory %08lx\n", pgd_val(*pgdir));
		pgd_clear(pgdir);
		return;
	}
	pgmiddle = pmd_offset(pgdir, addr);
	if (pmd_none(*pgmiddle)) {
		do_no_page(tsk, vma, addr, 1);
		goto repeat;
	}
	if (pmd_bad(*pgmiddle)) {
		printk("ptrace: bad page middle %08lx\n", pmd_val(*pgmiddle));
		pmd_clear(pgmiddle);
		return;
	}
	pgtable = pte_offset(pgmiddle, addr);
	if (!pte_present(*pgtable)) {
		do_no_page(tsk, vma, addr, 1);
		goto repeat;
	}
	page = pte_page(*pgtable);
	if (!pte_write(*pgtable)) {
		do_wp_page(tsk, vma, addr, 1);
		goto repeat;
	}
/* this is a hack for non-kernel-mapped video buffers and similar */
	if (page < high_memory)
		*(unsigned long *) (page + (addr & ~PAGE_MASK)) = data;
/* we're bypassing pagetables, so we have to set the dirty bit ourselves */
/* this should also re-instate whatever read-only mode there was before */
	set_pte(pgtable, pte_mkdirty(mk_pte(page, vma->vm_page_prot)));
	flush_tlb();
}

static struct vm_area_struct * find_extend_vma(struct task_struct * tsk,
					       unsigned long addr)
{
	struct vm_area_struct * vma;

	addr &= PAGE_MASK;
	vma = find_vma(tsk->mm,addr);
	if (!vma)
		return NULL;
	if (vma->vm_start <= addr)
		return vma;
	if (!(vma->vm_flags & VM_GROWSDOWN))
		return NULL;
	if (vma->vm_end - addr > tsk->rlim[RLIMIT_STACK].rlim_cur)
		return NULL;
	vma->vm_offset -= vma->vm_start - addr;
	vma->vm_start = addr;
	return vma;
}

/*
 * This routine checks the page boundaries, and that the offset is
 * within the task area. It then calls get_long() to read a long.
 */
static int read_long(struct task_struct * tsk, unsigned long addr,
		     unsigned long * result)
{
	struct vm_area_struct * vma = find_extend_vma(tsk, addr);

	DBG(DBG_MEM_ALL, ("in read_long\n"));
	if (!vma) {
	        printk("Unable to find vma for addr 0x%lx\n",addr);
		return -EIO;
	}
	if ((addr & ~PAGE_MASK) > (PAGE_SIZE - sizeof(long))) {
		struct vm_area_struct * vma_high = vma;
		unsigned long low, align;

		if (addr + sizeof(long) >= vma->vm_end) {
			vma_high = vma->vm_next;
			if (!vma_high || vma_high->vm_start != vma->vm_end)
				return -EIO;
		}
		align = addr & (sizeof(long) - 1);
		addr -= align;
		low = get_long(tsk, vma, addr);
		if (align) {
		    unsigned long high;

		    high = get_long(tsk, vma_high, addr + sizeof(long));
		    low >>= align * 8;
		    low  |= high << (64 - align * 8);
		}
		*result = low;
	} else {
	        long l = get_long(tsk, vma, addr);

		DBG(DBG_MEM_ALL, ("value is 0x%lx\n", l));
		*result = l;
	}
	return 0;
}

/*
 * This routine checks the page boundaries, and that the offset is
 * within the task area. It then calls put_long() to write a long.
 */
static int write_long(struct task_struct * tsk, unsigned long addr,
	unsigned long data)
{
	struct vm_area_struct * vma = find_extend_vma(tsk, addr);

	if (!vma)
		return -EIO;
	if ((addr & ~PAGE_MASK) > PAGE_SIZE-sizeof(long)) {
		unsigned long low, high, align;
		struct vm_area_struct * vma_high = vma;

		if (addr + sizeof(long) >= vma->vm_end) {
			vma_high = vma->vm_next;
			if (!vma_high || vma_high->vm_start != vma->vm_end)
				return -EIO;
		}
		align = addr & (sizeof(long) - 1);
		addr -= align;
		low  = get_long(tsk, vma, addr);
		high = get_long(tsk, vma_high, addr + sizeof(long));
		low  &= ~0UL >> (64 - align * 8);
		high &= ~0UL << (align * 8);
		low  |= data << (align * 8);
		high |= data >> (64 - align * 8);
		put_long(tsk, vma, addr, low);
		put_long(tsk, vma_high, addr + sizeof(long), high);
	} else
		put_long(tsk, vma, addr, data);
	return 0;
}

/*
 * Read a 32bit int from address space TSK.
 */
static int read_int(struct task_struct * tsk, unsigned long addr,
		    unsigned int *data)
{
	unsigned long l, align;
	int res;

	align = addr & 0x7;
	addr &= ~0x7;

	res = read_long(tsk, addr, &l);
	if (res < 0)
	  return res;

	if (align == 0) {
		*data = l;
	} else {
		*data = l >> 32;
	}
	return 0;
}

/*
 * Write a 32bit word to address space TSK.
 *
 * For simplicity, do a read-modify-write of the 64bit word that
 * contains the 32bit word that we are about to write.
 */
static int write_int(struct task_struct * tsk, unsigned long addr,
		     unsigned int data)
{
	unsigned long l, align;
	int res;

	align = addr & 0x7;
	addr &= ~0x7;

	res = read_long(tsk, addr, &l);
	if (res < 0)
	  return res;

	if (align == 0) {
		l = (l & 0xffffffff00000000UL) | ((unsigned long) data <<  0);
	} else {
		l = (l & 0x00000000ffffffffUL) | ((unsigned long) data << 32);
	}
	return write_long(tsk, addr, l);
}

/*
 * Set breakpoint.
 */
int ptrace_set_bpt(struct task_struct * child)
{
	int displ, i, res, reg_b, nsaved = 0;
	u32 insn, op_code;
	unsigned long pc;

	pc  = get_reg(child, REG_PC);
	res = read_int(child, pc, &insn);
	if (res < 0)
	  return res;

	op_code = insn >> 26;
	if (op_code >= 0x30) {
		/*
		 * It's a branch: instead of trying to figure out
		 * whether the branch will be taken or not, we'll put
		 * a breakpoint at either location.  This is simpler,
		 * more reliable, and probably not a whole lot slower
		 * than the alternative approach of emulating the
		 * branch (emulation can be tricky for fp branches).
		 */
		displ = ((s32)(insn << 11)) >> 9;
		child->debugreg[nsaved++] = pc + 4;
		if (displ)		/* guard against unoptimized code */
		  child->debugreg[nsaved++] = pc + 4 + displ;
		DBG(DBG_BPT, ("execing branch\n"));
	} else if (op_code == 0x1a) {
		reg_b = (insn >> 16) & 0x1f;
		child->debugreg[nsaved++] = get_reg(child, reg_b);
		DBG(DBG_BPT, ("execing jump\n"));
	} else {
		child->debugreg[nsaved++] = pc + 4;
		DBG(DBG_BPT, ("execing normal insn\n"));
	}

	/* install breakpoints: */
	for (i = 0; i < nsaved; ++i) {
		res = read_int(child, child->debugreg[i], &insn);
		if (res < 0)
		  return res;
		child->debugreg[i + 2] = insn;
		DBG(DBG_BPT, ("    -> next_pc=%lx\n", child->debugreg[i]));
		res = write_int(child, child->debugreg[i], BREAKINST);
		if (res < 0)
		  return res;
	}
	child->debugreg[4] = nsaved;
	return 0;
}

/*
 * Ensure no single-step breakpoint is pending.  Returns non-zero
 * value if child was being single-stepped.
 */
int ptrace_cancel_bpt(struct task_struct * child)
{
	int i, nsaved = child->debugreg[4];

	child->debugreg[4] = 0;

	if (nsaved > 2) {
	    printk("ptrace_cancel_bpt: bogus nsaved: %d!\n", nsaved);
	    nsaved = 2;
	}

	for (i = 0; i < nsaved; ++i) {
		write_int(child, child->debugreg[i], child->debugreg[i + 2]);
	}
	return (nsaved != 0);
}

asmlinkage long sys_ptrace(long request, long pid, long addr, long data,
			   int a4, int a5, struct pt_regs regs)
{
	struct task_struct *child;
	struct user * dummy;

	dummy = NULL;

	DBG(DBG_MEM, ("request=%ld pid=%ld addr=0x%lx data=0x%lx\n",
		      request, pid, addr, data));
	if (request == PTRACE_TRACEME) {
		/* are we already being traced? */
		if (current->flags & PF_PTRACED)
			return -EPERM;
		/* set the ptrace bit in the process flags. */
		current->flags |= PF_PTRACED;
		return 0;
	}
	if (pid == 1)		/* you may not mess with init */
		return -EPERM;
	if (!(child = get_task(pid)))
		return -ESRCH;
	if (request == PTRACE_ATTACH) {
		if (child == current)
			return -EPERM;
		if ((!child->dumpable ||
		     (current->uid != child->euid) ||
		     (current->uid != child->suid) ||
		     (current->uid != child->uid) ||
		     (current->gid != child->egid) ||
		     (current->gid != child->sgid) ||
		     (current->gid != child->gid)) && !suser())
			return -EPERM;
		/* the same process cannot be attached many times */
		if (child->flags & PF_PTRACED)
			return -EPERM;
		child->flags |= PF_PTRACED;
		if (child->p_pptr != current) {
			REMOVE_LINKS(child);
			child->p_pptr = current;
			SET_LINKS(child);
		}
		send_sig(SIGSTOP, child, 1);
		return 0;
	}
	if (!(child->flags & PF_PTRACED)) {
		DBG(DBG_MEM, ("child not traced\n"));
		return -ESRCH;
	}
	if (child->state != TASK_STOPPED) {
		DBG(DBG_MEM, ("child process not stopped\n"));
		if (request != PTRACE_KILL)
			return -ESRCH;
	}
	if (child->p_pptr != current) {
		DBG(DBG_MEM, ("child not parent of this process\n"));
		return -ESRCH;
	}

	switch (request) {
	/* when I and D space are separate, these will need to be fixed. */
		case PTRACE_PEEKTEXT: /* read word at location addr. */
		case PTRACE_PEEKDATA: {
			unsigned long tmp;
			int res;

			res = read_long(child, addr, &tmp);
			DBG(DBG_MEM, ("peek %#lx->%#lx\n", addr, tmp));
			if (res < 0)
				return res;
			regs.r0 = 0;	/* special return: no errors */
			return tmp;
		}

	/* read register number ADDR. */
		case PTRACE_PEEKUSR:
			regs.r0 = 0;	/* special return: no errors */
			DBG(DBG_MEM, ("peek $%ld=%#lx\n", addr, regs.r0));
			return get_reg(child, addr);

	/* when I and D space are separate, this will have to be fixed. */
		case PTRACE_POKETEXT: /* write the word at location addr. */
		case PTRACE_POKEDATA:
			DBG(DBG_MEM, ("poke %#lx<-%#lx\n", addr, data));
			return write_long(child, addr, data);

		case PTRACE_POKEUSR: /* write the specified register */
			DBG(DBG_MEM, ("poke $%ld<-%#lx\n", addr, data));
			return put_reg(child, addr, data);

		case PTRACE_SYSCALL: /* continue and stop at next
					(return from) syscall */
		case PTRACE_CONT: { /* restart after signal. */
			if ((unsigned long) data > NSIG)
				return -EIO;
			if (request == PTRACE_SYSCALL)
				child->flags |= PF_TRACESYS;
			else
				child->flags &= ~PF_TRACESYS;
			child->exit_code = data;
			wake_up_process(child);
        /* make sure single-step breakpoint is gone. */
			ptrace_cancel_bpt(child);
			return data;
		}

/*
 * make the child exit.  Best I can do is send it a sigkill.
 * perhaps it should be put in the status that it wants to
 * exit.
 */
		case PTRACE_KILL: {
			if (child->state != TASK_ZOMBIE) {
				wake_up_process(child);
				child->exit_code = SIGKILL;
			}
        /* make sure single-step breakpoint is gone. */
			ptrace_cancel_bpt(child);
			return 0;
		}

		case PTRACE_SINGLESTEP: {  /* execute single instruction. */
			if ((unsigned long) data > NSIG)
				return -EIO;
			child->debugreg[4] = -1;	/* mark single-stepping */
			child->flags &= ~PF_TRACESYS;
			wake_up_process(child);
			child->exit_code = data;
	/* give it a chance to run. */
			return 0;
		}

		case PTRACE_DETACH: { /* detach a process that was attached. */
			if ((unsigned long) data > NSIG)
				return -EIO;
			child->flags &= ~(PF_PTRACED|PF_TRACESYS);
			wake_up_process(child);
			child->exit_code = data;
			REMOVE_LINKS(child);
			child->p_pptr = child->p_opptr;
			SET_LINKS(child);
        /* make sure single-step breakpoint is gone. */
			ptrace_cancel_bpt(child);
			return 0;
		}

		default:
		  return -EIO;
	  }
}

asmlinkage void syscall_trace(void)
{
	if ((current->flags & (PF_PTRACED|PF_TRACESYS))
			!= (PF_PTRACED|PF_TRACESYS))
		return;
	current->exit_code = SIGTRAP;
	current->state = TASK_STOPPED;
	notify_parent(current);
	schedule();
	/*
	 * this isn't the same as continuing with a signal, but it will do
	 * for normal use.  strace only continues with a signal if the
	 * stopping signal is not SIGTRAP.  -brl
	 */
	if (current->exit_code)
		current->signal |= (1 << (current->exit_code - 1));
	current->exit_code = 0;
}
