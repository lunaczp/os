�_interrupt
�
_interrupt:
�
�
��,#10
��32
�
�
�_port_out
�
�
�_pc_at�
je I0013
��,#-6
�I0013
��32
�
�60
�
�_port_out
�
�
�
�
��
���
�
sal �cl
��,�
�
�
��-1
�
�_mini_send
�
�
je I0017
�
�
��_busy_map
�
��,�
��,#-3
�I001A
�_lost_ticks
�I0018
I001A:
�
�
���
�_busy_map
�
�
or ��
�_busy_map,�
�
��
sal �1
�
���
�_task_mess�,�
�I0018
I0017:
���
not �
�
�
�_busy_map
�
�
���
�_busy_map,�
�
�
��_busy_map
�
��,�
I0018:
���
je I001D
��,#2
I00112:
��,#8
jg I001D
���
���
sar �cl
testb al,#1
je I00110
�
sal �1
���
�
��
�_task_mess�
�
��-1
�
�_mini_send
�
�
���
�I00110
���
�
sal �cl
not �
�
�
�_busy_map
�
�
���
�_busy_map,�
I00110:
��
�2
I001D:
�_rdy_head�
je I0011A
�_cur_proc�
jge I00119
�_cur_proc,#-999
�I0011A
I00119:
�_pick_proc
I0011A:
�
�
�
�_sys_call
_sys_call:
�
�
�
�
��86
mul �
��688
�
��_proc
��,�
��,#-8
jl I0022
��,�
jl I0023
��,#116
je I0023
I0022:
�
��,#-2
�
�
�
I0023:
��,#3
je I0028
��,#2
jl I0028
�
��,#-8
�
�
�
I0028:
test �,#1
je I002C
�10(�)
��
�
�_mini_send
�
�
��,#1
je I002E
���
je I002F
I002E:
�
�
�
I002F:
���
je I002C
�sp,�
�
�
I002C:
test �,#2
je I00216
�10(�)
��
�
�_mini_rec
�
�
�
�
�
I00216:
�
�
�
�_mini_send
_mini_send:
�
�
��,�
��,#2
jl I0033
��,#1
je I0033
���
je I0033
��-1
�
�I0031
I0033:
��86
mul �
��688
�
��_proc
��,�
��86
mul �
��688
�
��_proc
��,�
���
�30�
�
�
�
�
testb al,#1
je I0038
��-1
�
�I0031
I0038:
�
��42�
�-1�,�
���
��,�
��4
���
shr �cl
�-10(�),�
���
��24
��1
shr �cl
�-12(�),�
��-10(�)
�-12(�),�
jb I003A
�
��-12(�)
��38�
�-1�,�
ja I003B
I003A:
��-10
�
�I0031
I003B:
���
�30�
�
�
�
�
testb al,#8
je I003F
���
�80�,#116
je I003E
���
�
�80�,�
�I003F
I003E:
���
���
���
�78�
�40(�)
��
�40(�)
�
�_cp_mess
��,#10
���
��30
�-1�,�
��
�
�
�
�
��65527
�
�
�
��-1�
�
���
�30��
�I00310
�
�_ready
�
�I00310
I003F:
��,#-1
�I00317
��-4
�
�I0031
I00317:
�
���
�78�,�
�
��30
�-1�,�
��
�
�
�
�
or �4
�
�
�
��-1�
�
�
�_unready
�
���
��74�
��,�
���
�I0031D
���
�
�74�,�
�I0031B
I0031D:
���
�76��
je I0031C
���
��76�
��,�
�I0031D
I0031C:
���
�
�76(bx),�
I0031B:
�
�76��
I00310:
�
�
I0031:
�
�
�
�
_mini_rec:
�
�
��,#10
��86
mul �
��688
�
��_proc
��,�
�
��74�
�
I0043:
���
je I0042
�
��_proc
��86
cwd
i�v �
��8
��,�
��,#116
je I0045
���
��,�
�I0046
I0045:
�
���
���
��
�40�
�78(�)
�40(�)
��
�_cp_mess
��,#10
���
��30
�-10(�),�
��
�
�
�
�
��65531
�
�
�
��-10(�)
�
���
�30��
�I004A
�
�_ready
�
I004A:
�
��74�
��,�
�I004D
���
���
��76�
�74(�),�
�I004E
I004D:
���
���
��76�
�76(�),�
I004E:
�
�
�I0041
I0046:
��-4(�)
��,�
���
��76�
�
�I0043
I0042:
�
���
�80�,�
�
���
�78�,�
�
��30
�-10(�),�
��
�
�
�
�
or �8
�
�
�
��-10(�)
�
�
�_unready
�
�_�g_procs�
jle I00410
���
�I00410
��,#116
�I00410
�
�
�_inform
�
I00410:
�
�
I0041:
�
�
�
�
�_pick_proc
_pick_proc:
�
�
�
�_rdy_head�
je I0053
�
�I0054
I0053:
�_rdy_head+2�
je I0056
��,#1
�I0054
I0056:
��,#2
I0054:
��_cur_proc
�_prev_proc,�
�
sal �1
�
�_rdy_head��
je I0059
�
sal �1
�
��_rdy_head�
��_proc
��86
cwd
i�v �
��8
�_cur_proc,�
�
sal �1
�
��_rdy_head�
�_proc_ptr,�
�_cur_proc,#2
jl I005A
��_proc_ptr
�_bill_ptr,�
�I005A
I0059:
�_cur_proc,#-999
�_proc_ptr,#_proc+602
��_proc_ptr
�_bill_ptr,�
I005A:
�
�
�
�_ready
_ready:
�
�
�
�
�_lock
�
��_proc
��86
cwd
i�v �
��8
�
���
jge I0063
�
�
�I0064
I0063:
��,#2
jge I0066
�
�
�I0064
I0066:
�
�
I0064:
��
�
sal �1
�
�_rdy_head��
�I0069
�
sal �1
�
�
�_rdy_head�,�
�I006A
I0069:
�
sal �1
�
��_rdy_tail�
�
�82�,�
I006A:
�
sal �1
�
�
�_rdy_tail�,�
�
�82��
�_restore
�
�
�
�_unready
_unready:
�
�
��,#6
�_lock
�
��_proc
��86
cwd
i�v �
��8
�
���
jge I0073
�
�
jmp I0074
I0073:
��,#2
jge I0076
�
�
�I0074
I0076:
�
�
I0074:
��
���
sal �1
�
��_rdy_head�
�
���
�I0079
�
�
�
I0079:
�
��,�
�I007F
�
���
sal �1
���
��82�
�_rdy_head(�),�
�_pick_proc
�I007D
I007F:
�
�
�82�,�
je I007E
�
��82�
�
���
�I007F
�
�
�
I007E:
�
��82�
���
��82�
�82(�),�
I00715:
�
�82��
je I00714
�
��82�
�
�I00715
I00714:
���
sal �1
�
�
�_rdy_tail�,�
I007D:
�_restore
�
�
�
�_sched
_sched:
�
�
�_lock
�_rdy_head+4�
�I0083
�_restore
�
�
�
I0083:
��_rdy_tail+4
��_rdy_head+4
�82�,�
��_rdy_head+4
�_rdy_tail+4,�
��_rdy_head+4
��82�
�_rdy_head+4,�
��_rdy_tail+4
�82��
�_pick_proc
�_restore
�
�
�
