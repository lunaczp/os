/*
 *  linux/fs/buffer.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 *  'buffer.c' implements the buffer-cache functions. Race-conditions have
 * been avoided by NEVER letting an interrupt change a buffer (except for the
 * data, of course), but instead letting the caller do it.
 */

/* Some bdflush() changes for the dynamic ramdisk - Paul Gortmaker, 12/94 */
/* Start bdflush() with kernel_thread not syscall - Paul Gortmaker, 12/95 */

/* Removed a lot of unnecessary code and simplified things now that
   the buffer cache isn't our primary cache - Andrew Tridgell 12/96 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/locks.h>
#include <linux/errno.h>
#include <linux/malloc.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/swapctl.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/io.h>
#include <asm/bitops.h>

#define NR_SIZES 5
static char buffersize_index[17] =
{-1,  0,  1, -1,  2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4};

#define BUFSIZE_INDEX(X) ((int) buffersize_index[(X)>>9])
#define MAX_BUF_PER_PAGE (PAGE_SIZE / 512)
#define MAX_UNUSED_BUFFERS 30 /* don't ever have more than this number of 
				 unused buffer heads */
#define HASH_PAGES         4  /* number of pages to use for the hash table */
#define NR_HASH (HASH_PAGES*PAGE_SIZE/sizeof(struct buffer_head *))
#define HASH_MASK (NR_HASH-1)

static int grow_buffers(int pri, int size);

static struct buffer_head ** hash_table;
static struct buffer_head * lru_list[NR_LIST] = {NULL, };
static struct buffer_head * free_list[NR_SIZES] = {NULL, };

static struct buffer_head * unused_list = NULL;
static struct buffer_head * reuse_list = NULL;
static struct wait_queue * buffer_wait = NULL;

static int nr_buffers = 0;
static int nr_buffers_type[NR_LIST] = {0,};
static int nr_buffer_heads = 0;
static int nr_unused_buffer_heads = 0;
static int refilled = 0;       /* Set NZ when a buffer freelist is refilled 
				  this is used by the loop device */

/* this is used by some architectures to estimate available memory */
int buffermem = 0;

/* Here is the parameter block for the bdflush process. If you add or
 * remove any of the parameters, make sure to update kernel/sysctl.c.
 */

static void wakeup_bdflush(int);

#define N_PARAM 9

/* the dummy values in this structure are left in there for compatibility
   with old programs that play with the /proc entries */
union bdflush_param{
	struct {
		int nfract;  /* Percentage of buffer cache dirty to 
				activate bdflush */
		int ndirty;  /* Maximum number of dirty blocks to write out per
				wake-cycle */
		int nrefill; /* Number of clean buffers to try to obtain
				each time we call refill */
		int nref_dirt; /* Dirty buffer threshold for activating bdflush
				  when trying to refill buffers. */
		int dummy1;    /* unused */
		int age_buffer;  /* Time for normal buffer to age before 
				    we flush it */
		int age_super;  /* Time for superblock to age before we 
				   flush it */
		int dummy2;    /* unused */
		int dummy3;    /* unused */
	} b_un;
	unsigned int data[N_PARAM];
} bdf_prm = {{60, 500, 64, 256, 15, 30*HZ, 5*HZ, 1884, 2}};

/* These are the min and max parameter values that we will allow to be assigned */
int bdflush_min[N_PARAM] = {  0,  10,    5,   25,  0,   100,   100, 1, 1};
int bdflush_max[N_PARAM] = {100,5000, 2000, 2000,100, 60000, 60000, 2047, 5};

/*
 * Rewrote the wait-routines to use the "new" wait-queue functionality,
 * and getting rid of the cli-sti pairs. The wait-queue routines still
 * need cli-sti, but now it's just a couple of 386 instructions or so.
 *
 * Note that the real wait_on_buffer() is an inline function that checks
 * if 'b_wait' is set before calling this, so that the queues aren't set
 * up unnecessarily.
 */
void __wait_on_buffer(struct buffer_head * bh)
{
	struct wait_queue wait = { current, NULL };

	bh->b_count++;
	add_wait_queue(&bh->b_wait, &wait);
repeat:
	run_task_queue(&tq_disk);
	current->state = TASK_UNINTERRUPTIBLE;
	if (buffer_locked(bh)) {
		schedule();
		goto repeat;
	}
	remove_wait_queue(&bh->b_wait, &wait);
	bh->b_count--;
	current->state = TASK_RUNNING;
}

/* Call sync_buffers with wait!=0 to ensure that the call does not
   return until all buffer writes have completed.  Sync() may return
   before the writes have finished; fsync() may not. */


/* Godamity-damn.  Some buffers (bitmaps for filesystems)
   spontaneously dirty themselves without ever brelse being called.
   We will ultimately want to put these in a separate list, but for
   now we search all of the lists for dirty buffers */

static int sync_buffers(kdev_t dev, int wait)
{
	int i, retry, pass = 0, err = 0;
	struct buffer_head * bh, *next;

	/* One pass for no-wait, three for wait:
	   0) write out all dirty, unlocked buffers;
	   1) write out all dirty buffers, waiting if locked;
	   2) wait for completion by waiting for all buffers to unlock. */
	do {
		retry = 0;
repeat:
	/* We search all lists as a failsafe mechanism, not because we expect
	   there to be dirty buffers on any of the other lists. */
		bh = lru_list[BUF_DIRTY];
		if (!bh)
			goto repeat2;
		for (i = nr_buffers_type[BUF_DIRTY]*2 ; i-- > 0 ; bh = next) {
			if (bh->b_list != BUF_DIRTY)
				goto repeat;
			next = bh->b_next_free;
			if (!lru_list[BUF_DIRTY])
				break;
			if (dev && bh->b_dev != dev)
				continue;
			if (buffer_locked(bh)) {
				/* Buffer is locked; skip it unless wait is
				   requested AND pass > 0. */
				if (!wait || !pass) {
					retry = 1;
					continue;
				}
				wait_on_buffer (bh);
				goto repeat;
			}
			/* If an unlocked buffer is not uptodate, there has
			    been an IO error. Skip it. */
			if (wait && buffer_req(bh) && !buffer_locked(bh) &&
			    !buffer_dirty(bh) && !buffer_uptodate(bh)) {
				err = 1;
				continue;
			}
			/* Don't write clean buffers.  Don't write ANY buffers
			   on the third pass. */
			if (!buffer_dirty(bh) || pass >= 2)
				continue;
			/* don't bother about locked buffers */
			if (buffer_locked(bh))
				continue;
			bh->b_count++;
			next->b_count++;
			bh->b_flushtime = 0;
			ll_rw_block(WRITE, 1, &bh);
			bh->b_count--;
			next->b_count--;
			retry = 1;
		}

    repeat2:
		bh = lru_list[BUF_LOCKED];
		if (!bh)
			break;
		for (i = nr_buffers_type[BUF_LOCKED]*2 ; i-- > 0 ; bh = next) {
			if (bh->b_list != BUF_LOCKED)
				goto repeat2;
			next = bh->b_next_free;
			if (!lru_list[BUF_LOCKED])
				break;
			if (dev && bh->b_dev != dev)
				continue;
			if (buffer_locked(bh)) {
				/* Buffer is locked; skip it unless wait is
				   requested AND pass > 0. */
				if (!wait || !pass) {
					retry = 1;
					continue;
				}
				wait_on_buffer (bh);
				goto repeat2;
			}
		}

	/* If we are waiting for the sync to succeed, and if any dirty
	   blocks were written, then repeat; on the second pass, only
	   wait for buffers being written (do not pass to write any
	   more buffers on the second pass). */
	} while (wait && retry && ++pass<=2);
	return err;
}

void sync_dev(kdev_t dev)
{
	sync_buffers(dev, 0);
	sync_supers(dev);
	sync_inodes(dev);
	sync_buffers(dev, 0);
	sync_dquots(dev, -1);
}

int fsync_dev(kdev_t dev)
{
	sync_buffers(dev, 0);
	sync_supers(dev);
	sync_inodes(dev);
	sync_dquots(dev, -1);
	return sync_buffers(dev, 1);
}

asmlinkage int sys_sync(void)
{
	fsync_dev(0);
	return 0;
}

int file_fsync (struct inode *inode, struct file *filp)
{
	return fsync_dev(inode->i_dev);
}

asmlinkage int sys_fsync(unsigned int fd)
{
	struct file * file;
	struct inode * inode;

	if (fd>=NR_OPEN || !(file=current->files->fd[fd]) || !(inode=file->f_inode))
		return -EBADF;
	if (!file->f_op || !file->f_op->fsync)
		return -EINVAL;
	if (file->f_op->fsync(inode,file))
		return -EIO;
	return 0;
}

asmlinkage int sys_fdatasync(unsigned int fd)
{
	struct file * file;
	struct inode * inode;

	if (fd>=NR_OPEN || !(file=current->files->fd[fd]) || !(inode=file->f_inode))
		return -EBADF;
	if (!file->f_op || !file->f_op->fsync)
		return -EINVAL;
	/* this needs further work, at the moment it is identical to fsync() */
	if (file->f_op->fsync(inode,file))
		return -EIO;
	return 0;
}

void invalidate_buffers(kdev_t dev)
{
	int i;
	int nlist;
	struct buffer_head * bh;

	for(nlist = 0; nlist < NR_LIST; nlist++) {
		bh = lru_list[nlist];
		for (i = nr_buffers_type[nlist]*2 ; --i > 0 ; bh = bh->b_next_free) {
			if (bh->b_dev != dev)
				continue;
			wait_on_buffer(bh);
			if (bh->b_dev != dev)
				continue;
			if (bh->b_count)
				continue;
			bh->b_flushtime = 0;
			clear_bit(BH_Protected, &bh->b_state);
			clear_bit(BH_Uptodate, &bh->b_state);
			clear_bit(BH_Dirty, &bh->b_state);
			clear_bit(BH_Req, &bh->b_state);
		}
	}
}

#define _hashfn(dev,block) (((unsigned)(HASHDEV(dev)^block))&HASH_MASK)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

static inline void remove_from_hash_queue(struct buffer_head * bh)
{
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	if (hash(bh->b_dev,bh->b_blocknr) == bh)
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
	bh->b_next = bh->b_prev = NULL;
}

static inline void remove_from_lru_list(struct buffer_head * bh)
{
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("VFS: LRU block list corrupted");
	if (bh->b_dev == B_FREE)
		panic("LRU list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;

	if (lru_list[bh->b_list] == bh)
		 lru_list[bh->b_list] = bh->b_next_free;
	if (lru_list[bh->b_list] == bh)
		 lru_list[bh->b_list] = NULL;
	bh->b_next_free = bh->b_prev_free = NULL;
}

static inline void remove_from_free_list(struct buffer_head * bh)
{
	int isize = BUFSIZE_INDEX(bh->b_size);
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("VFS: Free block list corrupted");
	if(bh->b_dev != B_FREE)
		panic("Free list corrupted");
	if(!free_list[isize])
		panic("Free list empty");
	if(bh->b_next_free == bh)
		 free_list[isize] = NULL;
	else {
		bh->b_prev_free->b_next_free = bh->b_next_free;
		bh->b_next_free->b_prev_free = bh->b_prev_free;
		if (free_list[isize] == bh)
			 free_list[isize] = bh->b_next_free;
	}
	bh->b_next_free = bh->b_prev_free = NULL;
}

static inline void remove_from_queues(struct buffer_head * bh)
{
	if(bh->b_dev == B_FREE) {
		remove_from_free_list(bh); /* Free list entries should not be
					      in the hash queue */
		return;
	}
	nr_buffers_type[bh->b_list]--;
	remove_from_hash_queue(bh);
	remove_from_lru_list(bh);
}

static inline void put_last_lru(struct buffer_head * bh)
{
	if (!bh)
		return;
	if (bh == lru_list[bh->b_list]) {
		lru_list[bh->b_list] = bh->b_next_free;
		return;
	}
	if(bh->b_dev == B_FREE)
		panic("Wrong block for lru list");
	remove_from_lru_list(bh);
/* add to back of free list */

	if(!lru_list[bh->b_list]) {
		lru_list[bh->b_list] = bh;
		lru_list[bh->b_list]->b_prev_free = bh;
	}

	bh->b_next_free = lru_list[bh->b_list];
	bh->b_prev_free = lru_list[bh->b_list]->b_prev_free;
	lru_list[bh->b_list]->b_prev_free->b_next_free = bh;
	lru_list[bh->b_list]->b_prev_free = bh;
}

static inline void put_last_free(struct buffer_head * bh)
{
	int isize;
	if (!bh)
		return;

	isize = BUFSIZE_INDEX(bh->b_size);	
	bh->b_dev = B_FREE;  /* So it is obvious we are on the free list */
	/* add to back of free list */
	if(!free_list[isize]) {
		free_list[isize] = bh;
		bh->b_prev_free = bh;
	}

	bh->b_next_free = free_list[isize];
	bh->b_prev_free = free_list[isize]->b_prev_free;
	free_list[isize]->b_prev_free->b_next_free = bh;
	free_list[isize]->b_prev_free = bh;
}

static inline void insert_into_queues(struct buffer_head * bh)
{
	/* put at end of free list */
	if(bh->b_dev == B_FREE) {
		put_last_free(bh);
		return;
	}
	if(!lru_list[bh->b_list]) {
		lru_list[bh->b_list] = bh;
		bh->b_prev_free = bh;
	}

	if (bh->b_next_free) panic("VFS: buffer LRU pointers corrupted");
	bh->b_next_free = lru_list[bh->b_list];
	bh->b_prev_free = lru_list[bh->b_list]->b_prev_free;
	lru_list[bh->b_list]->b_prev_free->b_next_free = bh;
	lru_list[bh->b_list]->b_prev_free = bh;
	nr_buffers_type[bh->b_list]++;
/* put the buffer in new hash-queue if it has a device */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!(bh->b_dev))
		return;
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);
	hash(bh->b_dev,bh->b_blocknr) = bh;
	if (bh->b_next)
		bh->b_next->b_prev = bh;
}

static inline struct buffer_head * find_buffer(kdev_t dev, int block, int size)
{		
	struct buffer_head * tmp;

	for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
		if (tmp->b_blocknr == block && tmp->b_dev == dev)
			if (tmp->b_size == size)
				return tmp;
			else {
				printk("VFS: Wrong blocksize on device %s\n",
					kdevname(dev));
				return NULL;
			}
	return NULL;
}

/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are reading them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 */
struct buffer_head * get_hash_table(kdev_t dev, int block, int size)
{
	struct buffer_head * bh;

	for (;;) {
		if (!(bh=find_buffer(dev,block,size)))
			return NULL;
		bh->b_count++;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_blocknr == block
					     && bh->b_size == size)
			return bh;
		bh->b_count--;
	}
}

void set_blocksize(kdev_t dev, int size)
{
	extern int *blksize_size[];
	int i, nlist;
	struct buffer_head * bh, *bhnext;

	if (!blksize_size[MAJOR(dev)])
		return;

	if (size > PAGE_SIZE)
		size = 0;

	switch (size) {
		default: panic("Invalid blocksize passed to set_blocksize");
		case 512: case 1024: case 2048: case 4096: case 8192: ;
	}

	if (blksize_size[MAJOR(dev)][MINOR(dev)] == 0 && size == BLOCK_SIZE) {
		blksize_size[MAJOR(dev)][MINOR(dev)] = size;
		return;
	}
	if (blksize_size[MAJOR(dev)][MINOR(dev)] == size)
		return;
	sync_buffers(dev, 2);
	blksize_size[MAJOR(dev)][MINOR(dev)] = size;

  /* We need to be quite careful how we do this - we are moving entries
     around on the free list, and we can get in a loop if we are not careful.*/

	for(nlist = 0; nlist < NR_LIST; nlist++) {
		bh = lru_list[nlist];
		for (i = nr_buffers_type[nlist]*2 ; --i > 0 ; bh = bhnext) {
			if(!bh) break;
			bhnext = bh->b_next_free; 
			if (bh->b_dev != dev)
				 continue;
			if (bh->b_size == size)
				 continue;
			
			wait_on_buffer(bh);
			if (bh->b_dev == dev && bh->b_size != size) {
				clear_bit(BH_Dirty, &bh->b_state);
				clear_bit(BH_Uptodate, &bh->b_state);
				clear_bit(BH_Req, &bh->b_state);
				bh->b_flushtime = 0;
			}
			remove_from_hash_queue(bh);
		}
	}
}


/* check if a buffer is OK to be reclaimed */
static inline int can_reclaim(struct buffer_head *bh, int size)
{
	if (bh->b_count || 
	    buffer_protected(bh) || buffer_locked(bh))
		return 0;
			 
	if (mem_map[MAP_NR((unsigned long) bh->b_data)].count != 1 ||
	    buffer_dirty(bh)) {
		refile_buffer(bh);
		return 0;
	}

	if (bh->b_size != size)
		return 0;

	return 1;
}

/* find a candidate buffer to be reclaimed */
static struct buffer_head *find_candidate(struct buffer_head *list,int *list_len,int size)
{
	struct buffer_head *bh;
	
	for (bh = list; 
	     bh && (*list_len) > 0; 
	     bh = bh->b_next_free, (*list_len)--) {
		if (size != bh->b_size) {
			/* this provides a mechanism for freeing blocks
			   of other sizes, this is necessary now that we
			   no longer have the lav code. */
			try_to_free_buffer(bh,&bh,1);
			continue;
		}

		if (buffer_locked(bh) && 
		    (bh->b_list == BUF_LOCKED || bh->b_list == BUF_LOCKED1)) {
			/* Buffers are written in the order they are placed 
			   on the locked list. If we encounter a locked
			   buffer here, this means that the rest of them
			   are also locked */
			(*list_len) = 0;
			return NULL;
		}

		if (can_reclaim(bh,size))
		    return bh;
	}

	return NULL;
}

static void refill_freelist(int size)
{
	struct buffer_head * bh;
	struct buffer_head * candidate[BUF_DIRTY];
	unsigned int best_time, winner;
	int buffers[BUF_DIRTY];
	int i;
	int needed;

	refilled = 1;
	/* If there are too many dirty buffers, we wake up the update process
	   now so as to ensure that there are still clean buffers available
	   for user processes to use (and dirty) */
	
	/* We are going to try to locate this much memory */
	needed =bdf_prm.b_un.nrefill * size;  

	while (nr_free_pages > min_free_pages*2 && needed > 0 &&
	       grow_buffers(GFP_BUFFER, size)) {
		needed -= PAGE_SIZE;
	}

repeat:
	/* OK, we cannot grow the buffer cache, now try to get some
	   from the lru list */

	/* First set the candidate pointers to usable buffers.  This
	   should be quick nearly all of the time. */

	if(needed <= 0) return;

	for(i=0; i<BUF_DIRTY; i++){
		buffers[i] = nr_buffers_type[i];
		candidate[i] = find_candidate(lru_list[i], &buffers[i], size);
	}
	
	/* Now see which candidate wins the election */
	
	winner = best_time = UINT_MAX;	
	for(i=0; i<BUF_DIRTY; i++){
		if(!candidate[i]) continue;
		if(candidate[i]->b_lru_time < best_time){
			best_time = candidate[i]->b_lru_time;
			winner = i;
		}
	}
	
	/* If we have a winner, use it, and then get a new candidate from that list */
	if(winner != UINT_MAX) {
		i = winner;
		while (needed>0 && (bh=candidate[i])) {
			candidate[i] = bh->b_next_free;
			if(candidate[i] == bh) candidate[i] = NULL;  /* Got last one */
			remove_from_queues(bh);
			bh->b_dev = B_FREE;
			put_last_free(bh);
			needed -= bh->b_size;
			buffers[i]--;
			if(buffers[i] == 0) candidate[i] = NULL;
		
			if (candidate[i] && !can_reclaim(candidate[i],size))
				candidate[i] = find_candidate(candidate[i],&buffers[i], size);
		}
		if (needed >= 0)
			goto repeat;
	}
	
	if(needed <= 0) return;
	
	/* Too bad, that was not enough. Try a little harder to grow some. */
	
	if (nr_free_pages > min_free_pages + 5) {
		if (grow_buffers(GFP_BUFFER, size)) {
			needed -= PAGE_SIZE;
			goto repeat;
		};
	}

	/* and repeat until we find something good */
	if (!grow_buffers(GFP_ATOMIC, size))
		wakeup_bdflush(1);
	needed -= PAGE_SIZE;
	goto repeat;
}

/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algorithm is changed: hopefully better, and an elusive bug removed.
 *
 * 14.02.92: changed it to sync dirty buffers a bit: better performance
 * when the filesystem starts to get full of dirty blocks (I hope).
 */
struct buffer_head * getblk(kdev_t dev, int block, int size)
{
	struct buffer_head * bh;
	int isize = BUFSIZE_INDEX(size);

	/* If there are too many dirty buffers, we wake up the update process
	   now so as to ensure that there are still clean buffers available
	   for user processes to use (and dirty) */
repeat:
	bh = get_hash_table(dev, block, size);
	if (bh) {
		if (!buffer_dirty(bh)) {
			if (buffer_uptodate(bh))
				 put_last_lru(bh);
			bh->b_flushtime = 0;
		}
		set_bit(BH_Touched, &bh->b_state);
		return bh;
	}

	while(!free_list[isize]) {
		refill_freelist(size);
	}
	
	if (find_buffer(dev,block,size))
		 goto repeat;

	bh = free_list[isize];
	remove_from_free_list(bh);

/* OK, FINALLY we know that this buffer is the only one of its kind, */
/* and that it's unused (b_count=0), unlocked (buffer_locked=0), and clean */
	bh->b_count=1;
	bh->b_flushtime=0;
	bh->b_state=(1<<BH_Touched);
	bh->b_dev=dev;
	bh->b_blocknr=block;
	insert_into_queues(bh);
	return bh;
}

void set_writetime(struct buffer_head * buf, int flag)
{
	int newtime;

	if (buffer_dirty(buf)) {
		/* Move buffer to dirty list if jiffies is clear */
		newtime = jiffies + (flag ? bdf_prm.b_un.age_super : 
				     bdf_prm.b_un.age_buffer);
		if(!buf->b_flushtime || buf->b_flushtime > newtime)
			 buf->b_flushtime = newtime;
	} else {
		buf->b_flushtime = 0;
	}
}


/*
 * A buffer may need to be moved from one buffer list to another
 * (e.g. in case it is not shared any more). Handle this.
 */
void refile_buffer(struct buffer_head * buf)
{
	int dispose;

	if(buf->b_dev == B_FREE) {
		printk("Attempt to refile free buffer\n");
		return;
	}
	if (buffer_dirty(buf))
		dispose = BUF_DIRTY;
	else if (buffer_locked(buf))
		dispose = BUF_LOCKED;
	else
		dispose = BUF_CLEAN;
	if(dispose == BUF_CLEAN) buf->b_lru_time = jiffies;
	if(dispose != buf->b_list)  {
		if(dispose == BUF_DIRTY)
			 buf->b_lru_time = jiffies;
		if(dispose == BUF_LOCKED && 
		   (buf->b_flushtime - buf->b_lru_time) <= bdf_prm.b_un.age_super)
			 dispose = BUF_LOCKED1;
		remove_from_queues(buf);
		buf->b_list = dispose;
		insert_into_queues(buf);
		if (dispose == BUF_DIRTY) {
		/* This buffer is dirty, maybe we need to start flushing. */
		/* If too high a percentage of the buffers are dirty... */
		if (nr_buffers_type[BUF_DIRTY] > nr_buffers * bdf_prm.b_un.nfract/100)
			 wakeup_bdflush(0);
		/* If this is a loop device, and
		 * more than half of the buffers are dirty... */ 
		/* (Prevents no-free-buffers deadlock with loop device.) */
		if (MAJOR(buf->b_dev) == LOOP_MAJOR &&
		    nr_buffers_type[BUF_DIRTY]*2>nr_buffers)
			wakeup_bdflush(1);
		}
	}
}

/*
 * Release a buffer head
 */
void __brelse(struct buffer_head * buf)
{
	wait_on_buffer(buf);

	/* If dirty, mark the time this buffer should be written back */
	set_writetime(buf, 0);
	refile_buffer(buf);

	if (buf->b_count) {
		buf->b_count--;
		return;
	}
	printk("VFS: brelse: Trying to free free buffer\n");
}

/*
 * bforget() is like brelse(), except it removes the buffer
 * from the hash-queues (so that it won't be re-used if it's
 * shared).
 */
void __bforget(struct buffer_head * buf)
{
	wait_on_buffer(buf);
	mark_buffer_clean(buf);
	clear_bit(BH_Protected, &buf->b_state);
	buf->b_count--;
	remove_from_hash_queue(buf);
	buf->b_dev = NODEV;
	refile_buffer(buf);
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
struct buffer_head * bread(kdev_t dev, int block, int size)
{
	struct buffer_head * bh;

	if (!(bh = getblk(dev, block, size))) {
		printk("VFS: bread: impossible error\n");
		return NULL;
	}
	if (buffer_uptodate(bh))
		return bh;
	ll_rw_block(READ, 1, &bh);
	wait_on_buffer(bh);
	if (buffer_uptodate(bh))
		return bh;
	brelse(bh);
	return NULL;
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */

#define NBUF 16

struct buffer_head * breada(kdev_t dev, int block, int bufsize,
	unsigned int pos, unsigned int filesize)
{
	struct buffer_head * bhlist[NBUF];
	unsigned int blocks;
	struct buffer_head * bh;
	int index;
	int i, j;

	if (pos >= filesize)
		return NULL;

	if (block < 0 || !(bh = getblk(dev,block,bufsize)))
		return NULL;

	index = BUFSIZE_INDEX(bh->b_size);

	if (buffer_uptodate(bh))
		return(bh);   
	else ll_rw_block(READ, 1, &bh);

	blocks = (filesize - pos) >> (9+index);

	if (blocks < (read_ahead[MAJOR(dev)] >> index))
		blocks = read_ahead[MAJOR(dev)] >> index;
	if (blocks > NBUF) 
		blocks = NBUF;

/*	if (blocks) printk("breada (new) %d blocks\n",blocks); */


	bhlist[0] = bh;
	j = 1;
	for(i=1; i<blocks; i++) {
		bh = getblk(dev,block+i,bufsize);
		if (buffer_uptodate(bh)) {
			brelse(bh);
			break;
		}
		else bhlist[j++] = bh;
	}

	/* Request the read for these buffers, and then release them */
	if (j>1)  
		ll_rw_block(READA, (j-1), bhlist+1); 
	for(i=1; i<j; i++)
		brelse(bhlist[i]);

	/* Wait for this buffer, and then continue on */
	bh = bhlist[0];
	wait_on_buffer(bh);
	if (buffer_uptodate(bh))
		return bh;
	brelse(bh);
	return NULL;
}

static void put_unused_buffer_head(struct buffer_head * bh)
{
	if (nr_unused_buffer_heads >= MAX_UNUSED_BUFFERS) {
		nr_buffer_heads--;
		kfree(bh);
		return;
	}
	memset(bh,0,sizeof(*bh));
	nr_unused_buffer_heads++;
	bh->b_next_free = unused_list;
	unused_list = bh;
	wake_up(&buffer_wait);
}

static void get_more_buffer_heads(void)
{
	struct buffer_head * bh;

	while (!unused_list) {
		/*
		 * This is critical.  We can't swap out pages to get
		 * more buffer heads, because the swap-out may need
		 * more buffer-heads itself.  Thus GFP_ATOMIC.
		 */
		/* we now use kmalloc() here instead of gfp as we want
                   to be able to easily release buffer heads - they
                   took up quite a bit of memory (tridge) */
		bh = (struct buffer_head *) kmalloc(sizeof(*bh),GFP_ATOMIC);
		if (bh) {
			put_unused_buffer_head(bh);
			nr_buffer_heads++;
			return;
		}

		/*
		 * Uhhuh. We're _really_ low on memory. Now we just
		 * wait for old buffer heads to become free due to
		 * finishing IO..
		 */
		run_task_queue(&tq_disk);
		sleep_on(&buffer_wait);
	}

}

/* 
 * We can't put completed temporary IO buffer_heads directly onto the
 * unused_list when they become unlocked, since the device driver
 * end_request routines still expect access to the buffer_head's
 * fields after the final unlock.  So, the device driver puts them on
 * the reuse_list instead once IO completes, and we recover these to
 * the unused_list here.
 *
 * The reuse_list receives buffers from interrupt routines, so we need
 * to be IRQ-safe here (but note that interrupts only _add_ to the
 * reuse_list, never take away. So we don't need to worry about the
 * reuse_list magically emptying).
 */
static inline void recover_reusable_buffer_heads(void)
{
	if (reuse_list) {
		struct buffer_head *head;

		head = xchg(&reuse_list, NULL);
	
		do {
			struct buffer_head *bh = head;
			head = head->b_next_free;
			put_unused_buffer_head(bh);
		} while (head);
	}
}

static struct buffer_head * get_unused_buffer_head(void)
{
	struct buffer_head * bh;

	recover_reusable_buffer_heads();
	get_more_buffer_heads();
	if (!unused_list)
		return NULL;
	bh = unused_list;
	unused_list = bh->b_next_free;
	nr_unused_buffer_heads--;
	return bh;
}

/*
 * Create the appropriate buffers when given a page for data area and
 * the size of each buffer.. Use the bh->b_this_page linked list to
 * follow the buffers created.  Return NULL if unable to create more
 * buffers.
 */
static struct buffer_head * create_buffers(unsigned long page, unsigned long size)
{
	struct buffer_head *bh, *head;
	long offset;

	head = NULL;
	offset = PAGE_SIZE;
	while ((offset -= size) >= 0) {
		bh = get_unused_buffer_head();
		if (!bh)
			goto no_grow;

		bh->b_dev = B_FREE;  /* Flag as unused */
		bh->b_this_page = head;
		head = bh;

		bh->b_state = 0;
		bh->b_next_free = NULL;
		bh->b_count = 0;
		bh->b_size = size;

		bh->b_data = (char *) (page+offset);
		bh->b_list = 0;
	}
	return head;
/*
 * In case anything failed, we just free everything we got.
 */
no_grow:
	bh = head;
	while (bh) {
		head = bh;
		bh = bh->b_this_page;
		put_unused_buffer_head(head);
	}
	return NULL;
}

/* Run the hooks that have to be done when a page I/O has completed. */
static inline void after_unlock_page (struct page * page)
{
	if (clear_bit(PG_decr_after, &page->flags))
		atomic_dec(&nr_async_pages);
	if (clear_bit(PG_free_after, &page->flags))
		__free_page(page);
	if (clear_bit(PG_swap_unlock_after, &page->flags))
		swap_after_unlock_page(page->swap_unlock_entry);
}

/*
 * Free all temporary buffers belonging to a page.
 * This needs to be called with interrupts disabled.
 */
static inline void free_async_buffers (struct buffer_head * bh)
{
	struct buffer_head * tmp;

	tmp = bh;
	do {
		if (!test_bit(BH_FreeOnIO, &tmp->b_state)) {
			printk ("Whoops: unlock_buffer: "
				"async IO mismatch on page.\n");
			return;
		}
		tmp->b_next_free = reuse_list;
		reuse_list = tmp;
		clear_bit(BH_FreeOnIO, &tmp->b_state);
		tmp = tmp->b_this_page;
	} while (tmp != bh);
}

/*
 * Start I/O on a page.
 * This function expects the page to be locked and may return before I/O is complete.
 * You then have to check page->locked, page->uptodate, and maybe wait on page->wait.
 */
int brw_page(int rw, struct page *page, kdev_t dev, int b[], int size, int bmap)
{
	struct buffer_head *bh, *prev, *next, *arr[MAX_BUF_PER_PAGE];
	int block, nr;

	if (!PageLocked(page))
		panic("brw_page: page not locked for I/O");
	clear_bit(PG_uptodate, &page->flags);
	clear_bit(PG_error, &page->flags);
	/*
	 * Allocate buffer heads pointing to this page, just for I/O.
	 * They do _not_ show up in the buffer hash table!
	 * They are _not_ registered in page->buffers either!
	 */
	bh = create_buffers(page_address(page), size);
	if (!bh) {
		clear_bit(PG_locked, &page->flags);
		wake_up(&page->wait);
		return -ENOMEM;
	}
	nr = 0;
	next = bh;
	do {
		struct buffer_head * tmp;
		block = *(b++);

		set_bit(BH_FreeOnIO, &next->b_state);
		next->b_list = BUF_CLEAN;
		next->b_dev = dev;
		next->b_blocknr = block;
		next->b_count = 1;
		next->b_flushtime = 0;
		set_bit(BH_Uptodate, &next->b_state);

		/*
		 * When we use bmap, we define block zero to represent
		 * a hole.  ll_rw_page, however, may legitimately
		 * access block zero, and we need to distinguish the
		 * two cases.
		 */
		if (bmap && !block) {
			memset(next->b_data, 0, size);
			next->b_count--;
			continue;
		}
		tmp = get_hash_table(dev, block, size);
		if (tmp) {
			if (!buffer_uptodate(tmp)) {
				if (rw == READ)
					ll_rw_block(READ, 1, &tmp);
				wait_on_buffer(tmp);
			}
			if (rw == READ) 
				memcpy(next->b_data, tmp->b_data, size);
			else {
				memcpy(tmp->b_data, next->b_data, size);
				mark_buffer_dirty(tmp, 0);
			}
			brelse(tmp);
			next->b_count--;
			continue;
		}
		if (rw == READ)
			clear_bit(BH_Uptodate, &next->b_state);
		else
			set_bit(BH_Dirty, &next->b_state);
		arr[nr++] = next;
	} while (prev = next, (next = next->b_this_page) != NULL);
	prev->b_this_page = bh;

	if (nr) {
		ll_rw_block(rw, nr, arr);
		/* The rest of the work is done in mark_buffer_uptodate()
		 * and unlock_buffer(). */
	} else {
		unsigned long flags;
		clear_bit(PG_locked, &page->flags);
		set_bit(PG_uptodate, &page->flags);
		wake_up(&page->wait);
		save_flags(flags);
		cli();
		free_async_buffers(bh);
		restore_flags(flags);
		after_unlock_page(page);
	}
	++current->maj_flt;
	return 0;
}

/*
 * This is called by end_request() when I/O has completed.
 */
void mark_buffer_uptodate(struct buffer_head * bh, int on)
{
	if (on) {
		struct buffer_head *tmp = bh;
		set_bit(BH_Uptodate, &bh->b_state);
		/* If a page has buffers and all these buffers are uptodate,
		 * then the page is uptodate. */
		do {
			if (!test_bit(BH_Uptodate, &tmp->b_state))
				return;
			tmp=tmp->b_this_page;
		} while (tmp && tmp != bh);
		set_bit(PG_uptodate, &mem_map[MAP_NR(bh->b_data)].flags);
		return;
	}
	clear_bit(BH_Uptodate, &bh->b_state);
}

/*
 * This is called by end_request() when I/O has completed.
 */
void unlock_buffer(struct buffer_head * bh)
{
	unsigned long flags;
	struct buffer_head *tmp;
	struct page *page;

	clear_bit(BH_Lock, &bh->b_state);
	wake_up(&bh->b_wait);

	if (!test_bit(BH_FreeOnIO, &bh->b_state))
		return;
	/* This is a temporary buffer used for page I/O. */
	page = mem_map + MAP_NR(bh->b_data);
	if (!PageLocked(page))
		goto not_locked;
	if (bh->b_count != 1)
		goto bad_count;

	if (!test_bit(BH_Uptodate, &bh->b_state))
		set_bit(PG_error, &page->flags);

	/*
	 * Be _very_ careful from here on. Bad things can happen if
	 * two buffer heads end IO at almost the same time and both
	 * decide that the page is now completely done.
	 *
	 * Async buffer_heads are here only as labels for IO, and get
	 * thrown away once the IO for this page is complete.  IO is
	 * deemed complete once all buffers have been visited
	 * (b_count==0) and are now unlocked. We must make sure that
	 * only the _last_ buffer that decrements its count is the one
	 * that free's the page..
	 */
	save_flags(flags);
	cli();
	bh->b_count--;
	tmp = bh;
	do {
		if (tmp->b_count)
			goto still_busy;
		tmp = tmp->b_this_page;
	} while (tmp != bh);

	/* OK, the async IO on this page is complete. */
	free_async_buffers(bh);
	restore_flags(flags);
	clear_bit(PG_locked, &page->flags);
	wake_up(&page->wait);
	after_unlock_page(page);
	wake_up(&buffer_wait);
	return;

still_busy:
	restore_flags(flags);
	return;

not_locked:
	printk ("Whoops: unlock_buffer: async io complete on unlocked page\n");
	return;

bad_count:
	printk ("Whoops: unlock_buffer: b_count != 1 on async io.\n");
	return;
}

/*
 * Generic "readpage" function for block devices that have the normal
 * bmap functionality. This is most of the block device filesystems.
 * Reads the page asynchronously --- the unlock_buffer() and
 * mark_buffer_uptodate() functions propagate buffer state into the
 * page struct once IO has completed.
 */
int generic_readpage(struct inode * inode, struct page * page)
{
	unsigned long block;
	int *p, nr[PAGE_SIZE/512];
	int i;

	page->count++;
	set_bit(PG_locked, &page->flags);
	set_bit(PG_free_after, &page->flags);

	i = PAGE_SIZE >> inode->i_sb->s_blocksize_bits;
	block = page->offset >> inode->i_sb->s_blocksize_bits;
	p = nr;
	do {
		*p = inode->i_op->bmap(inode, block);
		i--;
		block++;
		p++;
	} while (i > 0);

	/* IO start */
	brw_page(READ, page, inode->i_dev, nr, inode->i_sb->s_blocksize, 1);
	return 0;
}

/*
 * Try to increase the number of buffers available: the size argument
 * is used to determine what kind of buffers we want.
 */
static int grow_buffers(int pri, int size)
{
	unsigned long page;
	struct buffer_head *bh, *tmp;
	struct buffer_head * insert_point;
	int isize;

	if ((size & 511) || (size > PAGE_SIZE)) {
		printk("VFS: grow_buffers: size = %d\n",size);
		return 0;
	}

	isize = BUFSIZE_INDEX(size);

	if (!(page = __get_free_page(pri)))
		return 0;
	bh = create_buffers(page, size);
	if (!bh) {
		free_page(page);
		return 0;
	}

	insert_point = free_list[isize];

	tmp = bh;
	while (1) {
		if (insert_point) {
			tmp->b_next_free = insert_point->b_next_free;
			tmp->b_prev_free = insert_point;
			insert_point->b_next_free->b_prev_free = tmp;
			insert_point->b_next_free = tmp;
		} else {
			tmp->b_prev_free = tmp;
			tmp->b_next_free = tmp;
		}
		insert_point = tmp;
		++nr_buffers;
		if (tmp->b_this_page)
			tmp = tmp->b_this_page;
		else
			break;
	}
	tmp->b_this_page = bh;
	free_list[isize] = bh;
	mem_map[MAP_NR(page)].buffers = bh;
	buffermem += PAGE_SIZE;
	return 1;
}


/* =========== Reduce the buffer memory ============= */

static inline int buffer_waiting(struct buffer_head * bh)
{
	return waitqueue_active(&bh->b_wait);
}

/*
 * try_to_free_buffer() checks if all the buffers on this particular page
 * are unused, and free's the page if so.
 */
int try_to_free_buffer(struct buffer_head * bh, struct buffer_head ** bhp,
		       int priority)
{
	unsigned long page;
	struct buffer_head * tmp, * p;

	*bhp = bh;
	page = (unsigned long) bh->b_data;
	page &= PAGE_MASK;
	tmp = bh;
	do {
		if (!tmp)
			return 0;
		if (tmp->b_count || buffer_protected(tmp) ||
		    buffer_dirty(tmp) || buffer_locked(tmp) ||
		    buffer_waiting(tmp))
			return 0;
		if (priority && buffer_touched(tmp))
			return 0;
		tmp = tmp->b_this_page;
	} while (tmp != bh);
	tmp = bh;
	do {
		p = tmp;
		tmp = tmp->b_this_page;
		nr_buffers--;
		if (p == *bhp)
		  {
		    *bhp = p->b_prev_free;
		    if (p == *bhp) /* Was this the last in the list? */
		      *bhp = NULL;
		  }
		remove_from_queues(p);
		put_unused_buffer_head(p);
	} while (tmp != bh);
	buffermem -= PAGE_SIZE;
	mem_map[MAP_NR(page)].buffers = NULL;
	free_page(page);
	return !mem_map[MAP_NR(page)].count;
}

/* ================== Debugging =================== */

void show_buffers(void)
{
	struct buffer_head * bh;
	int found = 0, locked = 0, dirty = 0, used = 0, lastused = 0;
	int protected = 0;
	int nlist;
	static char *buf_types[NR_LIST] = {"CLEAN","LOCKED","LOCKED1","DIRTY"};

	printk("Buffer memory:   %6dkB\n",buffermem>>10);
	printk("Buffer heads:    %6d\n",nr_buffer_heads);
	printk("Buffer blocks:   %6d\n",nr_buffers);

	for(nlist = 0; nlist < NR_LIST; nlist++) {
	  found = locked = dirty = used = lastused = protected = 0;
	  bh = lru_list[nlist];
	  if(!bh) continue;

	  do {
		found++;
		if (buffer_locked(bh))
			locked++;
		if (buffer_protected(bh))
			protected++;
		if (buffer_dirty(bh))
			dirty++;
		if (bh->b_count)
			used++, lastused = found;
		bh = bh->b_next_free;
	  } while (bh != lru_list[nlist]);
	  printk("%8s: %d buffers, %d used (last=%d), "
		 "%d locked, %d protected, %d dirty\n",
		 buf_types[nlist], found, used, lastused,
		 locked, protected, dirty);
	};
}

/* ===================== Init ======================= */

/*
 * allocate the hash table and init the free list
 */
void buffer_init(void)
{
	hash_table = (struct buffer_head **)vmalloc(NR_HASH*sizeof(struct buffer_head *));
	if (!hash_table)
		panic("Failed to allocate buffer hash table\n");
	memset(hash_table,0,NR_HASH*sizeof(struct buffer_head *));

	lru_list[BUF_CLEAN] = 0;
	grow_buffers(GFP_KERNEL, BLOCK_SIZE);
}


/* ====================== bdflush support =================== */

/* This is a simple kernel daemon, whose job it is to provide a dynamic
 * response to dirty buffers.  Once this process is activated, we write back
 * a limited number of buffers to the disks and then go back to sleep again.
 */
struct wait_queue * bdflush_wait = NULL;
struct wait_queue * bdflush_done = NULL;
struct task_struct *bdflush_tsk = 0;

static void wakeup_bdflush(int wait)
{
	if (current == bdflush_tsk)
		return;
	wake_up(&bdflush_wait);
	if (wait) {
		run_task_queue(&tq_disk);
		sleep_on(&bdflush_done);
	}
}


/* 
 * Here we attempt to write back old buffers.  We also try to flush inodes 
 * and supers as well, since this function is essentially "update", and 
 * otherwise there would be no way of ensuring that these quantities ever 
 * get written back.  Ideally, we would have a timestamp on the inodes
 * and superblocks so that we could write back only the old ones as well
 */

asmlinkage int sync_old_buffers(void)
{
	int i;
	int ndirty, nwritten;
	int nlist;
	int ncount;
	struct buffer_head * bh, *next;

	sync_supers(0);
	sync_inodes(0);

	ncount = 0;
#ifdef DEBUG
	for(nlist = 0; nlist < NR_LIST; nlist++)
#else
	for(nlist = BUF_DIRTY; nlist <= BUF_DIRTY; nlist++)
#endif
	{
		ndirty = 0;
		nwritten = 0;
	repeat:

		bh = lru_list[nlist];
		if(bh) 
			 for (i = nr_buffers_type[nlist]; i-- > 0; bh = next) {
				 /* We may have stalled while waiting for I/O to complete. */
				 if(bh->b_list != nlist) goto repeat;
				 next = bh->b_next_free;
				 if(!lru_list[nlist]) {
					 printk("Dirty list empty %d\n", i);
					 break;
				 }
				 
				 /* Clean buffer on dirty list?  Refile it */
				 if (nlist == BUF_DIRTY && !buffer_dirty(bh) && !buffer_locked(bh))
				  {
					  refile_buffer(bh);
					  continue;
				  }
				 
				 if (buffer_locked(bh) || !buffer_dirty(bh))
					  continue;
				 ndirty++;
				 if(bh->b_flushtime > jiffies) continue;
				 nwritten++;
				 bh->b_count++;
				 bh->b_flushtime = 0;
#ifdef DEBUG
				 if(nlist != BUF_DIRTY) ncount++;
#endif
				 ll_rw_block(WRITE, 1, &bh);
				 bh->b_count--;
			 }
	}
#ifdef DEBUG
	if (ncount) printk("sync_old_buffers: %d dirty buffers not on dirty list\n", ncount);
	printk("Wrote %d/%d buffers\n", nwritten, ndirty);
#endif
	
	return 0;
}


/* This is the interface to bdflush.  As we get more sophisticated, we can
 * pass tuning parameters to this "process", to adjust how it behaves. 
 * We would want to verify each parameter, however, to make sure that it 
 * is reasonable. */

asmlinkage int sys_bdflush(int func, long data)
{
	if (!suser())
		return -EPERM;

	if (func == 1)
		 return sync_old_buffers();

	/* Basically func 1 means read param 1, 2 means write param 1, etc */
	if (func >= 2) {
		int i = (func-2) >> 1;
		if (i < 0 || i >= N_PARAM)
			return -EINVAL;
		if((func & 1) == 0) {
			int error = verify_area(VERIFY_WRITE, (int*)data, 4);
			if (!error)
				put_user(bdf_prm.data[i], (int*)data);
			return error;
		}
		if (data < bdflush_min[i] || data > bdflush_max[i])
			return -EINVAL;
		bdf_prm.data[i] = data;
	}

	/* Having func 0 used to launch the actual bdflush and then never
	 * return (unless explicitly killed). We return zero here to 
	 * remain semi-compatible with present update(8) programs.
	 */
	return 0;
}

/* This is the actual bdflush daemon itself. It used to be started from
 * the syscall above, but now we launch it ourselves internally with
 * kernel_thread(...)  directly after the first thread in init/main.c */

/* To prevent deadlocks for a loop device:
 * 1) Do non-blocking writes to loop (avoids deadlock with running
 *	out of request blocks).
 * 2) But do a blocking write if the only dirty buffers are loop buffers
 *	(otherwise we go into an infinite busy-loop).
 * 3) Quit writing loop blocks if a freelist went low (avoids deadlock
 *	with running out of free buffers for loop's "real" device).
*/
int bdflush(void * unused) 
{
	int i;
	int ndirty;
	int nlist;
	int ncount;
	struct buffer_head * bh, *next;
	int major;
	int wrta_cmd = WRITEA;	/* non-blocking write for LOOP */

	/*
	 *	We have a bare-bones task_struct, and really should fill
	 *	in a few more things so "top" and /proc/2/{exe,root,cwd}
	 *	display semi-sane things. Not real crucial though...  
	 */

	current->session = 1;
	current->pgrp = 1;
	sprintf(current->comm, "kflushd");
	bdflush_tsk = current;

	/*
	 *	As a kernel thread we want to tamper with system buffers
	 *	and other internals and thus be subject to the SMP locking
	 *	rules. (On a uniprocessor box this does nothing).
	 */


#ifdef __SMP__
	lock_kernel();
	syscall_count++;
#endif
	for (;;) {
#ifdef DEBUG
		printk("bdflush() activated...");
#endif
		
		ncount = 0;
#ifdef DEBUG
		for(nlist = 0; nlist < NR_LIST; nlist++)
#else
		for(nlist = BUF_DIRTY; nlist <= BUF_DIRTY; nlist++)
#endif
		 {
			 ndirty = 0;
			 refilled = 0;
		 repeat:

			 bh = lru_list[nlist];
			 if(bh) 
				  for (i = nr_buffers_type[nlist]; i-- > 0 && ndirty < bdf_prm.b_un.ndirty; 
				       bh = next) {
					  /* We may have stalled while waiting for I/O to complete. */
					  if(bh->b_list != nlist) goto repeat;
					  next = bh->b_next_free;
					  if(!lru_list[nlist]) {
						  printk("Dirty list empty %d\n", i);
						  break;
					  }
					  
					  /* Clean buffer on dirty list?  Refile it */
					  if (nlist == BUF_DIRTY && !buffer_dirty(bh) && !buffer_locked(bh))
					   {
						   refile_buffer(bh);
						   continue;
					   }
					  
					  if (buffer_locked(bh) || !buffer_dirty(bh))
						   continue;
					  major = MAJOR(bh->b_dev);
					  /* Should we write back buffers that are shared or not??
					     currently dirty buffers are not shared, so it does not matter */
					  if (refilled && major == LOOP_MAJOR)
						   continue;
					  bh->b_count++;
					  ndirty++;
					  bh->b_flushtime = 0;
					  if (major == LOOP_MAJOR) {
						  ll_rw_block(wrta_cmd,1, &bh);
						  wrta_cmd = WRITEA;
						  if (buffer_dirty(bh))
							  --ndirty;
					  }
					  else
					  ll_rw_block(WRITE, 1, &bh);
#ifdef DEBUG
					  if(nlist != BUF_DIRTY) ncount++;
#endif
					  bh->b_count--;
				  }
		 }
#ifdef DEBUG
		if (ncount) printk("sys_bdflush: %d dirty buffers not on dirty list\n", ncount);
		printk("sleeping again.\n");
#endif
		/* If we didn't write anything, but there are still
		 * dirty buffers, then make the next write to a
		 * loop device to be a blocking write.
		 * This lets us block--which we _must_ do! */
		if (ndirty == 0 && nr_buffers_type[BUF_DIRTY] > 0) {
			wrta_cmd = WRITE;
			continue;
		}
		run_task_queue(&tq_disk);
		wake_up(&bdflush_done);
		
		/* If there are still a lot of dirty buffers around, skip the sleep
		   and flush some more */
		if(nr_buffers_type[BUF_DIRTY] <= nr_buffers * bdf_prm.b_un.nfract/100) {
			current->signal = 0;
			interruptible_sleep_on(&bdflush_wait);
		}
	}
}


/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * Emacs will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-indent-level: 8
 * c-brace-imaginary-offset: 0
 * c-brace-offset: -8
 * c-argdecl-indent: 8
 * c-label-offset: -8
 * c-continued-statement-offset: 8
 * c-continued-brace-offset: 0
 * End:
 */
