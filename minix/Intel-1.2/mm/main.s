�_main
�
_main:
�
�
�
�_mm_init
�
�_get_work
��40
mul _who
�
��_mproc
�_mp,�
�
�_dont_reply�
�_err_code,#-999
�_mm_call�
jl I0015
�_mm_call,#69
jl I0016
I0015:
��,#-102
�I0017
�
��_mm_call
sal �1
��_call_vec�
�(�)
�
I0017:
�_dont_reply�
je I001A
�
I001A:
�_mm_call,#59
�I001D
���
�I001D
�
I001D:
�_res_ptr
�_result2
�
�_who
�_reply
��,#8
�
_get_work:
�
�
��_mm_in
�
�16
�
�_receive
�
�
�
je I0023
��32768
�
��_1
�
�_panic
�
�
I0023:
��_mm_in
�_who,�
�_who,#-1
jl I0025
�_who,�
jl I0026
I0025:
�_who
��_2
�
�_panic
�
�
I0026:
��_mm_in+2
�_mm_call,�
�
�
�
�_reply
_reply:
�
�
�
��40
mul �
mov ��
��_mproc
��,�
�
testb 38�,#1
je I0032
�
testb 38�,#4
je I0033
I0032:
�
�
�
I0033:
�
�_mm_out+2,�
�
�_mm_out+4,�
��10(�)
�_mm_out+18,�
��_mm_out
�
�
�_send
�
�
�
je I0037
��32768
�
��_3
�
�_panic
�
�
I0037:
�
�
�
_mm_init:
�
�
��,#6
�_get_tot_mem
�_tot_mem,�
�_tot_mem
�_mem_init
�
or _mproc+38,#1
or _mproc+78,#1
or _mproc+118,#1
�_procs_in_use,#3
�
�
�
�_do_brk2
_do_brk2:
�
�
��,#24
�_who,#1
je I0053
��-1
�
�I0051
I0053:
�
�
��_mm_in+4
�
�-20(�),�
�
�
��_mm_in+6
�
�-22(�),�
�
�
��_mm_in+8
�
�-1�,�
��_mm_in+10
�-10(�),�
��-22(�)
��-20(�)
�-12(�),�
��-12(�)
��-10(�)
mov -1�,�
��-1�
��-1�
�-1�,�
�-1�
�_alloc_mem
�
�
�
�_tot_mem
�
��64
�
�.dvu4
�
��-1�
��32
���
�
�
�
�
��64
�
�.dvu4
�
�
�
�-1�
�
��64
�
�.dvu4
��,�
�7
�
�
��_4
�
�_printk
�
�
��_5
�
�_printk
�
�
�
��_6
�
�_printk
�
�
��
��_7
�
�_printk
�
�
�
���
���
�
��_8
�
�_printk
�
�
�
���
���
��32
jge I0056
��32768
�
��_9
�
�_printk
�
�
�_sys_abort
I0056:
��,#_mproc+80
���
��-10(�)
�2�,�
���
��-20(�)
�4�,�
��-20(�)
��-10(�)
���
�8�,�
���
��-22(�)
�10�,�
���
��-12(�)
�12�,�
��-12(�)
��-10(�)
���
�14�,�
�-20(�)�
je I0059
���
��38
�-2�,�
�
or �32
�
I0059:
�
�
I0051:
�
�
�
�
_set_map:
�
�
�
�
��40
mul �
�
��_mproc
��,�
���
�
�
���
�
�4��
�
���
�2�,�
�
�6��
�
�
�10�,�
�
���
�8�,�
�
�
�12�,�
�
�16��
�
���
�
�14�,�
�
�
�_sys_newmap
�
�
�
�
�
�
_tot_mem: .zerow 2/2
�
_1:
�19789
�29216
�25445
�26981
�25974
�25888
�29298
�29295
�
_2:
�19789
�25376
�27745
�25964
�8292
�31074
�
_3:
�19789
�25376
�28257
�29735
�29216
�28773
�31084
�
_4:
�25381
�14368
�25381
�12414
�
_5:
�25933
�28525
�31090
�29472
�31337
�8293
�8253
�25637
�8267
�
�
�
_6:
�18765
�18766
�8280
�8253
�25637
�8267
�
�
�
_7:
�16722
�8269
�26980
�27507
�15648
�9504
�19300
�
�
�32
_8:
�30273
�26977
�24940
�27746
�8293
�8253
�25637
�2635
�10
_9:
�19978
�29807
�25888
�28526
�26485
�8296
�25965
�28525
�31090
�29728
�8303
�30066
�8302
�18765
�18766
�2648
�10
�
