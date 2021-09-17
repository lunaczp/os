# Procedure Calls, Interrupts, and Exceptions
Ref: _Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3-System Programming Guide. Chapter 6 Procedure Calls, Interrupts, and Exceptions_

所有的过程调用，中断，异常，其本质都是指令转移。而指令转移都需要堆栈的支持。

## stack
- stack的生长是向下的。（高地址到低地址）
- 任意时刻，只有一个有效stack

> The stack (see Figure 6-1) is a contiguous array of memory locations. It is contained in a segment and identified by the segment selector in the SS register. When using the flat memory model, the stack can be located anywhere in the linear address space for the program. A stack can be up to 4 GBytes long, the maximum size of a segment.
> Items are placed on the stack using the PUSH instruction and removed from the stack using the POP instruction. When an item is pushed onto the stack, the processor decrements the ESP register, then writes the item at the new top of stack. When an item is popped off the stack, the processor reads the item from the top of stack, then incre- ments the ESP register. In this manner, the stack grows down in memory (towards lesser addresses) when items are pushed on the stack and shrinks up (towards greater addresses) when the items are popped from the stack.
> A program or operating system/executive can set up many stacks. For example, in multitasking systems, each task can be given its own stack. The number of stacks in a system is limited by the maximum number of segments and the available physical memory.
> When a system sets up many stacks, only one stack—the current stack—is available at a time. The current stack is the one contained in the segment referenced by the SS register.

## Procedure Calls

当调用`call`指令，cpu负责保存当前procedure的`eip`，自动压栈。当调用`ret`时，cp负责自动出栈`eip`。
* 如果指明了参数个数，cpu也负责自动压栈，和出栈时的管理
* 如果是`far call`，cpu负责`cs`的压栈和出栈。
* 如果发生了特权级转换，cpu会自动从tss获取新的特权级对应的`ss`,`esp`，然后在`ret`的时候，再重新切换回去。
以上过程，对调用方和被调用方透明。

> The CALL instruction allows control transfers to procedures within the current code segment (near call) and in a different code segment (far call). Near calls usually provide access to local procedures within the currently running program or task. Far calls are usually used to access operating system procedures or procedures in a different task. See “CALL—Call Procedure” in Chapter 3, “Instruction Set Reference, A-M,” of the Intel® 64 and IA-32 Architec- tures Software Developer’s Manual, Volume 2A, for a detailed description of the CALL instruction.
> The RET instruction also allows near and far returns to match the near and far versions of the CALL instruction. In addition, the RET instruction allows a program to increment the stack pointer on a return to release parameters from the stack. The number of bytes released from the stack is determined by an optional argument (n) to the RET instruction. See “RET—Return from Procedure” in Chapter 4, “Instruction Set Reference, N-Z,” of the Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 2B, for a detailed description of the RET instruction.

### Near CALL and RET Operation
When executing a near call, the processor does the following (see Figure 6-2):
1. Pushes the current value of the EIP register on the stack.
2. Loads the offset of the called procedure in the EIP register.
3. Begins execution of the called procedure.

When executing a near return, the processor performs these actions:
1. Pops the top-of-stack value (the return instruction pointer) into the EIP register.
2. If the RET instruction has an optional n argument, increments the stack pointer by the number of bytes specified with the n operand to release parameters from the stack.
3. Resumes execution of the calling procedure.

### Far CALL and RET Operation
When executing a far call, the processor performs these actions (see Figure 6-2):
1. Pushes the current value of the CS register on the stack.
2. Pushes the current value of the EIP register on the stack.
3. Loads the segment selector of the segment that contains the called procedure in the CS register.
4. Loads the offset of the called procedure in the EIP register.
5. Begins execution of the called procedure.

When executing a far return, the processor does the following:
1. Pops the top-of-stack value (the return instruction pointer) into the EIP register.
2. Pops the top-of-stack value (the segment selector for the code segment being returned to) into the CS register.
3. If the RET instruction has an optional n argument, increments the stack pointer by the number of bytes specified with the n operand to release parameters from the stack.
4. Resumes execution of the calling procedure.

```
              Stack During
              Near Call                                                         Stack During
             +           +                                                      Far Call
 Stack       |           |                                                     +           +
 Frame       +-----------+                                         Stack       |           |
 Before  ^   |           |                                         Frame       +-----------+
 Call    |   +-----------+                                         Before  ^   |           |
         |   | Param 1   |                                         Call    |   +-----------+
         |   +-----------+                                                 |   | Param 1   |
         |     Param 2   |                                                 |   +-----------+
         |   +-----------+                                                 |     Param 2   |
         +---+ Param 3   | <----+  ESP Before Call                         |   +-----------+
             +-----------+                                                 +---+ Param 3   | <----+  ESP Before Call
Stack    +---+Calling EIP| <----+  ESP After Call                              +-----------+
Frame    |   +-----------+                                        Stack    +---+Calling CS |
After    |   |           |                                        Frame    |   +-----------+
Call     v   |           |                                        After    |   |Calling EIP| <----+  ESP After Call
             +           +                                        Call     v   +-----------+
                                                                               +           +

              Stack During
              Near Return                                                       Stack During
             +           +                                                      Far Return
             |           |                                                     +           +
             +-----------+                                                     |           |
             |           | <----+  ESP After Return                            +-----------+
             +-----------+                                                     |           | <----+  ESP After Return
             | Param 1   |                                                     +-----------+
             +-----------+                                                     | Param 1   |
               Param 2   |                                                     +-----------+
             +-----------+                                                       Param 2   |
             | Param 3   |                                                     +-----------+
             +-----------+                                                     | Param 3   |
             |Calling EIP| <----+  ESP Before Return                           +-----------+
             +-----------+                                                     |Calling CS |
             |           |                                                     +-----------+
             |           |                                                     |Calling EIP| <----+  ESP Before Return
             +           +                                                     +-----------+
                                                                               +           +


   Note: On a near or far return, parameters are
   released from the stack based on the
   optional n operand in the RET n instruction.

```


## INTERRUPTS AND EXCEPTIONS
中断和异常的处理是一样的，都是通过IDT表。回调函数注册在IDT表内，一旦cpu检测到中断或者异常，就会在IDT内找到对应偏移的回调地址，然后跳转过去执行。

The processor provides two mechanisms for interrupting program execution, interrupts and exceptions:
- An interrupt is an asynchronous event that is typically triggered by an I/O device.
- An exception is a synchronous event that is generated when the processor detects one or more predefined conditions while executing an instruction. The IA-32 architecture specifies three classes of exceptions: faults, traps, and aborts.

The processor responds to interrupts and exceptions in essentially the same way. When an interrupt or exception is signaled, the processor halts execution of the current program or task and switches to a handler procedure that has been written specifically to handle the interrupt or exception condition. The processor accesses the handler procedure through an entry in the interrupt descriptor table (IDT). When the handler has completed handling the interrupt or exception, program control is returned to the interrupted program or task.

IA-32 有18个定义好的中断和异常，以及224个用户定义的中断
> The IA-32 Architecture defines 18 predefined interrupts and exceptions and 224 user defined interrupts, which are associated with entries in the IDT. Each interrupt and exception in the IDT is identified with a number, called a vector. Table 6-1 lists the interrupts and exceptions with entries in the IDT and their respective vectors. Vectors 0 through 8, 10 through 14, and 16 through 19 are the predefined interrupts and exceptions; vectors 32 through 255 are for software-defined interrupts, which are for either software interrupts or maskable hardware inter- rupts.

When the processor detects an interrupt or exception, it does one of the following things:
- Executes an implicit call to a handler procedure.
- Executes an implicit call to a handler task.

### Handler Procedure
A call to an interrupt or exception handler procedure is similar to a procedure call to another protection level (see Section 6.3.6, “CALL and RET Operation Between Privilege Levels”). Here, the vector references one of two kinds of gates in the IDT: an interrupt gate or a trap gate. Interrupt and trap gates are similar to call gates in that they provide the following information:
- Access rights information
- The segment selector for the code segment that contains the handler procedure
- An offset into the code segment to the first instruction of the handler procedure

The difference between an interrupt gate and a trap gate is as follows.
- If an interrupt or exception handler is called through an interrupt gate, the processor clears the interrupt enable (IF) flag in the EFLAGS register to prevent subsequent interrupts from interfering with the execution of the handler.
- When a handler is called through a trap gate, the state of the IF flag is not changed.


> If the code segment for the handler procedure has the same privilege level as the currently executing program or task, the handler procedure uses the current stack.

> If the handler executes at a more privileged level, the processor switches to the stack for the handler’s privilege level.

#### 不涉及特权级切换的情况

If no stack switch occurs, the processor does the following when calling an interrupt or exception handler (see Figure 6-5):
1. Pushes the current contents of the EFLAGS, CS, and EIP registers (in that order) on the stack.
2. Pushes an error code (if appropriate) on the stack.
3. Loads the segment selector for the new code segment and the new instruction pointer (from the interrupt gate or trap gate) into the CS and EIP registers, respectively.
4. If the call is through an interrupt gate, clears the IF flag in the EFLAGS register.
5. Begins execution of the handler procedure.

A return from an interrupt or exception handler is initiated with the IRET instruction. The IRET instruction is similar to the far RET instruction, except that it also restores the contents of the EFLAGS register for the interrupted proce- dure. When executing a return from an interrupt or exception handler from the same privilege level as the interrupted procedure, the processor performs these actions:
1. Restores the CS and EIP registers to their values prior to the interrupt or exception.
2. Restores the EFLAGS register.
3. Increments the stack pointer appropriately.
4. Resumes execution of the interrupted procedure.

#### 发生特权级切换的情况
If a stack switch does occur, the processor does the following:
1. Temporarily saves (internally) the current contents of the SS, ESP, EFLAGS, CS, and EIP registers.
2. Loads the segment selector and stack pointer for the new stack (that is, the stack for the privilege level being called) from the TSS into the SS and ESP registers and switches to the new stack.
3. Pushes the temporarily saved SS, ESP, EFLAGS, CS, and EIP values for the interrupted procedure’s stack onto the new stack.
4. Pushes an error code on the new stack (if appropriate).
5. Loads the segment selector for the new code segment and the new instruction pointer (from the interrupt gate or trap gate) into the CS and EIP registers, respectively.
6. If the call is through an interrupt gate, clears the IF flag in the EFLAGS register.
7. Begins execution of the handler procedure at the new privilege level.


When executing a return from an interrupt or exception handler from a different privilege level than the interrupted procedure, the processor performs these actions:
1. Performs a privilege check.
2. Restores the CS and EIP registers to their values prior to the interrupt or exception.
3. Restores the EFLAGS register.
4. Restores the SS and ESP registers to their values prior to the interrupt or exception, resulting in a stack switch back to the stack of the interrupted procedure.
5. Resumes execution of the interrupted procedure.

### Calls to Interrupt or Exception Handler Tasks
Interrupt and exception handler routines can also be executed in a separate task. Here, an interrupt or exception causes a task switch to a handler task. The handler task is given its own address space and (optionally) can execute at a higher protection level than application programs or tasks.

The switch to the handler task is accomplished with an implicit task call that references a task gate descriptor. The task gate provides access to the address space for the handler task. As part of the task switch, the processor saves complete state information for the interrupted program or task. Upon returning from the handler task, the state of the interrupted program or task is restored and execution continues. See Chapter 6, “Interrupt and Excep- tion Handling,” in the Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 3A, for more infor- mation on handling interrupts and exceptions through handler tasks.

## 其他

### 关于中断
~~每次发生中断，cpu都会调用对应子例程。其实就是就是转移到特定指令位置执行。设想如果硬件中断足够快，
可能会产生一种cpu总是卡在某条指令的情况。因为每次中断，都相当于一次强制重置。而没等执行完，又重新来过。当然这个前提是~~
* ~~允许中断重入~~
* ~~硬件中断足够快。和cpu执行速度相当甚至超过，（那cpu就废掉了，没有意义了)~~

(实际情况是，中断发生时，CPU会自动关闭中断。程序可以选择是否打开)