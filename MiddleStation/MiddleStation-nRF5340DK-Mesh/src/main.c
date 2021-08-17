/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic Mesh light sample
 */

#include <dk_buttons_and_leds.h>


#include <logging/log.h>
LOG_MODULE_REGISTER(chat, CONFIG_LOG_DEFAULT_LEVEL);

#include "eth_comms.h"

void main(void)
{

	printk("Initializing...\n");
	
	eth_init();
	
	server_init();
}
