#ifndef __RSSI_THREAD__
#define __RSSI_THREAD__
#include <zephyr.h>

#define RSSI_STACK_SIZE 1024
#define RSSI_PRIORITY 8

typedef struct RSSI_INFO
{
	struct k_work work;
	uint16_t address;
	int8_t rssi;
}RSSI;

extern struct k_work_q rssi_work_q;
void init_rssi_thread();
void process_rssi(struct k_work *item);
#endif
