#ifndef _CONN_QUEUE_
#define _CONN_QUEUE_
#define MAX_CONN_QUEUE_SIZE CONFIG_BT_MAX_CONN - 1
#include <power/reboot.h>
#include <zephyr.h>
#include "ble_comms.h"

typedef struct pq_conn_node {
	bt_addr_le_t addr;
	int64_t priority;
	struct pq_conn_node * Next;
}ConnPQNode;

typedef struct pq_conn {
	ConnPQNode *Front, *Rear;
}ConnPQueue;

void InitQueue(ConnPQueue *queue);
int IsEmptyQueue(ConnPQueue *queue);
int IsFullQueue(ConnPQueue *queue);
int EnQueue(ConnPQueue *queue, const bt_addr_le_t *addr, int64_t priority);
int DeQueue(ConnPQueue *queue, ConnPQNode *node);


#endif