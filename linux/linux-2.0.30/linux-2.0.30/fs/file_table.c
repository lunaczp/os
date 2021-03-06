/*lux 系统的打开文件表。
 * 组织成了双向链表，first_file指向表头
 *
 */

/*
 *  linux/fs/file_table.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/config.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mm.h>

/*
 * first_file points to a doubly linked list of all file structures in
 *            the system.
 * nr_files   holds the length of this list.
 */
struct file * first_file = NULL;
int nr_files = 0;
int max_files = NR_FILE;

/*
 * Insert a new file structure at the head of the list of available ones.
 */
static inline void insert_file_free(struct file *file)
{
	struct file *next, *prev;

	next = first_file;
	first_file = file;
	file->f_count = 0;
	prev = next->f_prev;
	file->f_next = next;
	next->f_prev = file;
	file->f_prev = prev;
	prev->f_next = file;
}

/*
 * Remove a file structure from the list of available ones.
 */
static inline void remove_file_free(struct file *file)
{
	struct file *next, *prev;

	next = file->f_next;
	prev = file->f_prev;
	file->f_next = file->f_prev = NULL;
	if (first_file == file)
		first_file = next;
	next->f_prev = prev;
	prev->f_next = next;
}

/*
 * Insert a file structure at the end of the list of available ones.
 */
static inline void put_last_free(struct file *file)
{
	struct file *next, *prev;

	next = first_file;
	file->f_next = next;
	prev = next->f_prev;
	next->f_prev = file;
	file->f_prev = prev;
	prev->f_next = file;
}

/*
 * Allocate a new memory page for file structures and
 * insert the new structures into the global list.
 * Returns 0, if there is no more memory, 1 otherwise.
 */
static int grow_files(void)
{
	struct file * file;
	int i;

	/*
	 * We don't have to clear the page because we only look into
	 * f_count, f_prev and f_next and they get initialized in
	 * insert_file_free.  The rest of the file structure is cleared
	 * by get_empty_filp before it is returned.
	 */
	file = (struct file *) __get_free_page(GFP_KERNEL);

	if (!file)
		return 0;

	nr_files += i = PAGE_SIZE/sizeof(struct file);

	if (!first_file)
		file->f_count = 0,
		file->f_next = file->f_prev = first_file = file++,
		i--;

	for (; i ; i--)
		insert_file_free(file++);

	return 1;
}

unsigned long file_table_init(unsigned long start, unsigned long end)
{
	return start;
}
/*lux 从系统中的打开文件表中找到一个空闲表项
 *
 */
/*
 * Find an unused file structure and return a pointer to it.
 * Returns NULL, if there are no more free file structures or
 * we run out of memory.
 */
struct file * get_empty_filp(void)
{
	int i;
	int max = max_files;
	struct file * f;

	/*
	 * Reserve a few files for the super-user..
	 */
	if (current->euid)//lux 非super-user，max小于最大值
		max -= 10;

	/* if the return is taken, we are in deep trouble */
	if (!first_file && !grow_files())
		return NULL;

	do {
		for (f = first_file, i=0; i < nr_files; i++, f = f->f_next)
			if (!f->f_count) {//lux 为0，可以使用
				remove_file_free(f);
				memset(f,0,sizeof(*f));
				put_last_free(f);
				f->f_count = 1;
				f->f_version = ++event;
				return f;
			}
	} while (nr_files < max && grow_files());

	return NULL;
}

#ifdef CONFIG_QUOTA

void add_dquot_ref(kdev_t dev, short type)
{
	struct file *filp;
	int cnt;

	for (filp = first_file, cnt = 0; cnt < nr_files; cnt++, filp = filp->f_next) {
		if (!filp->f_count || !filp->f_inode || filp->f_inode->i_dev != dev)
			continue;
		if (filp->f_mode & FMODE_WRITE && filp->f_inode->i_sb->dq_op) {
			filp->f_inode->i_sb->dq_op->initialize(filp->f_inode, type);
			filp->f_inode->i_flags |= S_WRITE;
		}
	}
}

void reset_dquot_ptrs(kdev_t dev, short type)
{
	struct file *filp;
	int cnt;

	for (filp = first_file, cnt = 0; cnt < nr_files; cnt++, filp = filp->f_next) {
		if (!filp->f_count || !filp->f_inode || filp->f_inode->i_dev != dev)
			continue;
		if (IS_WRITABLE(filp->f_inode)) {
			filp->f_inode->i_dquot[type] = NODQUOT;
			filp->f_inode->i_flags &= ~S_WRITE;
		}
	}
}

#endif
