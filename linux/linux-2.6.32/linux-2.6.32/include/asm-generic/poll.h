#ifndef __ASM_GENERIC_POLL_H
#define __ASM_GENERIC_POLL_H
//lux see http://man7.org/linux/man-pages/man2/poll.2.html
/* These are specified by iBCS2 */
#define POLLIN		0x0001	//lux	there is data to read
#define POLLPRI		0x0002	//lux	There is some exceptional condition on the file descriptor
#define POLLOUT		0x0004	//lux	Writing is now possible, though a write larger that the available space in a socket or pipe will still block (unless O_NONBLOCK is set)
#define POLLERR		0x0008	//lux	Error condition
#define POLLHUP		0x0010	//lux	Hang up (only returned in revents; ignored in events). Note that when reading from a channel such as a pipe or a stream socket, this event merely indicates that the peer closed its end of the channel. Subsequent reads from the channel will return 0 (end of file) only after all outstanding data in the channel has been consumed.
#define POLLNVAL	0x0020	//lux	Invalid request: fd not open (only returned in revents; ignored in events)

/* The rest seem to be more-or-less nonstandard. Check them! */
#define POLLRDNORM	0x0040	//lux	Equivalent to POLLIN
#define POLLRDBAND	0x0080	//lux	Priority band data can be read (generally unused on Linux).
#ifndef POLLWRNORM
#define POLLWRNORM	0x0100	//lux	Equivalent to POLLOUT.
#endif
#ifndef POLLWRBAND
#define POLLWRBAND	0x0200	//lux	Priority data may be written.
#endif
#ifndef POLLMSG
#define POLLMSG		0x0400
#endif
#ifndef POLLREMOVE
#define POLLREMOVE	0x1000
#endif
#ifndef POLLRDHUP
#define POLLRDHUP       0x2000
#endif

struct pollfd {
	int fd;
	short events;
	short revents;
};

#endif	/* __ASM_GENERIC_POLL_H */
