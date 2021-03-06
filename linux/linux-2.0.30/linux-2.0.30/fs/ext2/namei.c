/*
 *  linux/fs/ext2/namei.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/namei.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <asm/segment.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ext2_fs.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/locks.h>

/*
 * define how far ahead to read directories while searching them.
 */
#define NAMEI_RA_CHUNKS  2
#define NAMEI_RA_BLOCKS  4
#define NAMEI_RA_SIZE        (NAMEI_RA_CHUNKS * NAMEI_RA_BLOCKS)
#define NAMEI_RA_INDEX(c,b)  (((c) * NAMEI_RA_BLOCKS) + (b))

/*
 * NOTE! unlike strncmp, ext2_match returns 1 for success, 0 for failure.
 */
static int ext2_match (int len, const char * const name,
		       struct ext2_dir_entry * de)
{
	if (!de || !de->inode || len > EXT2_NAME_LEN)
		return 0;
	/*
	 * "" means "." ---> so paths like "/usr/lib//libc.a" work
	 */
	if (!len && de->name_len == 1 && (de->name[0] == '.') &&
	   (de->name[1] == '\0'))
		return 1;
	if (len != de->name_len)
		return 0;
	return !memcmp(name, de->name, len);
}

/*
 *	ext2_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as a parameter - res_dir). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 */
static struct buffer_head * ext2_find_entry (struct inode * dir,
					     const char * const name, int namelen,
					     struct ext2_dir_entry ** res_dir)
{
	struct super_block * sb;
	struct buffer_head * bh_use[NAMEI_RA_SIZE];
	struct buffer_head * bh_read[NAMEI_RA_SIZE];
	unsigned long offset;
	int block, toread, i, err;

	*res_dir = NULL;
	if (!dir)
		return NULL;
	sb = dir->i_sb;

	if (namelen > EXT2_NAME_LEN)
		return NULL;

	memset (bh_use, 0, sizeof (bh_use));
	toread = 0;
	for (block = 0; block < NAMEI_RA_SIZE; ++block) {
		struct buffer_head * bh;

		if ((block << EXT2_BLOCK_SIZE_BITS (sb)) >= dir->i_size)
			break;
		bh = ext2_getblk (dir, block, 0, &err);
		bh_use[block] = bh;
		if (bh && !buffer_uptodate(bh))
			bh_read[toread++] = bh;
	}

	for (block = 0, offset = 0; offset < dir->i_size; block++) {
		struct buffer_head * bh;
		struct ext2_dir_entry * de;
		char * dlimit;

		if ((block % NAMEI_RA_BLOCKS) == 0 && toread) {
			ll_rw_block (READ, toread, bh_read);
			toread = 0;
		}
		bh = bh_use[block % NAMEI_RA_SIZE];
		if (!bh) {
			ext2_error (sb, "ext2_find_entry",
				    "directory #%lu contains a hole at offset %lu",
				    dir->i_ino, offset);
			offset += sb->s_blocksize;
			continue;
		}
		wait_on_buffer (bh);
		if (!buffer_uptodate(bh)) {
			/*
			 * read error: all bets are off
			 */
			break;
		}

		de = (struct ext2_dir_entry *) bh->b_data;
		dlimit = bh->b_data + sb->s_blocksize;
		while ((char *) de < dlimit) {
			if (!ext2_check_dir_entry ("ext2_find_entry", dir,
						   de, bh, offset))
				goto failure;
			if (de->inode != 0 && ext2_match (namelen, name, de)) {
				for (i = 0; i < NAMEI_RA_SIZE; ++i) {
					if (bh_use[i] != bh)
						brelse (bh_use[i]);
				}
				*res_dir = de;
				return bh;
			}
			offset += de->rec_len;
			de = (struct ext2_dir_entry *)
				((char *) de + de->rec_len);
		}

		brelse (bh);
		if (((block + NAMEI_RA_SIZE) << EXT2_BLOCK_SIZE_BITS (sb)) >=
		    dir->i_size)
			bh = NULL;
		else
			bh = ext2_getblk (dir, block + NAMEI_RA_SIZE, 0, &err);
		bh_use[block % NAMEI_RA_SIZE] = bh;
		if (bh && !buffer_uptodate(bh))
			bh_read[toread++] = bh;
	}

failure:
	for (i = 0; i < NAMEI_RA_SIZE; ++i)
		brelse (bh_use[i]);
	return NULL;
}

int ext2_lookup (struct inode * dir, const char * name, int len,
		 struct inode ** result)
{
	unsigned long ino;
	struct ext2_dir_entry * de;
	struct buffer_head * bh;

	*result = NULL;
	if (!dir)
		return -ENOENT;
	if (!S_ISDIR(dir->i_mode)) {
		iput (dir);
		return -ENOTDIR;
	}
	if (len > EXT2_NAME_LEN) {
		iput (dir);
		return -ENAMETOOLONG;
	}
	if (dcache_lookup(dir, name, len, &ino)) {
		if (!ino) {
			iput(dir);
			return -ENOENT;
		}
		if (!(*result = iget (dir->i_sb, ino))) {
			iput (dir);
			return -EACCES;
		}
		iput (dir);
		return 0;
	}
	ino = dir->i_version;
	if (!(bh = ext2_find_entry (dir, name, len, &de))) {
		if (ino == dir->i_version)
			dcache_add(dir, name, len, 0);
		iput (dir);
		return -ENOENT;
	}
	ino = de->inode;
	dcache_add(dir, name, len, ino);
	brelse (bh);
	if (!(*result = iget (dir->i_sb, ino))) {
		iput (dir);
		return -EACCES;
	}
	iput (dir);
	return 0;
}

/*
 *	ext2_add_entry()
 *
 * adds a file entry to the specified directory, using the same
 * semantics as ext2_find_entry(). It returns NULL if it failed.
 *
 * NOTE!! The inode part of 'de' is left at 0 - which means you
 * may not sleep between calling this and putting something into
 * the entry, as someone else might have used it while you slept.
 */
static struct buffer_head * ext2_add_entry (struct inode * dir,
					    const char * name, int namelen,
					    struct ext2_dir_entry ** res_dir,
					    int *err)
{
	unsigned long offset;
	unsigned short rec_len;
	struct buffer_head * bh;
	struct ext2_dir_entry * de, * de1;
	struct super_block * sb;

	*err = -EINVAL;
	*res_dir = NULL;
	if (!dir)
		return NULL;
	sb = dir->i_sb;

	if (namelen > EXT2_NAME_LEN)
	{
		*err = -ENAMETOOLONG;
		return NULL;
	}

	if (!namelen)
		return NULL;
	/*
	 * Is this a busy deleted directory?  Can't create new files if so
	 */
	if (dir->i_size == 0)
	{
		*err = -ENOENT;
		return NULL;
	}
	bh = ext2_bread (dir, 0, 0, err);
	if (!bh)
		return NULL;
	rec_len = EXT2_DIR_REC_LEN(namelen);
	offset = 0;
	de = (struct ext2_dir_entry *) bh->b_data;
	*err = -ENOSPC;
	while (1) {
		if ((char *)de >= sb->s_blocksize + bh->b_data) {
			brelse (bh);
			bh = NULL;
			bh = ext2_bread (dir, offset >> EXT2_BLOCK_SIZE_BITS(sb), 1, err);
			if (!bh)
				return NULL;
			if (dir->i_size <= offset) {
				if (dir->i_size == 0) {
					*err = -ENOENT;
					return NULL;
				}

				ext2_debug ("creating next block\n");

				de = (struct ext2_dir_entry *) bh->b_data;
				de->inode = 0;
				de->rec_len = sb->s_blocksize;
				dir->i_size = offset + sb->s_blocksize;
				dir->i_dirt = 1;
			} else {

				ext2_debug ("skipping to next block\n");

				de = (struct ext2_dir_entry *) bh->b_data;
			}
		}
		if (!ext2_check_dir_entry ("ext2_add_entry", dir, de, bh,
					   offset)) {
			*err = -ENOENT;
			brelse (bh);
			return NULL;
		}
		if (de->inode != 0 && ext2_match (namelen, name, de)) {
				*err = -EEXIST;
				brelse (bh);
				return NULL;
		}
		if ((de->inode == 0 && de->rec_len >= rec_len) ||
		    (de->rec_len >= EXT2_DIR_REC_LEN(de->name_len) + rec_len)) {
			offset += de->rec_len;
			if (de->inode) {
				de1 = (struct ext2_dir_entry *) ((char *) de +
					EXT2_DIR_REC_LEN(de->name_len));
				de1->rec_len = de->rec_len -
					EXT2_DIR_REC_LEN(de->name_len);
				de->rec_len = EXT2_DIR_REC_LEN(de->name_len);
				de = de1;
			}
			de->inode = 0;
			de->name_len = namelen;
			memcpy (de->name, name, namelen);
			/*
			 * XXX shouldn't update any times until successful
			 * completion of syscall, but too many callers depend
			 * on this.
			 *
			 * XXX similarly, too many callers depend on
			 * ext2_new_inode() setting the times, but error
			 * recovery deletes the inode, so the worst that can
			 * happen is that the times are slightly out of date
			 * and/or different from the directory change time.
			 */
			dir->i_mtime = dir->i_ctime = CURRENT_TIME;
			dir->i_dirt = 1;
			dir->i_version = ++event;
			mark_buffer_dirty(bh, 1);
			*res_dir = de;
			*err = 0;
			return bh;
		}
		offset += de->rec_len;
		de = (struct ext2_dir_entry *) ((char *) de + de->rec_len);
	}
	brelse (bh);
	return NULL;
}

/*
 * ext2_delete_entry deletes a directory entry by merging it with the
 * previous entry
 */
static int ext2_delete_entry (struct ext2_dir_entry * dir,
			      struct buffer_head * bh)
{
	struct ext2_dir_entry * de, * pde;
	int i;

	i = 0;
	pde = NULL;
	de = (struct ext2_dir_entry *) bh->b_data;
	while (i < bh->b_size) {
		if (!ext2_check_dir_entry ("ext2_delete_entry", NULL, 
					   de, bh, i))
			return -EIO;
		if (de == dir)  {
			if (pde)
				pde->rec_len += dir->rec_len;
			dir->inode = 0;
			return 0;
		}
		i += de->rec_len;
		pde = de;
		de = (struct ext2_dir_entry *) ((char *) de + de->rec_len);
	}
	return -ENOENT;
}

int ext2_create (struct inode * dir,const char * name, int len, int mode,
		 struct inode ** result)
{
	struct inode * inode;
	struct buffer_head * bh;
	struct ext2_dir_entry * de;
	int err;

	*result = NULL;
	if (!dir)
		return -ENOENT;
	inode = ext2_new_inode (dir, mode, &err);
	if (!inode) {
		iput (dir);
		return err;
	}
	inode->i_op = &ext2_file_inode_operations;
	inode->i_mode = mode;
	inode->i_dirt = 1;
	bh = ext2_add_entry (dir, name, len, &de, &err);
	if (!bh) {
		inode->i_nlink--;
		inode->i_dirt = 1;
		iput (inode);
		iput (dir);
		return err;
	}
	de->inode = inode->i_ino;
	dir->i_version = ++event;
	dcache_add(dir, de->name, de->name_len, de->inode);
	mark_buffer_dirty(bh, 1);
	if (IS_SYNC(dir)) {
		ll_rw_block (WRITE, 1, &bh);
		wait_on_buffer (bh);
	}
	brelse (bh);
	iput (dir);
	*result = inode;
	return 0;
}

int ext2_mknod (struct inode * dir, const char * name, int len, int mode,
		int rdev)
{
	struct inode * inode;
	struct buffer_head * bh;
	struct ext2_dir_entry * de;
	int err;

	if (!dir)
		return -ENOENT;

	if (len > EXT2_NAME_LEN) {
		iput (dir);
		return -ENAMETOOLONG;
	}
	bh = ext2_find_entry (dir, name, len, &de);
	if (bh) {
		brelse (bh);
		iput (dir);
		return -EEXIST;
	}
	inode = ext2_new_inode (dir, mode, &err);
	if (!inode) {
		iput (dir);
		return err;
	}
	inode->i_uid = current->fsuid;
	inode->i_mode = mode;
	inode->i_op = NULL;
	if (S_ISREG(inode->i_mode))
		inode->i_op = &ext2_file_inode_operations;
	else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &ext2_dir_inode_operations;
		if (dir->i_mode & S_ISGID)
			inode->i_mode |= S_ISGID;
	}
	else if (S_ISLNK(inode->i_mode))
		inode->i_op = &ext2_symlink_inode_operations;
	else if (S_ISCHR(inode->i_mode))
		inode->i_op = &chrdev_inode_operations;
	else if (S_ISBLK(inode->i_mode))
		inode->i_op = &blkdev_inode_operations;
	else if (S_ISFIFO(inode->i_mode)) 
		init_fifo(inode);
	if (S_ISBLK(mode) || S_ISCHR(mode))
		inode->i_rdev = to_kdev_t(rdev);
	inode->i_dirt = 1;
	bh = ext2_add_entry (dir, name, len, &de, &err);
	if (!bh) {
		inode->i_nlink--;
		inode->i_dirt = 1;
		iput (inode);
		iput (dir);
		return err;
	}
	de->inode = inode->i_ino;
	dir->i_version = ++event;
	dcache_add(dir, de->name, de->name_len, de->inode);
	mark_buffer_dirty(bh, 1);
	if (IS_SYNC(dir)) {
		ll_rw_block (WRITE, 1, &bh);
		wait_on_buffer (bh);
	}
	brelse (bh);
	iput (dir);
	iput (inode);
	return 0;
}

int ext2_mkdir (struct inode * dir, const char * name, int len, int mode)
{
	struct inode * inode;
	struct buffer_head * bh, * dir_block;
	struct ext2_dir_entry * de;
	int err;

	if (!dir)
		return -ENOENT;
	if (len > EXT2_NAME_LEN) {
		iput (dir);
		return -ENAMETOOLONG;
	}
	bh = ext2_find_entry (dir, name, len, &de);
	if (bh) {
		brelse (bh);
		iput (dir);
		return -EEXIST;
	}
	if (dir->i_nlink >= EXT2_LINK_MAX) {
		iput (dir);
		return -EMLINK;
	}
	inode = ext2_new_inode (dir, S_IFDIR, &err);
	if (!inode) {
		iput (dir);
		return err;
	}
	inode->i_op = &ext2_dir_inode_operations;
	inode->i_size = inode->i_sb->s_blocksize;
	dir_block = ext2_bread (inode, 0, 1, &err);
	if (!dir_block) {
		iput (dir);
		inode->i_nlink--;
		inode->i_dirt = 1;
		iput (inode);
		return err;
	}
	inode->i_blocks = inode->i_sb->s_blocksize / 512;
	de = (struct ext2_dir_entry *) dir_block->b_data;
	de->inode = inode->i_ino;
	de->name_len = 1;
	de->rec_len = EXT2_DIR_REC_LEN(de->name_len);
	strcpy (de->name, ".");
	de = (struct ext2_dir_entry *) ((char *) de + de->rec_len);
	de->inode = dir->i_ino;
	de->rec_len = inode->i_sb->s_blocksize - EXT2_DIR_REC_LEN(1);
	de->name_len = 2;
	strcpy (de->name, "..");
	inode->i_nlink = 2;
	mark_buffer_dirty(dir_block, 1);
	brelse (dir_block);
	inode->i_mode = S_IFDIR | (mode & (S_IRWXUGO|S_ISVTX) & ~current->fs->umask);
	if (dir->i_mode & S_ISGID)
		inode->i_mode |= S_ISGID;
	inode->i_dirt = 1;
	bh = ext2_add_entry (dir, name, len, &de, &err);
	if (!bh) {
		iput (dir);
		inode->i_nlink = 0;
		inode->i_dirt = 1;
		iput (inode);
		return err;
	}
	de->inode = inode->i_ino;
	dir->i_version = ++event;
	dcache_add(dir, de->name, de->name_len, de->inode);
	mark_buffer_dirty(bh, 1);
	if (IS_SYNC(dir)) {
		ll_rw_block (WRITE, 1, &bh);
		wait_on_buffer (bh);
	}
	dir->i_nlink++;
	dir->i_dirt = 1;
	iput (dir);
	iput (inode);
	brelse (bh);
	return 0;
}

/*
 * routine to check that the specified directory is empty (for rmdir)
 */
static int empty_dir (struct inode * inode)
{
	unsigned long offset;
	struct buffer_head * bh;
	struct ext2_dir_entry * de, * de1;
	struct super_block * sb;
	int err;

	sb = inode->i_sb;
	if (inode->i_size < EXT2_DIR_REC_LEN(1) + EXT2_DIR_REC_LEN(2) ||
	    !(bh = ext2_bread (inode, 0, 0, &err))) {
	    	ext2_warning (inode->i_sb, "empty_dir",
			      "bad directory (dir #%lu) - no data block",
			      inode->i_ino);
		return 1;
	}
	de = (struct ext2_dir_entry *) bh->b_data;
	de1 = (struct ext2_dir_entry *) ((char *) de + de->rec_len);
	if (de->inode != inode->i_ino || !de1->inode || 
	    strcmp (".", de->name) || strcmp ("..", de1->name)) {
	    	ext2_warning (inode->i_sb, "empty_dir",
			      "bad directory (dir #%lu) - no `.' or `..'",
			      inode->i_ino);
		return 1;
	}
	offset = de->rec_len + de1->rec_len;
	de = (struct ext2_dir_entry *) ((char *) de1 + de1->rec_len);
	while (offset < inode->i_size ) {
		if ((void *) de >= (void *) (bh->b_data + sb->s_blocksize)) {
			brelse (bh);
			bh = ext2_bread (inode, offset >> EXT2_BLOCK_SIZE_BITS(sb), 1, &err);
			if (!bh) {
				ext2_error (sb, "empty_dir",
					    "directory #%lu contains a hole at offset %lu",
					    inode->i_ino, offset);
				offset += sb->s_blocksize;
				continue;
			}
			de = (struct ext2_dir_entry *) bh->b_data;
		}
		if (!ext2_check_dir_entry ("empty_dir", inode, de, bh,
					   offset)) {
			brelse (bh);
			return 1;
		}
		if (de->inode) {
			brelse (bh);
			return 0;
		}
		offset += de->rec_len;
		de = (struct ext2_dir_entry *) ((char *) de + de->rec_len);
	}
	brelse (bh);
	return 1;
}

int ext2_rmdir (struct inode * dir, const char * name, int len)
{
	int retval;
	struct inode * inode;
	struct buffer_head * bh;
	struct ext2_dir_entry * de;

repeat:
	if (!dir)
		return -ENOENT;
	inode = NULL;
	if (len > EXT2_NAME_LEN) {
		iput (dir);
		return -ENAMETOOLONG;
	}
	bh = ext2_find_entry (dir, name, len, &de);
	retval = -ENOENT;
	if (!bh)
		goto end_rmdir;
	retval = -EPERM;
	if (!(inode = iget (dir->i_sb, de->inode)))
		goto end_rmdir;
	if (inode->i_sb->dq_op)
		inode->i_sb->dq_op->initialize (inode, -1);
	if (inode->i_dev != dir->i_dev) {
		retval = -EBUSY;
		goto end_rmdir;
	}
	if (de->inode != inode->i_ino) {
		iput(inode);
		brelse(bh);
		current->counter = 0;
		schedule();
		goto repeat;
	}
        if ((dir->i_mode & S_ISVTX) && !fsuser() &&
            current->fsuid != inode->i_uid &&
            current->fsuid != dir->i_uid)
		goto end_rmdir;
	if (inode == dir)	/* we may not delete ".", but "../dir" is ok */
		goto end_rmdir;
	if (!S_ISDIR(inode->i_mode)) {
		retval = -ENOTDIR;
		goto end_rmdir;
	}
	down(&inode->i_sem);
	if (!empty_dir (inode))
		retval = -ENOTEMPTY;
	else if (de->inode != inode->i_ino)
		retval = -ENOENT;
	else {
		if (inode->i_count > 1) {
		/*
		 * Are we deleting the last instance of a busy directory?
		 * Better clean up if so.
		 *
		 * Make directory empty (it will be truncated when finally
		 * dereferenced).  This also inhibits ext2_add_entry.
		 */
			inode->i_size = 0;
		}
		retval = ext2_delete_entry (de, bh);
		dir->i_version = ++event;
	}
	up(&inode->i_sem);
	if (retval)
		goto end_rmdir;
	mark_buffer_dirty(bh, 1);
	if (IS_SYNC(dir)) {
		ll_rw_block (WRITE, 1, &bh);
		wait_on_buffer (bh);
	}
	if (inode->i_nlink != 2)
		ext2_warning (inode->i_sb, "ext2_rmdir",
			      "empty directory has nlink!=2 (%d)",
			      inode->i_nlink);
	inode->i_version = ++event;
	inode->i_nlink = 0;
	inode->i_dirt = 1;
	dir->i_nlink--;
	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	dir->i_dirt = 1;
end_rmdir:
	iput (dir);
	iput (inode);
	brelse (bh);
	return retval;
}

int ext2_unlink (struct inode * dir, const char * name, int len)
{
	int retval;
	struct inode * inode;
	struct buffer_head * bh;
	struct ext2_dir_entry * de;

repeat:
	if (!dir)
		return -ENOENT;
	retval = -ENOENT;
	inode = NULL;
	if (len > EXT2_NAME_LEN) {
		iput (dir);
		return -ENAMETOOLONG;
	}
	bh = ext2_find_entry (dir, name, len, &de);
	if (!bh)
		goto end_unlink;
	if (!(inode = iget (dir->i_sb, de->inode)))
		goto end_unlink;
	if (inode->i_sb->dq_op)
		inode->i_sb->dq_op->initialize (inode, -1);
	retval = -EPERM;
	if (S_ISDIR(inode->i_mode))
		goto end_unlink;
	if (IS_APPEND(inode) || IS_IMMUTABLE(inode))
		goto end_unlink;
	if (de->inode != inode->i_ino) {
		iput(inode);
		brelse(bh);
		current->counter = 0;
		schedule();
		goto repeat;
	}
	if ((dir->i_mode & S_ISVTX) && !fsuser() &&
	    current->fsuid != inode->i_uid &&
	    current->fsuid != dir->i_uid)
		goto end_unlink;
	if (!inode->i_nlink) {
		ext2_warning (inode->i_sb, "ext2_unlink",
			      "Deleting nonexistent file (%lu), %d",
			      inode->i_ino, inode->i_nlink);
		inode->i_nlink = 1;
	}
	retval = ext2_delete_entry (de, bh);
	if (retval)
		goto end_unlink;
	dir->i_version = ++event;
	mark_buffer_dirty(bh, 1);
	if (IS_SYNC(dir)) {
		ll_rw_block (WRITE, 1, &bh);
		wait_on_buffer (bh);
	}
	dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	dir->i_dirt = 1;
	inode->i_nlink--;
	inode->i_dirt = 1;
	inode->i_ctime = dir->i_ctime;
	retval = 0;
end_unlink:
	brelse (bh);
	iput (inode);
	iput (dir);
	return retval;
}

int ext2_symlink (struct inode * dir, const char * name, int len,
		  const char * symname)
{
	struct ext2_dir_entry * de;
	struct inode * inode = NULL;
	struct buffer_head * bh = NULL, * name_block = NULL;
	char * link;
	int i, err;
	int l;
	char c;

	if (!(inode = ext2_new_inode (dir, S_IFLNK, &err))) {
		iput (dir);
		return err;
	}
	inode->i_mode = S_IFLNK | S_IRWXUGO;
	inode->i_op = &ext2_symlink_inode_operations;
	for (l = 0; l < inode->i_sb->s_blocksize - 1 &&
	     symname [l]; l++)
		;
	if (l >= sizeof (inode->u.ext2_i.i_data)) {

		ext2_debug ("l=%d, normal symlink\n", l);

		name_block = ext2_bread (inode, 0, 1, &err);
		if (!name_block) {
			iput (dir);
			inode->i_nlink--;
			inode->i_dirt = 1;
			iput (inode);
			return err;
		}
		link = name_block->b_data;
	} else {
		link = (char *) inode->u.ext2_i.i_data;

		ext2_debug ("l=%d, fast symlink\n", l);

	}
	i = 0;
	while (i < inode->i_sb->s_blocksize - 1 && (c = *(symname++)))
		link[i++] = c;
	link[i] = 0;
	if (name_block) {
		mark_buffer_dirty(name_block, 1);
		brelse (name_block);
	}
	inode->i_size = i;
	inode->i_dirt = 1;

	bh = ext2_find_entry (dir, name, len, &de);
	if (bh) {
		inode->i_nlink--;
		inode->i_dirt = 1;
		iput (inode);
		brelse (bh);
		iput (dir);
		return -EEXIST;
	}
	bh = ext2_add_entry (dir, name, len, &de, &err);
	if (!bh) {
		inode->i_nlink--;
		inode->i_dirt = 1;
		iput (inode);
		iput (dir);
		return err;
	}
	de->inode = inode->i_ino;
	dir->i_version = ++event;
	dcache_add(dir, de->name, de->name_len, de->inode);
	mark_buffer_dirty(bh, 1);
	if (IS_SYNC(dir)) {
		ll_rw_block (WRITE, 1, &bh);
		wait_on_buffer (bh);
	}
	brelse (bh);
	iput (dir);
	iput (inode);
	return 0;
}

int ext2_link (struct inode * oldinode, struct inode * dir,
	       const char * name, int len)
{
	struct ext2_dir_entry * de;
	struct buffer_head * bh;
	int err;

	if (S_ISDIR(oldinode->i_mode)) {
		iput (oldinode);
		iput (dir);
		return -EPERM;
	}
	if (IS_APPEND(oldinode) || IS_IMMUTABLE(oldinode)) {
		iput (oldinode);
		iput (dir);
		return -EPERM;
	}
	if (oldinode->i_nlink >= EXT2_LINK_MAX) {
		iput (oldinode);
		iput (dir);
		return -EMLINK;
	}
	bh = ext2_find_entry (dir, name, len, &de);
	if (bh) {
		brelse (bh);
		iput (dir);
		iput (oldinode);
		return -EEXIST;
	}
	bh = ext2_add_entry (dir, name, len, &de, &err);
	if (!bh) {
		iput (dir);
		iput (oldinode);
		return err;
	}
	de->inode = oldinode->i_ino;
	dir->i_version = ++event;
	dcache_add(dir, de->name, de->name_len, de->inode);
	mark_buffer_dirty(bh, 1);
	if (IS_SYNC(dir)) {
		ll_rw_block (WRITE, 1, &bh);
		wait_on_buffer (bh);
	}
	brelse (bh);
	iput (dir);
	oldinode->i_nlink++;
	oldinode->i_ctime = CURRENT_TIME;
	oldinode->i_dirt = 1;
	iput (oldinode);
	return 0;
}

static int subdir (struct inode * new_inode, struct inode * old_inode)
{
	int ino;
	int result;

	new_inode->i_count++;
	result = 0;
	for (;;) {
		if (new_inode == old_inode) {
			result = 1;
			break;
		}
		if (new_inode->i_dev != old_inode->i_dev)
			break;
		ino = new_inode->i_ino;
		if (ext2_lookup (new_inode, "..", 2, &new_inode))
			break;
		if (new_inode->i_ino == ino)
			break;
	}
	iput (new_inode);
	return result;
}

#define PARENT_INO(buffer) \
	((struct ext2_dir_entry *) ((char *) buffer + \
	((struct ext2_dir_entry *) buffer)->rec_len))->inode

#define PARENT_NAME(buffer) \
	((struct ext2_dir_entry *) ((char *) buffer + \
	((struct ext2_dir_entry *) buffer)->rec_len))->name

/*
 * rename uses retrying to avoid race-conditions: at least they should be
 * minimal.
 * it tries to allocate all the blocks, then sanity-checks, and if the sanity-
 * checks fail, it tries to restart itself again. Very practical - no changes
 * are done until we know everything works ok.. and then all the changes can be
 * done in one fell swoop when we have claimed all the buffers needed.
 *
 * Anybody can rename anything with this: the permission checks are left to the
 * higher-level routines.
 */
static int do_ext2_rename (struct inode * old_dir, const char * old_name,
			   int old_len, struct inode * new_dir,
			   const char * new_name, int new_len,
			   int must_be_dir)
{
	struct inode * old_inode, * new_inode;
	struct buffer_head * old_bh, * new_bh, * dir_bh;
	struct ext2_dir_entry * old_de, * new_de;
	int retval;

	goto start_up;
try_again:
	if (new_bh && new_de) {
		ext2_delete_entry(new_de, new_bh);
		new_dir->i_version = ++event;
	}
	brelse (old_bh);
	brelse (new_bh);
	brelse (dir_bh);
	iput (old_inode);
	iput (new_inode);
	current->counter = 0;
	schedule ();
start_up:
	old_inode = new_inode = NULL;
	old_bh = new_bh = dir_bh = NULL;
	new_de = NULL;
	retval = -ENAMETOOLONG;
	if (old_len > EXT2_NAME_LEN)
		goto end_rename;

	old_bh = ext2_find_entry (old_dir, old_name, old_len, &old_de);
	retval = -ENOENT;
	if (!old_bh)
		goto end_rename;
	old_inode = __iget (old_dir->i_sb, old_de->inode, 0); /* don't cross mnt-points */
	if (!old_inode)
		goto end_rename;
	if (must_be_dir && !S_ISDIR(old_inode->i_mode))
		goto end_rename;
	retval = -EPERM;
	if ((old_dir->i_mode & S_ISVTX) && 
	    current->fsuid != old_inode->i_uid &&
	    current->fsuid != old_dir->i_uid && !fsuser())
		goto end_rename;
	if (IS_APPEND(old_inode) || IS_IMMUTABLE(old_inode))
		goto end_rename;
	new_bh = ext2_find_entry (new_dir, new_name, new_len, &new_de);
	if (new_bh) {
		new_inode = __iget (new_dir->i_sb, new_de->inode, 0); /* no mntp cross */
		if (!new_inode) {
			brelse (new_bh);
			new_bh = NULL;
		} else {
			if (new_inode->i_sb->dq_op)
				new_inode->i_sb->dq_op->initialize (new_inode, -1);
		}
	}
	if (new_inode == old_inode) {
		retval = 0;
		goto end_rename;
	}
	if (new_inode && S_ISDIR(new_inode->i_mode)) {
		retval = -EISDIR;
		if (!S_ISDIR(old_inode->i_mode))
			goto end_rename;
		retval = -EINVAL;
		if (subdir (new_dir, old_inode))
			goto end_rename;
		retval = -ENOTEMPTY;
		if (!empty_dir (new_inode))
			goto end_rename;
		retval = -EBUSY;
		if (new_inode->i_count > 1)
			goto end_rename;
	}
	retval = -EPERM;
	if (new_inode && (new_dir->i_mode & S_ISVTX) &&
	    current->fsuid != new_inode->i_uid &&
	    current->fsuid != new_dir->i_uid && !fsuser())
		goto end_rename;
	if (S_ISDIR(old_inode->i_mode)) {
		retval = -ENOTDIR;
		if (new_inode && !S_ISDIR(new_inode->i_mode))
			goto end_rename;
		retval = -EINVAL;
		if (subdir (new_dir, old_inode))
			goto end_rename;
		dir_bh = ext2_bread (old_inode, 0, 0, &retval);
		if (!dir_bh)
			goto end_rename;
		if (PARENT_INO(dir_bh->b_data) != old_dir->i_ino)
			goto end_rename;
		retval = -EMLINK;
		if (!new_inode && new_dir->i_nlink >= EXT2_LINK_MAX)
			goto end_rename;
	}
	if (!new_bh)
		new_bh = ext2_add_entry (new_dir, new_name, new_len, &new_de,
					 &retval);
	if (!new_bh)
		goto end_rename;
	new_dir->i_version = ++event;
	/*
	 * sanity checking before doing the rename - avoid races
	 */
	if (new_inode && (new_de->inode != new_inode->i_ino))
		goto try_again;
	if (new_de->inode && !new_inode)
		goto try_again;
	if (old_de->inode != old_inode->i_ino)
		goto try_again;
	/*
	 * ok, that's it
	 */
	new_de->inode = old_inode->i_ino;
	dcache_add(new_dir, new_de->name, new_de->name_len, new_de->inode);
	retval = ext2_delete_entry (old_de, old_bh);
	if (retval == -ENOENT)
		goto try_again;
	if (retval)
		goto end_rename;
	old_dir->i_version = ++event;
	if (new_inode) {
		new_inode->i_nlink--;
		new_inode->i_ctime = CURRENT_TIME;
		new_inode->i_dirt = 1;
	}
	old_dir->i_ctime = old_dir->i_mtime = CURRENT_TIME;
	old_dir->i_dirt = 1;
	if (dir_bh) {
		PARENT_INO(dir_bh->b_data) = new_dir->i_ino;
		dcache_add(old_inode, "..", 2, new_dir->i_ino);
		mark_buffer_dirty(dir_bh, 1);
		old_dir->i_nlink--;
		old_dir->i_dirt = 1;
		if (new_inode) {
			new_inode->i_nlink--;
			new_inode->i_dirt = 1;
		} else {
			new_dir->i_nlink++;
			new_dir->i_dirt = 1;
		}
	}
	mark_buffer_dirty(old_bh,  1);
	if (IS_SYNC(old_dir)) {
		ll_rw_block (WRITE, 1, &old_bh);
		wait_on_buffer (old_bh);
	}
	mark_buffer_dirty(new_bh, 1);
	if (IS_SYNC(new_dir)) {
		ll_rw_block (WRITE, 1, &new_bh);
		wait_on_buffer (new_bh);
	}
	retval = 0;
end_rename:
	brelse (dir_bh);
	brelse (old_bh);
	brelse (new_bh);
	iput (old_inode);
	iput (new_inode);
	iput (old_dir);
	iput (new_dir);
	return retval;
}

/*
 * Ok, rename also locks out other renames, as they can change the parent of
 * a directory, and we don't want any races. Other races are checked for by
 * "do_rename()", which restarts if there are inconsistencies.
 *
 * Note that there is no race between different filesystems: it's only within
 * the same device that races occur: many renames can happen at once, as long
 * as they are on different partitions.
 *
 * In the second extended file system, we use a lock flag stored in the memory
 * super-block.  This way, we really lock other renames only if they occur
 * on the same file system
 */
int ext2_rename (struct inode * old_dir, const char * old_name, int old_len,
		 struct inode * new_dir, const char * new_name, int new_len,
		 int must_be_dir)
{
	int result;

	while (old_dir->i_sb->u.ext2_sb.s_rename_lock)
		sleep_on (&old_dir->i_sb->u.ext2_sb.s_rename_wait);
	old_dir->i_sb->u.ext2_sb.s_rename_lock = 1;
	result = do_ext2_rename (old_dir, old_name, old_len, new_dir,
				 new_name, new_len, must_be_dir);
	old_dir->i_sb->u.ext2_sb.s_rename_lock = 0;
	wake_up (&old_dir->i_sb->u.ext2_sb.s_rename_wait);
	return result;
}
