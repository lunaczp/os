# TTY
Linux下的终端设备统称为TTY。
> 终端是一种字符设备，它有多种类型。我们通常用tty来简称各种类型的终端设备。tty是Teletype的缩写。Teletype是一种由Teletype公司生产的最早出现的终端设备。

- 串行端口终端(/dev/ttySn)
- 伪终端（/dev/ptyp）
- 控制终端（/dev/tty）
- 控制台（/dev/ttyn，/dev/console）

Ref:[Difference between tty,ttyx,console](https://unix.stackexchange.com/questions/60641/linux-difference-between-dev-console-dev-tty-and-dev-tty0)
```
/dev/tty        Current TTY device
/dev/console    System console
/dev/tty0       Current virtual console
```
> In the good old days /dev/console was System Administrator console. And TTYs were users' serial devices attached to a server. Now /dev/console and /dev/tty0 represent current display and usually are the same. You can override it for example by adding console=ttyS0 to grub.conf. After that your /dev/tty0 is a monitor and /dev/console is /dev/ttyS0.