�
_caps_off:
�1
_num_off:
�1
_unsh:
�6912
�12849
�13363
�13877
�14391
�12345
�15661
�2312
�30577
�29285
�31092
�26997
�28783
�23899
�-32243
�29537
�26212
�26727
�27498
�15212
�24615
�23680
�30842
�30307
�28258
�11373
�12078
�10881
�8323
�-24188
�-23646
�-23132
�-22618
�-22104
�-31318
�-18552
�-17992
�-19319
�-18763
�-20084
�-19534
�32560
_sh:
�6912
�16417
�9251
�24101
�10790
�10536
�11103
�2312
�22353
�21061
�22868
�18773
�20559
�32123
�-32243
�21313
�17988
�18503
�19274
�14924
�32290
�31872
�22618
�22083
�20034
�15437
�16190
�10881
�8323
�-28284
�-27758
�-27244
�-26730
�-26216
�-31590
�14219
�14648
�13449
�13877
�12684
�13106
�11824
_unm24:
�6912
�12849
�13363
�13877
�14391
�12345
�24109
�2312
�30577
�29285
�31092
�26997
�28783
�23360
�-32243
�29537
�26212
�26727
�27498
�15212
�23866
�23680
�30842
�30307
�28258
�11373
�12078
�10881
�8323
�-24188
�-23646
�-23132
�-22618
�-22104
�5034
�-18552
�-17992
�-19319
�-18763
�-20084
�-19534
�11824
�3104
�3466
�-19788
�-18250
�-29926
�12064
�-21333
�-20819
�-29265
�-28786
_m24:
�6912
�8737
�9251
�9765
�10279
�24361
�32317
�2312
�22353
�21061
�22868
�18773
�20559
�31584
�-32243
�21313
�17988
�18503
�19274
�11084
�32042
�31872
�22618
�22083
�20034
�15437
�16190
.word 10881
�8323
�-28284
�-27758
�-27244
�-26730
�-26216
�-18278
�14099
�14648
�13449
�13877
�12684
�13106
�32647
�3257
�3514
�2568
�7692
�-17638
�12220
�-25445
�-24931
�-16993
�_tty_task
�-16450
�
_tty_task:
�
�
��,#26
�
_1:
�I001D
�
�5
�I001B
�I0017
�I001D
�I0018
�I0019
�I001A
�
�_tty_init
�
��-2�
�
�16
�
�_receive
�
�
��898
mul -20(�)
�
��_tty_struct
�-2�,�
�-22(�)
�I0015
I0017:
��-2�
�
�_do_charint
�
�
I0018:
��-2�
�
�-2�
�_do_read
�
�
�
I0019:
��-2�
�
�-2�
�_do_write
�
�
�
I001A:
��-2�
�
�-2�
�_do_ioctl
�
�
�
I001B:
��-2�
�
�-2�
�_do_cancel
�
�
�
I001D:
�
�
�
�
�
��-22
�
�-1�
�-2�
��68
�
�_tty_reply
�
�
I0015:
��#_1
�
�.csa2
_do_charint:
�
�
��,#20
�_lock
�
��18�
�-12(�),�
�-1�,#_tty_copy_buf
��-12(�)
�
�
�
�
�
��,�
�
��,�
�-12(�),#2
I0023:
�
��
�
�
jle I0022
�-12(�)
�-12(�),#1
�
��-1�
�
�(�),al
�-1�,#1
�I0023
I0022:
�
��18�
�-12(�),�
��-12(�)
���
�_unlock
�-1�,#_tty_copy_buf
I0026:
��
��
�
�
jle I0025
�-1�
�-1�,#1
�
�
�-15(�),al
�-1�
�-1�,#1
�
�
�
�
�
�al,-15(�)
�
�
�
�
�_in_char
�
pop �
��898
mul �
�
��_tty_struct
�-1�,�
��-1�
�882��
jle I0026
��-1�
�860�
�
�
�
�
��34
�
�
�
�
��-1�
�206��
jg I002B
���
je I0026
��-1�
�204��
jle I0026
I002B:
�-1�
�_rd_chars
�
�
��-1�
�al,878�
�
�
��,�
�al,879�
�
�
�-10(�),�
�
�
�
�
�
�
�-10(�)
��
��67
�
�_tty_reply
�
�I0026
I0025:
�
�
�
_in_char:
�
�
��,#10
��898
mul �
�
��_tty_struct
��,�
�al,�
�
�
��59
jl I0033
�al,�
�
�
��68
jg I0033
�al,�
�
�
�
�_func_key
�
�
�
�
I0033:
�
�204�,#200
jl I0037
�
�
�
I0037:
mov ��
�860�
�
�
�
�
��34
�
�
�
�
�
cmpb 869��
je I003A
�al,�
�
�
�
�_make_break
�
��,al
�I003B
I003A:
�
�
�
�
��32
je I003B
�
�
�al,�
�
�
��127
�
�
�
��,al
I003B:
cmpb ��
�I00310
�
�
�
I00310:
���
je I00312
�
�
�
�
��2
�I00313
I00312:
���
�I00317
�al,�
�
�
�
�
�al,871�
�
�
�
���
�I0031A
�
cmpb 867��
�I0031A
�
�_chuck
�
��-1
je I0031E
��8
�
�
�_echo
�
�
��32
�
�
�_echo
�
�
��8
�
�
�_echo
�
�
I0031E:
�
�
�
I0031A:
�al,�
�
�
�
�
�al,872�
�
�
�
���
�I00321
�
cmpb 867��
�I00321
I00325:
�
�_chuck
�
�
�I00324
�I00325
I00324:
�
�al,872�
�
�
�
�
�_echo
�
�
�0
�
�
�_echo
�
�
�
�
�
I00321:
�
cmpb 867��
�I00328
cmpb �,#92
�I0032B
�
�867�,#1
�al,�
�
�
�
�
�_echo
�
�
�
�
�
I0032B:
�al,�
�
�
�
�
�al,877�
�
�
�
���
�I00317
���
�I00317
I00328:
�
�867��
�al,�
�
�
�
�
�al,871�
�
�
�
���
je I00317
�al,�
�
�
�
�
�al,872�
�
�
�
���
je I00317
�al,�
�
�
�
�
�al,877�
�
�
�
���
je I00317
�
��200
�-10(�),�
��-10(�)
��92
�
��
�(�),#1
�
�
�
�
��204
�-10(�),�
�
��
�
�
���
��#200
�200�,�
�I00317
�
�
�200�,�
I00317:
cmpb �,#13
�I00339
�
�860�
�
�
�
�
testb al,�
je I00339
��,#10
I00339:
�al,�
�
�
�
�
�al,873�
�
�
�
���
je I0033C
�al,�
�
�
�
�
�al,874�
�
�
�
���
�I0033D
I0033C:
�al,�
�
�
�
�
�al,873�
�
�
�
���
�I00341
�
�
�I00342
I00341:
��3
�
I00342:
��
��
�
�
�_�gchar
�
�
�
�
I0033D:
�al,6(�)
�
�
�
�
�al,876�
�
�
�
���
�I00344
�
�868�,#1
�
�
�
I00344:
�al,�
�
�
�
�
�al,875�
�
�
�
���
�I00313
�
�868��
�
��858�
�
�(�)
�
�
�
�
I00313:
cmpb �,#10
je I00349
cmpb ��
�I0034A
I00349:
�
��206
�-10(�),�
�
��
�
I0034A:
�
��200
�-10(�),�
��-10(�)
�
�al,�
�
��
�(�),#1
�
�
�
�
���
��#200
�200�,�
�I0034E
�
�
�200�,�
I0034E:
�
��204
�-10(�),�
�
��
�
�al,�
�
�
�
�
�_echo
�
�
�
�
�
_make_break:
�
�
��,�
�
_2:
�I00444
�
.word 5
�I00445
�I00446
�I00447
�I00448
�I00449
�I0044E
�
�al,�
�
�
�
�
�
��127
�
�
�
�
�al,�
�
�
�
�
�
testb al,#128
je I0043
�
�
�I0044
I0043:
�
�
I0044:
��
�_olivetti�
�I0046
�_shift1�
�I0048
�_shift2�
je I0049
I0048:
�
�al,_sh�
�
�
�
�I004A
I0049:
�
�al,_unsh�
�
�
�
I004A:
��
�_control�
je I004D
��,#14
jge I004D
�
�al,_sh�
�
�
��,�
I004D:
��,#70
jle I0047
�_numlock�
je I0047
�_shift1�
�I00414
�_shift2�
je I00415
I00414:
�
�al,_unsh�
�
�
�
�I00416
I00415:
�
�al,_sh�
�
�
�
I00416:
��
�I0047
I0046:
�_shift1�
�I00418
�_shift2�
je I00419
I00418:
�
�al,_m24�
�
�
�
�I0041A
I00419:
�
�al,_unm24�
�
�
�
I0041A:
��
�_control�
je I0041D
��,#14
jge I0041D
�
�al,_sh�
�
�
��,�
I0041D:
��,#70
jle I0047
�_numlock�
je I0047
�_shift1�
�I00424
�_shift2�
je I00425
I00424:
�
�al,_unm24�
�
�
�
�I00426
I00425:
�
�al,_m24�
�
�
�
I00426:
��
I0047:
�
�
���
�
�
�
�
�
��,�
�
�
���
�
��128
jb I00428
�
�
���
�
��134
jb I00429
I00428:
�_capslock�
je I0042D
��,#65
jl I00430
��,#90
jg I00430
��,#32
�I0042D
I00430:
��,#97
jl I0042D
��,#122
jg I0042D
���
��32
��,�
I0042D:
�_alt�
je I00438
�
�bx,#2
���
�
or �128
�
�
�
��,�
I00438:
�_control�
je I0043B
�
�
���
�
��31
�
�
�
��,�
I0043B:
���
�I0043E
��,#144
I0043E:
���
�I00441
���
I00441:
��
�I0041
I00429:
�
�
���
�
��128
�
�I00443
I00445:
���
�_shift1,�
�I00444
I00446:
���
�_shift2,�
�I00444
I00447:
���
�_control,�
�I00444
I00448:
���
�_alt,�
�I00444
I00449:
���
je I0044B
�_caps_off�
je I0044B
�
��_capslock
�_capslock,�
I0044B:
�
���
�_caps_off,�
�I00444
I0044E:
���
je I00450
�_num_off�
je I00450
�
��_numlock
�_numlock,�
I00450:
�
���
�_num_off,�
�I00444
I00443:
��#_2
�
�.csa2
I00444:
�
�
I0041:
�
�
�
�
_echo:
�
�
�
�860�
�
�
�
�
testb al,#8
�I0053
�
�
�
I0053:
cmpb ��
je I0056
�al,�
�
�
�
�
�_out_char
�
�
I0056:
�
�_flush
�
�
�
�
_chuck:
�
�
�
�
�
�204��
�I0063
��-1
�
�I0061
I0063:
�
�
�200�,�
je I0066
�
��200�
��
�
�I0067
I0066:
�
��199
�
I0067:
��
�
cmpb �,#10
je I0068
�
cmpb �,#13
�I0069
I0068:
��-1
�
�I0061
I0069:
�
�
�200�,�
�
��204
��,�
�
��
�
�
�
I0061:
�
�
�
�
_do_read:
�
�
�
�
�
�882��
jle I0073
�
���
�
�
�
�
��
��-3
�
�6�
�(�)
��68
�
�_tty_reply
�
�
�
�
I0073:
�
���
�
�878(�),al
�
���
��6�
�879(�),al
�
���
��18�
�880(�),�
�
���
��8�
�882(�),�
�
�_rd_chars
�
�
�
�al,879�
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
�
��
��68
�
�_tty_reply
�
�
�
�
_rd_chars:
�
�
��,#38
�
�860�
�
�
�
�
testb al,#34
je I0083
�
�
�I0084
I0083:
�
�
I0084:
��
�
�204��
je I0085
���
je I0086
�
�206��
�I0086
I0085:
��-998
�
�I0081
I0086:
�
�al,879�
�
�
��86
mul �
��688
�
add �_proc
�-32(�),�
�
��880�
�-1�,�
�882�
�
�
�
�
�-1�,�
�-1�
�-1�
�
�
�-32(�)
�_umap
��,#8
�-22(�),�
�-20(�),�
��-22(�)
��-20(�)
��0
sbb �0
�1f
or ��
1: 
or ��
�I008B
��-10
�
�I0081
I008B:
�56
�
��_tty_buf
�
�
�
��_proc+86
�
�_umap
��,#8
�-2�,�
�-2�,�
�-10(�)�
�-12(�)�
�-1��
I008E:
�
�882��
jle I008D
�
���
��204(�)
�882�,�
jge I00811
�
�882�
�I00812
I00811:
�
�204�
I00812:
��
��,#256
jge I00814
��
�I00815
I00814:
�56
�
I00815:
��
���
�-30(�),#_tty_buf
I00817:
��
��
�
�
jle I00816
�
��202
�-3�,�
��-3�
��
�(�),#1
pop �
�
�-27(�),al
�
���
��#200
�202�,�
�I0081A
�
�
�202�,�
I0081A:
��-30(�)
�al,-27(�)
�
�-30(�),#1
��
cmpb -27(�),#10
je I0081C
cmpb -27(�)�
�I00817
I0081C:
�
��206
�-3�,�
�
��
�
���
je I00821
cmpb -27(�)�
�I00821
�-1�
I00821:
�-12(�)
���
je I00817
I00816:
�-1��
je I00828
�
��
�
�I00829
I00828:
�
I00829:
��
���
cwd
�
�
�-20(�)
�-22(�)
�-2�
�-2�
�_phys_copy
��,#12
���
cwd
��-22(�)
adc �-20(�)
�-22(�),�
�-20(�),�
���
�-10(�),�
�
��882
�-3�,�
�
���
�
�
��204
�-3�,�
�
���
�
�
�204��
je I008D
�-12(�)�
je I008E
I008D:
��4(bp)
�882��
�-10(�)
I0081:
�
�
�
�
_finish:
�
�
�
�
�
�848��
�
�892��
�
cmpb 870��
�I0093
�
�
�
I0093:
�
�al,884�
�
�
�
�al,885�
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
�
��68
�
�_tty_reply
�
�
�870��
�
�
�
_do_write:
�
�
��,#6
�
���
�
�884(�),al
�
���
��6�
�885(�),al
�
���
��18�
�886(�),�
�
���
��8�
�892(�),�
�
�870�,#1
�
�894��
�
�al,885�
�
�
��86
mul �
��688
�
��_proc
��,�
�
��886�
�
�892�
�
�
�
call �
�
�
�
�
�
��
�_umap
��,#8
�
�888�,�
�890�,�
�
��888�
��890�
��0
sbb �0
�1f
or ��
1: 
or ��
�I00A3
�
�894�,#-10
�
�892��
I00A3:
�
��858�
�
�(�)
�
�
�
�
_do_ioctl:
�
�
��,#38
�
_3:
�I00B8
�4
�29704
�I00B6
�29705
�I00B4
�29713
�I00B5
�29714
�I00B7
�
�-3��
���
�
���
���
�
�8�
�I00B2
I00B4:
�
��8
��10�
��12�
�
2: sar �#1
rcr �1
�
1:
��4
��4
�
�
�
�
��0
�
�871�,al
�
���
��10�
��12�
�
2: sar �#1
rcr �1
�
1:
��4
��4
�
�
�
�
��0
mov ��
�872�,al
�
���
��14�
�860(�),�
�I00B3
I00B5:
�
�4
��10�
��12�
�
2: sar �#1
rcr �1
�
1:
��4
��4
�
�
�
�
��0
�
�873�,al
�
��16
��10�
��12�
�
2: sar �#1
rcr �1
�
1:
��4
��4
�
�
�
�
��0
�
�874�,al
�
��8
��10�
��12�
�
2: sar �#1
rcr �1
�
1:
��4
��4
�
�
�
�
��0
�
�875�,al
�
���
��10�
��12�
�
2: sar �#1
rcr �1
�
1:
��4
��4
�
�
�
�
��0
�
�876�,al
�
��8
��14�
��16�
�
2: sar �#1
rcr �1
�
1:
��4
��4
�
�
�
�
��0
��4(�)
�877�,al
�I00B3
I00B6:
�
�al,871�
�
�
cwd
��4
��4
�
�
�
�
��0
�
��4
��4
�
�-12(�),�
�-10(�)
�
�al,872�
�
�
cwd
��4
��4
�
�
�
�
��0
�
��4
��4
�
�-1�,�
�-1�
��8
��-12(�)
��-10(�)
�
2: sal �1
rcl �1
�
1:
or �-1�
or �-1�
��,�
��,�
�
��860�
cwd
�
��,�
�I00B3
I00B7:
�
�al,873�
�
�
cwd
��4
��4
�
�
�
�
��0
�
��4
��4
�
�-20(�),�
�-1�
�
�al,874�
�
�
cwd
��4
��4
�
�
�
�
��0
�
��4
��4
�
�-2�,�
�-22(�)
�
�al,875�
�
�
cwd
mov �4
��4
�
�
�
�
��0
�
��4
��4
�
�-2�,�
�-2�
�
�al,876�
�
�
cwd
��4
��4
�
�
�
�
��0
�
��4
��4
�
�-32(�),�
�-30(�)
�
�al,877�
�
�
cwd
��4
��4
�
�
�
�
��0
�
��4
��4
�
�-3�,�
�-3�
��16
��-2�
��-22(�)
�
2: sal �1
rcl �1
�
1:
�4
��-20(�)
��-1�
�
2: sal �#1
rcl �#1
�
1:
or ��
or ��
��8
��-2�
��-2�
�
2: sal �1
rcl �1
�
1:
or ��
or ��
���
��-32(�)
��-30(�)
�
2: sal �#1
rcl �#1
�
1:
or ��
or ��
��,�
��,�
��8
��-3�
��-3�
�
2: sal �1
rcl �1
�
1:
�
��,�
�I00B3
I00B8:
�-3�,#-22
�I00B3
I00B2:
��#_3
��
�.csb2
I00B3:
�
���
��
��
�
�
�-3�
�6�
�(�)
��68
�
�_tty_reply
�
�
�
�
_do_cancel:
�
�
�
�882��
�I00C3
�
�892��
�I00C3
�
�
�
I00C3:
�
�
�200�,�
�
�
�202�,�
�
�204��
�
�206��
�
�882��
�
�892��
�
�870��
�
�868��
�
���
�
�
�
�
�
��-4
�
�6�
�(�)
��68
�
�_tty_reply
�
�
�
�
_tty_reply:
�
�
��,#24
�
�-22(�),�
���
�-20(�),�
��10(�)
�-1�,�
��12(�)
�-10(�),�
��1�
��,�
��1�
�-1�,ax
��1�
�-12(�),�
��-2�
�
�
�_send
�
�
�
�
�
_�gchar:
�
�
�
�868��
��-4
�
�
�_finish
�
�
�
�
�200�,�
�
�
�202�,�
�
�204��
�
�206��
���
��3
��
�
�_cause_�g
�
�
�
�
�
�_keyboard
_keyboard:
�
�
��,#10
���
�
��96
�
�_port_in
�
�
���
�
��97
�
�_port_in
�
�
�
�
�
�
or �128
�
��97
�
�_port_out
�
�
�
��97
�
�_port_out
�
�
�
�
�
�
��128
�
�
�
��,�
���
jle I00F3
��,#29
je I00F4
��,#42
je I00F4
��,#54
je I00F4
cmp �,#56
je I00F4
��,#58
je I00F4
��,#69
je I00F4
��32
�
�
�_port_out
�
�
�
�
�
I00F3:
�
�
��_tty_struct+860
�
��32
�
�
�
��,�
�cl,_tty_struct+876
�-9(�),cl
���
�I00F4
�_control�
je I00F4
��,#31
�I00F4
cmpb -9(�),#19
�I00F4
�_tty_struct+868,#1
��32
�
�
�_port_out
�
�
�
�
�
I00F4:
�_control�
je I00F14
�_alt�
je I00F14
��,#83
�I00F14
�_reboot
I00F14:
�al,_tty_driver_buf
�
�
��,�
�al,_tty_driver_buf+1
�
�
��,�
jge I00F19
���
��,�
���
��2
�
�
�_tty_driver_buf�,al
���
��3
�
�_tty_driver_buf��
�al,_tty_driver_buf
�
�
��
�_tty_driver_buf,al
�_keybd_mess+2,#1
�_keybd_mess+18,#_tty_driver_buf
��#_keybd_mess
�
��-7
�
�_interrupt
�
�
�I00F1A
I00F19:
��32
�
�
�_port_out
�
�
I00F1A:
�
�
�
_console:
�
�
��,#12
�
��4
��888�
��890�
�
2: sar �#1
rcr �1
�
1:
��4
��4
�
�
�
��65535
��0
��,�
�
�890�
�888�
��4
��4
�
�
�
��15
��0
��,�
���
�-10(�),�
�
I0103:
�
�892��
jle I0102
�
cmpb 868��
�I0102
��
��
�_get_byte
�
�
�-3(�),al
�al,-3(�)
�
�
�
�
�_out_char
�
�
���
��1
��,�
�
��892
�-12(�),�
�
��
�
�I0103
I0102:
�
�_flush
�
���
��-10(�)
�
�
�
�
��4(bp)
��888
�-12(�),�
��-12(�)
�
cwd
���
adc �2�
�
�2�,�
�
��894
�-12(�),�
�
���
�
�
�892��
�I0107
�
�894�
�
�_finish
�
�
I0107:
�
�
�
_out_char:
�
�
�
�
_4:
�I0111F
�9
�7
�I011A
�8
�I011E
�9
�I01117
�10
�I011F
�11
�I011B
�12
�I011C
�13
�I01116
�14
�I011D
�27
�I0111E
�
�
cmpb 854�,#1
�I0113
�
�al,�
�855�,al
�
�854�,#2
�
�
�
I0113:
�
cmpb 854�,#2
�I0116
�al,�
�
�
�
�
�al,855�
�
�
�
�
�_escape
�
�
�854��
�
�
�
I0116:
�al,�
�
�
�
�I0118
I011A:
�
�_flush
pop �
�331
�
�_beep
�
�
�
�
I011B:
�
��864�
��
�
�862�
�
�_move_to
�
�
�
�
I011C:
�
���
��862(�)
��
�864�
�
�
�_move_to
�
�
�
�
I011D:
�
���
��862(�)
��
�864�
�
�
�_move_to
�
�
�
�
I011E:
�
���
��862(�)
��
�864�
�
�
�_move_to
�
�
�
�
I011F:
�
�860�
�
�
�
�
testb al,�
je I01111
�3
�
�
�_out_char
�
�
I01111:
�
�864��
�I01114
�
�
�
�_scroll_screen
�
�
�I01115
I01114:
�
��864
��,�
�
��
�
I01115:
�
���
�864�
�862(�)
�
�_move_to
��,#6
�
�
�
I01116:
�
�864�
�
�
�
�_move_to
�
�
�
�
I01117:
�
�860�
�
�
�
�
��3072
��3072
�I01119
I0111D:
��32
�
�
�_out_char
�
�
�
�862�
�
�
�
�
testb al,#7
�I0111D
�
�
�
I01119:
�
�
�
I0111E:
�
�_flush
�
�
�854�,#1
�
�
�
I0111F:
�
�862�,#80
jl I01121
�
�
�
I01121:
�
�848�,#320
�I01124
�
�_flush
�
I01124:
�al,�
�
�
�
or �856�
���
��#848
��,�
���
��(�)
��
�
��208
�
�(�)
�(�),�
�
sal �1
�
���
��
�
��862
��,�
�
��
�
�
�
�
I0118:
��#_4
��
jmp .csb2
_scroll_screen:
�
�
�
�
���
�I0123
�60
�
�I0124
I0123:
��-160
�
I0124:
��
�
��850�
���
��_vid_mask
�850�,�
���
�I0126
�
��850�
��3840
��_vid_mask
�
�I0127
I0126:
�
��850�
�
I0127:
��80
�
�
�_vid_base
�
�
�_vid_copy
��,#8
�
��850�
sar �1
�
�2
�
�_set_6845
�
�
�
�
�
_flush:
�
�
�
�848��
�I0133
�
�
�
I0133:
�
���
���
�848�
�852(�)
�_vid_base
��#208
��
�_vid_copy
��,#8
�
��848�
sal �1
��852�
�852�,�
�
��852�
sar �1
�
�4
�
�_set_6845
�
�
�
�848��
�
�bp
�
_move_to:
�
�
�
�_flush
�
���
jl I0142
��,#80
jge I0142
���
jl I0142
��,#25
jl I0143
I0142:
�
�
�
I0143:
�
���
�862�,�
�
���
�864�,�
�4
���
sal �1
��80
mul �
�
��850�
�
sal �1
���
���
�852(�),�
�
��852�
sar �1
�
�4
�
�_set_6845
�
�
�
�
�
_escape:
�
�
��,#10
cmpb �,#122
�I0153
�al,�
�
�
��8
sal �cl
�
�856�,�
�
�
�
I0153:
cmpb �,#126
�I0156
cmpb �,#48
�I0159
�
�60
mul 864�
��160
��862�
sal �1
���
��
��,�
�
��852�
��,�
I015C:
���
jle I015A
��_vid_�race
��,�
jge I015F
�
�I01510
I015F:
�_vid_�race
I01510:
��
�
�
cwd
i�v �
�
��
�_vid_base
�
�
�_vid_copy
��,#8
�
��,�
�
���
�
�I015C
I0159:
cmpb �,#49
�I015A
�
�
�
�_scroll_screen
�
�
I015A:
�
�
�
I0156:
�al,�
�
�
��32
�
�al,�
�
�
��32
�
�
�_move_to
�
�
�
�
_set_6845:
�
�
��4
��_vid_port
�
�
�_port_out
�
�
��8
���
sar �cl
�
�
�
�
��5
��_vid_port
�
�
�_port_out
�
�
�
��
��4
��_vid_port
�
�
�_port_out
�
�
�
�
���
�
�
��5
��_vid_port
�
�
�_port_out
�
�
�
�
�
_beep:
�
�
�
�
�_lock
�82
�
��67
�
�_port_out
�
�
�
�
�
�
�
�
��66
�
�_port_out
�
�
��8
�
sar �cl
�
�
�
�
�
��66
�
�_port_out
�
�
���
�
��97
�
�_port_in
�
�
�
or �3
�
��97
�
�_port_out
�
�
���
I0175:
�
�
�
�
��8192
jae I0172
��
�I0175
I0172:
�
��97
�
�_port_out
�
�
�_unlock
�
�
�
_tty_init:
�
�
��,�
��4
�
��964
�
�_port_out
�
�
�
�
��965
�
�_port_out
�
�
��,#_tty_struct
I0185:
��,#_tty_struct+898
jae I0182
�
�
�200�,�
�
�
�202�,�
�bx,�
�860�,#3096
�
�858�,#_console
�
�871�,#8
�
�872�,#64
�
�873�,#127
�
�874�,#28
�
�875�,#17
�
�876�,#19
�
�877�,#4
��,#898
�I0185
I0182:
�_tty_struct+869,#1
�_color�
je I0187
�_vid_base,#47104
�_vid_mask,�383
�_vid_port,#976
�_vid_�race,#768
�I0188
I0187:
�_vid_base,#45056
�_vid_mask,#4095
�_vid_port,#944
�_vid_�race,#28672
I0188:
�_tty_struct+856,#1792
�_tty_driver_buf+1,�
��31
�
�0
�
�_set_6845
�
�
�
�
�2
�
�_set_6845
�
�
�
�
�
��_tty_struct
�
�_move_to
�
�_scan_code,#12
�I018A
�_olivetti,#1
I018A:
�
�
�
�_putc
_putc:
�
�
�al,�
�
�
�
��_tty_struct
�
�_out_char
�
�
mov �,�
�
�
_func_key:
�
�
cmpb �,#59
�I01A3
�_p_dmp
I01A3:
cmpb �,#60
�I01A6
�_map_dmp
I01A6:
cmpb �,#67
�I01A9
��9
�
�
�
��_tty_struct
�
�_�gchar
�
I01A9:
�
�
�
�
_vid_port: .zerow 2/2
�_vid_mask
_vid_mask: .zerow 2/2
_vid_base: .zerow 2/2
_vid_�race: .zerow 2/2
�_keybd_mess
_keybd_mess: .zerow 24/2
�_scan_code
_scan_code: .zerow 2/2
�_color
_color: .zerow 2/2
_alt: .zerow 2/2
_control: .zerow 2/2
_numlock: .zerow 2/2
_capslock: .zerow 2/2
_shift2: .zerow 2/2
_shift1: .zerow 2/2
_tty_buf: .zerow 256/2
_tty_copy_buf: .zerow 32/2
_tty_driver_buf: .zerow 34/2
_tty_struct: .zerow 898/2
�
