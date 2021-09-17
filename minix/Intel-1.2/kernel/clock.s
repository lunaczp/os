Ѓ
_sched_ticks:
…_clock_task
¬6
Ж
_clock_task:
Г
В
А
Ѓ
_2:
¬I001B
¬1
¬3
¬I0017
¬I001A
¬I0018
¬I0019
Ж
”_init_clock
Х
јб_mc
А
П16
А
”_receive
К
К
је_mc+2
ј∆,м
°
ћI0015
I0017:
јб_mc
А
”_do_setalarm
К
ћI0016
I0018:
”_do_get_time
ћI0016
I0019:
јб_mc
А
”_do_set_time
К
ћI0016
I001A:
”_do_clocktick
ћI0016
I001B:
Ѕ_mc+2
јб_1
А
”_panic
К
К
ћI0016
I0015:
ји#_2
Й
ћ.csa2
љ
ј_mc+2ц
Ќ∆,#2
je I0013
јб_mc
А
Ѕ_mc
”_send
К
К
≠
_do_setalarm:
Г
В
„т,#10
Т
јд4«
ђ
јд10«
Ѕ12«
јЎ,л
√–
јд14«
ј-10(с),л
јб86
mul ѕ
ƒб688
ї
ƒв_proc
ј∆,м
І
јд70«
јж72«
„лц
sbb г0
ў1f
or жл
1: 
or жн
ўI0023
З
А
А
ћI0024
I0023:
І
јд70«
јж72«
„д_realtime
sbb ж_realtime+2
Ѕн
А
З
А
јб60
А
”.dvi4
Ѕн
А
I0024:
√_mc+10
√_mc+10+2
јдЎ
је–
„б0
sbb в0
ў1f
or ел
1: 
or ем
ўI0026
З
А
А
ћI0027
I0026:
јдЎ
је–
ƒд_realtime
adc е_realtime+2
¶
А
I0027:
І
Н
√н
ј70«,л
ј72«,н
Ќѕц
jge I0029
£
фл
sal б1
ї
јд-10(с)
ј_watch_dog«,л
I0029:
ј_next_alarm,#65535
ј_next_alarm+2,#32767
ј∆,#_proc
I002E:
Ќ∆,#_proc+2064
jae I002B
І
јд70«
јж72«
„б0
sbb г0
ў1f
or жл
1: 
or жн
je I002C
І
јд70«
јж72«
„д_next_alarm
sbb ж_next_alarm+2
ў1f
÷дл
je 1f
№н
1: 
or жн
jge I002C
І
mov ж70«
Ѕ72«
ј_next_alarm,н
√_next_alarm+2
I002C:
ƒ∆,#86
ћI002E
I002B:
Е
Д
Б
_do_get_time:
Г
В
ј_mc+2,#1
Ѕ_realtime+2
Ѕ_realtime
З
А
јб60
А
”.dvi4
ƒд_boot_time
adc ж_boot_time+2
ј_mc+10,л
ј_mc+10+2,н
Е
Д
Б
_do_set_time:
Г
В
Т
Ѕ12«
Ѕ10«
Ѕ_realtime+2
Ѕ_realtime
З
А
јб60
А
”.dvi4
Й
√о
„ел
sbb зн
ј_boot_time,м
ј_boot_time+2,о
Е
Д
Б
_do_clocktick:
Г
В
„т,#10
је_lost_ticks
јѕ,м
£
№л
cwd
ƒд_realtime
adc з_realtime+2
ј_realtime,л
ј_realtime+2,о
је_lost_ticks
„еѕ
ј_lost_ticks,м
је_next_alarm
јж_next_alarm+2
„е_realtime
sbb ж_realtime+2
ў1f
÷ем
je 1f
№н
1: 
or жн
jg I0053
ј_next_alarm,#65535
ј_next_alarm+2,#32767
ј∆,#_proc
I0058:
Ќ∆,#_proc+2064
jae I0053
І
јд70«
јж72«
„б0
sbb г0
ў1f
or жл
1: 
or жн
je I0056
І
јд70«
јж72«
„д_realtime
sbb ж_realtime+2
ў1f
÷дл
je 1f
№н
1: 
or жн
jg I005D
Ч
„б_proc
јв86
cwd
iрv м
„б8
ј–,л
Ќ–ц
jl I00510
П4
А
Ѕ–
”_cause_пg
К
К
ћI00511
I00510:
јд–
фл
sal б1
ї
јд_watch_dog«
”(л)
I00511:
І
ј70«ц
ј72«ц
I005D:
І
јд70«
јж72«
„б0
sbb г0
ў1f
or жл
1: 
or жн
je I0056
І
јд70«
јж72«
„д_next_alarm
sbb ж_next_alarm+2
ў1f
÷дл
je 1f
№н
1: 
or жн
jge I0056
І
јж70«
Ѕ72«
ј_next_alarm,н
√_next_alarm+2
I0056:
ƒ∆,#86
ћI0058
I0053:
”_accounting
у_sched_ticks
ўI00517
је_prev_ptr
Ќ_bill_ptr,м
ўI0051A
”_sched
I0051A:
ј_sched_ticks,#6
је_bill_ptr
ј_prev_ptr,м
Ќ_pr_busyц
je I0051D
Ќ_pcountц
jle I0051D
је_prev_ct
Ќ_cum_count,м
ўI0051D
”_pr_char
I0051D:
је_cum_count
ј_prev_ct,м
I00517:
Е
Д
Б
_accounting:
Г
В
А
Ќ_prev_proc,#2
jl I0063
је_bill_ptr
ƒв54
ј∆,м
І
Ђ
јж2«
ƒб1
adc г0
∞
ј2«,н
ћI0064
I0063:
је_bill_ptr
ƒв58
ј∆,м
І
Ђ
јж2«
ƒб1
adc г0
∞
ј2«,н
I0064:
Е
Д
Б
_init_clock:
Г
В
„т,#6
ј∆,#19886
Ч
•
ђ
јг8
Ч
shr дcl
•
ј–,л
јб54
А
јб67
А
”_port_out
К
К
є
јб64
А
”_port_out
К
К
Ѕ–
јб64
А
”_port_out
К
К
Е
Д
Б
≥
_watch_dog: .zerow 18/2
_mc: .zerow 24/2
_prev_ptr: .zerow 2/2
_next_alarm: .zerow 4/2
_boot_time: .zerow 4/2
Ѓ
_1:
¬27747
¬25455
¬8299
¬24948
¬27507
¬26400
¬29807
¬25120
¬25697
¬27936
¬29541
¬24947
¬25959
Є
Ж
