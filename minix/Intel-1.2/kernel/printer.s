�_printer_task
�
_printer_task:
�
�
��,#24
�
_1:
�I0013
�
�4
�I0018
�I0013
�I0019
�I0013
�I0017
�
�_print_init
�
��-2�
�
�16
�
�_receive
�
�
�-22(�)
�I0015
I0017:
��-2�
�
�_do_write
�
�
I0018:
��-2�
�
�_do_cancel
�
�
I0019:
��-2�
�
�_do_done
�
�
I0015:
��#_1
�
�.csa2
_do_write:
�
�
��,#14
���
�_pr_busy�
je I0023
��,#-11
I0023:
�
�8��
jg I0026
��,#-22
I0026:
�
��86
mul 6�
��688
�
��_proc
�-10(�),�
�
���
�8�
�18(�)
�
�
�-10(�)
�_umap
��,#8
�-1�,�
�-12(�),�
��-1�
��-12(�)
��0
sbb �0
�1f
or ��
1: 
or ��
�I0029
��,#-10
I0029:
���
jne I002C
�_lock
�
���
�_caller,�
�
��6�
�_proc_nr,�
�
��8�
�_pcount,�
�
��8�
�_orig_count,�
��4
��-1�
��-12(�)
�
2: sar �1
rcr �1
�
1:
�_es,�
��4
��4
��-1�
�-12(�)
�
�
��15
��0
�_offset,�
�
I00211:
��,#1000
jge I002C
��_port_base
��
���
�
�
�_port_in
�
�
�
�
���
�
��176
��144
�I00213
�_pr_busy,#1
�_pr_char
��,#-998
�I002C
I00213:
�
�
���
�
��176
��16
�I00216
���
I0021B:
��,#1000
jge I002F
��
�I0021B
I00216:
��
�_pr_error
�
��,#-5
�I002C
I002F:
��
�I00211
I002C:
�
�
���
�
��176
��16
�I0021D
��,#-11
I0021D:
�
���
��
�6�
�(�)
��68
�
�_reply
��,#8
�
�
�
_do_done:
�
�
�
�
�6��
�I0033
�_orig_count
�I0034
I0033:
��-5
�
I0034:
��
�_proc_nr,#-999
je I0036
�
�_proc_nr
�_caller
��67
�
�_reply
��,#8
��,#-5
�I0036
�
�6�
�_pr_error
�
I0036:
�_pr_busy�
�
�
�
_do_cancel:
�
�
�_pr_busy�
�I0043
�
�
�
I0043:
�_pr_busy�
�_pcount�
�_proc_nr,#-999
�
���
��-4
�
�6�
�(�)
��68
�
�_reply
��,#8
�
�
�
_reply:
�
�
��,#24
�
�-22(�),�
��10(�)
�-1�,�
���
�-20(�),�
��-2�
�
�
�_send
�
�
�
�
�
_pr_error:
�
�
�
�
�
�
testb al,#32
je I0063
��_2
�
�_printk
�
I0063:
�
�
�
�
testb al,�
�I0066
��_3
�
�_printk
�
I0066:
�
�
�
�
testb al,#8
�I0069
��_4
�
�_printk
�
I0069:
�
�
�
_print_init:
�
�
�
�_color�
je I0073
��888
�
�I0074
I0073:
��956
�
I0074:
�
�
�
�
�_port_base,�
�_pr_busy�
�
��_port_base
��8
�
�
�_port_out
�
�
�
I0078:
��,#100
jge I0075
��
�I0078
I0075:
�
��_port_base
��12
�
�
�_port_out
�
�
�
�
�
�_pr_char
_pr_char:
�
�
��,#8
��_orig_count
�_pcount,�
je I0083
��32
�
�
�_port_out
�
�
I0083:
�_pr_busy�
�I0089
�
�
�
I0089:
�_pcount�
jle I0088
��_port_base
��
���
�
�
�_port_in
�
�
�
�
�
�
��176
��144
�I008C
�_offset
�_es
�_get_byte
�
�
�-7(�),al
�
�
�al,-7(�)
�
�
�
�
�_port_base
�_port_out
�
�
�
��_port_base
�9
�
�
�_port_out
�
�
�
��_port_base
�8
�
�
�_port_out
�
�
�_offset
�_pcount
�_cum_count
���
I00811:
��,#100
jge I0089
��
�I00811
I008C:
�
�
�
�
��176
��16
�I0088
�
�
�
I0088:
�_int_mess+2,#2
�_pcount�
�I00816
�
�
�I00817
I00816:
�
I00817:
�_int_mess+6
��_int_mess
�
��-8
�
�_interrupt
�
�
�
�
�
�_prev_ct
�
_prev_ct: .zerow 2/2
�_cum_count
_cum_count: .zerow 2/2
�_pr_busy
_pr_busy: .zerow 2/2
�_pcount
_pcount: .zerow 2/2
_offset: .zerow 2/2
_es: .zerow 2/2
_orig_count: .zerow 2/2
_proc_nr: .zerow 2/2
_caller: .zerow 2/2
_port_base: .zerow 2/2
�
_2:
�29264
�28265
�25972
�8306
�29545
�28448
�29813
�28448
�8294
�24944
�25968
�2674
�
_3:
�29264
�28265
�25972
�8306
�29545
�28192
�29807
�28448
�8302
�26988
�25966
�10
_4:
�29264
�28265
�25972
�8306
�29285
�28530
�2674
�
�