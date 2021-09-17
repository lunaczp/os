# TASK MANAGEMENT
Ref: _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 7 Task Management_

Task 是一组任务，是cpu执行的一个上下文。
> A task is a unit of work that a processor can dispatch, execute, and suspend. It can be used to execute a program, a task or process, an operating-system service utility, an interrupt or exception handler, or a kernel or executive utility.

> The IA-32 architecture provides a mechanism for saving the state of a task, for dispatching tasks for execution, and for switching from one task to another. When operating in protected mode, all processor execution takes place from within a task. Even simple systems must define at least one task. More complex systems can use the processor’s task management facilities to support multitasking applications.

IA-32 提供了多TASK管理切换的机制，但是好像实际使用中，并没有得到很好的应用。//todo

## Task Structure
> _A task is made up of two parts: a task execution space and a task-state segment (TSS). The task execution space consists of a code segment, a stack segment, and one or more data segments (see Figure 7-1). If an operating system or executive uses the processor’s privilege-level protection mechanism, the task execution space also provides a separate stack for each privilege level._

> The TSS specifies the segments that make up the task execution space and provides a storage place for task state information. In multitasking systems, the TSS also provides a mechanism for linking tasks.

> A task is identified by the segment selector for its TSS. When a task is loaded into the processor for execution, the segment selector, base address, limit, and segment descriptor attributes for the TSS are loaded into the task register (see Section 2.4.4, “Task Register (TR)”).

> If paging is implemented for the task, the base address of the page directory used by the task is loaded into control register CR3.

## Task-State Segment (tss)
tss内保存了0、1、2 Level的ss和esp，用于在发生特权级切换的时候使用。tss通过`ltr tss_selector`指令来装载。`tss_selector`是tss的选择子（在gdt或ldt中）

> The processor state information needed to restore a task is saved in a system segment called the task-state segment (TSS). Figure 7-2 shows the format of a TSS for tasks designed for 32-bit CPUs. The fields of a TSS are divided into two main categories: dynamic fields and static fields.

The processor updates dynamic fields when a task is suspended during a task switch. The following are dynamic fields:
- General-purpose register fields — State of the EAX, ECX, EDX, EBX, ESP, EBP, ESI, and EDI registers prior to the task switch.
- Segment selector fields — Segment selectors stored in the ES, CS, SS, DS, FS, and GS registers prior to the task switch.
- EFLAGS register field — State of the EFAGS register prior to the task switch.
- EIP (instruction pointer) field — State of the EIP register prior to the task switch.
- Previous task link field — Contains the segment selector for the TSS of the previous task (updated on a task switch that was initiated by a call, interrupt, or exception). This field (which is sometimes called the back link field) permits a task switch back to the previous task by using the IRET instruction.

The processor reads the static fields, but does not normally change them. These fields are set up when a task is created. The following are static fields:
- LDT segment selector field — Contains the segment selector for the task's LDT.
- CR3 control register field — Contains the base physical address of the page directory to be used by the task. Control register CR3 is also known as the page-directory base register (PDBR).
- Privilege level-0, -1, and -2 stack pointer fields — These stack pointers consist of a logical address made up of the segment selector for the stack segment (SS0, SS1, and SS2) and an offset into the stack (ESP0, ESP1, and ESP2). Note that the values in these fields are static for a particular task; whereas, the SS and ESP values will change if stack switching occurs within the task.
- T (debug trap) flag (byte 100, bit 0) — When set, the T flag causes the processor to raise a debug exception when a task switch to this task occurs (see Section 17.3.1.5, “Task-Switch Exception Condition”).
- I/O map base address field — Contains a 16-bit offset from the base of the TSS to the I/O permission bit map and interrupt redirection bitmap. When present, these maps are stored in the TSS at higher addresses. The I/O map base address points to the beginning of the I/O permission bit map and the end of the interrupt redirection bit map. See Chapter 17, “Input/Output,” in the Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 1, for more information about the I/O permission bit map. See Section 20.3, “Interrupt and Exception Handling in Virtual-8086 Mode,” for a detailed description of the interrupt redirection bit map.

If paging is used:
- Avoid placing a page boundary in the part of the TSS that the processor reads during a task switch (the first 104 bytes). The processor may not correctly perform address translations if a boundary occurs in this area. During a task switch, the processor reads and writes into the first 104 bytes of each TSS (using contiguous physical addresses beginning with the physical address of the first byte of the TSS). So, after TSS access begins, if part of the 104 bytes is not physically contiguous, the processor will access incorrect information without generating a page-fault exception.
- Pages corresponding to the previous task’s TSS, the current task’s TSS, and the descriptor table entries for each all should be marked as read/write.
- Task switches are carried out faster if the pages containing these structures are present in memory before the task switch is initiated.

## TSS Descriptor
The TSS, like all other segments, is defined by a segment descriptor. Figure 7-3 shows the format of a TSS descriptor. TSS descriptors may only be placed in the GDT; they cannot be placed in an LDT or the IDT.
