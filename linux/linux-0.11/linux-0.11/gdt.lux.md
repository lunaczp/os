# Gdt(global descriptor table)
全局描述符表，表项可以是segment、tss、ldt等。

## Linux 0.11下，gdt的使用
- `setup.s`中，在进入保护模式前，定义了两个基本段: cs,ds
- `head.s`中，初始化了32位模式下，系统使用的gdt，共256个预留表项，包含了内核使用的CS、DS:   
[boot/head.s](boot/head.s)
```
_gdt:	.quad 0x0000000000000000	/* NULL descriptor */
	.quad 0x00c09a0000000fff	/* 16Mb */ /*lux 内核代码段*/
	.quad 0x00c0920000000fff	/* 16Mb */ /*lux 内核数据段*/
	.quad 0x0000000000000000	/* TEMPORARY - don't use */
	.fill 252,8,0			/* space for LDT's and TSS's etc */
```

- 进入`kernel main`函数，进行调度器初始化(`sched_init`)的时候，会初始化一遍gdt  
[kernel/sched.c](kernel/sched.c)
```
	set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));/*lux 设定任务0的tss和ldt*/
	set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));
	p = gdt+2+FIRST_TSS_ENTRY;
	for(i=1;i<NR_TASKS;i++) {/*Lux 初始化task列表，并填充gdt（初始化所有task条目在gdt内的ldt和tss，注意当前其实没有那么多task，只不过先在gdt中占位，清理）*/
		task[i] = NULL;/*i=0为INIT TASK，保持不变*/
		p->a=p->b=0;
		p++;
		p->a=p->b=0;
		p++;
	}
```
`NR_TASKS=64`，所以共填充了128个选项。

- 其他进程在fork的时候，会将自己的ldt和tss初始化后填入gdt相应的位置。  

ldt初始化 [kernel/fork.c](kernel/fork.c)
```
	set_base(p->ldt[1],new_code_base);//lux 设定新进程的code base
	set_base(p->ldt[2],new_data_base);//lux 设定新进程的data base
```

填充gdt [kernel/fork.c](kernel/fork.c)
```
	set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));//lux 设定gdt内的该进程的tss表项
	set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));//lux 设定gdt内的该进程的ldt表项
```

最终gdt内的效果是
```
|NULL   |Kernel CS  |Kernel DS  |TEMP   |LDT(0) |TSS(0) |LDT(1) |TSS(1) |...    |LDT(63)    |TSS(63)
```
