#ifndef __ETH_COMMS_H__
#define __ETH_COMMS_H__

#include <zephyr.h>
#include <linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#else
#include <net/socket.h>
#include <kernel.h>
#include <net/net_pkt.h>
#endif

#define CHECK(r) { if (r == -1) { printk("Error: " #r "\n"); exit(1); } }


void eth_init();
void server_init();
extern int publisher(char * payload);
#endif // !__ETH_COMMS_H__