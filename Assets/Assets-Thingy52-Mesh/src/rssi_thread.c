#include "rssi_thread.h"

K_THREAD_STACK_DEFINE(rssi_stack_area, RSSI_STACK_SIZE);
struct k_work_q rssi_work_q;
rssi_buffer nearest_three[3];

K_MUTEX_DEFINE(buffer_mutex);


void init_buffer()
{
	k_mutex_lock(&buffer_mutex, K_FOREVER);
	for (int i = 0; i < 3; i++)
	{
		nearest_three[i].address = 0xffff;
		nearest_three[i].rssi = -127;
	}
	k_mutex_unlock(&buffer_mutex);
}

void init_rssi_thread()
{
	k_work_queue_start(&rssi_work_q, rssi_stack_area, K_THREAD_STACK_SIZEOF(rssi_stack_area), RSSI_PRIORITY, NULL);
	
	init_buffer();
}

static int getFarthestRecordIndex()
{
	int i = -1;
	if (k_mutex_lock(&buffer_mutex, K_MSEC(500)) == 0)
	{
		int min = 127;
		for (int n = 0; n < 3; n++)
		{
			if (min > nearest_three[n].rssi)
			{
				min = nearest_three[n].rssi;
				i = n;
			}
		}
		k_mutex_unlock(&buffer_mutex);
	}
	return i;
}

int getRecords(rssi_buffer *buf)
{
	int i = -1;
	
	if (k_mutex_lock(&buffer_mutex, K_MSEC(500)) == 0)
	{
		int max = -127;
		for (int n = 0; n < 3; n++)
		{
			buf[n].address = nearest_three[n].address;
			buf[n].rssi = nearest_three[n].rssi;
			
			if (max <= nearest_three[n].rssi)
			{
				max = nearest_three[n].rssi;
				i = n;
			}
		}
		k_mutex_unlock(&buffer_mutex);
	}
	
	return i;
}

static int hasRecord(uint16_t addr)
{
	for (int x = 0; x < 3; x++)
	{
		if (nearest_three[x].address == addr)
		{
			return x;
		}
	}
	return -1;
}


void update_buffer(uint16_t address, int8_t rssi)
{
	int i = getFarthestRecordIndex();
	if (i != -1)
	{
		if (k_mutex_lock(&buffer_mutex, K_MSEC(200)) == 0)
		{
			if (nearest_three[i].rssi <= rssi)
			{
				int y = hasRecord(address);
				if (y!=-1)
				{
					nearest_three[y].rssi = rssi;
				}
				else
				{
					nearest_three[i].address = address;
					nearest_three[i].rssi = rssi;
				}
			}
			k_mutex_unlock(&buffer_mutex);
		}
	}
}

void process_rssi(struct k_work *item)
{
	RSSI *rssi = CONTAINER_OF(item, RSSI, work);
	
	//printk("RSSI %04x : %d dB\n", rssi->address, rssi->rssi);
	
	update_buffer(rssi->address, rssi->rssi);
	
	k_free(rssi);
}
