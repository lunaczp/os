#ifndef _LINUX_WAIT_H
#define _LINUX_WAIT_H

#define WNOHANG		0x00000001
#define WUNTRACED	0x00000002

#define __WCLONE	0x80000000

#ifdef __KERNEL__
//lux 等待队列
struct wait_queue {
	struct task_struct * task;
	struct wait_queue * next;
};

#define WAIT_QUEUE_HEAD(x) ((struct wait_queue *)((x)-1))

static inline void init_waitqueue(struct wait_queue **q)
{
	*q = WAIT_QUEUE_HEAD(q);
}

static inline int waitqueue_active(struct wait_queue **q)
{
	struct wait_queue *head = *q;
	return head && head != WAIT_QUEUE_HEAD(q);
}

struct select_table_entry {
	struct wait_queue wait;
	struct wait_queue ** wait_address;//lux 要监听的地址，指向资源的等待列表
};

typedef struct select_table_struct {
	int nr;
	struct select_table_entry * entry;
} select_table;//lux 一个select可管理的条目表，一个entry代表一个监听项

#define __MAX_SELECT_TABLE_ENTRIES (4096 / sizeof (struct select_table_entry)) //lux 一个page能保存的最大条数

#endif /* __KERNEL__ */

#endif
