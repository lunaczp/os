; ==========================================
; pmtest1.asm
; 编译方法：nasm pmtest1.asm -o pmtest1.bin
; ==========================================

%include	"pm.inc"	; 常量, 宏, 以及一些说明

xchg bx,bx
;org	07c00h
org	0100h
	jmp	LABEL_BEGIN

[SECTION .gdt]
; GDT
;                              段基址,       段界限     , 属性
LABEL_GDT:	   Descriptor       0,                0, 0           ; 空描述符
LABEL_DESC_CODE32: Descriptor       0, SegCode32Len - 1, DA_C + DA_32; 非一致代码段
LABEL_DESC_VIDEO:  Descriptor 0B8000h,           0ffffh, DA_DRW	     ; 显存首地址
; GDT 结束

GdtLen		equ	$ - LABEL_GDT	; GDT长度
;zhp 如下定义GdtPtr，6字节：GDT界限（2）+ GDT基地址（4）
;基地址后面由程序计算后填充。
;参考：64-ia-32-architectures-software-developer-manual-vol-2-Instruction set reference p.550
GdtPtr		dw	GdtLen - 1	; GDT界限
		dd	0		; GDT基地址

; GDT 选择子
;zhp
;参考：64-ia-32-architectures-software-developer-manual-vol-3-System Programming chp3.4.2 Segment Selectors
SelectorCode32		equ	LABEL_DESC_CODE32	- LABEL_GDT
SelectorVideo		equ	LABEL_DESC_VIDEO	- LABEL_GDT
; END of [SECTION .gdt]

[SECTION .s16]
[BITS	16]
LABEL_BEGIN:
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, 0100h

	; 初始化 32 位代码段描述符
	;zhp
	; 需要真实物理地址，计算方法：段+偏移
	; 参考：王爽 汇编语言 chp2.6
	xor	eax, eax
	mov	ax, cs
	shl	eax, 4
	add	eax, LABEL_SEG_CODE32
	mov	word [LABEL_DESC_CODE32 + 2], ax
	shr	eax, 16
	mov	byte [LABEL_DESC_CODE32 + 4], al
	mov	byte [LABEL_DESC_CODE32 + 7], ah

	; 为加载 GDTR 作准备
	xor	eax, eax
	mov	ax, ds
	shl	eax, 4
	add	eax, LABEL_GDT		; eax <- gdt 基地址
	mov	dword [GdtPtr + 2], eax	; [GdtPtr + 2] <- gdt 基地址

	; 加载 GDTR
	lgdt	[GdtPtr]

	; 关中断
	cli

	; 打开地址线A20
	in	al, 92h
	or	al, 00000010b
	out	92h, al

	; 准备切换到保护模式
	mov	eax, cr0
	or	eax, 1
	mov	cr0, eax

	; 真正进入保护模式
	jmp	dword SelectorCode32:0	; 执行这一句会把 SelectorCode32 装入 cs,
					; 并跳转到 Code32Selector:0  处
; END of [SECTION .s16]


[SECTION .s32]; 32 位代码段. 由实模式跳入.
[BITS	32]

LABEL_SEG_CODE32:
	mov	ax, SelectorVideo
	mov	gs, ax			; 视频段选择子(目的)

  ;清空屏幕
  mov ecx,80*80
  mov edi,0
l0:
  mov al,0x20
  mov [gs:edi], ax
  add edi, 2
  loop l0

  ;计算屏幕行数
  mov ecx,25
  mov edi,0
  xor ebx,ebx

lz:
  cmp ebx, 10;大于10，则需要显示2位。（暂时最大只支持2位）
  jae two
  ;小于10
  mov al, bl
  add al,0x30; ascii 数字开始于 0x30
  mov	ah, 0bh			; 黑底青字
  mov [gs:edi], ax
  add edi, 80*2
  jmp fin
two:
  ;大于10
  push bx
  mov ax, bx
  mov dl, 10
  div dl
  mov bx, ax

  ;显示十位
  add al,0x30; ascii 数字开始于 0x30
  mov	ah, 0bh			; 黑底青字
  mov [gs:edi], ax

  ;显示个位
  add edi, 2
  mov al, bh
  add al,0x30; ascii 数字开始于 0x30
  mov	ah, 0bh			; 黑底青字
  mov [gs:edi], ax
  add edi, 80*2-2
  pop bx

fin:
  inc bx
  loop lz

  ; 到此停止
  jmp $

SegCode32Len	equ	$ - LABEL_SEG_CODE32
; END of [SECTION .s32]

