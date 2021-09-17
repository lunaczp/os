# Softirq 软中断

系统维护一个`softirq_vec`数组，包含了定义好的软中断回调函数（静态编译），在处理软中断的时候，会遍历调用这些回调函数
```
//kernel/softirq.c
static struct softirq_action softirq_vec[NR_SOFTIRQS] __cacheline_aligned_in_smp;

//kernel/softirq.c
enum
{
	HI_SOFTIRQ=0,
	TIMER_SOFTIRQ,
	NET_TX_SOFTIRQ,
	NET_RX_SOFTIRQ,
	BLOCK_SOFTIRQ,
	BLOCK_IOPOLL_SOFTIRQ,
	TASKLET_SOFTIRQ,
	SCHED_SOFTIRQ,
	HRTIMER_SOFTIRQ,
	RCU_SOFTIRQ,	/* Preferable RCU should always be the last softirq */

	NR_SOFTIRQS
};

//inlcude/linux/interupt.h
struct softirq_action
{
	void	(*action)(struct softirq_action *);
};

```

系统启动的时候，可以设定指定的回调函数。比如对于高精度定时器组件`hrtimer`：
```
//init/main.c
asmlinkage void __init start_kernel(void)
{
    ...
    	hrtimers_init();//lux 高精度定时器


//kernel/hrtimer.c
void __init hrtimers_init(void)
{
	hrtimer_cpu_notify(&hrtimers_nb, (unsigned long)CPU_UP_PREPARE,
			  (void *)(long)smp_processor_id());
	register_cpu_notifier(&hrtimers_nb);
#ifdef CONFIG_HIGH_RES_TIMERS
	open_softirq(HRTIMER_SOFTIRQ, run_hrtimer_softirq);
#endif
}

//kernel/softirq.c
void open_softirq(int nr, void (*action)(struct softirq_action *))
{
	softirq_vec[nr].action = action;
}

如上，就在`softirq_vec`中设定了`hrtimer`的回调`run_hrtimer_softirq`
```

## 触发
以上是注册了不同的软中断及其回调。使用的时候，
- 先激活需要使用的软中断`raise_softirq`
- 系统会择机处理激活的软中断
	- 硬件中断处理完毕返回时
		- `do_IRQ`
		- `irq_exit`
		- `invoke_softirq`
		- `do_softirq`
		- `__do_softirq`

## 说明
软中断一般是配合硬中断使用。位于所谓中断下半部分("__bottom half__")。  
- 硬件中断需要关闭中断，处理核心逻辑，（上半部分）
- 然后在退出之前，设置软中断。然后由软中断异步去处理耗时的逻辑。（下半部分）