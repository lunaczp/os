�
_len:
�255
�-255
�-254
�1023
�-1
�-1
�-1
�1279
_interleave:
�513
�1027
�1541
�2055
�2569
�3083
�3597
�15
_gap:
�42
�27
�42
�42
�35
�35
_rate:
�2
�
�2
�2
�1
�1
_nr_sectors:
�9
�15
�9
�9
�9
�9
_nr_blocks:
�720
�2400
�720
�1440
�720
�1440
_steps_per_cyl:
�1
�1
�2
�1
�2
�1
_mtr_setup:
�15
�45
�15
�15
�45
�_floppy_task
�45
�
_floppy_task:
�
�
��,#6
�
_2:
�I001C
�3
�1
�I001A
�I001B
�
�
��_mess
�
�16
�
�_receive
�
�
�_mess�
jge I0016
�_mess
��_1
�
�_panic
�
�
�
��_mess
��,�
��_mess+6
��,�
�_mess+2
�I0018
I001A:
��_mess
�
�_do_rdwt
�
�
�I0019
I001B:
��_mess
�
�_do_rdwt
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
�_mess+2,#68
���
�_mess+4,�
�
�_mess+6,�
��_mess
�
�
�_send
�
�
�
_do_rdwt:
�
�
��,#12
�
��4�
��,�
���
jl I0022
��,#2
jl I0023
I0022:
��-5
�
�I0021
I0023:
�8
mul �
�
��_floppy
��,�
�
���
�6�,�
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
je I0027
��-22
�
�I0021
I0027:
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
��2400
sbb �0
�1f
���
je 1f
��
1: 
or ��
jl I002A
��-104
�
�I0021
I002A:
�
�al,27�
�
�
�_d,�
��4
��4
��-12(�)
�-10(�)
�
��_d
sal �1
�
�_nr_sectors�
�
�
�
�
��1
sal �cl
���
�
�
�.dvu4
�
�8�,�
��_d
sal �1
��_nr_sectors�
cwd
��_interleave
�
�-10(�)
�-12(�)
�
�
�.rmi4
�
���
�
�
�
�
�10�,�
��4
��4
��-12(�)
�-10(�)
�
��_d
sal �1
�
�_nr_sectors�
�
�
�
�
��1
sal �cl
���
�
�
�.rmu4
��_d
sal �1
���
cwd
i�v _nr_sectors�
�
�12�,�
�
���
��8�
�14(�),�
�
���
��18�
�16(�),�
�
���
��6�
�4(�),�
�
�14�,#1024
je I002D
��-22
push �
�I0021
I002D:
���
I00210:
��,#20
jg I002F
��
��3
���
cwd
i�v �
or ��
�I00213
��_d
��
��6
���
cwd
i�v �
�_d,�
�
�27�,dl
�_need_reset,#1
I00213:
��_d
sal �1
��_nr_blocks�
cwd
��-12(�)
��-10(�)
���
sbb ��
�1f
���
je 1f
��
1: 
or ��
jl I00216
�I00210
I00216:
�_need_reset�
je I00219
�_reset
I00219:
�
�_dma_setup
�
�
�_start_motor
�
�
�_seek
�
�
���
je I0021C
�I00210
I0021C:
�
�_transfer
�
�
���
�I0021F
�I002F
I0021F:
��,#-5
�I00210
I002F:
�_motor_goal,#12
��_stop_motor
�
�80
�
�_clock_mess
�
�
���
�I00225
�
�8��
jle I00225
�_initialized,#1
I00225:
���
�I00229
�024
�
�I0021
I00229:
��-5
�
I0021:
�
�
�
�
_dma_setup:
�
�
��,#22
�
��,#3
�I0033
��70
�
�I0034
I0033:
��74
�
I0034:
�
�
�
�
�
�
��16�
�-1�,�
�14�
�
�
�
�
�-1�,�
�
��86
mul 4�
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
���
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
���
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
push �
��4
�
�_port_out
�
�
��
��4
�
�_port_out
�
�
��
�29
�
�_port_out
�
�
�-10(�)
��5
�
�_port_out
�
�
�-12(�)
��5
�
�_port_out
�
�
�_unlock
�
�
�0
�
�_port_out
�
�
�
�
�
_start_motor:
�
�
��,#6
�_lock
�
��6�
��4
���
�
sal �cl
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
�
or �12
�
or ��
�
�
�
�_motor_goal,�
��_motor_status
test _prev_motor,�
je I0043
��_prev_motor
or _motor_goal,�
I0043:
�
��_motor_status
�
�_motor_goal
�010
�
�_port_out
�
�
��_motor_goal
�_motor_status,�
�
�_prev_motor,�
�_unlock
���
je I0046
�
�
�
I0046:
��_d
sal �1
��_send_mess
�
�_mtr_setup�
�_clock_mess
�
�
��_mess
�
��-3
�
�_receive
�
�
�
�
�
_stop_motor:
�
�
�
�
��_motor_goal
�
��240
�
�
�
��_motor_status
�
��240
�
���
je I0053
�_motor_goal
�010
�
�_port_out
�
�
��_motor_goal
�_motor_status,�
I0053:
�
�
�
_seek:
�
�
�
�
cmpb 26��
�I0063
�
�_recalibrate
�
�
je I0063
��-1
�
�I0061
I0063:
�
���
��8(�)
�2�,�
�I0069
�
�
�I0061
I0069:
�5
�
�_fdc_out
�
�
�
��12�
sal �cl
or �6�
�
�_fdc_out
�
��_d
sal �1
���
��8(�)
mul _steps_per_cyl�
�
�_fdc_out
�
�_need_reset�
je I006C
��-1
�
�I0061
I006C:
��_mess
�
��-1
�
�_receive
�
�
��8
�
�_fdc_out
�
�
�_fdc_results
�
�
�
�al,18�
�
�
�
�
�
��248
��32
je I006F
��,#-1
I006F:
�
�al,19�
�
�
��_d
sal �1
���
�
��8(�)
mul _steps_per_cyl�
�
���
je I00612
��,#-1
I00612:
���
je I00615
�
�_recalibrate
�
�
je I00615
��-1
�
�I0061
I00615:
�
I0061:
�
�
�
�
_transfer:
�
�
��,#10
�
cmpb 26��
�I0073
��-2
�
�I0071
I0073:
�
��6�
��4
���
��_motor_status
sar �cl
testb bl,#1
�I0076
��-2
�
�I0071
I0076:
�_pc_at�
je I0079
��_d
sal �1
�_rate�
�015
�
call _port_out
�
�
I0079:
�
��,#3
�I007C
�30
�
�I007D
I007C:
�97
�
I007D:
�
�
�
�
��,�
��
�_fdc_out
�
�
�
��12�
sal �cl
or �6�
�
�_fdc_out
�
�
�8�
�_fdc_out
�
�
�12�
�_fdc_out
�
�
�10�
�_fdc_out
�
�al,_len+4
�
�
�
�_fdc_out
�
��_d
sal �1
�_nr_sectors�
�_fdc_out
�
��_d
sal �1
�_gap�
�_fdc_out
�
�55
�
�_fdc_out
�
�_need_reset�
je I007F
��-2
�
�I0071
I007F:
��_mess
�
��-1
�
�_receive
�
�
�
�_fdc_results
�
�
���
je I00712
�
�I0071
I00712:
�
�al,19�
�
�
�
�
�
testb al,#5
�I00714
�
�al,20�
�ah,ah
�
�
�
�
testb al,#31
je I00715
I00714:
�
�26��
I00715:
�
�al,19�
�
�
�
�
�
testb al,#2
je I00719
�
�6�
��_5
�
�_printk
�
�
��-5
�
�I0071
I00719:
�
�al,18�
�
�
�
�
�
testb al,#248
je I0071C
��-2
�
�I0071
I0071C:
�
�al,20�
�
�
�
�al,19�
�
�
�
or ��
�
je I0071F
��-2
�
�I0071
I0071F:
�
�al,21�
�
�
��8�
�
�
�
��1
sal �cl
��_d
sal �1
�
�_nr_sectors�
�
�
�
�
�
mul �
�
�
�
�
�
�al,22�
�
�
��12�
��_d
sal �1
mul _nr_sectors�
���
�
�
�al,23�
�
�
��10(bx)
���
�
��512
mul �
�14�,�
je I00722
��-2
�
�I0071
I00722:
�
�
I0071:
�
�
�
�
_fdc_results:
�
�
��,#8
�
I0085:
��,#8
jge I0082
���
���
I0089:
��,#100
jge I0086
���
�
�012
�
�_port_in
�
�
�
�
���
�
testb al,#128
je I0087
��,#1
�I0086
I0087:
��
�I0089
I0086:
���
�I008E
��-3
�
�I0081
I008E:
�
�
���
�
testb al,�
�I00811
�
�
�I0081
I00811:
�
�
���
�
testb al,#64
�I00814
��-3
�
�I0081
I00814:
���
�
�013
�
�_port_in
�
�
�
�
���
�
�
�
�
�
�
���
�18�,al
��
�I0085
I0082:
�_need_reset,#1
��-3
�
I0081:
�
�
�
�
_fdc_out:
�
�
��,#6
�_need_reset�
je I0093
�
�
�
I0093:
��,#100
I0096:
�
��
�
�
jle I0095
���
�
�012
�
�_port_in
�
�
�
�
�
�
��192
�
�
�
�
�
�
�
�
��128
je I0099
�I0096
I0099:
�
�013
�
�_port_out
�
�
�
�
�
I0095:
�_need_reset,#1
�
�
�
_recalibrate:
�
�
�
�
�_start_motor
�
��7
�
�_fdc_out
�
�
�6�
�_fdc_out
�
�_need_reset�
je I00A3
��-1
�
�I00A1
I00A3:
��_mess
�
��-1
�
�_receive
�
�
��8
�
�_fdc_out
�
�
�_fdc_results
�
�
�
�2�,#-1
���
�I00A5
�
�al,18�
�
�
�
�
�
��248
��32
�I00A5
�
cmpb 19��
je I00A6
I00A5:
�_need_reset,#1
�
�26��
��-4
�
�I00A1
I00A6:
�
�26�,#1
�
�
I00A1:
�
�
�
�
_reset:
�
�
��,#8
�_need_reset�
�_lock
�_motor_status�
�_motor_goal�
�
�
�010
�
�_port_out
�
�
�2
�
�010
�
�_port_out
�
�
�_unlock
��_mess
�
��-1
�
�_receive
�
�
��,#_floppy
���
�18��
��8
�
�_fdc_out
�
��
�_fdc_results
�
�
���
�
�al,18�
�
�
�
�
�
��,�
��3
�
�_fdc_out
�
�23
�
�_fdc_out
�
�
�
�_fdc_out
�
mov ��
I00B5:
��,#2
jge I00B2
�8
mul �
�
�_floppy+26��
��
�I00B5
I00B2:
�
�
�
_clock_mess:
�
�
�_mess+2,#1
�_mess+4,#-5
�
cwd
�_mess+10,�
�_mess+10+2,�
�
�_mess+14,�
��_mess
�
��-3
�
�_sendrec
�
�
�
�
�
_send_mess:
�
�
�_mess+2,�
��_mess
�
��-5
�
�_send
�
�
�
�
�
�
_mess: .zerow 24/2
_d: .zerow 2/2
_initialized: .zerow 2/2
_need_reset: .zerow 2/2
_prev_motor: .zerow 2/2
_motor_goal: .zerow 2/2
_motor_status: .zerow 2/2
_floppy: .zerow 56/2
�
_1:
�26980
�27507
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
�32
_3:
�21318
�26400
�30305
�8293
�27750
�28783
�31088
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
�26948
�27507
�29797
�25972
�26912
�8302
�29284
�30313
�8293
�25637
�26912
�8307
�29303
�29801
�8293
�29296
�29807
�25445
�25972
�11876
�10
�
