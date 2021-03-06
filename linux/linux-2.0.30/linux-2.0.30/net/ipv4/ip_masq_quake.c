/*
 *		IP_MASQ_QUAKE quake masquerading module
 *
 *
 * Version:	@(#)ip_masq_quake.c 1.00   22/02/97
 *
 * Author:	Harald Hoyer mailto:HarryH@Royal.Net
 *		
 *
 * Fixes: 
 *      Harald Hoyer            :       Unofficial Quake Specs found at 
 *                                 http://www.gamers.org/dEngine/quake/spec/ 
 *      Harald Hoyer            :       Check for QUAKE-STRING
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *  
 *  
 */

#include <linux/config.h>
#include <linux/module.h>

#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <net/protocol.h>
#include <net/udp.h>
#include <net/ip_masq.h>

#ifndef DEBUG_CONFIG_IP_MASQ_QUAKE
#define DEBUG_CONFIG_IP_MASQ_QUAKE 0
#endif

/* 
 * List of ports (up to MAX_MASQ_APP_PORTS) to be handled by helper
 * First ports are set to the default port.
 */
int ports[MAX_MASQ_APP_PORTS] = { 26000,   /* I rely on the trailing items */
				  27000 }; /* being set to zero */
struct ip_masq_app *masq_incarnations[MAX_MASQ_APP_PORTS];


typedef struct
{ 
        __u16 type;     // (Little Endian) Type of message.
	__u16 length;   // (Little Endian) Length of message, header included. 
	char  message[0];  // The contents of the message.
} QUAKEHEADER;

struct quake_priv_data {
	/* Have we seen a client connect message */
	char	cl_connect;
};

static int
masq_quake_init_1 (struct ip_masq_app *mapp, struct ip_masq *ms)
{
        MOD_INC_USE_COUNT;
	if ((ms->app_data = kmalloc(sizeof(struct quake_priv_data),
				    GFP_ATOMIC)) == NULL) 
		printk(KERN_INFO "Quake: No memory for application data\n");
	else 
	{
		struct quake_priv_data *priv = 
			(struct quake_priv_data *)ms->app_data;
		priv->cl_connect = 0;
	}
        return 0;
}

static int
masq_quake_done_1 (struct ip_masq_app *mapp, struct ip_masq *ms)
{
	MOD_DEC_USE_COUNT;
	if (ms->app_data)
		kfree_s(ms->app_data, sizeof(struct quake_priv_data));
	return 0;
}

int
masq_quake_in (struct ip_masq_app *mapp, struct ip_masq *ms, struct sk_buff **skb_p, struct device *dev)
{
	struct sk_buff *skb;
	struct iphdr *iph;
	struct udphdr *uh;
	QUAKEHEADER *qh;
	__u16 udp_port;
	char *data;
	unsigned char code;
	struct quake_priv_data *priv = (struct quake_priv_data *)ms->app_data;
        
	if(priv->cl_connect == -1)
	  return 0;

	skb = *skb_p;
	iph = skb->h.iph;
/*	iph = skb->nh.iph; */

	uh = (struct udphdr *)&(((char *)iph)[iph->ihl*4]);

	/* Check for lenght */
	if(ntohs(uh->len) < 5)
	  return 0;
	
	qh = (QUAKEHEADER *)&uh[1];

	if(qh->type != 0x0080)
	  return 0;

	
	code = qh->message[0];

#if DEBUG_CONFIG_IP_MASQ_QUAKE
	  printk("Quake_in: code = %d \n", (int)code);
#endif

	switch(code) {
	case 0x01:
	  /* Connection Request */

	  if(ntohs(qh->length) < 0x0c) {
#if DEBUG_CONFIG_IP_MASQ_QUAKE
	    printk("Quake_in: length < 0xc \n");
#endif
	    return 0;
	  }

	  data = &qh->message[1];

	  /* Check for stomping string */
	  if(memcmp(data,"QUAKE\0\3",7)) {
#if DEBUG_CONFIG_IP_MASQ_QUAKE
	    printk("Quake_out: memcmp failed \n");
#endif
	    return 0;
	  }
	  else {
	    priv->cl_connect = 1;
#if DEBUG_CONFIG_IP_MASQ_QUAKE
	    printk("Quake_out: memcmp ok \n");
#endif
	  }
	  break;

	case 0x81:
	  /* Accept Connection */
	  if((ntohs(qh->length) < 0x09) || (priv->cl_connect == 0))
	    return 0;
	  data = &qh->message[1];

	  memcpy(&udp_port, data, 2);

	  ms->dport = htons(udp_port);

#if DEBUG_CONFIG_IP_MASQ_QUAKE
	  printk("Quake_in: in_rewrote UDP port %d \n", udp_port);
#endif
	  priv->cl_connect = -1;

	  break;
	}
	 
	return 0;
}

int
masq_quake_out (struct ip_masq_app *mapp, struct ip_masq *ms, struct sk_buff **skb_p, struct device *dev)
{
	struct sk_buff *skb;
	struct iphdr *iph;
	struct udphdr *uh;
	QUAKEHEADER *qh;
	__u16 udp_port;
	char *data;
	unsigned char code;
	struct ip_masq *n_ms;
	struct quake_priv_data *priv = (struct quake_priv_data *)ms->app_data;

	if(priv->cl_connect == -1)
	  return 0;
        
	skb = *skb_p;
	iph = skb->h.iph;
/*	iph = skb->nh.iph; */

	uh = (struct udphdr *)&(((char *)iph)[iph->ihl*4]);

	/* Check for lenght */
	if(ntohs(uh->len) < 5)
	  return 0;
	
	qh = (QUAKEHEADER *)&uh[1];

#if DEBUG_CONFIG_IP_MASQ_QUAKE
	  printk("Quake_out: qh->type = %d \n", (int)qh->type);
#endif

	if(qh->type != 0x0080)
	  return 0;
	
	code = qh->message[0];

#if DEBUG_CONFIG_IP_MASQ_QUAKE
	  printk("Quake_out: code = %d \n", (int)code);
#endif

	switch(code) {
	case 0x01:
	  /* Connection Request */

	  if(ntohs(qh->length) < 0x0c) {
#if DEBUG_CONFIG_IP_MASQ_QUAKE
	    printk("Quake_out: length < 0xc \n");
#endif
	    return 0;
	  }

	  data = &qh->message[1];

	  /* Check for stomping string */
	  if(memcmp(data,"QUAKE\0\3",7)) {
#if DEBUG_CONFIG_IP_MASQ_QUAKE
	    printk("Quake_out: memcmp failed \n");
#endif
	    return 0;
	  }
	  else {
	    priv->cl_connect = 1;
#if DEBUG_CONFIG_IP_MASQ_QUAKE
	    printk("Quake_out: memcmp ok \n");
#endif
	  }
	  break;

	case 0x81:
	  /* Maybe a redirection of a quake-server at the inner side works in
             the future? */

	  /* Accept Connection */
	  if((ntohs(qh->length) < 0x09) || (priv->cl_connect == 0))
	    return 0;

	  data = &qh->message[1];

	  memcpy(&udp_port, data, 2);
	  
	  n_ms = ip_masq_new(dev, IPPROTO_UDP,
			     ms->saddr, htons(udp_port),
			     ms->daddr, ms->dport,
			     0);

	  if (n_ms==NULL)
	    return 0;

#if DEBUG_CONFIG_IP_MASQ_QUAKE
	  printk("Quake_out: out_rewrote UDP port %d -> %d\n",
		 udp_port, ntohs(n_ms->mport));
#endif
	  udp_port = ntohs(n_ms->mport);
	  memcpy(data, &udp_port, 2);

	  break;
	}
	 
	return 0;
}

struct ip_masq_app ip_masq_quake = {
        NULL,			/* next */
	"Quake",	       	/* name */
        0,                      /* type */
        0,                      /* n_attach */
        masq_quake_init_1,      /* ip_masq_init_1 */
        masq_quake_done_1,      /* ip_masq_done_1 */
        masq_quake_out,         /* pkt_out */
        masq_quake_in           /* pkt_in */
};

/*
 * 	ip_masq_quake initialization
 */

int ip_masq_quake_init(void)
{
	int i, j;

	for (i=0; (i<MAX_MASQ_APP_PORTS); i++) {
		if (ports[i]) {
			if ((masq_incarnations[i] = kmalloc(sizeof(struct ip_masq_app),
							    GFP_KERNEL)) == NULL)
				return -ENOMEM;
			memcpy(masq_incarnations[i], &ip_masq_quake, sizeof(struct ip_masq_app));
			if ((j = register_ip_masq_app(masq_incarnations[i], 
						      IPPROTO_UDP, 
						      ports[i]))) {
				return j;
			}
#if DEBUG_CONFIG_IP_MASQ_QUAKE
			printk("Quake: loaded support on port[%d] = %d\n",
			       i, ports[i]);
#endif
		} else {
			/* To be safe, force the incarnation table entry to NULL */
			masq_incarnations[i] = NULL;
		}
	}
	return 0;
}

/*
 * 	ip_masq_quake fin.
 */

int ip_masq_quake_done(void)
{
	int i, j, k;

	k=0;
	for (i=0; (i<MAX_MASQ_APP_PORTS); i++) {
		if (masq_incarnations[i]) {
			if ((j = unregister_ip_masq_app(masq_incarnations[i]))) {
				k = j;
			} else {
				kfree(masq_incarnations[i]);
				masq_incarnations[i] = NULL;
#if DEBUG_CONFIG_IP_MASQ_QUAKE
				printk("Quake: unloaded support on port[%d] = %d\n",
				       i, ports[i]);
#endif
			}
		}
	}
	return k;
}

#ifdef MODULE

int init_module(void)
{
        if (ip_masq_quake_init() != 0)
                return -EIO;
        register_symtab(0);
        return 0;
}

void cleanup_module(void)
{
        if (ip_masq_quake_done() != 0)
                printk("ip_masq_quake: can't remove module");
}

#endif /* MODULE */
