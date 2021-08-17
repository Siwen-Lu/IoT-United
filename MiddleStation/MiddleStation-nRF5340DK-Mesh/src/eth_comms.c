#include "eth_comms.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(ethernet_sockets, LOG_LEVEL_DBG);
static struct net_mgmt_event_callback mgmt_cb;

static void handler(struct net_mgmt_event_callback *cb,
	uint32_t mgmt_event,
	struct net_if *iface)
{
	int i = 0;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		char buf[NET_IPV4_ADDR_LEN];

		if (iface->config.ip.ipv4->unicast[i].addr_type !=
							NET_ADDR_DHCP) {
			continue;
		}

		LOG_INF("Your address: %s",
			log_strdup(net_addr_ntop(AF_INET,
				&iface->config.ip.ipv4->unicast[i].address.in_addr,
				buf,
				sizeof(buf))));
		LOG_INF("Lease time: %u seconds",
			iface->config.dhcpv4.lease_time);
		LOG_INF("Subnet: %s",
			log_strdup(net_addr_ntop(AF_INET,
				&iface->config.ip.ipv4->netmask,
				buf,
				sizeof(buf))));
		LOG_INF("Router: %s",
			log_strdup(net_addr_ntop(AF_INET,
				&iface->config.ip.ipv4->gw,
				buf,
				sizeof(buf))));
	}
}

void eth_init()
{
	struct net_if *iface;

	LOG_INF("Run dhcpv4 client");

	net_mgmt_init_event_callback(&mgmt_cb,
		handler,
		NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	iface = net_if_get_default();

	net_dhcpv4_start(iface);
}

static void udp_server(void*, void*, void*);
#define UDP_THREAD_STACK_SIZE 1024
#define UDP_THREAD_PRIORITY 5
K_THREAD_STACK_DEFINE(udp_server_stack_area, UDP_THREAD_STACK_SIZE);
struct k_thread UDP_SERVER_THREAD;
void server_init()
{
	//todo: some preparation
	k_thread_create(&UDP_SERVER_THREAD,
		udp_server_stack_area,
		K_THREAD_STACK_SIZEOF(udp_server_stack_area),
		udp_server, NULL, NULL, NULL, UDP_THREAD_PRIORITY, 0,K_NO_WAIT);
}
void udp_server(void* indata1, void* indata2, void* indata3)
{
	int serv;
	struct sockaddr_in bind_addr, client_addr;
	
	serv = socket(AF_INET, SOCK_DGRAM, 0);
	CHECK(serv);
	
	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(UDP_PORT);
	CHECK(bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr)));

	LOG_INF("Single-threaded UDP server waits for a connection on "
	       "port %d...",UDP_PORT);
	
	uint8_t buf_size = 20;
	uint8_t buff[buf_size];
	socklen_t cli_addr_len = sizeof(client_addr);
	while (1)
	{
		memset(&buff, 0, buf_size);
		recvfrom(serv, buff, buf_size, 0, (struct sockaddr *)&client_addr, &cli_addr_len);
		LOG_INF("Received: %s", log_strdup(buff));
		//todo: command queue
	}
}