…_mem_task
Ж
_mem_task:
Г
В
„т,#6
Ѓ
_2:
¬I001D
¬3
¬2
¬I001A
¬I001B
¬I001C
Ж
”_get_base
Ћем
јг4
©
2: sal б1
rcl в1
®
1:
ј_ram_origin+8,л
ј_ram_origin+8+2,м
је_пzes+2
ƒе_пzes
јг4
sal еcl
З
ј_ram_limit+8,м
ј_ram_limit+8+2,л
ј_ram_limit+4ц
ј_ram_limit+4+2,#10
Х
јб_mess
А
П16
А
”_receive
К
К
Ќ_messц
jge I0016
Ѕ_mess
јб_1
А
”_panic
К
К
љ
је_mess
јѕ,м
је_mess+6
ј–,м
Ѕ_mess+2
ћI0018
I001A:
јб_mess
А
”_do_mem
К
Ф
ћI0019
I001B:
јб_mess
А
”_do_mem
К
Ф
ћI0019
I001C:
јб_mess
А
”_do_setup
К
Ф
ћI0019
I001D:
ј∆,#-22
ћI0019
I0018:
ји#_2
Й
ћ.csa2
I0019:
ј_mess+2,#68
је–
ј_mess+4,м
І
ј_mess+6,м
mov б_mess
А
є
”_send
К
К
≠
_do_mem:
Г
В
„т,#14
Т
јд4«
Ф
Ќ∆ц
jl I0022
Ќ∆,#4
jl I0023
I0022:
јб-6
А
ћI0021
I0023:
Ќ∆,#3
ўI0027
Т
Ќ2«,#3
ўI002A
јб-104
А
ћI0021
I002A:
Т
Ѕ8«
ћI0021
I0027:
Т
јд10«
јж12«
„б0
sbb г0
ў1f
÷дл
je 1f
№н
1: 
or жн
jge I002D
јб-6
А
ћI0021
I002D:
Э
Ч
sal дcl
ї
ји≈
јд_ram_origin«
јж_ram_origin+2«
ƒд10(п)
adc ж12(п)
јЎ,л
ј–,н
Э
Ч
sal дcl
ї
јдЎ
јж–
„д_ram_limit«
sbb ж_ram_limit+2«
ў1f
÷дл
je 1f
№н
1: 
or жн
jl I00210
јб-104
А
ћI0021
I00210:
Т
јд8«
ђ
£
cwd
ƒдЎ
adc з–
Э
І
sal еcl
„л,_ram_limit«
sbb з_ram_limit+2«
ў1f
÷дл
je 1f
№о
1: 
or зо
jle I00213
Э
Ч
sal дcl
ї
јд_ram_limit«
јж_ram_limit+2«
„дЎ
sbb ж–
ђ
I00213:
Т
јб86
mul 6«
ƒб688
ї
ƒв_proc
ј-1≈,м
Э
Ю
£
Ц
Т
А
Ѕ18«
П
А
Ѕ-1≈
”_umap
ƒт,#8
ј-12(с),л
ј-10(с),о
јд-12(с)
је-10(с)
„б0
sbb в0
ў1f
or ел
1: 
or ем
ўI00216
јб-10
А
ћI0021
I00216:
Т
Ќ2«,#3
ўI00219
£
cwd
Ь
А
Ѕ-10(с)
Ѕ-12(с)
Ѕ–
ЅЎ
”_phys_copy
ƒт,#12
ћI0021A
I00219:
£
cwd
Ь
А
Ѕ–
ЅЎ
Ѕ-10(с)
Ѕ-12(с)
”_phys_copy
ƒт,#12
I0021A:
є
I0021:
Н
Е
Д
Б
_do_setup:
Г
В
А
Т
јд4«
Ф
Ќ-2(с)ц
jl I0032
Ќ∆,#4
jl I0033
I0032:
јб-6
А
ћI0031
I0033:
Т
Э
Ч
sal дcl
јил
јд10«
је12«
ј_ram_origin(п),л
ј_ram_origin+2(п),м
Т
јд8«
cwd
ји#1024
Ћйр
ї
јдо
”.mli4
Т
ƒд10«
adc з12«
Э
І
sal еcl
ј_ram_limit«,л
ј_ram_limit+2«,о
З
А
I0031:
Н
Е
Д
Б
≥
_ram_limit: .zerow 16/2
_ram_origin: .zerow 16/2
_mess: .zerow 24/2
Ѓ
_1:
¬25965
¬8301
¬24948
¬27507
¬26400
¬29807
¬27936
¬29541
¬24947
¬25959
¬26144
¬28530
¬8301
Є
Ж
