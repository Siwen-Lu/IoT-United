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

typedef struct rssi_buffer
{
	uint16_t address;
	int8_t rssi;

}rssi_buffer;

extern rssi_buffer nearest_three[3];
extern struct k_work_q rssi_work_q;
void init_rssi_thread();
void process_rssi(struct k_work *item);
void init_buffer();
int getRecords();
#endif
