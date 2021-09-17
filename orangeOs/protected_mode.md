# 保护模式

## Intel有哪些模式
The IA-32 architecture supports three operating modes and one quasi-operating mode:
- Protected mode — This is the native operating mode of the processor. It provides a rich set of architectural features, flexibility, high performance and backward compatibility to existing software base.
- Real-address mode — This operating mode provides the programming environment of the Intel 8086 processor, with a few extensions (such as the ability to switch to protected or system management mode).
- System management mode (SMM) — SMM is a standard architectural feature in all IA-32 processors, beginning with the Intel386 SL processor. This mode provides an operating system or executive with a transparent mechanism for implementing power management and OEM differentiation features. SMM is entered through activation of an external system interrupt pin (SMI#), which generates a system management interrupt (SMI).  
    In SMM, the processor switches to a separate address space while saving the context of the currently running program or task. SMM-specific code may then be executed transparently. Upon returning from SMM, the processor is placed back into its state prior to the SMI.
- Virtual-8086 mode — In protected mode, the processor supports a quasi-operating mode known as virtual- 8086 mode. This mode allows the processor execute 8086 software in a protected, multitasking environment(quasi-operating mode，并不算是真的一个，是Protected Mode下的一个特性)

Ref:
- _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 1-Basic Architecture. Section 3.1 Modes of Operation_
- _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Section 2.2 Modes Of Operation_


## Protection
See: Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 5 Protection

- 提供服务，就是要允许被访问
- 提供保护，就是要限制访问

看似是矛盾又复杂的举动，其目的是实现 __可控访问__ 。简单地来讲，就是我先关闭所有入口，再提供凭票消费入口。有我发的粮票，就允许你来使用粮食；有我发的糖票，就允许你来使用糖。

这开关之间，就是权利划分。

特权级是保护模式的基础，赋予了一个事物权利的同时，本质上是默认对其他事物同样权利的剥夺。

### 特权级
Intel通过特权级实现了访问控制，类似上面我们说的。
- 默认关闭所有入口
- 给不同人不同的权限凭证
- 所有的资源访问，凭票使用

如此，就实现了控制访问。

The processor’s segment-protection mechanism recognizes 4 privilege levels, numbered from 0 to 3. The greater numbers mean lesser privileges. Figure 5-3 shows how these levels of privilege can be interpreted as rings of protection.

The center (reserved for the most privileged code, data, and stacks) is used for the segments containing the critical software, usually the kernel of an operating system. Outer rings are used for less critical software. (Systems that use only 2 of the 4 possible privilege levels should use levels 0 and 3.)

The processor uses privilege levels to prevent a program or task operating at a lesser privilege level from accessing a segment with a greater privilege, except under controlled situations. When the processor detects a privilege level violation, it generates a general-protection exception (#GP).
To carry out privilege-level checks between code segments and data segments, the processor recognizes the following three types of privilege levels:

- Current privilege level (CPL) — The CPL is the privilege level of the currently executing program or task. It is stored in bits 0 and 1 of the CS and SS segment registers. Normally, the CPL is equal to the privilege level of the code segment from which instructions are being fetched. The processor changes the CPL when program control is transferred to a code segment with a different privilege level. The CPL is treated slightly differently when accessing conforming code segments. Conforming code segments can be accessed from any privilege level that is equal to or numerically greater (less privileged) than the DPL of the conforming code segment. Also, the CPL is not changed when the processor accesses a conforming code segment that has a different privilege level than the CPL.
- Descriptor privilege level (DPL) — The DPL is the privilege level of a segment or gate. It is stored in the DPL field of the segment or gate descriptor for the segment or gate. When the currently executing code segment attempts to access a segment or gate, the DPL of the segment or gate is compared to the CPL and RPL of the segment or gate selector (as described later in this section). The DPL is interpreted differently, depending on the type of segment or gate being accessed:

    - Data segment — The DPL indicates the numerically highest privilege level that a program or task can have to be allowed to access the segment. For example, if the DPL of a data segment is 1, only programs running at a CPL of 0 or 1 can access the segment.
    - Nonconforming code segment (without using a call gate) — The DPL indicates the privilege level that a program or task must be at to access the segment. For example, if the DPL of a nonconforming code segment is 0, only programs running at a CPL of 0 can access the segment.
    - Call gate — The DPL indicates the numerically highest privilege level that the currently executing program or task can be at and still be able to access the call gate. (This is the same access rule as for a data segment.)
    - Conforming code segment and nonconforming code segment accessed through a call gate — The DPL indicates the numerically lowest privilege level that a program or task can have to be allowed to access the segment. For example, if the DPL of a conforming code segment is 2, programs running at a CPL of 0 or 1 cannot access the segment.
    - TSS — The DPL indicates the numerically highest privilege level that the currently executing program or task can be at and still be able to access the TSS. (This is the same access rule as for a data segment.)
- Requested privilege level (RPL) — The RPL is an override privilege level that is assigned to segment selectors. It is stored in bits 0 and 1 of the segment selector. The processor checks the RPL along with the CPL to determine if access to a segment is allowed. Even if the program or task requesting access to a segment has sufficient privilege to access the segment, access is denied if the RPL is not of sufficient privilege level. That is, if the RPL of a segment selector is numerically greater than the CPL, the RPL overrides the CPL, and vice versa. The RPL can be used to insure that privileged code does not access a segment on behalf of an application program unless the program itself has access privileges for that segment. See Section 5.10.4, “Checking Caller Access Privileges (ARPL Instruction),” for a detailed description of the purpose and typical use of the RPL.

> Privilege levels are checked when the segment selector of a segment descriptor is loaded into a segment register. The checks used for data access differ from those used for transfers of program control among code segments; therefore, the two kinds of accesses are considered separately in the following sections.
如上，特权级检查主要通过`CPL`,`DPL`,`RPL`，并且区分数据段检查和代码段检查。
- 5.6 PRIVILEGE LEVEL CHECKING WHEN ACCESSING DATA SEGMENTS
- 5.7 PRIVILEGE LEVEL CHECKING WHEN LOADING THE SS REGISTER
- 5.8 PRIVILEGE LEVEL CHECKING WHEN TRANSFERRING PROGRAM CONTROL BETWEEN CODE SEGMENTS
    - 5.8.1 Direct Calls or Jumps to Code Segments
    - 5.8.2 Gate Descriptors
    - 5.8.3 Call Gates
    - 5.8.4 Accessing a Code Segment Through a Call Gate
    - 5.8.5 Stack Switching
    - 5.8.6 Returning from a Called Procedure
    - 5.8.7 Performing Fast Calls to System Procedures with the SYSENTER and SYSEXIT Instructions
    - 5.8.8 Fast System Calls in 64-Bit Mode
Ref: _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 5 Protection_

### 可控访问
关闭了所有入口，但是又需要有票用户访问，怎么办呢，开启特殊窗口，凭票验证消费：
> To provide controlled access to code segments with different privilege levels, the processor provides special set of descriptors called gate descriptors. There are four kinds of gate descriptors:
- Call gates
- Trap gates
- Interrupt gates
- Task gates

> Task gates are used for task switching and are discussed in Chapter 7, “Task Management”. Trap and interrupt gates are special kinds of call gates used for calling exception and interrupt handlers. The are described in Chapter 6, “Interrupt and Exception Handling.”


### call gate的特权级转换
Ref: _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter  5.8.5 Stack Switching_

When a procedure call through a call gate results in a change in privilege level, the processor performs the following steps to switch stacks and begin execution of the called procedure at a new privilege level:
1. Uses the DPL of the destination code segment (the new CPL) to select a pointer to the new stack (segment selector and stack pointer) from the TSS.
2. Reads the segment selector and stack pointer for the stack to be switched to from the current TSS. Any limit violations detected while reading the stack-segment selector, stack pointer, or stack-segment descriptor cause an invalid TSS (#TS) exception to be generated.
3. Checks the stack-segment descriptor for the proper privileges and type and generates an invalid TSS (#TS) exception if violations are detected.
4. Temporarily saves the current values of the SS and ESP registers.
5. Loads the segment selector and stack pointer for the new stack in the SS and ESP registers.
6. Pushes the temporarily saved values for the SS and ESP registers (for the calling procedure) onto the new stack (see Figure 5-13).
7. Copies the number of parameter specified in the parameter count field of the call gate from the calling procedure’s stack to the new stack. If the count is 0, no parameters are copied.
8. Pushes the return instruction pointer (the current contents of the CS and EIP registers) onto the new stack.
9. Loads the segment selector for the new code segment and the new instruction pointer from the call gate into the CS and EIP registers, respectively, and begins execution of the called procedure.


## 实模式与保护模式相互转换

### Switching to Protected Mode
Before switching to protected mode from real mode, a minimum set of system data structures and code modules must be loaded into memory, as described in Section 9.8, “Software Initialization for Protected-Mode Operation.” Once these tables are created, software initialization code can switch into protected mode.

Protected mode is entered by executing a MOV CR0 instruction that sets the PE flag in the CR0 register. (In the same instruction, the PG flag in register CR0 can be set to enable paging.) Execution in protected mode begins with a CPL of 0.

Intel 64 and IA-32 processors have slightly different requirements for switching to protected mode. To insure upwards and downwards code compatibility with Intel 64 and IA-32 processors, we recommend that you follow these steps:

1. Disable interrupts. A CLI instruction disables maskable hardware interrupts. NMI interrupts can be disabled with external circuitry. (Software must guarantee that no exceptions or interrupts are generated during the mode switching operation.)
2. Execute the LGDT instruction to load the GDTR register with the base address of the GDT.
3. Execute a MOV CR0 instruction that sets the PE flag (and optionally the PG flag) in control register CR0.
4. Immediately following the MOV CR0 instruction, execute a far JMP or far CALL instruction. (This operation is typically a far jump or call to the next instruction in the instruction stream.)
5. The JMP or CALL instruction immediately after the MOV CR0 instruction changes the flow of execution and serializes the processor.
6. If paging is enabled, the code for the MOV CR0 instruction and the JMP or CALL instruction must come from a page that is identity mapped (that is, the linear address before the jump is the same as the physical address after paging and protected mode is enabled). The target instruction for the JMP or CALL instruction does not need to be identity mapped.
7. If a local descriptor table is going to be used, execute the LLDT instruction to load the segment selector for the LDT in the LDTR register.
8. Execute the LTR instruction to load the task register with a segment selector to the initial protected-mode task or to a writable area of memory that can be used to store TSS information on a task switch.
9. After entering protected mode, the segment registers continue to hold the contents they had in real-address mode. The JMP or CALL instruction in step 4 resets the CS register. Perform one of the following operations to update the contents of the remaining segment registers.
    — Reload segment registers DS, SS, ES, FS, and GS. If the ES, FS, and/or GS registers are not going to be used, load them with a null selector.
    — Perform a JMP or CALL instruction to a new task, which automatically resets the values of the segment registers and branches to a new code segment.
10. Execute the LIDT instruction to load the IDTR register with the address and limit of the protected-mode IDT.
11. Execute the STI instruction to enable maskable hardware interrupts and perform the necessary hardware operation to enable NMI interrupts.

Random failures can occur if other instructions exist between steps 3 and 4 above. Failures will be readily seen in some situations, such as when instructions that reference memory are inserted between steps 3 and 4 while in system management mode.

### Switching Back to Real-Address Mode
The processor switches from protected mode back to real-address mode if software clears the PE bit in the CR0 register with a MOV CR0 instruction. A procedure that re-enters real-address mode should perform the following steps:
1. Disable interrupts. A CLI instruction disables maskable hardware interrupts. NMI interrupts can be disabled with external circuitry.
2. If paging is enabled, perform the following operations:
    — Transfer program control to linear addresses that are identity mapped to physical addresses (that is, linear addresses equal physical addresses).
    — Insure that the GDT and IDT are in identity mapped pages.
    — Clear the PG bit in the CR0 register.
    — Move 0H into the CR3 register to flush the TLB.
3. Transfer program control to a readable segment that has a limit of 64 KBytes (FFFFH). This operation loads the CS register with the segment limit required in real-address mode.
4. Load segment registers SS, DS, ES, FS, and GS with a selector for a descriptor containing the following values, which are appropriate for real-address mode:
    — Limit = 64 KBytes (0FFFFH)
    — Byte granular (G = 0)
    — Expand up (E = 0)
    — Writable (W = 1)
    — Present (P = 1)
    — Base = any value
The segment registers must be loaded with non-null segment selectors or the segment registers will be unusable in real-address mode. Note that if the segment registers are not reloaded, execution continues using the descriptor attributes loaded during protected mode.
5. Execute an LIDT instruction to point to a real-address mode interrupt table that is within the 1-MByte real- address mode address range.
6. Clear the PE flag in the CR0 register to switch to real-address mode.
7. Execute a far JMP instruction to jump to a real-address mode program. This operation flushes the instruction queue and loads the appropriate base-address value in the CS register.
8. Load the SS, DS, ES, FS, and GS registers as needed by the real-address mode code. If any of the registers are not going to be used in real-address mode, write 0s to them.
9. Execute the STI instruction to enable maskable hardware interrupts and perform the necessary hardware operation to enable NMI interrupts.
NOTE: All the code that is executed in steps 1 through 9 must be in a single page and the linear addresses in that page must be identity mapped to physical addresses.

Ref: _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Section 9.9 Model Switching_

## 其他

### 处理器初始化
Ref: _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 9 Processor Management and Initialization_

### 16-bit 32-bit混合代码
Ref: _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 21 Mixing 16-Bit and 32-Bit Code_

### 特权级转换与堆栈
- 当从ring3到ring0时，cpu会从从当前的tss中，获取到ss esp的值并加载（初始化堆栈）。然后把调用方的`ss`,`esp`,`cs`,`eip`压栈。如果有参数，参数也压栈。
- 当从ring0到ring3时（执行iretd的），cpu会自动出栈`ss`,`esp`,`cs`,`eip`，并恢复执行。