# Hrtimer

## 基础
- Hrtimer通过高精度定时器硬件工作，而不是传统的系统时钟（`e0`的会是一个只，如比PIT，或者HPET。
jiffies`e0`的会是一个只，如比PIT，或者HPET。
）
	- 高精度定时器硬件，可以指定比如10ns后触发，则10ns后会产生一个中断，而不是固定频率的时钟中断（所谓定时器，就是可以定时触发）
- Hrtimer维护一个红黑树，记录要定时触发的任务，比如10ns，30ns，50ns。那么
	- Hrtimer检查红黑树，发现第一个是10ns触发，设置高精度定时器10ns后产生中断
	- 10ns后，Hrtimer的中断回调`hrtimer_interrupt`被调用，发现第一个任务到期，执行之。
		- 同时发现下一个任务需要20ns后触发，设置高精度定时器20ns后产生中断。
	- ...


## 实现
- `hrtimer`注册`hrtimer_interrupt`到高精度定时器硬件
- `hrtimer_interrupt`负责任务的到期检测
	- 发现到到期任务，执行`__run_hrtimer`：
    	- 调用用户注册好的回调函数`	restart = fn(timer);//lux 调用回调函数`
		- 根据回调函数的返回值，看是否把该timer重新加入队列（如此，用户可以选择是一次性的，还是周期性的定时器）
		```
		if (restart != HRTIMER_NORESTART) {//lux 回调函数会返回告知是否重启该timer，是则重新加入队列。
			BUG_ON(timer->state != HRTIMER_STATE_CALLBACK);
			enqueue_hrtimer(timer, base);///lux 重新把timer加入队列。
		}
		```
	- 如果队列里还有下一个timer，则计算到期时间，并更新高精度定时器硬件

另外，
`hrtimer`通过注册软中断来实现手动唤起`hrtimer_interrupt`的功能。软中断的使用参考[Softirq](softirq.lux.md)。

之所以需要这个，是在比如用户新定义一个timer，`hrtimer`插入红黑树的同时，发现该`timer`比其他都早，那么就重新编程高精度定时器硬件，并激活软中断，利用软中断主动调用`hrtimer_interrupt`。//todo why

### 软中断注册流程

系统启动时，`hrtimer`设置自己的软中断回调函数：
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
```
注册了自身的回调函数`run_hrtimer_softirq`。它每次触发的时候，都会检测是否都有到期的任务，有则调用任务的回调，从而实现定时器功能。

其本身实现的调用栈：
- `run_hrtimer_softirq`
- `hrtimer_peek_ahead_timers`
- `__hrtimer_peek_ahead_timers`
- `hrtimer_interrupt`


## 使用

### `schedule_hrtimeout_range`
这个函数提供`sleep until timeout`的功能，也就是说调用该函数，可以sleep直到设定的时间才会返回。

以`poll`系统调用为例，当它检查监听的句柄没有变化的时候，需要休眠，
```
//linux/select.c
		if (!poll_schedule_timeout(wait, TASK_INTERRUPTIBLE, to, slack))
			timed_out = 1;//lux 超时

//linux/select.c
int poll_schedule_timeout(struct poll_wqueues *pwq, int state,
			  ktime_t *expires, unsigned long slack)
{
	int rc = -EINTR;

	set_current_state(state);
	if (!pwq->triggered)
		rc = schedule_hrtimeout_range(expires, slack, HRTIMER_MODE_ABS);
	__set_current_state(TASK_RUNNING);
```
如上，`poll`就使用了`schedule_hrtimeout_range`来休眠到指定时间。

### 其他
在有了`hrtimer`后，内核新的定时任务都有`hrtimer`来完成，旧的也逐渐迁移过来了。

### 关于系统tick
在使用hrtimer之前，系统tick由硬件`irq0`时钟中断调用global clock event的回调`tick_handle_periodic`产生。
- 时钟中断
- `tick_handle_periodic`
- `tick_periodic`
- ...
```
//tick-internal.h
static inline void tick_set_periodic_handler(struct clock_event_device *dev,
					     int broadcast)
{
	dev->event_handler = tick_handle_periodic;//lux 设定回调函数
}
```

但是在开启了hrtimer之后，hrtimer注册了global clock event回调：
```
//hrtimer.c
static int hrtimer_switch_to_hres(void)
{
	...
	if (tick_init_highres()) {//lux 初始化高精度模式


//tick-oneshot.c
int tick_init_highres(void)//lux 初始化高精度模式
{
	return tick_switch_to_oneshot(hrtimer_interrupt);//lux 切换到oneshot模式，设置回调为hrtimer_interrupt
}


//tick-oneshot.c
int tick_switch_to_oneshot(void (*handler)(struct clock_event_device *))
{
	...
	td->mode = TICKDEV_MODE_ONESHOT;//lux 设定模式
	dev->event_handler = handler;//lux 设定回调
	clockevents_set_mode(dev, CLOCK_EVT_MODE_ONESHOT);//lux 设定模式
	tick_broadcast_switch_to_oneshot();//lux 通知
	return 0;
}
```

那么系统tick怎么办呢，因为很多服务都是基于系统tick的，系统tick必须要实现，而`jiffies`也是需要维护的。

hrtimer注册了一个定时器，设置其频率为HZ，到期回调调系统原来tick逻辑函数，如此其实就模拟了系统tick：
```
//hrtimer.c
static int hrtimer_switch_to_hres(void)
{
	...
	tick_setup_sched_timer();//lux 设定schedule timer，模拟原先的系统心跳

//tick-sched.c
void tick_setup_sched_timer(void)
{
	...
	/*
	 * Emulate tick processing via per-CPU hrtimers:
	 */
	hrtimer_init(&ts->sched_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	ts->sched_timer.function = tick_sched_timer;//lux 设定到期回调

//tick-sched.c
static enum hrtimer_restart tick_sched_timer(struct hrtimer *timer)
{
	...
		update_process_times(user_mode(regs));//lux 更新进程时间
		profile_tick(CPU_PROFILING);
	}

	hrtimer_forward(timer, now, tick_period);//lux 后移，设定下次触发时间

	return HRTIMER_RESTART;//lux 标记是重复触发，而不是执行一次

```

## 关于hrtimer与高精度
hrtimer是高精度定时器，自身使用了高精度定时器硬件。内核很多代码都逐渐使用hrtimer的API。但是hrtimer其实内部是支持高精度配置的：
```
//hrtimer.c
/*
 * Enable / Disable high resolution mode
 */
static int __init setup_hrtimer_hres(char *str)
{
	if (!strcmp(str, "off"))
		hrtimer_hres_enabled = 0;
	else if (!strcmp(str, "on"))
		hrtimer_hres_enabled = 1;
	else
		return 0;
	return 1;
}

__setup("highres=", setup_hrtimer_hres);
```

如上，内核是可以配置高精度是开是关的。默认是开。
```
//hrtimer.c
static int hrtimer_hres_enabled __read_mostly  = 1;//lux 是否启用了高精度
```
那如果关掉，是否还能用呢。答案是可以，也不可以。可以是说，所有调用hrtimer的API的代码依然可以工作。不可以是说，它的精度会下降。

在关掉的情况下，hrtimer是如何维护自己的定时器呢。
- 时钟中断
- `tick_handle_periodic`
- `tick_periodic`
- `run_local_timers`
- `hrtimer_run_queues`
- `__run_hrtimer`
如上，在没有启用高精度的情况下，hrtimer通过系统时钟中断来完成自己的工作。这种情况下，其精度就是系统频率HZ。


## 关于General Timer Framework
|分层	|说明	
|-------------------------------|-----------------------
| tick device, hrtimer,...		|	应用层
|	clock event device			|	提供定时功能的设备
|	clock source				|	时钟源


在前面起到了General Timer Framework。对于hrtimer而言，它主要是使用了Clock Event Device的功能，用来设定定时触发。
从`hrtimer_interrupt`的函数定义就能看出来：
```
//hrtimer.c
void hrtimer_interrupt(struct clock_event_device *dev)
{
```
调用`hrtimer_interrupt`的就是clock event device。当clock event device收到一个中断，就会调用`hrtimer_interrupt`。

另外，hrtimer主要是用到了`tick_program_event`功能，用来指定设备的下一个中断时间。
```
//tick-oneshot.c
//lux 编程clock设备，指定给你下一个事件的时间
/**
 * tick_program_event
 */
int tick_program_event(ktime_t expires, int force)
{
	struct clock_event_device *dev = __get_cpu_var(tick_cpu_device).evtdev;

	return tick_dev_program_event(dev, expires, force);
}

```

另外，系统有很多的clock event device，最终只会有一个成为`global_clock_event`，而该设备会在`irq0`的中断中被调用。
```
//time.c
static struct irqaction irq0  = {//lux irq0 时钟中断
	.handler = timer_interrupt,
	.flags = IRQF_DISABLED | IRQF_NOBALANCING | IRQF_IRQPOLL | IRQF_TIMER,
	.name = "timer"
};
//lux 设定时钟中断
void __init setup_default_timer_irq(void)
{
	setup_irq(0, &irq0);//lux 设定irq0（时钟中断）
}
```

```
//time.c
static irqreturn_t timer_interrupt(int irq, void *dev_id)
{
	...
	global_clock_event->event_handler(global_clock_event);//lux 调用event handler。global_clock_event是pit或者hpet
	...
}
```

注意`irq0`中断由8259A产生，连接到8259A的可以是PIT或者HPET，不同的硬件需要设置不同的回调。

### Clock Source
通过`clocksource_register`来注册各种时钟源
```
//jiffies.c
struct clocksource clocksource_jiffies = {//lux clocksource jiffies是一个clocksource
	.name		= "jiffies",
	.rating		= 1, /* lowest valid rating*/
	.read		= jiffies_read,
	.mask		= 0xffffffff, /*32bits*/
	.mult		= NSEC_PER_JIFFY << JIFFIES_SHIFT, /* details above */
	.shift		= JIFFIES_SHIFT,
};

static int __init init_jiffies_clocksource(void)
{
	return clocksource_register(&clocksource_jiffies);//lux 注册clocksource
}
```

```
//hpet.c
static struct clocksource clocksource_hpet = {
        .name           = "hpet",
        .rating         = 250,
        .read           = read_hpet,
        .mask           = CLOCKSOURCE_MASK(64),
	.mult		= 0, /* to be calculated */
        .shift          = 10,
        .flags          = CLOCK_SOURCE_IS_CONTINUOUS,
};
static struct clocksource *hpet_clocksource;
```

```
//acpi_pm.c
static struct clocksource clocksource_acpi_pm = {
	.name		= "acpi_pm",
	.rating		= 200,
	.read		= acpi_pm_read,
	.mask		= (cycle_t)ACPI_PM_MASK,
	.mult		= 0, /*to be calculated*/
	.shift		= 22,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};
```

### Clock Event Device
通过`clockevents_register_device`来注册各种clock event设备

```
//hpet.c
static struct clock_event_device hpet_clockevent = {
	.name		= "hpet",
	.features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode	= hpet_legacy_set_mode,
	.set_next_event = hpet_legacy_next_event,
	.shift		= 32,
	.irq		= 0,
	.rating		= 50,
};
```

## 其他
- `tick-*`相关的文件，都是与系统tick有关
- `CLOCK_EVT_FEAT_ONESHOT`代表该clock device支持定时触发；
- `CLOCK_EVT_FEAT_PERIODIC`代表该clock device支持周期触发。