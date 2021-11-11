#include "eth_comms.h"
#include <zephyr.h>
#include <net/socket.h>
#include <net/mqtt.h>
#include <random/rand32.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(ethernet_sockets, LOG_LEVEL_DBG);

K_MUTEX_DEFINE(dhcp_mutex);

#if defined(CONFIG_USERSPACE)
#include <app_memory/app_memdomain.h>
K_APPMEM_PARTITION_DEFINE(app_partition);
struct k_mem_domain app_domain;
#define APP_BMEM K_APP_BMEM(app_partition)
#define APP_DMEM K_APP_DMEM(app_partition)
#else
#define APP_BMEM
#define APP_DMEM
#endif

/* Buffers for MQTT client. */
static APP_BMEM uint8_t rx_buffer[APP_MQTT_BUFFER_SIZE];
static APP_BMEM uint8_t tx_buffer[APP_MQTT_BUFFER_SIZE];

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
/* Making RX buffer large enough that the full IPv6 packet can fit into it */
#define MQTT_LIB_WEBSOCKET_RECV_BUF_LEN 1280

/* Websocket needs temporary buffer to store partial packets */
static APP_BMEM uint8_t temp_ws_rx_buf[MQTT_LIB_WEBSOCKET_RECV_BUF_LEN];
#endif

/* The mqtt client struct */
static APP_BMEM struct mqtt_client client_ctx;

/* MQTT Broker details. */
static APP_BMEM struct sockaddr_storage broker;

#if defined(CONFIG_SOCKS)
static APP_BMEM struct sockaddr socks5_proxy;
#endif

static APP_BMEM struct zsock_pollfd fds[1];
static APP_BMEM int nfds;

static APP_BMEM bool connected;

#if defined(CONFIG_MQTT_LIB_TLS)

#include "test_certs.h"

#define TLS_SNI_HOSTNAME "192.169.137.1"
#define APP_CA_CERT_TAG 1
#define APP_PSK_TAG 2

static APP_DMEM sec_tag_t m_sec_tags[] = {
#if defined(MBEDTLS_X509_CRT_PARSE_C) || defined(CONFIG_NET_SOCKETS_OFFLOAD)
		APP_CA_CERT_TAG,
#endif
#if defined(MBEDTLS_KEY_EXCHANGE_SOME_PSK_ENABLED)
		APP_PSK_TAG,
#endif
};

static int tls_init(void)
{
	int err = -EINVAL;

	err = tls_credential_add(APP_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate, sizeof(ca_certificate));

	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_PSK_ENABLED)
	err = tls_credential_add(APP_PSK_TAG, TLS_CREDENTIAL_PSK,
				 client_psk, sizeof(client_psk));
	if (err < 0) {
		LOG_ERR("Failed to register PSK: %d", err);
		return err;
	}

	err = tls_credential_add(APP_PSK_TAG, TLS_CREDENTIAL_PSK_ID,
				 client_psk_id, sizeof(client_psk_id) - 1);
	if (err < 0) {
		LOG_ERR("Failed to register PSK ID: %d", err);
	}
#endif

	return err;
}

#endif /* CONFIG_MQTT_LIB_TLS */

static void prepare_fds(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	}
#if defined(CONFIG_MQTT_LIB_TLS)
	else if (client->transport.type == MQTT_TRANSPORT_SECURE) {
		fds[0].fd = client->transport.tls.sock;
	}
#endif

	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	nfds = 0;
}

static int wait(int timeout)
{
	int ret = 0;

	if (nfds > 0) {
		ret = zsock_poll(fds, nfds, timeout);
		if (ret < 0) {
			LOG_ERR("poll error: %d", errno);
		}
	}

	return ret;
}

void mqtt_evt_handler(struct mqtt_client *const client,
		      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed %d", evt->result);
			break;
		}

		connected = true;
		
		LOG_INF("MQTT client connected!");
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT client disconnected %d", evt->result);

		connected = false;
		clear_fds();

		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error %d", evt->result);
			break;
		}

		// LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);

		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBREC error %d", evt->result);
			break;
		}

		// LOG_INF("PUBREC packet id: %u", evt->param.pubrec.message_id);

		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};

		err = mqtt_publish_qos2_release(client, &rel_param);
		if (err != 0) {
			LOG_ERR("Failed to send MQTT PUBREL: %d", err);
		}
		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBCOMP error %d", evt->result);
			break;
		}

		// LOG_INF("PUBCOMP packet id: %u",
		// 	evt->param.pubcomp.message_id);

		break;

	case MQTT_EVT_PINGRESP:
		// LOG_INF("PINGRESP packet");
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT SUBACK error: %d", evt->result);
			break;
		}

		// LOG_INF("SUBACK packet id: %u", evt->param.suback.message_id);
		break;
	default:
		break;
	}
}

//change this to set the topic
static char *get_mqtt_topic(void)
{
	return CONFIG_MQTT_PUB_TOPIC;
}

static int publish(struct mqtt_client *client, enum mqtt_qos qos,char * payload)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = (uint8_t *)get_mqtt_topic();
	param.message.topic.topic.size =
			strlen(param.message.topic.topic.utf8);
	param.message.payload.data = payload;
	param.message.payload.len =
			strlen(param.message.payload.data);
	param.message_id = sys_rand32_get();
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	return mqtt_publish(client, &param);
}

/**@brief Function to subscribe to the configured topic
 */
static int subscribe(struct mqtt_client *client)
{
	int rc;
	struct mqtt_topic topic;
	struct mqtt_subscription_list sub;

	topic.topic.utf8 = (uint8_t *)CONFIG_MQTT_SUB_TOPIC;
	topic.topic.size = strlen(topic.topic.utf8);
	topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	sub.list = &topic;
	sub.list_count = 1U;
	sub.message_id = sys_rand32_get();

	rc = mqtt_subscribe(&client_ctx, &sub);
	if (rc != 0) {
		return rc;
	}

	// wait(APP_SLEEP_MSECS);
	// mqtt_input(&client_ctx);

	return rc;
}

#define RC_STR(rc) ((rc) == 0 ? "OK" : "ERROR")

#define PRINT_RESULT(func, rc) \
	LOG_INF("%s: %d <%s>", (func), rc, RC_STR(rc))

static void broker_init(void)
{
#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 *broker6 = (struct sockaddr_in6 *)&broker;

	broker6->sin6_family = AF_INET6;
	broker6->sin6_port = htons(SERVER_PORT);
	zsock_inet_pton(AF_INET6, SERVER_ADDR, &broker6->sin6_addr);

#if defined(CONFIG_SOCKS)
	struct sockaddr_in6 *proxy6 = (struct sockaddr_in6 *)&socks5_proxy;

	proxy6->sin6_family = AF_INET6;
	proxy6->sin6_port = htons(SOCKS5_PROXY_PORT);
	zsock_inet_pton(AF_INET6, SOCKS5_PROXY_ADDR, &proxy6->sin6_addr);
#endif
#else
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(SERVER_PORT);
	zsock_inet_pton(AF_INET, SERVER_ADDR, &broker4->sin_addr);
#if defined(CONFIG_SOCKS)
	struct sockaddr_in *proxy4 = (struct sockaddr_in *)&socks5_proxy;

	proxy4->sin_family = AF_INET;
	proxy4->sin_port = htons(SOCKS5_PROXY_PORT);
	zsock_inet_pton(AF_INET, SOCKS5_PROXY_ADDR, &proxy4->sin_addr);
#endif
#endif
}

static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = (uint8_t *)MQTT_CLIENTID;
	client->client_id.size = strlen(MQTT_CLIENTID);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
#if defined(CONFIG_MQTT_LIB_TLS)
#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	client->transport.type = MQTT_TRANSPORT_SECURE_WEBSOCKET;
#else
	client->transport.type = MQTT_TRANSPORT_SECURE;
#endif

	struct mqtt_sec_config *tls_config = &client->transport.tls.config;

	tls_config->peer_verify = TLS_PEER_VERIFY_NONE;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = m_sec_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
#if defined(MBEDTLS_X509_CRT_PARSE_C) || defined(CONFIG_NET_SOCKETS_OFFLOAD)
	tls_config->hostname = TLS_SNI_HOSTNAME;
#else
	tls_config->hostname = NULL;
#endif

#else
#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	client->transport.type = MQTT_TRANSPORT_NON_SECURE_WEBSOCKET;
#else
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif
#endif

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	client->transport.websocket.config.host = SERVER_ADDR;
	client->transport.websocket.config.url = "/mqtt";
	client->transport.websocket.config.tmp_buf = temp_ws_rx_buf;
	client->transport.websocket.config.tmp_buf_len =
						sizeof(temp_ws_rx_buf);
	client->transport.websocket.timeout = 5 * MSEC_PER_SEC;
#endif

#if defined(CONFIG_SOCKS)
	mqtt_client_set_proxy(client, &socks5_proxy,
			      socks5_proxy.sa_family == AF_INET ?
			      sizeof(struct sockaddr_in) :
			      sizeof(struct sockaddr_in6));
#endif
}

/* In this routine we block until the connected variable is 1 */
static int try_to_connect(struct mqtt_client *client)
{
	int rc, i = 0;

	while (i++ < APP_CONNECT_TRIES && !connected) {


		client_init(client);

		rc = mqtt_connect(client);

		if (rc != 0) {
			PRINT_RESULT("mqtt_connect", rc);
			k_sleep(K_MSEC(APP_SLEEP_MSECS));
			continue;
		}

		prepare_fds(client);

		if (wait(APP_CONNECT_TIMEOUT_MS)) {
			mqtt_input(client);
		}

		if (!connected) {
			mqtt_abort(client);
		}
	}

	if (connected) {
		return 0;
	}

	return -EINVAL;
}

static int process_mqtt_and_sleep(struct mqtt_client *client, int timeout)
{
	int64_t remaining = timeout;
	int64_t start_time = k_uptime_get();
	int rc;

	while (remaining > 0 && connected) {
		if (wait(remaining)) {
			rc = mqtt_input(client);
			if (rc != 0) {
				PRINT_RESULT("mqtt_input", rc);
				return rc;
			}

			//decoding subscribing packet
			char subscribe_packet[APP_MQTT_BUFFER_SIZE];
			if (mqtt_read_publish_payload(client,subscribe_packet,sizeof(subscribe_packet)) > 0) {
				char * inst = strtok(subscribe_packet," "); 
				printk("inst = %s\n",inst);
			}
		}

		rc = mqtt_live(client);
		if (rc != 0 && rc != -EAGAIN) {
			PRINT_RESULT("mqtt_live", rc);
			return rc;
		} else if (rc == 0) {
			rc = mqtt_input(client);
			if (rc != 0) {
				PRINT_RESULT("mqtt_input", rc);
				return rc;
			}
			//decoding subscribing packet
			char subscribe_packet[APP_MQTT_BUFFER_SIZE];
			if (mqtt_read_publish_payload(client,subscribe_packet,sizeof(subscribe_packet)) > 0) {
				char * inst = strtok(subscribe_packet," "); 
				printk("inst = %s\n",inst);
			}
		}

		remaining = timeout + start_time - k_uptime_get();
	}
	return 0;
}

#define SUCCESS_OR_EXIT(rc) { if (rc != 0) { return 1; } }
#define SUCCESS_OR_BREAK(rc) { if (rc != 0) { break; } }
static int subscriber(void)
{
	int rc, r = 0;

do_connect:
	LOG_INF("attempting to connect: ");
	rc = try_to_connect(&client_ctx);
	PRINT_RESULT("try_to_connect", rc);

	rc = mqtt_ping(&client_ctx);
	PRINT_RESULT("mqtt_ping", rc);
	subscribe(&client_ctx);	
	
	while (1) {
		process_mqtt_and_sleep(&client_ctx,APP_SLEEP_MSECS);
	}

	rc = mqtt_disconnect(&client_ctx);
	PRINT_RESULT("mqtt_disconnect", rc);
	r = 0;
	goto do_connect;
	return r;
}

int publisher(char * payload) {
	int rc = 0;

	rc = try_to_connect(&client_ctx);
	printk("successful connext connect\n");

	rc = publish(&client_ctx, MQTT_QOS_2_EXACTLY_ONCE,payload);
	PRINT_RESULT("mqtt_publish", rc);

	rc = process_mqtt_and_sleep(&client_ctx, APP_SLEEP_MSECS);

	rc = mqtt_disconnect(&client_ctx);
	PRINT_RESULT("mqtt_disconnect", rc);

	return rc;
}

void start_app(void* indata1, void* indata2, void* indata3)
{

	subscriber();

}

#if defined(CONFIG_USERSPACE)
#define STACK_SIZE 2048

#if IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

K_THREAD_DEFINE(app_thread, STACK_SIZE,
		start_app, NULL, NULL, NULL,
		THREAD_PRIORITY, K_USER, -1);

static K_HEAP_DEFINE(app_mem_pool, 1024 * 2);
#endif

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

#define MQTT_THREAD_STACK_SIZE 2048
#define MQTT_THREAD_PRIORITY 5
K_THREAD_STACK_DEFINE(mqtt_server_stack_area, MQTT_THREAD_STACK_SIZE);
struct k_thread MQTT_SERVER_THREAD;

void server_init()
{

#if defined(CONFIG_MQTT_LIB_TLS)
	int rc;
	rc = tls_init();

#endif

}
