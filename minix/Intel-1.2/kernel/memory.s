�_mem_task
�
_mem_task:
�
�
��,#6
�
_2:
�I001D
�3
�2
�I001A
�I001B
�I001C
�
�_get_base
���
��4
�
2: sal �1
rcl �1
�
1:
�_ram_origin+8,�
�_ram_origin+8+2,�
��_�zes+2
��_�zes
��4
sal �cl
�
�_ram_limit+8,�
�_ram_limit+8+2,�
�_ram_limit+4�
�_ram_limit+4+2,#10
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
�_do_mem
�
�
�I0019
I001B:
��_mess
�
�_do_mem
�
�
�I0019
I001C:
��_mess
�
�_do_setup
�
�
�I0019
I001D:
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
mov �_mess
�
�
�_send
�
�
�
_do_mem:
�
�
��,#14
�
��4�
�
���
jl I0022
��,#4
jl I0023
I0022:
��-6
�
�I0021
I0023:
��,#3
�I0027
�
�2�,#3
�I002A
��-104
�
�I0021
I002A:
�
�8�
�I0021
I0027:
�
��10�
��12�
��0
sbb �0
�1f
���
je 1f
��
1: 
or ��
jge I002D
��-6
�
�I0021
I002D:
�
�
sal �cl
�
���
��_ram_origin�
��_ram_origin+2�
��10(�)
adc �12(�)
��,�
��,�
�
�
sal �cl
�
���
���
��_ram_limit�
sbb �_ram_limit+2�
�1f
���
je 1f
��
1: 
or ��
jl I00210
��-104
�
�I0021
I00210:
�
��8�
�
�
cwd
���
adc ��
�
�
sal �cl
��,_ram_limit�
sbb �_ram_limit+2�
�1f
���
je 1f
��
1: 
or ��
jle I00213
�
�
sal �cl
�
��_ram_limit�
��_ram_limit+2�
���
sbb ��
�
I00213:
�
��86
mul 6�
��688
�
��_proc
�-1�,�
�
�
�
�
�
�
�18�
�
�
�-1�
�_umap
��,#8
�-12(�),�
�-10(�),�
��-12(�)
��-10(�)
��0
sbb �0
�1f
or ��
1: 
or ��
�I00216
��-10
�
�I0021
I00216:
�
�2�,#3
�I00219
�
cwd
�
�
�-10(�)
�-12(�)
��
��
�_phys_copy
��,#12
�I0021A
I00219:
�
cwd
�
�
��
��
�-10(�)
�-12(�)
�_phys_copy
��,#12
I0021A:
�
I0021:
�
�
�
�
_do_setup:
�
�
�
�
��4�
�
�-2(�)�
jl I0032
��,#4
jl I0033
I0032:
��-6
�
�I0031
I0033:
�
�
�
sal �cl
���
��10�
��12�
�_ram_origin(�),�
�_ram_origin+2(�),�
�
��8�
cwd
��#1024
���
�
���
�.mli4
�
��10�
adc �12�
�
�
sal �cl
�_ram_limit�,�
�_ram_limit+2�,�
�
�
I0031:
�
�
�
�
�
_ram_limit: .zerow 16/2
_ram_origin: .zerow 16/2
_mess: .zerow 24/2
�
_1:
�25965
�8301
�24948
�27507
�26400
�29807
�27936
�29541
�24947
�25959
�26144
�28530
�8301
�
�
