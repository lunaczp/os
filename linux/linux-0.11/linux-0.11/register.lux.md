# Register

## 内核态对用户态数据的寻址
当用户调用系统调用进入内核态的时候，内核代码段（`cs`）和内核数据段（`ds`）被建立。但是`fs`指向了用户数据段(`0x17`)。这样做的好处是，对于传入内核的参数，可以直接通过`fs:address`来寻址。

__实际上，所有的内核代码，都使用了`get_fs_byte`，`get_fs_word`这一类函数进行用户态数据的寻址。__ 否则是有问题的。

示例：
```
//block_dev.c:block_write
//讲用户态数据写入block

		while (chars-->0)
			*(p++) = get_fs_byte(buf++);//lux 写数据
		bh->b_dirt = 1;
		brelse(bh);
```

```
//file_dev.c:file_write
//写文件

		while (c-->0)
			*(p++) = get_fs_byte(buf++);//lux 写数据(注意，这里其实是写入到了bh里，并没有落地到磁盘)
		brelse(bh);
```

```
//namei.c:find_entry
//读取用户传入的文件名指针

	if (namelen==2 && get_fs_byte(name)=='.' && get_fs_byte(name+1)=='.') {
```


### 问题
由于这样的设计，那么当调用一个内核中的函数，而传入的参数不是来自用户态，而是来自内核态的时候，就需要特殊处理：
```
//exec.c
//由于namei期待的参数，是来自用户态的指针，而interp是来自于内核态，所以需要临时修改fs
		old_fs = get_fs();
		set_fs(get_ds());//lux 由于下面使用的interp是来自内核态，需要临时设置fs为内核数据段地址。
		if (!(inode=namei(interp))) { /*lux get executables inode，拿到解释器文件*/
			set_fs(old_fs);
			retval = -ENOENT;
			goto exec_error1;
		}
		set_fs(old_fs);
```