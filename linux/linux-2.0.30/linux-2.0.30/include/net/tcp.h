/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the TCP module.
 *
 * Version:	@(#)tcp.h	1.0.5	05/23/93
 *
 * Authors:	Ross Biro, <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef _TCP_H
#define _TCP_H

#include <linux/tcp.h>
#include <net/checksum.h>

/* This is for all connections with a full identity, no wildcards. */
#define TCP_HTABLE_SIZE		256

/* This is for listening sockets, thus all sockets which possess wildcards. */
#define TCP_LHTABLE_SIZE	32	/* Yes, really, this is all you need. */

/* This is for all sockets, to keep track of the local port allocations. */
#define TCP_BHTABLE_SIZE	64

/* tcp_ipv4.c: These need to be shared by v4 and v6 because the lookup
 *             and hashing code needs to work with different AF's yet
 *             the port space is shared.
 */
extern struct sock *tcp_established_hash[TCP_HTABLE_SIZE];
extern struct sock *tcp_listening_hash[TCP_LHTABLE_SIZE];
extern struct sock *tcp_bound_hash[TCP_BHTABLE_SIZE];

/* These are AF independant. */
static __inline__ int tcp_bhashfn(__u16 lport)
{
	return (lport ^ (lport >> 7)) & (TCP_BHTABLE_SIZE - 1);
}

static __inline__ int tcp_sk_bhashfn(struct sock *sk)
{
	__u16 lport = sk->num;
	return tcp_bhashfn(lport);
}

/* These can have wildcards, don't try too hard.
 * XXX deal with thousands of IP aliases for listening ports later
 */
static __inline__ int tcp_lhashfn(unsigned short num)
{
	return num & (TCP_LHTABLE_SIZE - 1);
}

static __inline__ int tcp_sk_listen_hashfn(struct sock *sk)
{
	return tcp_lhashfn(sk->num);
}

/* This is IPv4 specific. */
static __inline__ int tcp_hashfn(__u32 laddr, __u16 lport,
				 __u32 faddr, __u16 fport)
{
	return ((laddr ^ lport) ^ (faddr ^ fport)) & (TCP_HTABLE_SIZE - 1);
}

static __inline__ int tcp_sk_hashfn(struct sock *sk)
{
	__u32 laddr = sk->rcv_saddr;
	__u16 lport = sk->num;
	__u32 faddr = sk->daddr;
	__u16 fport = sk->dummy_th.dest;

	return tcp_hashfn(laddr, lport, faddr, fport);
}

/* Only those holding the sockhash lock call these two things here.
 * Note the slightly gross overloading of sk->prev, AF_UNIX is the
 * only other main benefactor of that member of SK, so who cares.
 */
static __inline__ void tcp_sk_bindify(struct sock *sk)
{
	int hashent = tcp_sk_bhashfn(sk);
	struct sock **htable = &tcp_bound_hash[hashent];

	if((sk->bind_next = *htable) != NULL)
		(*htable)->bind_pprev = &sk->bind_next;
	*htable = sk;
	sk->bind_pprev = htable;
}

static __inline__ void tcp_sk_unbindify(struct sock *sk)
{
	if(sk->bind_next)
		sk->bind_next->bind_pprev = sk->bind_pprev;
	*(sk->bind_pprev) = sk->bind_next;
}

/*
 * 40 is maximal IP options size
 * 4  is TCP option size (MSS)
 */
#define MAX_SYN_SIZE	(sizeof(struct iphdr) + 40 + sizeof(struct tcphdr) + 4 + MAX_HEADER + 15)
#define MAX_FIN_SIZE	(sizeof(struct iphdr) + 40 + sizeof(struct tcphdr) + MAX_HEADER + 15)
#define MAX_ACK_SIZE	(sizeof(struct iphdr) + 40 + sizeof(struct tcphdr) + MAX_HEADER + 15)
#define MAX_RESET_SIZE	(sizeof(struct iphdr) + 40 + sizeof(struct tcphdr) + MAX_HEADER + 15)

#define MAX_WINDOW	32767		/* Never offer a window over 32767 without using
					   window scaling (not yet supported). Some poor
					   stacks do signed 16bit maths! */
#define MIN_WINDOW	2048
#define MAX_ACK_BACKLOG	2
#define MAX_DUP_ACKS	2
#define MIN_WRITE_SPACE	2048
#define TCP_WINDOW_DIFF	2048

/* urg_data states */
#define URG_VALID	0x0100
#define URG_NOTYET	0x0200
#define URG_READ	0x0400

#define TCP_RETR1	7	/*
				 * This is how many retries it does before it
				 * tries to figure out if the gateway is
				 * down.
				 */

#define TCP_RETR2	15	/*
				 * This should take at least
				 * 90 minutes to time out.
				 */

#define TCP_TIMEOUT_LEN	(15*60*HZ) /* should be about 15 mins		*/
#define TCP_TIMEWAIT_LEN (60*HZ) /* how long to wait to successfully 
				  * close the socket, about 60 seconds	*/
#define TCP_FIN_TIMEOUT (3*60*HZ) /* BSD style FIN_WAIT2 deadlock breaker */				  
#define TCP_ACK_TIME	(3*HZ)	/* time to delay before sending an ACK	*/
#define TCP_DONE_TIME	(5*HZ/2)/* maximum time to wait before actually
				 * destroying a socket			*/
#define TCP_WRITE_TIME	(30*HZ)	/* initial time to wait for an ACK,
			         * after last transmit			*/
#define TCP_TIMEOUT_INIT (3*HZ)	/* RFC 1122 initial timeout value	*/
#define TCP_SYN_RETRIES	 5	/* number of times to retry opening a
				 * connection 	(TCP_RETR2-....)	*/
#define TCP_PROBEWAIT_LEN (1*HZ)/* time to wait between probes when
				 * I've got something to write and
				 * there is no window			*/

#define TCP_NO_CHECK	0	/* turn to one if you want the default
				 * to be no checksum			*/


/*
 *	TCP option
 */
 
#define TCPOPT_NOP		1	/* Padding */
#define TCPOPT_EOL		0	/* End of options */
#define TCPOPT_MSS		2	/* Segment size negotiating */
/*
 *	We don't use these yet, but they are for PAWS and big windows
 */
#define TCPOPT_WINDOW		3	/* Window scaling */
#define TCPOPT_TIMESTAMP	8	/* Better RTT estimations/PAWS */


/*
 * The next routines deal with comparing 32 bit unsigned ints
 * and worry about wraparound (automatic with unsigned arithmetic).
 */

extern __inline int before(__u32 seq1, __u32 seq2)
{
        return (__s32)(seq1-seq2) < 0;
}

extern __inline int after(__u32 seq1, __u32 seq2)
{
	return (__s32)(seq2-seq1) < 0;
}


/* is s2<=s1<=s3 ? */
extern __inline int between(__u32 seq1, __u32 seq2, __u32 seq3)
{
	return (after(seq1+1, seq2) && before(seq1, seq3+1));
}

static __inline__ int min(unsigned int a, unsigned int b)
{
	if (a > b)
		a = b;
	return a;
}

static __inline__ int max(unsigned int a, unsigned int b)
{
	if (a < b)
		a = b;
	return a;
}

extern struct proto tcp_prot;
extern struct tcp_mib tcp_statistics;

extern unsigned short		tcp_good_socknum(void);

extern void	tcp_err(int type, int code, unsigned char *header, __u32 daddr,
			__u32, struct inet_protocol *protocol, int len);
extern void	tcp_shutdown (struct sock *sk, int how);
extern int	tcp_rcv(struct sk_buff *skb, struct device *dev,
			struct options *opt, __u32 daddr,
			unsigned short len, __u32 saddr, int redo,
			struct inet_protocol *protocol);

extern int tcp_ioctl(struct sock *sk, int cmd, unsigned long arg);

extern void tcp_read_wakeup(struct sock *);
extern void tcp_write_xmit(struct sock *);
extern void tcp_time_wait(struct sock *);
extern void tcp_retransmit(struct sock *, int);
extern void tcp_do_retransmit(struct sock *, int);
extern void tcp_send_check(struct tcphdr *th, unsigned long saddr, 
		unsigned long daddr, int len, struct sk_buff *skb);

/* tcp_output.c */

extern void tcp_send_probe0(struct sock *);
extern void tcp_send_partial(struct sock *);
extern void tcp_write_wakeup(struct sock *);
extern void tcp_send_fin(struct sock *sk);
extern void tcp_send_synack(struct sock *, struct sock *, struct sk_buff *, int);
extern void tcp_send_skb(struct sock *, struct sk_buff *);
extern void tcp_send_ack(struct sock *sk);
extern void tcp_send_delayed_ack(struct sock *sk, int max_timeout, unsigned long timeout);
extern void tcp_send_reset(unsigned long saddr, unsigned long daddr, struct tcphdr *th,
	  struct proto *prot, struct options *opt, struct device *dev, int tos, int ttl);

extern void tcp_enqueue_partial(struct sk_buff *, struct sock *);
extern struct sk_buff * tcp_dequeue_partial(struct sock *);
extern void tcp_shrink_skb(struct sock *,struct sk_buff *,u32);

/* CONFIG_IP_TRANSPARENT_PROXY */
extern int tcp_chkaddr(struct sk_buff *);

/* tcp_timer.c */
#define     tcp_reset_msl_timer(x,y,z)	reset_timer(x,y,z)
extern void tcp_reset_xmit_timer(struct sock *, int, unsigned long);
extern void tcp_delack_timer(unsigned long);
extern void tcp_retransmit_timer(unsigned long);

static __inline__ int tcp_old_window(struct sock * sk)
{
	return sk->window - (sk->acked_seq - sk->lastwin_seq);
}

extern int tcp_new_window(struct sock *);

/*
 * Return true if we should raise the window when we
 * have cleaned up the receive queue. We don't want to
 * do this normally, only if it makes sense to avoid
 * zero window probes..
 *
 * We do this only if we can raise the window noticeably.
 */
static __inline__ int tcp_raise_window(struct sock * sk)
{
	int new = tcp_new_window(sk);
	return new && (new >= 2*tcp_old_window(sk));
}

static __inline__ unsigned short tcp_select_window(struct sock *sk)
{
	int window = tcp_new_window(sk);
	int oldwin = tcp_old_window(sk);

	/* Don't allow a shrinking window */
	if (window > oldwin) {
		sk->window = window;
		sk->lastwin_seq = sk->acked_seq;
		oldwin = window;
	}
	return oldwin;
}

/*
 * List all states of a TCP socket that can be viewed as a "connected"
 * state.  This now includes TCP_SYN_RECV, although I am not yet fully
 * convinced that this is the solution for the 'getpeername(2)'
 * problem. Thanks to Stephen A. Wood <saw@cebaf.gov>  -FvK
 */

extern __inline const int tcp_connected(const int state)
{
  return(state == TCP_ESTABLISHED || state == TCP_CLOSE_WAIT ||
	 state == TCP_FIN_WAIT1   || state == TCP_FIN_WAIT2 ||
	 state == TCP_SYN_RECV);
}

/*
 * Calculate(/check) TCP checksum
 */
static __inline__ u16 tcp_check(struct tcphdr *th, int len,
	unsigned long saddr, unsigned long daddr, unsigned long base)
{
	return csum_tcpudp_magic(saddr,daddr,len,IPPROTO_TCP,base);
}

#undef STATE_TRACE

#ifdef STATE_TRACE
static char *statename[]={
	"Unused","Established","Syn Sent","Syn Recv",
	"Fin Wait 1","Fin Wait 2","Time Wait", "Close",
	"Close Wait","Last ACK","Listen","Closing"
};
#endif

static __inline__ void tcp_set_state(struct sock *sk, int state)
{
	int oldstate = sk->state;

	sk->state = state;

#ifdef STATE_TRACE
	if(sk->debug)
		printk("TCP sk=%p, State %s -> %s\n",sk, statename[oldstate],statename[state]);
#endif	

	switch (state) {
	case TCP_ESTABLISHED:
		if (oldstate != TCP_ESTABLISHED) {
			tcp_statistics.TcpCurrEstab++;
		}
		break;

	case TCP_CLOSE:
		/* Should be about 2 rtt's */
		reset_timer(sk, TIME_DONE, min(sk->rtt * 2, TCP_DONE_TIME));
		/* fall through */
	default:
		if (oldstate==TCP_ESTABLISHED)
			tcp_statistics.TcpCurrEstab--;
	}
}

#endif	/* _TCP_H */
