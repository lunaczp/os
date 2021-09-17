# Basic

## 系统调用宏（System Call Macro）
在2.6.32中，系统调用都是通过宏来定义的，以`poll`为例，
```
//fs/select.c
SYSCALL_DEFINE3(poll, struct pollfd __user *, ufds, unsigned int, nfds,
		long, timeout_msecs)
{
```

不同于以前的直接定义，这里引入了复杂的宏定义，进一步观察：
```
//include/linux/syscalls.h
#define SYSCALL_DEFINE1(name, ...) SYSCALL_DEFINEx(1, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE2(name, ...) SYSCALL_DEFINEx(2, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE3(name, ...) SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE4(name, ...) SYSCALL_DEFINEx(4, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE5(name, ...) SYSCALL_DEFINEx(5, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE6(name, ...) SYSCALL_DEFINEx(6, _##name, __VA_ARGS__)

//include/linux/syscalls.h
#define SYSCALL_DEFINEx(x, sname, ...)				\
	__SYSCALL_DEFINEx(x, sname, __VA_ARGS__)
#endif

//include/linux/syscalls.h
#define __SYSCALL_DEFINEx(x, name, ...)					\
	asmlinkage long sys##name(__SC_DECL##x(__VA_ARGS__))

//include/linux/syscalls.h
#define __SC_DECL1(t1, a1)	t1 a1
#define __SC_DECL2(t2, a2, ...) t2 a2, __SC_DECL1(__VA_ARGS__)
#define __SC_DECL3(t3, a3, ...) t3 a3, __SC_DECL2(__VA_ARGS__)
#define __SC_DECL4(t4, a4, ...) t4 a4, __SC_DECL3(__VA_ARGS__)
#define __SC_DECL5(t5, a5, ...) t5 a5, __SC_DECL4(__VA_ARGS__)
#define __SC_DECL6(t6, a6, ...) t6 a6, __SC_DECL5(__VA_ARGS__)
```

### 宏替换
注意，这里的宏替换，有两个关键点
- 根据参数数量不同，宏不同。比如SYSCALL_DEFINE3，则函数有三个参数
- 分开了形参和是实参(`long timeout_msecs`，`long`为形参，`timeout_msecs`为实参)。之所以这样，是因为有些组件需要这些信息。比如FTRACE
```
//include/linux/syscalls.h
#ifdef CONFIG_FTRACE_SYSCALLS
#define SYSCALL_DEFINEx(x, sname, ...)				\
	static const char *types_##sname[] = {			\
		__SC_STR_TDECL##x(__VA_ARGS__)			\
	};							\
	static const char *args_##sname[] = {			\
		__SC_STR_ADECL##x(__VA_ARGS__)			\
	};							\
	SYSCALL_METADATA(sname, x);				\
	__SYSCALL_DEFINEx(x, sname, __VA_ARGS__)
#else
#define SYSCALL_DEFINEx(x, sname, ...)				\
	__SYSCALL_DEFINEx(x, sname, __VA_ARGS__)
#endif

//include/linux/syscalls.h
#define __SC_STR_TDECL1(t, a)		#t
#define __SC_STR_TDECL2(t, a, ...)	#t, __SC_STR_TDECL1(__VA_ARGS__)
#define __SC_STR_TDECL3(t, a, ...)	#t, __SC_STR_TDECL2(__VA_ARGS__)
#define __SC_STR_TDECL4(t, a, ...)	#t, __SC_STR_TDECL3(__VA_ARGS__)
#define __SC_STR_TDECL5(t, a, ...)	#t, __SC_STR_TDECL4(__VA_ARGS__)
#define __SC_STR_TDECL6(t, a, ...)	#t, __SC_STR_TDECL5(__VA_ARGS__)
```
我们看到，FTRACE
- 定义了 `char *types_poll[]`数组，专门存形参；最后:
```
static const char *types_poll[] = {"struct pollfd *", "unsigned int", "long"]
```
- 定义了`char *args_poll[]`数组，专门存放实参。最后:
```
static const *args_poll[] = {"ufds", "nfds", "timeout_msecs"]}
```

### 设计原因
如上，回到最开始的函数，展开之后，其实就是一个普通的函数定义：
```
asmlinkage long sys_poll(struct pollfd __user *ufds, unsigned int nfds, long timeout_msecs)
```

那么，为什么要引入宏整个复杂的表达式呢，答案是扩展性。使用宏替换，可以在不改变核心代码的情况下，接入不同的代码。还是以`poll`为例：
- 核心定义：
```
//fs/select.c
SYSCALL_DEFINE3(poll, struct pollfd __user *, ufds, unsigned int, nfds,
		long, timeout_msecs)
{
```
- 宏替换，根据不同的编译指令（`define`），可以将宏替换为不同的代码
```
#define SYSCALL_DEFINE3(name, ...) SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)

//是否开启FTRACE
#ifdef CONFIG_FTRACE_SYSCALLS
#define SYSCALL_DEFINEx(x, sname, ...)				\
	static const char *types_##sname[] = {			\
		__SC_STR_TDECL##x(__VA_ARGS__)			\
	};							\
	static const char *args_##sname[] = {			\
		__SC_STR_ADECL##x(__VA_ARGS__)			\
	};							\
	SYSCALL_METADATA(sname, x);				\
	__SYSCALL_DEFINEx(x, sname, __VA_ARGS__)
#else
#define SYSCALL_DEFINEx(x, sname, ...)				\
	__SYSCALL_DEFINEx(x, sname, __VA_ARGS__)
#endif
```

如上，当不开启FTRACE的时候，`SYSCALL_DEFINEx`被替换为简单的`__SYSCALL_DEFINEx`；而如果开启的时候，我们看到有很多属于FTRACE的代码。那么利用宏，我们就可以将这部分代码和业务核心代码隔离开。在不影响核心代码的情况下，提供扩展性。