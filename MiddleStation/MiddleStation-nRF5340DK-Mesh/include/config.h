/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef CONFIG_NET_CONFIG_SETTINGS
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		CONFIG_NET_CONFIG_MY_IPV6_ADDR
#define SERVER_ADDR		CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#else
#define ZEPHYR_ADDR		CONFIG_NET_CONFIG_MY_IPV4_ADDR
#define SERVER_ADDR		CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#endif
#else
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		"2001:db8::1"
#define SERVER_ADDR		"2001:db8::2"
#else
#define ZEPHYR_ADDR		"192.168.1.101"
#define SERVER_ADDR		"5.196.95.208"
#endif
#endif

#if defined(CONFIG_SOCKS)
#define SOCKS5_PROXY_ADDR	SERVER_ADDR
#define SOCKS5_PROXY_PORT	1080
#endif

#define SERVER_PORT		8883


#define APP_CONNECT_TIMEOUT_MS	2000
#define APP_SLEEP_MSECS		500

#define APP_CONNECT_TRIES	10

#define APP_MQTT_BUFFER_SIZE	128

#define MQTT_CLIENTID		"zephyr_publisher"

#define CONFIG_NET_SAMPLE_APP_MAX_ITERATIONS 10
#define CONFIG_NET_SAMPLE_APP_MAX_CONNECTIONS 10

//Changeable
#define CONFIG_MQTT_SUB_TOPIC "device/123/alarm"
#define CONFIG_MQTT_PUB_TOPIC "device/123/rssi"

#endif

