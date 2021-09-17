/**lux execve 系统调用，（内部通过int 80中断，调用sys_execve）
 */
/*
 *  linux/lib/execve.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

_syscall3(int,execve,const char *,file,char **,argv,char **,envp)
