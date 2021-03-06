/* -*- linux-c -*-
 * sysctl_net_ipv4.c: sysctl interface to net IPV4 subsystem.
 *
 * Begun April 1, 1996, Mike Shaver.
 * Added /proc/sys/net/ipv4 directory entry (empty =) ). [MS]
 */

#include <linux/mm.h>
#include <linux/sysctl.h>
#include <net/ip.h>

/* From arp.c */
extern int sysctl_arp_res_time;
extern int sysctl_arp_dead_res_time;
extern int sysctl_arp_max_tries;
extern int sysctl_arp_timeout;
extern int sysctl_arp_check_interval;
extern int sysctl_arp_confirm_interval;
extern int sysctl_arp_confirm_timeout;

extern int sysctl_ip_forward; 
static int proc_doipforward(ctl_table *ctl, int write, struct file *filp,
			    void *buffer, size_t *lenp) 
{
    int val = sysctl_ip_forward;    
    int retv;

    retv = proc_dointvec(ctl, write, filp, buffer, lenp);
    if (write) {
	if (sysctl_ip_forward && !val) {
	    printk(KERN_INFO "sysctl: ip forwarding enabled\n");
	    ip_statistics.IpForwarding = 1; 
	}
	if (!sysctl_ip_forward && val) {
	    printk(KERN_INFO "sysctl: ip forwarding off\n");
	    ip_statistics.IpForwarding = 2;
	}
    }
    return retv; 
}

ctl_table ipv4_table[] = {
        {NET_IPV4_ARP_RES_TIME, "arp_res_time",
         &sysctl_arp_res_time, sizeof(int), 0644, NULL, &proc_dointvec},
        {NET_IPV4_ARP_DEAD_RES_TIME, "arp_dead_res_time",
         &sysctl_arp_dead_res_time, sizeof(int), 0644, NULL, &proc_dointvec},
        {NET_IPV4_ARP_MAX_TRIES, "arp_max_tries",
         &sysctl_arp_max_tries, sizeof(int), 0644, NULL, &proc_dointvec},
        {NET_IPV4_ARP_TIMEOUT, "arp_timeout",
         &sysctl_arp_timeout, sizeof(int), 0644, NULL, &proc_dointvec},
        {NET_IPV4_ARP_CHECK_INTERVAL, "arp_check_interval",
         &sysctl_arp_check_interval, sizeof(int), 0644, NULL, &proc_dointvec},
        {NET_IPV4_ARP_CONFIRM_INTERVAL, "arp_confirm_interval",
         &sysctl_arp_confirm_interval, sizeof(int), 0644, NULL,
         &proc_dointvec},
        {NET_IPV4_ARP_CONFIRM_TIMEOUT, "arp_confirm_timeout",
         &sysctl_arp_confirm_timeout, sizeof(int), 0644, NULL,
         &proc_dointvec},
	{NET_IPV4_FORWARD, "ip_forward", &sysctl_ip_forward, sizeof(int),
	 0644, NULL, &proc_doipforward },
	{0}
};
