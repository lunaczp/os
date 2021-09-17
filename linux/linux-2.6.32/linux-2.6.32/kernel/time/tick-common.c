/*lux 系统用clock event device来为上层提供tick device
 * clock event device 来自 clocksource(如pit,hpet)
 * clock event device可以支持周期触发，或者定时触发（oneshot），为上层（tick device）提供定时功能
 * 
 * 系统的tick频率很低，是HZ，目前是1000，也即1s内1000个tick（滴答）。
 *
 */
/*
 * linux/kernel/time/tick-common.c
 *
 * This file contains the base functions to manage periodic tick
 * related events.
 *
 * Copyright(C) 2005-2006, Thomas Gleixner <tglx@linutronix.de>
 * Copyright(C) 2005-2007, Red Hat, Inc., Ingo Molnar
 * Copyright(C) 2006-2007, Timesys Corp., Thomas Gleixner
 *
 * This code is licenced under the GPL version 2. For details see
 * kernel-base/COPYING.
 */
#include <linux/cpu.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>
#include <linux/profile.h>
#include <linux/sched.h>
#include <linux/tick.h>

#include <asm/irq_regs.h>

#include "tick-internal.h"

/*
 * Tick devices
 */
DEFINE_PER_CPU(struct tick_device, tick_cpu_device);//lux 定义tick device设备，每个cpu一个
/*
 * Tick next event: keeps track of the tick time
 */
ktime_t tick_next_period;//lux 下一个心跳时间
ktime_t tick_period;//lux tick周期，用硬件来模拟一个系统心跳。比如系统心跳HZ=1000，而时钟设备是纳秒级别，那么tick_period是10^9/10^3=10^6，也即100W。
int tick_do_timer_cpu __read_mostly = TICK_DO_TIMER_BOOT;
DEFINE_SPINLOCK(tick_device_lock);

/*
 * Debugging: see timer_list.c
 */
struct tick_device *tick_get_device(int cpu)
{
	return &per_cpu(tick_cpu_device, cpu);
}

/**
 * tick_is_oneshot_available - check for a oneshot capable event device
 */
int tick_is_oneshot_available(void)
{
	struct clock_event_device *dev = __get_cpu_var(tick_cpu_device).evtdev;

	return dev && (dev->features & CLOCK_EVT_FEAT_ONESHOT);
}

/*
 * Periodic tick
 */
static void tick_periodic(int cpu)
{
	if (tick_do_timer_cpu == cpu) {//lux 当前cpu负责do_timer
		write_seqlock(&xtime_lock);

		/* Keep track of the next tick event */
		tick_next_period = ktime_add(tick_next_period, tick_period);//lux 下一个心跳时间

		do_timer(1);//lux do_timer
		write_sequnlock(&xtime_lock);
	}

	update_process_times(user_mode(get_irq_regs()));//lux 更新进程消耗的时间
	profile_tick(CPU_PROFILING);
}
/*lux tick event handler tick设备的tick回调
 * 每次tick设备心跳，则会调用该回调
 * see tick-internal.h tick_set_periodic_handler
 */
/*
 * Event handler for periodic ticks
 */
void tick_handle_periodic(struct clock_event_device *dev)
{
	int cpu = smp_processor_id();
	ktime_t next;

	tick_periodic(cpu);

	if (dev->mode != CLOCK_EVT_MODE_ONESHOT)//lux 周期模式，则不用处理。如果是oneshot模式，下面要模拟周期tick
		return;
	
	//lux 下面利用oneshot模式模拟周期tick
	/*
	 * Setup the next period for devices, which do not have
	 * periodic mode:
	 */
	next = ktime_add(dev->next_event, tick_period);
	for (;;) {
		if (!clockevents_program_event(dev, next, ktime_get()))
			return;
		/*
		 * Have to be careful here. If we're in oneshot mode,
		 * before we call tick_periodic() in a loop, we need
		 * to be sure we're using a real hardware clocksource.
		 * Otherwise we could get trapped in an infinite
		 * loop, as the tick_periodic() increments jiffies,
		 * when then will increment time, posibly causing
		 * the loop to trigger again and again.
		 */
		if (timekeeping_valid_for_hres())
			tick_periodic(cpu);
		next = ktime_add(next, tick_period);
	}
}
//lux 设定一个clock event device为周期模式
/*
 * Setup the device for a periodic tick
 */
void tick_setup_periodic(struct clock_event_device *dev, int broadcast)
{
	tick_set_periodic_handler(dev, broadcast);//lux 设定周期模式下的回调

	/* Broadcast setup ? */
	if (!tick_device_is_functional(dev))
		return;

	if ((dev->features & CLOCK_EVT_FEAT_PERIODIC) &&
	    !tick_broadcast_oneshot_active()) {
		clockevents_set_mode(dev, CLOCK_EVT_MODE_PERIODIC);
	} else {
		unsigned long seq;
		ktime_t next;

		do {
			seq = read_seqbegin(&xtime_lock);
			next = tick_next_period;
		} while (read_seqretry(&xtime_lock, seq));

		clockevents_set_mode(dev, CLOCK_EVT_MODE_ONESHOT);

		for (;;) {
			if (!clockevents_program_event(dev, next, ktime_get()))
				return;
			next = ktime_add(next, tick_period);
		}
	}
}
/*lux 为指定cpu的tick device配置指定的clock event device设备
 * *td 指定的cpu等等tick device
 * *newdev 新clock event device设备
 * cpu 指定cpu
 * *cpumask cpumask
 */
/*
 * Setup the tick device
 */
static void tick_setup_device(struct tick_device *td,
			      struct clock_event_device *newdev, int cpu,
			      const struct cpumask *cpumask)
{
	ktime_t next_event;
	void (*handler)(struct clock_event_device *) = NULL;

	/*
	 * First device setup ?
	 */
	if (!td->evtdev) {//lux 第一次
		/*
		 * If no cpu took the do_timer update, assign it to
		 * this cpu:
		 */
		if (tick_do_timer_cpu == TICK_DO_TIMER_BOOT) {//lux 还没有cpu承担do_timer的任务，那么使用这个cpu。下面初始化系统心跳
			tick_do_timer_cpu = cpu;
			tick_next_period = ktime_get();//lux 当前时间
			tick_period = ktime_set(0, NSEC_PER_SEC / HZ);//lux 计算tick周期，（用纳秒级别的设备，模拟/产生系统tick，假如HZ=1000，那么tick_period=10^9/10^3=10^6，也即100W纳秒，一次系统tick）
		}

		/*
		 * Startup in periodic mode first.
		 */
		td->mode = TICKDEV_MODE_PERIODIC;//lux 第一次启动，需要设置周期中断模式（兼容内核以前的工作模式）
	} else {
		handler = td->evtdev->event_handler;//lux 先从旧设备提取出用户设置的handler，要分配给新设备
		next_event = td->evtdev->next_event;//lux next_event也一样
		td->evtdev->event_handler = clockevents_handle_noop;//lux clear旧设备的handler
	}

	td->evtdev = newdev;//lux 设定新设备

	/*
	 * When the device is not per cpu, pin the interrupt to the
	 * current cpu:
	 */
	if (!cpumask_equal(newdev->cpumask, cpumask))//lux 将设备绑定到该cpu
		irq_set_affinity(newdev->irq, cpumask);

	/*
	 * When global broadcasting is active, check if the current
	 * device is registered as a placeholder for broadcast mode.
	 * This allows us to handle this x86 misfeature in a generic
	 * way.
	 */
	if (tick_device_uses_broadcast(newdev, cpu))
		return;

	if (td->mode == TICKDEV_MODE_PERIODIC)
		tick_setup_periodic(newdev, 0);//lux 周期模式
	else
		tick_setup_oneshot(newdev, handler, next_event);//lux 设置定时模式，以及handler、next_event
}
//lux 被底层通知有新设备加入了，这里处理
/*
 * Check, if the new registered device should be used.
 */
static int tick_check_new_device(struct clock_event_device *newdev)
{
	struct clock_event_device *curdev;
	struct tick_device *td;
	int cpu, ret = NOTIFY_OK;
	unsigned long flags;

	spin_lock_irqsave(&tick_device_lock, flags);

	cpu = smp_processor_id();
	if (!cpumask_test_cpu(cpu, newdev->cpumask))
		goto out_bc;

	td = &per_cpu(tick_cpu_device, cpu);//lux 获取cpu的tick device结构体
	curdev = td->evtdev;//lux 获取对应的deivce

	/* cpu local device ? */
	if (!cpumask_equal(newdev->cpumask, cpumask_of(cpu))) {

		/*
		 * If the cpu affinity of the device interrupt can not
		 * be set, ignore it.
		 */
		if (!irq_can_set_affinity(newdev->irq))//lux 新设备无法设置其工作的cpu，不用
			goto out_bc;

		/*
		 * If we have a cpu local device already, do not replace it
		 * by a non cpu local device
		 */
		if (curdev && cpumask_equal(curdev->cpumask, cpumask_of(cpu)))//lux 已经有了，不换
			goto out_bc;
	}

	/*
	 * If we have an active device, then check the rating and the oneshot
	 * feature.
	 */
	if (curdev) {
		/*
		 * Prefer one shot capable devices !
		 */
		if ((curdev->features & CLOCK_EVT_FEAT_ONESHOT) &&
		    !(newdev->features & CLOCK_EVT_FEAT_ONESHOT))
			goto out_bc;//lux 老设备好，不换
		/*
		 * Check the rating
		 */
		if (curdev->rating >= newdev->rating)
			goto out_bc;//lux 老设备好，不换
	}

	//lux 新设备好，使用新设备
	/*
	 * Replace the eventually existing device by the new
	 * device. If the current device is the broadcast device, do
	 * not give it back to the clockevents layer !
	 */
	if (tick_is_broadcast_device(curdev)) {
		clockevents_shutdown(curdev);//lux 关闭老设备
		curdev = NULL;
	}
	clockevents_exchange_device(curdev, newdev);//lux 更新设备
	tick_setup_device(td, newdev, cpu, cpumask_of(cpu));
	if (newdev->features & CLOCK_EVT_FEAT_ONESHOT)
		tick_oneshot_notify();

	spin_unlock_irqrestore(&tick_device_lock, flags);
	return NOTIFY_STOP;

out_bc:
	/*
	 * Can the new device be used as a broadcast device ?
	 */
	if (tick_check_broadcast_device(newdev))
		ret = NOTIFY_STOP;

	spin_unlock_irqrestore(&tick_device_lock, flags);

	return ret;
}

/*
 * Transfer the do_timer job away from a dying cpu.
 *
 * Called with interrupts disabled.
 */
static void tick_handover_do_timer(int *cpup)
{
	if (*cpup == tick_do_timer_cpu) {
		int cpu = cpumask_first(cpu_online_mask);

		tick_do_timer_cpu = (cpu < nr_cpu_ids) ? cpu :
			TICK_DO_TIMER_NONE;
	}
}

/*
 * Shutdown an event device on a given cpu:
 *
 * This is called on a life CPU, when a CPU is dead. So we cannot
 * access the hardware device itself.
 * We just set the mode and remove it from the lists.
 */
static void tick_shutdown(unsigned int *cpup)
{
	struct tick_device *td = &per_cpu(tick_cpu_device, *cpup);
	struct clock_event_device *dev = td->evtdev;
	unsigned long flags;

	spin_lock_irqsave(&tick_device_lock, flags);
	td->mode = TICKDEV_MODE_PERIODIC;
	if (dev) {
		/*
		 * Prevent that the clock events layer tries to call
		 * the set mode function!
		 */
		dev->mode = CLOCK_EVT_MODE_UNUSED;
		clockevents_exchange_device(dev, NULL);
		td->evtdev = NULL;
	}
	spin_unlock_irqrestore(&tick_device_lock, flags);
}

static void tick_suspend(void)
{
	struct tick_device *td = &__get_cpu_var(tick_cpu_device);
	unsigned long flags;

	spin_lock_irqsave(&tick_device_lock, flags);
	clockevents_shutdown(td->evtdev);
	spin_unlock_irqrestore(&tick_device_lock, flags);
}

static void tick_resume(void)
{
	struct tick_device *td = &__get_cpu_var(tick_cpu_device);
	unsigned long flags;
	int broadcast = tick_resume_broadcast();

	spin_lock_irqsave(&tick_device_lock, flags);
	clockevents_set_mode(td->evtdev, CLOCK_EVT_MODE_RESUME);

	if (!broadcast) {
		if (td->mode == TICKDEV_MODE_PERIODIC)
			tick_setup_periodic(td->evtdev, 0);
		else
			tick_resume_oneshot();
	}
	spin_unlock_irqrestore(&tick_device_lock, flags);
}
/*lux clock event事件回调，比如有新设备加入了，设备挂起了，等等
 * 分层：
 * |---------------------------|
 * |	timer service		   |
 * |---------------------------|
 * |	tick devive			   |
 * |---------------------------|
 * |	clock event device	   |
 * |---------------------------|
 * |	clock source		   |
 * |---------------------------|
 *
 * clock event层设备有变更的时候，会通知tick device层。
 */

/*
 * Notification about clock event devices
 */
static int tick_notify(struct notifier_block *nb, unsigned long reason,
			       void *dev)
{
	switch (reason) {

	case CLOCK_EVT_NOTIFY_ADD:
		return tick_check_new_device(dev);

	case CLOCK_EVT_NOTIFY_BROADCAST_ON:
	case CLOCK_EVT_NOTIFY_BROADCAST_OFF:
	case CLOCK_EVT_NOTIFY_BROADCAST_FORCE:
		tick_broadcast_on_off(reason, dev);
		break;

	case CLOCK_EVT_NOTIFY_BROADCAST_ENTER:
	case CLOCK_EVT_NOTIFY_BROADCAST_EXIT:
		tick_broadcast_oneshot_control(reason);
		break;

	case CLOCK_EVT_NOTIFY_CPU_DYING:
		tick_handover_do_timer(dev);
		break;

	case CLOCK_EVT_NOTIFY_CPU_DEAD:
		tick_shutdown_broadcast_oneshot(dev);
		tick_shutdown_broadcast(dev);
		tick_shutdown(dev);
		break;

	case CLOCK_EVT_NOTIFY_SUSPEND:
		tick_suspend();
		tick_suspend_broadcast();
		break;

	case CLOCK_EVT_NOTIFY_RESUME:
		tick_resume();
		break;

	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block tick_notifier = {
	.notifier_call = tick_notify,
};
/*lux tick组件初始化，（general clockevent framework）
 *
 */
 
/**
 * tick_init - initialize the tick control
 *
 * Register the notifier with the clockevents framework
 */
void __init tick_init(void)
{
	clockevents_register_notifier(&tick_notifier);//lux 注册clock event事件回调，有事件的时候会通知
}
