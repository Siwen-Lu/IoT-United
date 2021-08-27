#include "rssi_thread.h"

K_THREAD_STACK_DEFINE(rssi_stack_area, RSSI_STACK_SIZE);
struct k_work_q rssi_work_q;


void init_rssi_thread()
{
	k_work_queue_start(&rssi_work_q, rssi_stack_area, K_THREAD_STACK_SIZEOF(rssi_stack_area), RSSI_PRIORITY, NULL);	
}

void process_rssi(struct k_work *item)
{
	RSSI *rssi = CONTAINER_OF(item, RSSI, work);
	
	printk("RSSI %04x : %d dB\n", rssi->address, rssi->rssi);
	
	k_free(rssi);
}