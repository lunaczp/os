# Timer

# Timer
关于Linux的时间与定时器

# 基础
## 系统心跳/时钟中断
Linux的任务执行是基于时间片的，系统需要定期进行任务切换。这个周期就是系统心跳。一般地，是由硬件周期产生，即时钟中断。
比如，Linux0.11的时候，系统使用8253芯片，设置其输出频率为100,即每10ms产生一次中断，此时，系统的心跳就是100Hz。每10ms，系统处理一次时钟中断，进行任务切换都操作。

另外，系统维护一个`jiffies`字段，每次时钟中断加一，用来计算系统时间。

## 产生时钟的硬件
- 最早期的8253芯片，频率1.193180MHz
- PIT(Programmable Interval Timer)，早期的 PIT 设备是 8254，现在多数可以集成在 Intel 的 I/O Control Hub 电路中，可以通过端口 0x40~0x43 访问 PIT。系统利用 PIT 来产生周期性的时钟中断，时钟中断通过 8259A 的 IRQ0 向 CPU 报告。它的精度不高，其入口 clock 的频率为 1MHz，理论上能产生的最高时钟频率略小于 0.5MHz。实际系统往往使用 100 或者 1000Hz 的 PIT。
- HPET (High Precision Event Timer)，PIT 的精度较低，HPET 被设计来替代 PIT 提供高精度时钟中断（至少 10MHz）。它是由微软和 Intel 联合开发的。一个 HPET 包括了一个固定频率的数值增加的计数器以及 3 到 32 个独立的计时器，这每一个计时器有包涵了一个比较器和一个寄存器（保存一个数值，表示触发中断的时机）。每一个比较器都比较计数器中的数值和寄存器中的数值，当这两个数值相等时，将产生一个中断。
- APIC Timer (Advanced Programmable Interrupt Controller Timer)，是早期 PIC 中断控制器的升级，主要用于多处理器系统，用来支持复杂的中断控制以及多 CPU 之间的中断传递。APIC Timer 集成在 APIC 芯片中，用来提供高精度的定时中断，中断频率至少可以达到总线频率。系统中的每个 CPU 上都有一个 APIC Timer，而 PIT 则是由系统中所有的 CPU 共享的。Per CPU 的 Timer 简化了系统设计，目前 APIC Timer 已经集成到了所有 Intel x86 处理器中。

## 定时器
不管是内核还是用户程序，都需要定时执行一些任务，这就需要内核来提供定时服务。

### 原始阶段
这个阶段，系统维护一个定时任务列表，每次时钟中断的时候，遍历检查一次，对于到期的任务执行之。由于它基于时钟中断，那么它的精度也就是1/HZ，比如HZ=100，那么精度就是10ms。

这个阶段定时器精度不高，而且在定时器过多的时候，遍历的开销会升高，但基本够用。

### 时间轮阶段
随着定时器的增多，简单的链表实现效率太低。这时候出现了时间轮算法。其算法参考后文。

时间轮算法提升了一定效率，但是它还是基于系统时钟的，精度并没有提升。这段时间，随着一些应用对精度的要求，系统的频率HZ提高到了1000，也就是每1ms中断一次。这样在一定程度上提升了精度。但HZ并不是越大越好，因为越大系统开销越大，系统效率越低。为了提升定时器精度而增大系统频率，是不得已而为之。

### 高精度定时器阶段
随着对定时器精度的要求，出现了高精度定时器。在说明这之前，需要先说明以下 _Generic Timer Framework_

https://www.ibm.com/developerworks/cn/linux/1308_liuming_linuxtime4/index.html
> ## Generic Timer Framework
>
> 早期 Linux 内核也需要支持众多不同的时钟硬件设备，但内核本身对这些设备的使用相对简单。内核将硬件的不同操作封装在 Arch 层里面。比如 x86 体系结构下，设置 PIT(Programmable Interrupt Timer) 是在 8259 芯片初始化时完成的，调用的 API 名字叫做 setup_pit_timer()，而在其他体系结构中，没有 8259，其初始化 time_init()中的具体实现又有所不同，会采用不同的 API 命名和不同的表示 Timer 硬件的数据结构。因为早期 Linux 上只需要做一次时钟初始化，操作具体硬件的次数有限，因此这种不同体系结构用不同实现细节的做法没有什么问题。
>
> 新的内核能够支持 tickless 模式，即当内核空闲时为了省电而关闭时钟中断。为此，内核需要频繁操作 Timer 硬件，在这种情况下，采用统一的抽象层有助于代码的维护。这便是 Generic Timer Frame，它将各种不同硬件抽象为三个统一的数据结构：
>
> - Clock Source，由 struct clocksource 表示。这个数据结构主要用来抽象那些能够提供计时功能的系统硬件，比如 RTC>(Real Time Clock)、TSC(Time Stamp Counter) 等。
> - Clock Event Device，由 struct clock_event_device 表示。这个数据结构主要用来封装和抽象那些能提供定时中断能力的系统硬件，比如 HPET 等。
> - Tick Device，由 struct tick_device 表示。这个数据结构建立在 clock event device 之上，专门用来表示产生 tick 的设备。tick 是一个定时中断。因此归根结底需要一个 Clock Event Device 来完成，但 Clock Event Device 不仅可以用来提供 tick，在高精度 Timer 模式下，还用来提供其他功能。
>
> Generic Timer Frame 把各种不同时间硬件的区别同上层软件隔离开来，使得时间系统能够方便地支持新的时钟硬件，而无需大量修改硬件无关代码。


有了Generic Timer Framework之后，
- 底层所有的时钟硬件都被注册为Clock Source，向Clock Event Device提供输入
- Clock Event Decive向上层提供定时功能


高精度定时器依赖于拥有定时功能的硬件（如HPET）来提供纳秒级别的定时功能。
- 它自身维护一个红黑树，包含所有的定时器任务。
- 注册一个`hrtimer_interrupt`来接收硬件中断

每新增一个定时器任务，都把它加入红黑树，然后用最早触发的定时器，来编程硬件，让硬件定时产生中断（而不是传统的周期产生中断）。当中断产生，回调函数检测到期的任务执行之。同时计算下一个要到期的任务，用它的到期时间再次编程硬件，然后等待下一次中断的产生。比如继续下去。

更多请参考[hrtimer](hrtimer.lux.md)


# Timer History
内核需要提供的时间功能有：
- 提供tick中断
- 维护系统时间
- 提供给上层软件定时器

## First Phase
Linux使用硬件中断产生时钟信号。通过这个时钟信号，内核实现所有事件相关的功能。一般地，这个频率是100HZ，由 PIT (Programmable Interrupt Timer) 提供。也就是说，内核的时间精度是1s/100=10ms。
内核维护一个`jiffies`变量，每次时钟中断`++1`，也就是每10ms加1。

10ms应该说是精度较低，但是在早期而言是够用的。

这个阶段：
- tick中断
	- 由PIT提供
- 系统时间
	- 启动时候由RTC提供（记录为`xtime`），配合`jiffies`就可以得到当前时间
- 软件定时器
	- 维护一个列表，每次时钟中断，轮训列表，检查到期任务，触发之。

## Second Phase
第一阶段的定时器实现有些问题，因为操作本身（包括链表的维护）是有延迟的，当维护的定时器很多，延迟会更大，造成定时器不准确。这时候出现了 __时间轮__ 算法。

时间轮算法，每个时间点都维护一个队列，每次时钟中断，都只需要处理对应的时间点的队列即可，比如定义`q[8]`，维护8s内每1s的定时器。但这个的问题在于，`q[n]`的`n`有限制。比如我要在1天后触发一个操作，那么这个`n`至少是`86400`。
这是不现实的，而且没有可扩展性，效率也很低。

这时候，出现了改进的时间轮算法：Hierarchy时间轮算法。对于上面的例子，不再维护一个每秒的数组，还是维护一个级联数组，`q[h][m][s]`，所有的任务都挂载到某一个时间点的队列。而当每次时钟中断，都会从秒这一级处理，依次触发到期任务。
这样，可表示的范围大为增加，同时操作的时间复杂度依然很低。

对Linux实现而言，时间的最小单位是`jiffy`，是一个32bit的整型。它被分为5个部分，作为一个5级数组，来实现时间轮算法。

Linux2.6之前，都是使用该算法来实现上层的定时器服务。如：
- 动态timer
	- `init_timer`, `add_timer()`, `del_timer`
- Interval Timer
- Posix Timer

时间轮算法也是有问题的，主要是它的级联操作有可能会引起不确定的耗时。对于高精度的需求，不能够满足。

另外，目前为止的算法都是基于系统时钟的。而100HZ（每10ms触发）的时钟中断，显然精度不够。后来内核修改为1000HZ（每1ms触发）。但时钟频率的增大会给系统带来负担（开销过大，降低系统处理能力，都用来响应时钟中断了）。所以不能继续依靠时钟频率的调整。

## Third Phase （Hrtimer）
随着对高精度定时器的需求越来越多，从内核2.6.16开始，提供了高精度定时器（Hrtimer）

- Hrtimer不使用时间轮算法
- Hrtimer不依赖于系统时钟（`jiffies`）
- Hrtimer直接使用高精度定时器硬件触发
	- 高精度定时器硬件，可以指定比如10ns后触发，则10ns后会产生一个中断，而不是固定频率的时钟中断（所谓定时器，就是可以定时触发）

Hrtimer的实现简单说明：
> 我们所描述过的低精度定时器都是依赖系统定期产生的 tick 中断的。而高精度时钟模式下，定时器直接由高精度定时器硬件产生的中断触发。比如目前系统中有 3 个 hrtimer，其到期时间分别为 10ns、100ns 和 1000ns。添加第一个 hrtimer 时，系统通过当前默认的 clock_event_device 操作时钟硬件将其下一次中断触发时间设置为 10ns 之后；当 10ns 过去时，中断产生，通过系统的中断处理机制，最终会调用到 hrtimer_interrrupt() 函数，该函数从红黑树中得到所有到期的 Timer，并负责调用 hrtimer 数据结构中维护的用户处理函数（或者通过软中断执行用户指定操作）；hrtimer_interrupt 还从红黑树中读取下一个到期的 hrtimer，并且通过 clock_event_device 操作时钟硬件将下一次中断到期时间设置为 90ns 之后。如此反复操作。
>
> 这样就突破了 tick 的精度限制，用户操作可以精确到 ns 级别，当然中断依然存在延迟，这种延迟在几百个纳秒级别，还是比较高的精度。

再使用了Hrtimer后，系统还是需要一个定时触发的时钟中断来产生系统心跳。用Hrtimer可以很简单的实现。
> 在高精度时钟模式下，内核系统依然需要一个定时触发的 tick 中断，以便驱动任务切换等重要操作。可是我们在上一节看到，高精度时钟模式下，系统产生时间中断的间隔是不确定的，假如系统中没有创建任何 hrtimer，就不会有时钟中断产生了。但 Linux 内核必须要一个严格定时触发的 tick 中断。
>
>因此系统必须创建一个模拟 tick 时钟的特殊 hrtimer，并且该时钟按照 tick 的间隔时间（比如 10ms）定期启动自己，从而模拟出 tick 时钟，不过在 tickless 情况下，会跳过一些 tick。关于 tickless，和本文主旨无关，不再赘述。

自此，所有的timer都逐渐通过hrtimer来实现。
> 高精度时钟主要应用于实时系统。在用户层，实时时钟的编程接口就是我们在第一部分介绍的 POSIX Timer。本文的第三部分介绍了基于 2.6.16 之前内核的 POSIX Timer 实现细节。
>
>当 hrtimer 加入内核之后，POSIX Timer 的实现细节有一些改变，其中 per process 和 per thread 定时器的实现基本没有变化。但针对 CLOCK_REALTIME 和 CLOCK_MONOTONIC 两个时钟源的基本实现有所改变。以前它们依赖内核中的动态定时器实现，现在这类 Timer 都采用了新的 hrtimer。换句话说，每个时钟源为 CLOCK_REALTIME/CLOCK_MONOTONIC 的 POSIX Timer 都由一个内核 hrtimer 实现。
>
>传统的间隔 Timer 虽然不属于实时应用，也没有很高的时钟精度要求，但在新的内核中，间隔 Timer 也使用了 hrtimer，而非传统的动态 Timer。因此 setitimer 在内核中也不再由时间轮管理了。
>
>总体来说，用户请求的 Timer，无论是精度较低的间隔 Timer 还是精度高的 POSIX Timer，内核都采用 hrtimer 来支持。而由时间轮算法维护的内核动态 Timer 则仅仅在内核内部使用，比如一些驱动程序中还依旧使用 add_timer() 等动态 Timer 接口实现定时需求。


# 更多关于Linux 2.6的时间框架
## 硬件
PC 机里常见的时钟硬件有以下这些。

### RTC (Real Time Clock，实时时钟)

人们需要知道时间的时候，可以看看钟表。计算机系统中钟表类似的硬件就是外部时钟。它依靠主板上的电池，在系统断电的情况下，也能维持时钟的准确性。计算机需要知道时间的时候，就需要读取该时钟。

在 x86 体系中，这个时钟一般被称为 Real Time Clock。RTC 是主板上的一个 CMOS 芯片，比如 Motorola 146818，该芯片独立于 CPU 和其他芯片，可以通过 0x70 和 0x71 端口操作 RTC。RTC 可以周期性地在 IRQ 8 上触发中断，但精度很低，从 2HZ 到 8192HZ。


### TSC (Time Stamp Counter）

CPU 执行指令需要一个外部振荡器产生时钟信号，从 CLK 管脚输入。x86 提供了一个 TSC 寄存器，该寄存器的值在每次收到一个时钟信号时加一。比如 CPU 的主频为 1GHZ，则每一秒时间内，TSC 寄存器的值将增加 1G 次，或者说每一个纳秒加一次。x86 还提供了 rtdsc 指令来读取该值，因此 TSC 也可以作为时钟设备。TSC 提供了比 RTC 更高精度的时间，即纳秒级的时间精度。

### PIT (Programmable Interval Timer)

PIT 是 Programmable Interval Timer 的缩写，该硬件设备能定时产生中断。早期的 PIT 设备是 8254，现在多数可以集成在 Intel 的 I/O Control Hub 电路中，可以通过端口 0x40~0x43 访问 PIT。系统利用 PIT 来产生周期性的时钟中断，时钟中断通过 8259A 的 IRQ0 向 CPU 报告。它的精度不高，其入口 clock 的频率为 1MHz，理论上能产生的最高时钟频率略小于 0.5MHz。实际系统往往使用 100 或者 1000Hz 的 PIT。

### HPET (High Precision Event Timer)

PIT 的精度较低，HPET 被设计来替代 PIT 提供高精度时钟中断（至少 10MHz）。它是由微软和 Intel 联合开发的。一个 HPET 包括了一个固定频率的数值增加的计数器以及 3 到 32 个独立的计时器，这每一个计时器有包涵了一个比较器和一个寄存器（保存一个数值，表示触发中断的时机）。每一个比较器都比较计数器中的数值和寄存器中的数值，当这两个数值相等时，将产生一个中断。

### APIC Timer (Advanced Programmable Interrupt Controller Timer)

APIC ("Advanced Programmable Interrupt Controller") 是早期 PIC 中断控制器的升级，主要用于多处理器系统，用来支持复杂的中断控制以及多 CPU 之间的中断传递。APIC Timer 集成在 APIC 芯片中，用来提供高精度的定时中断，中断频率至少可以达到总线频率。系统中的每个 CPU 上都有一个 APIC Timer，而 PIT 则是由系统中所有的 CPU 共享的。Per CPU 的 Timer 简化了系统设计，目前 APIC Timer 已经集成到了所有 Intel x86 处理器中。

以上这些硬件仅仅是 x86 体系结构下常见的时间相关硬件，其他的体系结构如 mips、arm 等还有它们常用的硬件。这么多的硬件令人眼花缭乱，但其实无论这些硬件多么复杂，Linux 内核只需要两种功能：

- 一是定时触发中断的功能；
- 另一个是维护和读取当前时间的能力。

## 发展
近年来，随着 Linux 的广泛使用，对时间编程提出了更高的要求。实时应用、多媒体软件对时钟和定时器的精度要求不断提高，在早期 Linux 内核中，定时器所能支持的最高精度是一个 tick。为了提高时钟精度，人们只能提高内核的 HZ 值 (一个内核参数，代表内核时钟中断的频率)。更高的 HZ 值，意味着时钟中断更加频繁，内核要花更多的时间进行时钟处理。而内核的任何工作对于应用来说纯粹是无益的开销。当 HZ 值提高到 1000 之后，如果继续提高，Linux 的可用性将下降。

另外一方面，我们已看到，类似 HPET(High Precision Event Timer) 等系统硬件已经能够提供纳秒级别的时钟中断，如何利用这些高精度时钟硬件来提供更高精度的定时服务是这一部分的主要话题。

### 2.6.16 以来的新变化
在 2.6.16 之前，Linux 开发人员花了很多的努力试图在原有代码体系结构下实现高精度时钟，但这种努力被证明是徒劳的。

因此从 2.6.16 开始，RedHat 公司的 Ingo Molar 和 Thomas Gleixner 对时间系统进行了比较大的重构。引入了以下几个新的模块：

#### Generic Timer Framework

早期 Linux 内核也需要支持众多不同的时钟硬件设备，但内核本身对这些设备的使用相对简单。内核将硬件的不同操作封装在 Arch 层里面。比如 x86 体系结构下，设置 PIT(Programmable Interrupt Timer) 是在 8259 芯片初始化时完成的，调用的 API 名字叫做 setup_pit_timer()，而在其他体系结构中，没有 8259，其初始化 time_init()中的具体实现又有所不同，会采用不同的 API 命名和不同的表示 Timer 硬件的数据结构。因为早期 Linux 上只需要做一次时钟初始化，操作具体硬件的次数有限，因此这种不同体系结构用不同实现细节的做法没有什么问题。

新的内核能够支持 tickless 模式，即当内核空闲时为了省电而关闭时钟中断。为此，内核需要频繁操作 Timer 硬件，在这种情况下，采用统一的抽象层有助于代码的维护。这便是 Generic Timer Frame，它将各种不同硬件抽象为三个统一的数据结构：

- Clock Source，由 struct clocksource 表示。这个数据结构主要用来抽象那些能够提供计时功能的系统硬件，比如 RTC(Real Time Clock)、TSC(Time Stamp Counter) 等。
- Clock Event Device，由 struct clock_event_device 表示。这个数据结构主要用来封装和抽象那些能提供定时中断能力的系统硬件，比如 HPET 等。
- Tick Device，由 struct tick_device 表示。这个数据结构建立在 clock event device 之上，专门用来表示产生 tick 的设备。tick 是一个定时中断。因此归根结底需要一个 Clock Event Device 来完成，但 Clock Event Device 不仅可以用来提供 tick，在高精度 Timer 模式下，还用来提供其他功能。

Generic Timer Frame 把各种不同时间硬件的区别同上层软件隔离开来，使得时间系统能够方便地支持新的时钟硬件，而无需大量修改硬件无关代码。

#### 高精度定时器 hrtimer(High Resolution Timer)

高精度时钟不能建立在已有的时间轮算法上，虽然时间轮是一种有效的管理数据结构，但其 cascades 操作有不可预料的延迟。它更适于被称为"timeout”类型的低精度定时器，即不等触发便被取消的 Timer。这种情况下，cascades 可能造成的时钟到期延误不会有任何不利影响，因为根本等不到 cascades，换句话说，多数 Timer 都不会触发 cascades 操作。而高精度定时器的用户往往是需要等待其精确地被触发，执行对时间敏感的任务。因此 cascades 操作带来的延迟是无法接受的。所以内核开发人员不得不放弃时间轮算法，转而寻求其他的高精度时钟算法。最终，开发人员选择了内核中最常用的高性能查找算法红：黑树来实现 hrtimer。


# Ref
- [IBM developWorks:高精度Timer](https://www.ibm.com/developerworks/cn/linux/1308_liuming_linuxtime4/index.html)
- [IBM developWorks:时间硬件](https://www.ibm.com/developerworks/cn/linux/1307_liuming_linuxtime2/)