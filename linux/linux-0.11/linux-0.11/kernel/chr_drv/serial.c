/**lux 串口设备初始化程序 （标准：rs232)
 * 比如早期的鼠标、键盘
 * see: https://zh.wikipedia.org/wiki/RS-232
 */
/*
 *  linux/kernel/serial.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	serial.c
 *
 * This module implements the rs232 io functions
 *	void rs_write(struct tty_struct * queue);
 *	void rs_init(void);
 * and all interrupts pertaining to serial IO.
 */

#include <linux/tty.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/io.h>

#define WAKEUP_CHARS (TTY_BUF_SIZE/4)

extern void rs1_interrupt(void);
extern void rs2_interrupt(void);
//lux 指定端口初始化
static void init(int port)
{
	outb_p(0x80,port+3);	/* set DLAB of line control reg */
	outb_p(0x30,port);	/* LS of divisor (48 -> 2400 bps */
	outb_p(0x00,port+1);	/* MS of divisor */
	outb_p(0x03,port+3);	/* reset DLAB */
	outb_p(0x0b,port+4);	/* set DTR,RTS, OUT_2 */
	outb_p(0x0d,port+1);	/* enable all intrs but writes */
	(void)inb(port);	/* read data port to reset things (?) */
}
//lux rs初始化入口
void rs_init(void)
{
	set_intr_gate(0x24,rs1_interrupt);/*lux 8259A 串口1 irq4*/
	set_intr_gate(0x23,rs2_interrupt);/*lux 8259A 串口2 irq3*/
	init(tty_table[1].read_q.data);/*lux rs1 初始化*/
	init(tty_table[2].read_q.data);/*lux rs2 初始化*/
	outb(inb_p(0x21)&0xE7,0x21);/*lux 打开irq4（串口1） irq3（串口2） */
}
//lux rs写
/*
 * This routine gets called when tty_write has put something into
 * the write_queue. It must check wheter the queue is empty, and
 * set the interrupt register accordingly
 *
 *	void _rs_write(struct tty_struct * tty);
 */
void rs_write(struct tty_struct * tty)
{
	cli();
	if (!EMPTY(tty->write_q))
		outb(inb_p(tty->write_q.data+1)|0x02,tty->write_q.data+1);
	sti();
}
