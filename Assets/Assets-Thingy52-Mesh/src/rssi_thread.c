#include "rssi_thread.h"

K_THREAD_STACK_DEFINE(rssi_stack_area, RSSI_STACK_SIZE);
struct k_work_q rssi_work_q;

rssi_buffer nearest_three[3];

static void init_buffer()
{
	for (int i = 0; i < 3; i++)
	{
		nearest_three[i].address = 0xffff;
		nearest_three[i].rssi = -127;
	}
}

void init_rssi_thread()
{
	k_work_queue_start(&rssi_work_q, rssi_stack_area, K_THREAD_STACK_SIZEOF(rssi_stack_area), RSSI_PRIORITY, NULL);
	
	init_buffer();
}

static int getFarthestRecordIndex()
{
	int i = 0;
	int min = 127;
	
	for (int n = 0; n < 3; n++)
	{
		if (min > nearest_three[n].rssi)
		{
			min = nearest_three[n].rssi;
			i = n;
		}
	}
	return i;
}

int getNearestRecordIndex()
{
	int i = 0;
	int max = -127;
	
	for (int n = 0; n < 3; n++)
	{
		if (max <= nearest_three[n].rssi)
		{
			max = nearest_three[n].rssi;
			i = n;
		}
	}
	return i;
}


void update_buffer(uint16_t address, int8_t rssi)
{
	int i = getFarthestRecordIndex();
	if (nearest_three[i].rssi <= rssi)
	{
		nearest_three[i].address = address;
		nearest_three[i].rssi = rssi;
	}
}

void process_rssi(struct k_work *item)
{
	RSSI *rssi = CONTAINER_OF(item, RSSI, work);
	
	//printk("RSSI %04x : %d dB\n", rssi->address, rssi->rssi);
	
	update_buffer(rssi->address, rssi->rssi);
	
	k_free(rssi);
}
