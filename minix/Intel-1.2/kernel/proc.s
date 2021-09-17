É_interrupt
†
_interrupt:
ƒ
‚
×ò,#10
Àá32
€
€
Ó_port_out
Š
Š
Í_pc_atö
je I0013
ÍÅ,#-6
ÙI0013
Àá32
€
60
€
Ó_port_out
Š
Š
•
¢
ôë
Àæë

sal äcl
ÀØ,ë
“
ˆ
Àá-1
€
Ó_mini_send
¤
™
je I0017


Àä_busy_map
–
ÀĞ,ë
ÍÅ,#-3
ÙI001A
Ü_lost_ticks
ÌI0018
I001A:


ÀäØ
Á_busy_map
–
‰
or äì
À_busy_map,ë
¢
ôë
sal á1
»
ÀäÎ
À_task_messÇ,ë
ÌI0018
I0017:
ÀäØ
not ë


Á_busy_map
–
‰
Öåë
À_busy_map,ì


Àä_busy_map
–
ÀĞ,ë
I0018:
ÍĞö
je I001D
ÀÆ,#2
I00112:
ÍÆ,#8
jg I001D
ÀæÆ
ÀäĞ
sar äcl
testb al,#1
je I00110
—
sal á1
Àåë
—
ôë
Á_task_messÇ
€
Àá-1
€
Ó_mini_send
¤
¬
ÍÏö
ÙI00110
ÀæÆ

sal äcl
not ë


Á_busy_map
–
‰
Öåë
À_busy_map,ì
I00110:
ÜÆ
š2
I001D:
Í_rdy_headö
je I0011A
Í_cur_procö
jge I00119
Í_cur_proc,#-999
ÙI0011A
I00119:
Ó_pick_proc
I0011A:
…
„

É_sys_call
_sys_call:
ƒ
‚
€
€
Àá86
mul Î
Äá688
»
Äâ_proc
ÀÆ,ì
ÍÔ,#-8
jl I0022
ÍÔ,Ñ
jl I0023
ÍÔ,#116
je I0023
I0022:
§
ÀÇ,#-2
…
„

I0023:
ÍÅ,#3
je I0028
ÍÎ,#2
jl I0028
§
ÀÇ,#-8
…
„

I0028:
test Å,#1
je I002C
Á10(ñ)
ÁÔ
“
Ó_mini_send
¤
¬
ÍÅ,#1
je I002E
ÍÏö
je I002F
I002E:
£
§
°
I002F:
ÍÏö
je I002C
Àsp,ñ
„

I002C:
test Å,#2
je I00216
Á10(ñ)
ÁÔ
“
Ó_mini_rec
¤
¬
£
§
°
I00216:
…
„

É_mini_send
_mini_send:
ƒ
‚
×ò,Ñ
ÍÅ,#2
jl I0033
ÍÎ,#1
je I0033
ÍÎö
je I0033
Àá-1
€
ÌI0031
I0033:
Àá86
mul Å
Äá688
»
Äâ_proc
ÀÆ,ì
Àá86
mul Î
Äá688
»
Äâ_proc
ÀÏ,ì
ÀåÏ
Á30Ç



–
testb al,#1
je I0038
Àá-1
€
ÌI0031
I0038:
§
Àä42Ç
À-1Å,ë
ÀäÔ
ÀØ,ë
Àã4
ÀäØ
shr äcl
À-10(ñ),ë
ÀäØ
Äá24
×á1
shr äcl
À-12(ñ),ë
Àä-10(ñ)
Í-12(ñ),ë
jb I003A
§
Àä-12(ñ)
×ä38Ç
Í-1Å,ë
ja I003B
I003A:
Àá-10
€
ÌI0031
I003B:
ÀåÏ
Á30Ç



–
testb al,#8
je I003F
ÀåÏ
Í80Ç,#116
je I003E
ÀåÏ
¢
Í80Ç,ë
ÙI003F
I003E:
ÀåÏ
ÀèÏ
ÀéÆ
Á78Ç
Á40(ï)
ÁÔ
Á40(ğ)
ˆ
Ó_cp_mess
Äò,#10
ÀåÏ
Äâ30
À-1Î,ì
ÁÇ



–
Öá65527


–
Àå-1Î
°
ÀåÏ
Í30Çö
ÙI00310
¹
Ó_ready
Š
ÌI00310
I003F:
ÍÅ,#-1
ÙI00317
Àá-4
€
ÌI0031
I00317:
§
ÀäÔ
À78Ç,ë
§
Äâ30
À-1Î,ì
ÁÇ



–
or á4


–
Àå-1Î
°
¡
Ó_unready
Š
ÀåÏ
Àä74Ç
ÀĞ,ë
ÍĞö
ÙI0031D
ÀåÏ
—
À74Ç,ë
ÌI0031B
I0031D:
ÀåĞ
Í76Çö
je I0031C
ÀåĞ
Àä76Ç
ÀĞ,ë
ÌI0031D
I0031C:
ÀåĞ
—
À76(bx),ë
I0031B:
§
À76Çö
I00310:
‡
€
I0031:

…
„

_mini_rec:
ƒ
‚
×ò,#10
Àá86
mul Å
Äá688
»
Äâ_proc
ÀÆ,ì
§
Àä74Ç
¬
I0043:
ÍÏö
je I0042
£
×á_proc
Àâ86
cwd
iğv ì
×á8
ÀØ,ë
ÍÎ,#116
je I0045
ÀäØ
ÍÎ,ë
ÙI0046
I0045:
§
ÀèÏ
ÀéÏ
ÁÔ
Á40Ç
Á78(ï)
Á40(ğ)
ÁØ
Ó_cp_mess
Äò,#10
ÀåÏ
Äâ30
À-10(ñ),ì
ÁÇ



–
Öá65531


–
Àå-10(ñ)
°
ÀåÏ
Í30Çö
ÙI004A
¹
Ó_ready
Š
I004A:
§
Àä74Ç
ÍÏ,ë
ÙI004D
ÀåÏ
ÀèÆ
Àä76Ç
À74(ï),ë
ÌI004E
I004D:
ÀåÏ
ÀèĞ
Àä76Ç
À76(ï),ë
I004E:
‡
€
ÌI0041
I0046:
Àä-4(ñ)
ÀĞ,ë
ÀåÏ
Àä76Ç
¬
ÌI0043
I0042:
§
ÀäÎ
À80Ç,ë
§
ÀäÔ
À78Ç,ë
§
Äâ30
À-10(ñ),ì
ÁÇ



–
or á8


–
Àå-10(ñ)
°
¡
Ó_unready
Š
Í_ïg_procsö
jle I00410
ÍÅö
ÙI00410
ÍÎ,#116
ÙI00410
‡
€
Ó_inform
Š
I00410:
‡
€
I0041:

…
„

É_pick_proc
_pick_proc:
ƒ
‚
€
Í_rdy_headö
je I0053
¼
ÌI0054
I0053:
Í_rdy_head+2ö
je I0056
ÀÆ,#1
ÌI0054
I0056:
ÀÆ,#2
I0054:
Àå_cur_proc
À_prev_proc,ì
—
sal á1
»
Í_rdy_headÇö
je I0059
—
sal á1
»
Àä_rdy_headÇ
×á_proc
Àâ86
cwd
iğv ì
×á8
À_cur_proc,ë
—
sal á1
»
Àå_rdy_headÇ
À_proc_ptr,ì
Í_cur_proc,#2
jl I005A
Àå_proc_ptr
À_bill_ptr,ì
ÌI005A
I0059:
À_cur_proc,#-999
À_proc_ptr,#_proc+602
Àå_proc_ptr
À_bill_ptr,ì
I005A:
…
„

É_ready
_ready:
ƒ
‚
€
€
Ó_lock
¢
×á_proc
Àâ86
cwd
iğv ì
×á8
¬
ÍÏö
jge I0063
‡
€
ÌI0064
I0063:
ÍÏ,#2
jge I0066

€
ÌI0064
I0066:
´
€
I0064:
ÃÆ
—
sal á1
»
Í_rdy_headÇö
ÙI0069
—
sal á1
»
¢
À_rdy_headÇ,ë
ÌI006A
I0069:
—
sal á1
»
Àå_rdy_tailÇ
¢
À82Ç,ë
I006A:
—
sal á1
»
¢
À_rdy_tailÇ,ë
’
À82Çö
Ó_restore
…
„

É_unready
_unready:
ƒ
‚
×ò,#6
Ó_lock
¢
×á_proc
Àâ86
cwd
iğv ì
×á8
¬
ÍÏö
jge I0073
‡
€
jmp I0074
I0073:
ÍÏ,#2
jge I0076

€
ÌI0074
I0076:
´
€
I0074:
ÃĞ
ÀäĞ
sal á1
»
Àä_rdy_headÇ
”
ÍÆö
ÙI0079
…
„

I0079:
¢
ÍÆ,ë
ÙI007F
§
ÀäĞ
sal á1
Àèë
Àä82Ç
À_rdy_head(ï),ë
Ó_pick_proc
ÌI007D
I007F:
§
¢
Í82Ç,ë
je I007E
§
Àä82Ç
”
ÍÆö
ÙI007F
…
„

I007E:
§
Àå82Ç
ÀèÆ
Àä82Ç
À82(ï),ë
I00715:
§
Í82Çö
je I00714
§
Àä82Ç
”
ÌI00715
I00714:
ÀäĞ
sal á1
»
—
À_rdy_tailÇ,ë
I007D:
Ó_restore
…
„

É_sched
_sched:
ƒ
‚
Ó_lock
Í_rdy_head+4ö
ÙI0083
Ó_restore
…
„

I0083:
Àå_rdy_tail+4
Àæ_rdy_head+4
À82Ç,í
Àå_rdy_head+4
À_rdy_tail+4,ì
Àå_rdy_head+4
Àå82Ç
À_rdy_head+4,ì
Àå_rdy_tail+4
À82Çö
Ó_pick_proc
Ó_restore
…
„

