�
_w_need_reset:
�_winchester_task
�
�
_winchester_task:
�
�
��,#6
�
_2:
�I001C
�3
�1
�I001B
�I001B
�
�_init_param
�
��_w_mess
�
�16
�
�_receive
�
�
�_w_mess�
jge I0016
�_w_mess
��_1
�
�_printk
�
�
�
�
��_w_mess
��,�
��_w_mess+6
��,�
�_w_mess+2
�I0018
I001B:
��_w_mess
�
�_w_do_rdwt
�
�
�I0019
I001C:
��,#-22
�I0019
I0018:
��#_2
�
�.csa2
I0019:
�_w_mess+2,#68
���
�_w_mess+4,�
�
�_w_mess+6,�
��_w_mess
�
�
�_send
�
�
�
_w_do_rdwt:
�
�
��,#14
���
�
��4�
��,�
���
jl I0022
��,#10
jl I0023
I0022:
��-5
�
�I0021
I0023:
�
�8�,#1024
je I0027
��-22
�
�I0021
I0027:
��30
mul �
�
��_wini
��,�
��5
���
cwd
i�v �
�
�4�,�
�
��_nr_drives
�4�,�
jl I002A
��-5
�
�I0021
I002A:
�
��2�
�
�
�
�12�
�10�
�
�
�024
�
�.rmi4
��#0
sbb �0
�1f
or ��
1: 
or ��
je I002D
��-22
�
�I0021
I002D:
�
�12�
�10�
�
�
��512
�
�.dvi4
�-12(�),�
�-10(�),�
��-12(�)
��-10(�)
��2
adc �0
���
��18(�)
sbb �20(�)
�1f
���
je 1f
��
1: 
or ��
jle I00210
��-104
�
�I0021
I00210:
�
��-12(�)
��-10(�)
��14�
adc �16�
�-12(�),�
�-10(�),�
��4
��4
��-12(�)
�-10(�)
�
�
�
�12�
�
�
�
�
�5
mul �
���
�
�
�.dvu4
�
�6�,�
��4
��4
��-12(�)
�-10(�)
�
�
�
�
�5
�
�.rmu4
�
�8�,�
��4
��4
��-12(�)
�-10(�)
�
�
�
�12�
�
�
�
�
�5
mul �
���
�
�
�.rmu4
�
�
�
�
�5
�
�.dvu4
�
�10�,�
�
���
��8�
�22(�),�
�
���
��18�
�24(�),�
�
���
��6�
�2(�),�
I00213:
��,#4
jg I00212
��
��,#4
jl I00216
��-5
�
�I0021
I00216:
�_w_need_reset�
je I00219
�_w_reset
I00219:
�
�_w_dma_setup
�
�
�_w_transfer
�
�
���
�I00213
I00212:
���
�I0021F
�024
�
�I0021
I0021F:
��-5
�
I0021:
�
�
�
�
_w_dma_setup:
�
�
��,#22
�
��,#3
�I0033
��71
�
�I0034
I0033:
��75
�
I0034:
�
�
�
�
�
�
��24�
�-1�,�
�22�
�
�
�
�
�-1�,�
�
��86
mul 2�
��688
�
�-1�
�-1�
�
�
��_proc
�
�_umap
��,#8
�-22(�),�
�-20(�),�
�
�
��-22(�)
�
�
�
�
�
�
��8
��-22(�)
��-20(�)
�
2: sar �1
rcr �1
�
1:
�
�
�
�
�
�
�
��,�
��16
��-22(�)
��-20(�)
�
2: sar �1
rcr �1
�
1:
�
�
�
�
�
�
�
��,�
��-1�
��1
�
�
�
�
�
�
�
�
�
�.cuu
�-10(�),�
��-1�
��1
��8
shr �cl
�
�
�
�
�
�
�
�
�
�
�-12(�),�
��-22(�)
��-20(�)
��0
sbb �0
�1f
or ��
1: 
or ��
�I0036
�
�
��-1�
�
�
��_3
�
�_panic
�
�
I0036:
�
�
��4
��4
��-22(�)
�-1�
�-20(�)
�
�
��
��
���
adc ��
��1
sbb �#0
�
��
�6
�
��
�
�
�
2: shr �1
rcr �1
�
1:
�
��0
�-1�,�
���
�-1�,�
je I0039
��
��_4
�
�_panic
�
�
I0039:
�_lock
�
�2
�
�_port_out
�
�
�
�1
�
�_port_out
�
�
�
��6
�
�_port_out
�
�
��
��6
�
�_port_out
�
�
��
mov �130
�
�_port_out
�
�
�-10(�)
��7
�
�_port_out
�
�
�-12(�)
��7
�
�_port_out
�
�
�_unlock
�
�
�
_w_transfer:
�
�
�
��,#3
�I0043
��8
�
�I0044
I0043:
�0
�
I0044:
�
�
�
�
�_command,�
�
��5
��4�
sal �cl
or �10�
�_command+2,�
�6�
�
�
�
�
��768
�
shr �cl
�
�
�8�
�
�
�
�
or ��
�
�
�
�_command+4,�
�
�6�
�
�
�
�
�
�
�
�
�_command+6,�
�_command+8,#2
�_command+10,#5
��3
�
�_com_out
�
�
je I0046
��-1
�
�I0041
I0046:
��3
�
�0
�
�_port_out
�
�
��_w_mess
�
��-1
�
call _receive
�
�
�
�_win_results
�
�
�I0049
�
�
�I0041
I0049:
�
�al,26�
�
�
��63
��24
�I004C
�_read_ecc
�I004D
I004C:
�_w_need_reset,#1
I004D:
��-1
�
I0041:
�
�
�
�
_win_results:
�
�
�
�
���
�
��800
�
�_port_in
�
�
�
�
��803
�
�_port_out
�
�
test �,#2
�I0053
�
�
�I0051
I0053:
�_command,#3
�
��5
��4�
sal �cl
�_command+2,�
�
�
�_com_out
�
�
je I0056
��-1
�
�I0051
I0056:
�
I005B:
��,#4
jge I0058
�
�
�_hd_wait
�
�
je I005D
��-1
�
�I0051
I005D:
���
�
��800
�
�_port_in
�
�
�
�
�
�
�
�
�
�
mov ��
���
�26�,al
��
�I005B
I0058:
�
�al,26�
�
�
testb al,#63
je I00510
��-1
�
�I0051
I00510:
�
�
I0051:
�
�
�
�
_win_out:
�
�
�_w_need_reset�
je I0063
�
�
�
I0063:
�
�
�_hd_wait
�
�
�I0066
�
��800
�
�_port_out
�
�
I0066:
�
�
�
_w_reset:
�
�
�
�
��,#1
�
��801
�
�_port_out
�
�
���
I0075:
��,#10000
jge I0072
���
�
��801
�
�_port_in
�
�
�
�
�
�
testb al,#1
�I0073
�I0072
I0073:
��
�I0075
I0072:
test �,#2
je I007A
��_5
�
�_printk
�
��-1
�
�I0071
I007A:
�_w_need_reset�
�_win_init
�
I0071:
�
�
�
�
_win_init:
�
�
push �
�_command,#12
�_command+2�
�
�
�_com_out
�
�
je I0083
��-1
�
�I0081
I0083:
�_lock
��8
��_param0
sar �cl
�
�_win_out
�
�
�
��_param0
�
�
�
�_win_out
�
�_param0+2
�_win_out
�
��8
��_param0+4
sar �cl
�
�_win_out
�
�
�
��_param0+4
�
�
�
�_win_out
�
��8
��_param0+6
sar �cl
�
�_win_out
�
�
�
��_param0+6
�
�
�
�_win_out
�
�_param0+8
�_win_out
�
�_unlock
�_check_init
�
je I0086
�_w_need_reset,#1
��-1
�
�I0081
I0086:
�_nr_drives,#1
jle I0089
�_command+2,#32
�
�
�_com_out
�
�
je I008C
��-1
�
�I0081
I008C:
�_lock
��8
��_param1
sar �cl
�
�_win_out
�
�
�
��_param1
�
�
�
�_win_out
�
�_param1+2
�_win_out
�
��8
��_param1+4
sar �cl
�
�_win_out
�
�
�
��_param1+4
�
�
�
�_win_out
�
��8
��_param1+6
sar �cl
�
�_win_out
�
�
�
��_param1+6
�
�
�
�_win_out
�
�_param1+8
�_win_out
�
�_unlock
�_check_init
�
je I0089
�_w_need_reset,#1
��-1
�
�I0081
I0089:
�
I00814:
��_nr_drives
��,�
jge I00811
�_command,#1
��5
�
sal �cl
�_command+2,�
�_command+10,#5
�
�
�_com_out
�
�
je I00816
��-1
�
�I0081
I00816:
��_w_mess
�
��-1
�
�_receive
�
�
�_win_results
�
je I00812
�_w_need_reset,#1
��-1
�
�I0081
I00812:
��
�I00814
I00811:
�
�
I0081:
�
�
�
�
_check_init:
�
�
�
�
�
�_hd_wait
�
�
�I0093
���
�
��800
�
�_port_in
�
�
test �,#2
je I0096
��-1
�
�I0091
I0096:
�
�
�I0091
I0093:
�
�
�
I0091:
�
�
�
�
_read_ecc:
�
�
�
�_command,#13
�
�
�_com_out
�
�
�I00A3
�
�
�_hd_wait
�
�
�I00A3
���
�
��800
�
�_port_in
�
�
�
�
�_hd_wait
�
�
�I00A3
���
�
��800
�
�_port_in
�
�
test �,#1
je I00A3
�_w_need_reset,#1
I00A3:
��-1
�
�
�
_hd_wait:
�
�
��,#6
�
I00B4:
���
�
��801
�
�_port_in
�
�
�
��,�
�
��
�
��10000
jge I00B2
���
je I00B4
I00B2:
��,#10000
jl I00B7
�_w_need_reset,#1
��-1
�
�I00B1
I00B7:
�
�
I00B1:
�
�
�
�
_com_out:
�
�
�
�
�
�
��803
�
�_port_out
�
�
�
��802
�
�_port_out
�
�
�
I00C5:
��,#300
jge I00C2
���
�
��801
�
�_port_in
�
�
test �,#8
je I00C3
�I00C2
I00C3:
��
�I00C5
I00C2:
��,#300
�I00CA
�_w_need_reset,#1
��-1
�
�I00C1
I00CA:
�_lock
�
I00CF:
��,#6
jge I00CC
I00C13:
���
�
��801
�
�_port_in
�
�
test �,#1
je I00C15
�I00C10
I00C15:
test �,#8
�I00C13
�_w_need_reset,#1
�_unlock
��-1
�
�I00C1
I00C10:
�
�
�
�
��14
��12
je I00C1B
�_w_need_reset,#1
�_unlock
��-1
�
�I00C1
I00C1B:
���
sal �1
�
�_command�
��800
�
�_port_out
�
�
��
�I00CF
I00CC:
�_unlock
�
�
I00C1:
�
�
�
�
_init_params:
�
�
��,#14
���
�
��802
�
�_port_in
�
�
�
�
shr �cl
��3
�
�
��,�
�
��3
�
�
�
�-10(�),�
�
�
��_vec_table+260
�
��,�
�
�
��_vec_table+262
�
�
�
��4
���
�
2: sal �1
rcl �1
�
1:
�
�
��4
��4
�
�
���
��
���
adc ��
��
��4
��4
�
�-1�,�
�-12(�)
�
�
��64
�
�
��_buf
�
�
�
��_proc+172
�
�_umap
��,#8
�
�
�-12(�)
�-1�
�_phys_copy
��,#12
��,#4
���
sal �cl
�
��_param0
�
��_buf
�
�_copy_param
�
�
��4
��-10(�)
sal �cl
�
��_param1
�
��_buf
�
�_copy_param
�
�
�
�
�
�
�
��_buf
�
�
�
��_proc+172
�
�_umap
��,#8
�
�
�
�
�141
�
�_phys_copy
��,#12
�
�
�al,_buf
�
�
�_nr_drives,�
�
I00D5:
��,#5
jae I00D2
��30
mul �
�
��_param0+2
�_wini+12�,�
�
��1
�
�I00D5
I00D2:
�_wini+164�
�_wini+164+2�
��_wini+164
�_wini+164+2
�_wini+14,�
�_wini+14+2
��_param0+2
cwd
�
�
��_param0
cwd
���
���
�
�
�.mli4
��#25
���
�
���
�.mli4
�_wini+18,�
�_wini+18+2,�
��,#5
I00D9:
��,#10
jae I00D6
mov �30
mul �
�
��_param1+2
�_wini+12�,�
�
��1
�
�I00D9
I00D6:
��_param1+2
cwd
�
�
��_param1
cwd
���
���
�
�
�.mli4
��#25
���
�
���
�.mli4
�_wini+168,�
�_wini+168+2,�
�_nr_drives�
jle I00DB
�_win_init
�
je I00DB
�_nr_drives�
I00DB:
�
I00D11:
�
�
��_nr_drives
�
�
�
���
jae I00DE
��5
mul �
�
�
�
�_w_mess+4,�
�_w_mess+10�
�_w_mess+10+2�
�_w_mess+8,#1024
�_w_mess+18,#_buf
�_w_mess+6,#-6
�_w_mess+2,#3
��_w_mess
�
�_w_do_rdwt
�
��1024
je I00D13
�
��_6
�
�_panic
�
�
I00D13:
��5
mul �
�
�_copy_prt
�
�
��1
�
�I00D11
I00DE:
�
�
�
_copy_params:
�
�
�
�
�
mov �,�
�
�
�al,2�
�
�
�
�
�
�
�2�,�
�
���
��3�
�4(�),�
�
���
��5�
�6(�),�
�
�
�al,7�
�
�
�
�
�
�
�8�,�
�
�
�
_copy_prt:
�
�
��,#10
�
I00F5:
��,#4
jge I00F2
�-10(�)�
���
�
���
��30
mul �
��30
�
��_wini
��,�
�
�
�
�
��4
sal �cl
��454
�
�
�
�
���
���
��_buf�
��_buf+2�
�14(�),�
�16(�),�
���
�16�
�14�
�
�
�
�
�.rmi4
��#0
sbb �0
�1f
or ��
1: 
or ��
je I00F7
���
��14�
�16�
�-10(�),�
��
�16�
�14�
�
�
�
�
�.dvi4
��1
adc �0
��#2
���
�
���
�.mli4
���
�14�,�
�16�,�
���
��14�
��16�
��-10(�)
sbb ��
�-10(�),�
��,�
I00F7:
�
��4
�
��_buf�
��_buf+2�
��-10(�)
sbb ��
���
�18�,�
�20�,�
��
�I00F5
I00F2:
�
��
��30
mul �
�
��_wini
�
�_sort
�
�
�
�
�_sort
_sort:
�
�
�
�
�
I0105:
��,#4
jge I0102
���
I0109:
��,#3
jge I0103
��30
mul �
�
���
��14�
��16�
��0
sbb �0
�1f
or ��
1: 
or ��
�I010B
�
��
��30
mul �
�
���
��14�
��16�
��0
sbb �0
�1f
or ��
1: 
or ��
je I010B
�
��
��30
mul �
�
���
��30
mul �
���
���
�
��
�_swap
�
�
�I0107
I010B:
��30
mul �
�
���
�
��
��30
mul �
���
���
��14�
��16�
��14(�)
sbb �16(�)
�1f
���
je 1f
��
1: 
or ��
jle I0107
�
��
��30
mul �
�
���
��14�
��16�
��0
sbb �0
�1f
or ��
1: 
or ��
je I0107
�
��
��30
mul �
�
���
��30
mul �
���
���
�
��
�_swap
�
�
I0107:
��
�I0109
I0103:
��
�I0105
I0102:
�
�
�
�_swap
_swap:
�
�
��,#30
���
��30
�.loi
��-30(�)
��15
���
rep
mov
��,�
���
��30
�.loi
���
��15
���
rep
mov
��,�
��-30(�)
��30
�.loi
���
��15
���
rep
mov
��,�
�
�
�
�
_param1: .zerow 10/2
_param0: .zerow 10/2
_buf: .zerow 1024/2
_command: .zerow 12/2
_w_mess: .zerow 24/2
_nr_drives: .zerow 2/2
_wini: .zerow 300/2
�
_1:
�26999
�25454
�25960
�29811
�29285
�29728
�29537
�8299
�28519
�8308
�25965
�29555
�26465
�8293
�29286
�28015
�9504
�8292
�
_3:
�21318
�26400
�30305
�8293
�26999
�25454
�25960
�29811
�29285
�25632
�29545
�8299
�29284
�30313
�29285
�25120
�25697
�24864
�00
�114
_4:
�29268
�27001
�26478
�29728
�8303
�19780
�8257
�25441
�28530
�29555
�13856
�19252
�25120
�30063
�10
�29281
�121
_5:
�24904
�14
�25632
�29545
�8299
�28535
�10094
�8308
�25970
�25971
�2676
�
_6:
�24899
�10094
�8308
�25970
.word 25697
�28704
�29281
�26996
�26996
�28271
�29728
�25185
�25964
�28448
�8294
�26999
�25454
�25960
�29811
�29285
�32
�