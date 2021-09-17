# 启动流程

## init call
定义
```c
//include/linux/init.h
/* initcalls are now grouped by functionality into separate 
 * subsections. Ordering inside the subsections is determined
 * by link order. 
 * For backwards compatibility, initcall() puts the call in 
 * the device init subsection.
 *
 * The `id' arg to __define_initcall() is needed so that multiple initcalls
 * can point at the same handler without causing duplicate-symbol build errors.
 */

#define __define_initcall(level,fn,id) \
	static initcall_t __initcall_##fn##id __used \
	__attribute__((__section__(".initcall" level ".init"))) = fn

/*
 * Early initcalls run before initializing SMP.
 *
 * Only for built-in code, not modules.
 */
#define early_initcall(fn)		__define_initcall("early",fn,early)

/*
 * A "pure" initcall has no dependencies on anything else, and purely
 * initializes variables that couldn't be statically initialized.
 *
 * This only exists for built-in code, not for modules.
 */
#define pure_initcall(fn)		__define_initcall("0",fn,0)

#define core_initcall(fn)		__define_initcall("1",fn,1)
#define core_initcall_sync(fn)		__define_initcall("1s",fn,1s)
#define postcore_initcall(fn)		__define_initcall("2",fn,2)
#define postcore_initcall_sync(fn)	__define_initcall("2s",fn,2s)
#define arch_initcall(fn)		__define_initcall("3",fn,3)
#define arch_initcall_sync(fn)		__define_initcall("3s",fn,3s)
#define subsys_initcall(fn)		__define_initcall("4",fn,4)
#define subsys_initcall_sync(fn)	__define_initcall("4s",fn,4s)
#define fs_initcall(fn)			__define_initcall("5",fn,5)
#define fs_initcall_sync(fn)		__define_initcall("5s",fn,5s)
#define rootfs_initcall(fn)		__define_initcall("rootfs",fn,rootfs)
#define device_initcall(fn)		__define_initcall("6",fn,6)
#define device_initcall_sync(fn)	__define_initcall("6s",fn,6s)
#define late_initcall(fn)		__define_initcall("7",fn,7)
#define late_initcall_sync(fn)		__define_initcall("7s",fn,7s)

#define __initcall(fn) device_initcall(fn)

#define __exitcall(fn) \
	static exitcall_t __exitcall_##fn __exit_call = fn

#define console_initcall(fn) \
	static initcall_t __initcall_##fn \
	__used __section(.con_initcall.init) = fn

#define security_initcall(fn) \
	static initcall_t __initcall_##fn \
	__used __section(.security_initcall.init) = fn

```

顺序
```c
#define INIT_SETUP(initsetup_align)					\
		. = ALIGN(initsetup_align);				\
		VMLINUX_SYMBOL(__setup_start) = .;			\
		*(.init.setup)						\
		VMLINUX_SYMBOL(__setup_end) = .;

#define INITCALLS							\
	*(.initcallearly.init)						\
	VMLINUX_SYMBOL(__early_initcall_end) = .;			\
  	*(.initcall0.init)						\
  	*(.initcall0s.init)						\
  	*(.initcall1.init)						\
  	*(.initcall1s.init)						\
  	*(.initcall2.init)						\
  	*(.initcall2s.init)						\
  	*(.initcall3.init)						\
  	*(.initcall3s.init)						\
  	*(.initcall4.init)						\
  	*(.initcall4s.init)						\
  	*(.initcall5.init)						\
  	*(.initcall5s.init)						\
	*(.initcallrootfs.init)						\
  	*(.initcall6.init)						\
  	*(.initcall6s.init)						\
  	*(.initcall7.init)						\
  	*(.initcall7s.init)

#define INIT_CALLS							\
		VMLINUX_SYMBOL(__initcall_start) = .;			\
		INITCALLS						\
		VMLINUX_SYMBOL(__initcall_end) = .;

#define CON_INITCALL							\
		VMLINUX_SYMBOL(__con_initcall_start) = .;		\
		*(.con_initcall.init)					\
		VMLINUX_SYMBOL(__con_initcall_end) = .;

#define SECURITY_INITCALL						\
		VMLINUX_SYMBOL(__security_initcall_start) = .;		\
		*(.security_initcall.init)				\
		VMLINUX_SYMBOL(__security_initcall_end) = .;
```

### early_initcall
```c
//kernel/softirq.c
early_initcall(spawn_ksoftirqd);
```

### pure_initcall
```c
//net/core/net_namespace.c
pure_initcall(net_ns_init);
```

### core_initcall
```c
//net/socket.c
core_initcall(sock_init);	/* early initcall */
```

### core_initcall_sync
```c
```

### postcore_initcall
```c
//drivers/s390/kvm/kvm_virtio.c
postcore_initcall(kvm_devices_init);
```
### postcore_initcall_sync
```c
```
### arch_initcall
```c
//arch/x86/pci/init.c
arch_initcall(pci_arch_init);
```
### arch_initcall_sync
```c
```
### subsys_initcall
```c
//block/blk-iopoll.c
subsys_initcall(blk_iopoll_setup);
```
### subsys_initcall_sync
```c
//drivers/s390/cio/css.c
subsys_initcall_sync(channel_subsystem_init_sync);
```
### fs_initcall
```c
//drivers/acpi/event.c
fs_initcall(acpi_event_init);
```
### fs_initcall_sync
```c
//drivers/pci/quirks.c
fs_initcall_sync(pci_apply_final_quirks);
```
### rootfs_initcall
```c
//init/initramfs.c
rootfs_initcall(populate_rootfs);
```
### device_initcall
```c
//arch/x86/kernel/i8237.c
device_initcall(i8237A_init_sysfs);
```
### device_initcall_sync
```c
```
### late_initcall
```c
//arch/x86/kernel/acpi/boot.c
late_initcall(hpet_insert_resource);
```
### late_initcall_sync
```c
//net/core/dev.c
late_initcall_sync(initialize_hashrnd);
```