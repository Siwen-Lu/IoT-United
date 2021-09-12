#include "eth_comms.h"
#include <nrfx_clock.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);
void main(void)
{	
	// force the app core running at 128mhz
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
	k_sleep(K_SECONDS(1));
	
	LOG_INF("init eth dhcp");
	eth_init();
	k_sleep(K_SECONDS(5));
	
	LOG_INF("init udp server");
	server_init();
}
