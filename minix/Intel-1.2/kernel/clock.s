�
_sched_ticks:
�_clock_task
�6
�
_clock_task:
�
�
�
�
_2:
�I001B
�1
�3
�I0017
�I001A
�I0018
�I0019
�
�_init_clock
�
��_mc
�
�16
�
�_receive
�
�
��_mc+2
��,�
�
�I0015
I0017:
��_mc
�
�_do_setalarm
�
�I0016
I0018:
�_do_get_time
�I0016
I0019:
��_mc
�
�_do_set_time
�
�I0016
I001A:
�_do_clocktick
�I0016
I001B:
�_mc+2
��_1
�
�_panic
�
�
�I0016
I0015:
��#_2
�
�.csa2
�
�_mc+2�
��,#2
je I0013
��_mc
�
�_mc
�_send
�
�
�
_do_setalarm:
�
�
��,#10
�
��4�
�
��10�
�12�
��,�
��
��14�
�-10(�),�
��86
mul �
��688
�
��_proc
��,�
�
��70�
��72�
���
sbb �0
�1f
or ��
1: 
or ��
�I0023
�
�
�
�I0024
I0023:
�
��70�
��72�
��_realtime
sbb �_realtime+2
��
�
�
�
��60
�
�.dvi4
��
�
I0024:
�_mc+10
�_mc+10+2
���
���
��0
sbb �0
�1f
or ��
1: 
or ��
�I0026
�
�
�
�I0027
I0026:
���
���
��_realtime
adc �_realtime+2
�
�
I0027:
�
�
��
�70�,�
�72�,�
���
jge I0029
�
��
sal �1
�
��-10(�)
�_watch_dog�,�
I0029:
�_next_alarm,#65535
�_next_alarm+2,#32767
��,#_proc
I002E:
��,#_proc+2064
jae I002B
�
��70�
��72�
��0
sbb �0
�1f
or ��
1: 
or ��
je I002C
�
��70�
��72�
��_next_alarm
sbb �_next_alarm+2
�1f
���
je 1f
��
1: 
or ��
jge I002C
�
mov �70�
�72�
�_next_alarm,�
�_next_alarm+2
I002C:
��,#86
�I002E
I002B:
�
�
�
_do_get_time:
�
�
�_mc+2,#1
�_realtime+2
�_realtime
�
�
��60
�
�.dvi4
��_boot_time
adc �_boot_time+2
�_mc+10,�
�_mc+10+2,�
�
�
�
_do_set_time:
�
�
�
�12�
�10�
�_realtime+2
�_realtime
�
�
��60
�
�.dvi4
�
��
���
sbb ��
�_boot_time,�
�_boot_time+2,�
�
�
�
_do_clocktick:
�
�
��,#10
��_lost_ticks
��,�
�
��
cwd
��_realtime
adc �_realtime+2
�_realtime,�
�_realtime+2,�
��_lost_ticks
���
�_lost_ticks,�
��_next_alarm
��_next_alarm+2
��_realtime
sbb �_realtime+2
�1f
���
je 1f
��
1: 
or ��
jg I0053
�_next_alarm,#65535
�_next_alarm+2,#32767
��,#_proc
I0058:
��,#_proc+2064
jae I0053
�
��70�
��72�
��0
sbb �0
�1f
or ��
1: 
or ��
je I0056
�
��70�
��72�
��_realtime
sbb �_realtime+2
�1f
���
je 1f
��
1: 
or ��
jg I005D
�
��_proc
��86
cwd
i�v �
��8
��,�
���
jl I00510
�4
�
��
�_cause_�g
�
�
�I00511
I00510:
���
��
sal �1
�
��_watch_dog�
�(�)
I00511:
�
�70��
�72��
I005D:
�
��70�
��72�
��0
sbb �0
�1f
or ��
1: 
or ��
je I0056
�
��70�
��72�
��_next_alarm
sbb �_next_alarm+2
�1f
���
je 1f
��
1: 
or ��
jge I0056
�
��70�
�72�
�_next_alarm,�
�_next_alarm+2
I0056:
��,#86
�I0058
I0053:
�_accounting
�_sched_ticks
�I00517
��_prev_ptr
�_bill_ptr,�
�I0051A
�_sched
I0051A:
�_sched_ticks,#6
��_bill_ptr
�_prev_ptr,�
�_pr_busy�
je I0051D
�_pcount�
jle I0051D
��_prev_ct
�_cum_count,�
�I0051D
�_pr_char
I0051D:
��_cum_count
�_prev_ct,�
I00517:
�
�
�
_accounting:
�
�
�
�_prev_proc,#2
jl I0063
��_bill_ptr
��54
��,�
�
�
��2�
��1
adc �0
�
�2�,�
�I0064
I0063:
��_bill_ptr
��58
��,�
�
�
��2�
��1
adc �0
�
�2�,�
I0064:
�
�
�
_init_clock:
�
�
��,#6
��,#19886
�
�
�
��8
�
shr �cl
�
��,�
��54
�
��67
�
�_port_out
�
�
�
��64
�
�_port_out
�
�
��
��64
�
�_port_out
�
�
�
�
�
�
_watch_dog: .zerow 18/2
_mc: .zerow 24/2
_prev_ptr: .zerow 2/2
_next_alarm: .zerow 4/2
_boot_time: .zerow 4/2
�
_1:
�27747
�25455
�8299
�24948
�27507
�26400
�29807
�25120
�25697
�27936
�29541
�24947
�25959
�
�
