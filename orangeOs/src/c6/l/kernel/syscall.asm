
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
INT_VECTOR_SYS_CALL equ 0x90

global	get_ticks ; 导出符号

bits 32
[section .text]

;lux
;系统调用:
; int x 触发软中断
; 在idt中找到处理函数(这里对应是sys_call)，并调用
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL ;lux:用软中断实现系统调用
	ret

