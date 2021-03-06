#include <linux/module.h>
#include <linux/smp.h>
#include <linux/user.h>
#include <linux/elfcore.h>

#include <asm/semaphore.h>

extern void dump_thread(struct pt_regs *, struct user *);
extern int dump_fpu(elf_fpregset_t *);

static struct symbol_table arch_symbol_table = {
#include <linux/symtab_begin.h>
	/* platform dependent support */
	X(dump_thread),
	X(dump_fpu),
	XNOVERS(down_failed),
	XNOVERS(down_failed_interruptible),
	XNOVERS(up_wakeup),
#ifdef __SMP__
	X(apic_reg),		/* Needed internally for the I386 inlines */
	X(cpu_data),
	X(syscall_count),
	X(kernel_flag),
	X(kernel_counter),
	X(active_kernel_processor),
	X(smp_invalidate_needed),
#endif
#include <linux/symtab_end.h>
};

void arch_syms_export(void)
{
	register_symtab(&arch_symbol_table);
}
