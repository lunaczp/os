/**lux console 控制台/显示器操作类
 * 
 * 关于csi
 * 		ANSI escape sequences are a standard for in-band signaling to control the cursor location, color, 
 * and other options on video text terminals. Certain sequences of bytes, most starting with Esc and '[', 
 * are embedded into the text, which the terminal looks for and interprets as commands, not as character codes.
 * CSI sequences:
 * 		The ESC [ is followed by any number (including none) of "parameter bytes" in the range 0x30–0x3F (ASCII 0–9:;<=>?), 
 * then by any number of "intermediate bytes" in the range 0x20–0x2F (ASCII space and !"#$%&'()*+,-./), 
 * then finally by a single "final byte" in the range 0x40–0x7E (ASCII @A–Z[\]^_`a–z{|}~)
 * 
 * see:https://en.wikipedia.org/wiki/ANSI_escape_code
 * see console.lux.md
 */ 
/*
 *  linux/kernel/console.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *	console.c
 *
 * This module implements the console io functions
 *	'void con_init(void)'
 *	'void con_write(struct tty_queue * queue)'
 * Hopefully this will be a rather complete VT102 implementation.
 *
 * Beeping thanks to John T Kohl.
 */

/*
 *  NOTE!!! We sometimes disable and enable interrupts for a short while
 * (to put a word in video IO), but this will work even for keyboard
 * interrupts. We know interrupts aren't enabled when getting a keyboard
 * interrupt, as we use trap-gates. Hopefully all is well.
 */

/*
 * Code to check for different video-cards mostly by Galen Hunt,
 * <g-hunt@ee.utah.edu>
 */

#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <asm/system.h>

/*
 * These are set up by the setup-routine at boot-time:
 */
/*lux see setup.s:39*/
#define ORIG_X			(*(unsigned char *)0x90000)
#define ORIG_Y			(*(unsigned char *)0x90001)
#define ORIG_VIDEO_PAGE		(*(unsigned short *)0x90004)
#define ORIG_VIDEO_MODE		((*(unsigned short *)0x90006) & 0xff)
#define ORIG_VIDEO_COLS 	(((*(unsigned short *)0x90006) & 0xff00) >> 8)
#define ORIG_VIDEO_LINES	(25)
#define ORIG_VIDEO_EGA_AX	(*(unsigned short *)0x90008)
#define ORIG_VIDEO_EGA_BX	(*(unsigned short *)0x9000a)
#define ORIG_VIDEO_EGA_CX	(*(unsigned short *)0x9000c)

#define VIDEO_TYPE_MDA		0x10	/* Monochrome Text Display	*/
#define VIDEO_TYPE_CGA		0x11	/* CGA Display 			*/
#define VIDEO_TYPE_EGAM		0x20	/* EGA/VGA in Monochrome Mode	*/
#define VIDEO_TYPE_EGAC		0x21	/* EGA/VGA in Color Mode	*/

#define NPAR 16

extern void keyboard_interrupt(void);

static unsigned char	video_type;		/* Type of display being used	*/
static unsigned long	video_num_columns;	/* Number of text columns	*/
static unsigned long	video_size_row;		/* Bytes per row		*/
static unsigned long	video_num_lines;	/* Number of test lines		*/
static unsigned char	video_page;		/* Initial video page		*/
static unsigned long	video_mem_start;	/* Start of video RAM		*/
static unsigned long	video_mem_end;		/* End of video RAM (sort of)	*/
static unsigned short	video_port_reg;		/* Video register select port	*/
static unsigned short	video_port_val;		/* Video register value port	*/
static unsigned short	video_erase_char;	/* Char+Attrib to erase with	*/

static unsigned long	origin;		/* Used for EGA/VGA fast scroll	*/
static unsigned long	scr_end;	/* Used for EGA/VGA fast scroll	*/
static unsigned long	pos; //lux 显存指针，对*pos的操作会直接显示出来。
static unsigned long	x,y;//lux 光标x，y
static unsigned long	top,bottom;//lux 滚动时，头尾行号
static unsigned long	state=0;//lux ascii转义字符处理状态
static unsigned long	npar,par[NPAR];//lux ascii转义字符处理中间存储
static unsigned long	ques=0;//lux question mark(?)
static unsigned char	attr=0x07;

static void sysbeep(void);

/*
 * this is what the terminal answers to a ESC-Z or csi0c
 * query (= vt100 response).
 */
#define RESPONSE "\033[?1;2c"//lux 模拟vt100返回值

/* NOTE! gotoxy thinks x==video_num_columns is ok */
static inline void gotoxy(unsigned int new_x,unsigned int new_y)
{
	if (new_x > video_num_columns || new_y >= video_num_lines)
		return;
	x=new_x;
	y=new_y;
	pos=origin + y*video_size_row + (x<<1);
}

static inline void set_origin(void)
{
	cli();
	outb_p(12, video_port_reg);
	outb_p(0xff&((origin-video_mem_start)>>9), video_port_val);
	outb_p(13, video_port_reg);
	outb_p(0xff&((origin-video_mem_start)>>1), video_port_val);
	sti();
}
//lux 向上滚动一行
static void scrup(void)
{
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
		if (!top && bottom == video_num_lines) {
			origin += video_size_row;
			pos += video_size_row;
			scr_end += video_size_row;
			if (scr_end > video_mem_end) {
				__asm__("cld\n\t"
					"rep\n\t"
					"movsl\n\t"
					"movl _video_num_columns,%1\n\t"
					"rep\n\t"
					"stosw"
					::"a" (video_erase_char),
					"c" ((video_num_lines-1)*video_num_columns>>1),
					"D" (video_mem_start),
					"S" (origin)
					:"cx","di","si");
				scr_end -= origin-video_mem_start;
				pos -= origin-video_mem_start;
				origin = video_mem_start;
			} else {
				__asm__("cld\n\t"
					"rep\n\t"
					"stosw"
					::"a" (video_erase_char),
					"c" (video_num_columns),
					"D" (scr_end-video_size_row)
					:"cx","di");
			}
			set_origin();
		} else {
			__asm__("cld\n\t"
				"rep\n\t"
				"movsl\n\t"
				"movl _video_num_columns,%%ecx\n\t"
				"rep\n\t"
				"stosw"
				::"a" (video_erase_char),
				"c" ((bottom-top-1)*video_num_columns>>1),
				"D" (origin+video_size_row*top),
				"S" (origin+video_size_row*(top+1))
				:"cx","di","si");
		}
	}
	else		/* Not EGA/VGA */
	{
		__asm__("cld\n\t"
			"rep\n\t"
			"movsl\n\t"
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*top),
			"S" (origin+video_size_row*(top+1))
			:"cx","di","si");
	}
}
//lux 向下滚动一行
static void scrdown(void)
{
	if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
	{
		__asm__("std\n\t"
			"rep\n\t"
			"movsl\n\t"
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*bottom-4),
			"S" (origin+video_size_row*(bottom-1)-4)
			:"ax","cx","di","si");
	}
	else		/* Not EGA/VGA */
	{
		__asm__("std\n\t"
			"rep\n\t"
			"movsl\n\t"
			"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
			"movl _video_num_columns,%%ecx\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((bottom-top-1)*video_num_columns>>1),
			"D" (origin+video_size_row*bottom-4),
			"S" (origin+video_size_row*(bottom-1)-4)
			:"ax","cx","di","si");
	}
}

static void lf(void)
{//lux 换行
	if (y+1<bottom) {
		y++;
		pos += video_size_row;
		return;
	}
	scrup();//lux 滚屏
}
//lux 光标在同列上移一行
static void ri(void)
{
	if (y>top) {
		y--;
		pos -= video_size_row;
		return;
	}
	scrdown();
}

static void cr(void)
{//lux 换行
	pos -= x<<1;
	x=0;
}

static void del(void)
{//lux 删除
	if (x) {
		pos -= 2;
		x--;
		*(unsigned short *)pos = video_erase_char;
	}
}

/**lux https://en.wikipedia.org/wiki/ANSI_escape_code
 * Code:	CSI n J
 * Name:	ED – Erase in Display
 * Effect:	clears part of the screen.
 * 			If n is 0 (or missing), clear from cursor to end of screen. 
 * 			If n is 1, clear from cursor to beginning of the screen.
 * 			If n is 2, clear entire screen (and moves cursor to upper left on DOS ANSI.SYS).
 * 			If n is 3, clear entire screen and delete all lines saved in the scrollback buffer (this feature was added for xterm and is supported by other terminal applications).
 */
static void csi_J(int par)
{
	long count __asm__("cx");
	long start __asm__("di");

	switch (par) {
		case 0:	/* erase from cursor to end of display */
			count = (scr_end-pos)>>1;
			start = pos;
			break;
		case 1:	/* erase from start to cursor */
			count = (pos-origin)>>1;
			start = origin;
			break;
		case 2: /* erase whole display */
			count = video_num_columns * video_num_lines;
			start = origin;
			break;
		default:
			return;
	}
	__asm__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c" (count),
		"D" (start),"a" (video_erase_char)
		:"cx","di");
}

/**lux https://en.wikipedia.org/wiki/ANSI_escape_code
 * Code:	CSI n K
 * Name:	EL – Erase in Line
 * Effect:	Erases part of the line. Cursor position does not change.
 * 			If n is 0 (or missing), clear from cursor to the end of the line. 
 * 			If n is 1, clear from cursor to beginning of the line.
 * 			If n is 2, clear entire line.
 */
static void csi_K(int par)
{
	long count __asm__("cx");
	long start __asm__("di");

	switch (par) {
		case 0:	/* erase from cursor to end of line */
			if (x>=video_num_columns)
				return;
			count = video_num_columns-x;
			start = pos;
			break;
		case 1:	/* erase from start of line to cursor */
			start = pos - (x<<1);
			count = (x<video_num_columns)?x:video_num_columns;
			break;
		case 2: /* erase whole line */
			start = pos - (x<<1);
			count = video_num_columns;
			break;
		default:
			return;
	}
	__asm__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c" (count),
		"D" (start),"a" (video_erase_char)
		:"cx","di");
}

/**lux https://en.wikipedia.org/wiki/ANSI_escape_code
 * Code:	CSI n m
 * Name:	SGR – Select Graphic Rendition
 * Effect:	Sets SGR parameters, including text color. After CSI can be zero or more parameters separated with 
 * 			semicolon. If none, CSI m is treated as CSI 0 m (reset / normal).
 */
void csi_m(void)
{
	int i;

	for (i=0;i<=npar;i++)
		switch (par[i]) {
			case 0:attr=0x07;break;
			case 1:attr=0x0f;break;
			case 4:attr=0x0f;break;
			case 7:attr=0x70;break;
			case 27:attr=0x07;break;
		}
}

static inline void set_cursor(void)
{
	cli();
	outb_p(14, video_port_reg);
	outb_p(0xff&((pos-video_mem_start)>>9), video_port_val);
	outb_p(15, video_port_reg);
	outb_p(0xff&((pos-video_mem_start)>>1), video_port_val);
	sti();
}
//lux 响应ESC Z 查询
static void respond(struct tty_struct * tty)
{
	char * p = RESPONSE;

	cli();
	while (*p) {
		PUTCH(*p,tty->read_q);
		p++;
	}
	sti();
	copy_to_cooked(tty);
}

static void insert_char(void)
{
	int i=x;
	unsigned short tmp, old = video_erase_char;
	unsigned short * p = (unsigned short *) pos;

	while (i++<video_num_columns) {
		tmp=*p;
		*p=old;
		old=tmp;
		p++;
	}
}

static void insert_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom = video_num_lines;
	scrdown();
	top=oldtop;
	bottom=oldbottom;
}

static void delete_char(void)
{
	int i;
	unsigned short * p = (unsigned short *) pos;

	if (x>=video_num_columns)
		return;
	i = x;
	while (++i < video_num_columns) {
		*p = *(p+1);
		p++;
	}
	*p = video_erase_char;
}

static void delete_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom = video_num_lines;
	scrup();
	top=oldtop;
	bottom=oldbottom;
}
//lux 光标处插入nr个字符
static void csi_at(unsigned int nr)
{
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;
	while (nr--)
		insert_char();
}
//lux 光标处插入nr个行
static void csi_L(unsigned int nr)
{
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)
		nr = 1;
	while (nr--)
		insert_line();
}
//lux 光标处删除nr个字符
static void csi_P(unsigned int nr)
{
	if (nr > video_num_columns)
		nr = video_num_columns;
	else if (!nr)
		nr = 1;
	while (nr--)
		delete_char();
}
//lux 光标处删除nr个行
static void csi_M(unsigned int nr)
{
	if (nr > video_num_lines)
		nr = video_num_lines;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_line();
}

static int saved_x=0;
static int saved_y=0;
//lux 保存光标位置
static void save_cur(void)
{
	saved_x=x;
	saved_y=y;
}
//lux 恢复光标位置
static void restore_cur(void)
{
	gotoxy(saved_x, saved_y);
}
//lux console 写
void con_write(struct tty_struct * tty)
{
	int nr;
	char c;

	nr = CHARS(tty->write_q);
	while (nr--) {
		GETCH(tty->write_q,c);
		switch(state) {
			case 0:
				if (c>31 && c<127) {//lux 可打印字符
					if (x>=video_num_columns) {
						x -= video_num_columns;
						pos -= video_size_row;
						lf();
					}
					__asm__("movb _attr,%%ah\n\t"
						"movw %%ax,%1\n\t"
						::"a" (c),"m" (*(short *)pos)
						:"ax");//lux 显示到屏幕指定位置（pos）
					pos += 2;
					x++;
				} else if (c==27)//lux escape sequence 
					state=1;
				else if (c==10 || c==11 || c==12)//lux 10:lf 11:vt(vertical tab) 12:ff(NP form feed,new page)
					lf();
				else if (c==13)//lux carriage return
					cr();
				else if (c==ERASE_CHAR(tty))//lux del
					del();
				else if (c==8) {//lux backspace
					if (x) {
						x--;
						pos -= 2;
					}
				} else if (c==9) {//lux horizontal tab
					c=8-(x&7);
					x += c;
					pos += c<<1;
					if (x>video_num_columns) {
						x -= video_num_columns;
						pos -= video_size_row;
						lf();
					}
					c=9;
				} else if (c==7)//lux bell
					sysbeep();
				break;
			case 1:
				state=0;
				if (c=='[')//lux csi 控制序列 ESC + [ + ...
					state=2;
				else if (c=='E')//lux escape sequence ESC E，下一行行首
					gotoxy(0,y+1);
				else if (c=='M')//lux ESC M 光标上移一行
					ri();
				else if (c=='D')//lux ESC D 光标下移一行
					lf();
				else if (c=='Z')//lux ESC Z 设备属性查询
					respond(tty);
				else if (x=='7')//lux Save cursor position and attributes see http://ascii-table.com/ansi-escape-sequences-vt-100.php
					save_cur();
				else if (x=='8')//lux Restore cursor position and attributes
					restore_cur();
				break;
			case 2://lux csi 控制序列处理
				for(npar=0;npar<NPAR;npar++)
					par[npar]=0;
				npar=0;
				state=3;
				if (ques=(c=='?'))
					break;
			case 3:
				if (c==';' && npar<NPAR-1) {
					npar++;
					break;
				} else if (c>='0' && c<='9') {
					par[npar]=10*par[npar]+c-'0';
					break;
				} else state=4;
			case 4:
				state=0;
				switch(c) {//lux https://en.wikipedia.org/wiki/ANSI_escape_code
					case 'G': case '`'://lux Moves the cursor to column n
						if (par[0]) par[0]--;
						gotoxy(par[0],y);
						break;
					case 'A'://lux cursor up
						if (!par[0]) par[0]++;
						gotoxy(x,y-par[0]);
						break;
					case 'B': case 'e'://lux cursor down
						if (!par[0]) par[0]++;
						gotoxy(x,y+par[0]);
						break;
					case 'C': case 'a'://lux cursor forward
						if (!par[0]) par[0]++;
						gotoxy(x+par[0],y);
						break;
					case 'D'://lux cursor back
						if (!par[0]) par[0]++;
						gotoxy(x-par[0],y);
						break;
					case 'E'://lux cursor next line
						if (!par[0]) par[0]++;
						gotoxy(0,y+par[0]);
						break;
					case 'F'://lux cursor previous line
						if (!par[0]) par[0]++;
						gotoxy(0,y-par[0]);
						break;
					case 'd':
						if (par[0]) par[0]--;
						gotoxy(x,par[0]);
						break;
					case 'H': case 'f'://lux Move cursor to screen location x,y
						if (par[0]) par[0]--;
						if (par[1]) par[1]--;
						gotoxy(par[1],par[0]);
						break;
					case 'J':
						csi_J(par[0]);//lux clear screen
						break;
					case 'K':
						csi_K(par[0]);//lux clear line
						break;
					case 'L'://lux 插入n行
						csi_L(par[0]);
						break;
					case 'M'://lux 删除n行
						csi_M(par[0]);
						break;
					case 'P'://lux 删除n个字符
						csi_P(par[0]);
						break;
					case '@'://lux 插入n个字符
						csi_at(par[0]);
						break;
					case 'm'://lux SGR模式控制
						csi_m();
						break;
					case 'r':
						if (par[0]) par[0]--;
						if (!par[1]) par[1] = video_num_lines;
						if (par[0] < par[1] &&
						    par[1] <= video_num_lines) {
							top=par[0];
							bottom=par[1];
						}
						break;
					case 's'://lux  Save Cursor Position
						save_cur();
						break;
					case 'u'://lux  Restore Cursor Position
						restore_cur();
						break;
				}
		}
	}
	set_cursor();
}
/**lux 显示初始化
 * 1. 设定显示参数
 * 2. 打开键盘中断
 */
/*
 *  void con_init(void);
 *
 * This routine initalizes console interrupts, and does nothing
 * else. If you want the screen to clear, call tty_write with
 * the appropriate escape-sequece.
 *
 * Reads the information preserved by setup.s to determine the current display
 * type and sets everything accordingly.
 */
void con_init(void)
{
	register unsigned char a;
	char *display_desc = "????";
	char *display_ptr;

	video_num_columns = ORIG_VIDEO_COLS;
	video_size_row = video_num_columns * 2;
	video_num_lines = ORIG_VIDEO_LINES;
	video_page = ORIG_VIDEO_PAGE;
	video_erase_char = 0x0720;
	
	if (ORIG_VIDEO_MODE == 7)			/* Is this a monochrome display? */ /*lux 单色*/
	{
		video_mem_start = 0xb0000;
		video_port_reg = 0x3b4;
		video_port_val = 0x3b5;
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAM;
			video_mem_end = 0xb8000;
			display_desc = "EGAm";
		}
		else
		{
			video_type = VIDEO_TYPE_MDA;
			video_mem_end	= 0xb2000;
			display_desc = "*MDA";
		}
	}
	else								/* If not, it is color. */
	{
		video_mem_start = 0xb8000;
		video_port_reg	= 0x3d4;
		video_port_val	= 0x3d5;
		if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10)
		{
			video_type = VIDEO_TYPE_EGAC;
			video_mem_end = 0xbc000;
			display_desc = "EGAc";
		}
		else
		{
			video_type = VIDEO_TYPE_CGA;
			video_mem_end = 0xba000;
			display_desc = "*CGA";
		}
	}

	/* Let the user known what kind of display driver we are using */
	
	display_ptr = ((char *)video_mem_start) + video_size_row - 8;
	while (*display_desc)/*lux 直接写显存*/
	{
		*display_ptr++ = *display_desc++;
		display_ptr++;
	}
	
	/* Initialize the variables used for scrolling (mostly EGA/VGA)	*/
	
	origin	= video_mem_start;
	scr_end	= video_mem_start + video_num_lines * video_size_row;
	top	= 0;
	bottom	= video_num_lines;

	gotoxy(ORIG_X,ORIG_Y);
	set_trap_gate(0x21,&keyboard_interrupt);/*lux 设置键盘中断回调*/
	outb_p(inb_p(0x21)&0xfd,0x21);/*lux 打开键盘中断*/
	a=inb_p(0x61);/*lux 键盘端口0x61，先设置禁止，再开启。（复位）*/
	outb_p(a|0x80,0x61);
	outb(a,0x61);
}
/* from bsd-net-2: */

void sysbeepstop(void)
{
	/* disable counter 2 */
	outb(inb_p(0x61)&0xFC, 0x61);
}

int beepcount = 0;

static void sysbeep(void)
{
	/* enable counter 2 */
	outb_p(inb_p(0x61)|3, 0x61);
	/* set command for counter 2, 2 byte write */
	outb_p(0xB6, 0x43);
	/* send 0x637 for 750 HZ */
	outb_p(0x37, 0x42);
	outb(0x06, 0x42);
	/* 1/8 second */
	beepcount = HZ/8;	
}
