org 07c00h
mov ax, cs
mov ds, ax
mov es, ax
call Disp
jmp $
Disp:
  mov ax, msg
  mov bp, ax
  mov cx, 16
  mov ax, 01300h
  mov dl, 0
  int 10h
  ret
msg: db "Hello, OS world!"
times 510-($-$$) db 0
dw 0xaa55
