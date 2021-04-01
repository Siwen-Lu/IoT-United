#ifndef _CONN_QUEUE_
#define _CONN_QUEUE_
#define MAX_CONN_QUEUE_SIZE CONFIG_BT_MAX_CONN
#include <power/reboot.h>
#include <zephyr.h>
#include "ble_comms.h"

typedef struct pq_conn_node {
    //ConnHandler conn;
    uint32_t priority;
    struct pq_conn_node * Next;
}ConnPQNode;


typedef struct pq_conn {
    ConnPQNode *Front, *Rear;
}ConnPQueue;

void InitQueue(ConnPQueue *queue);
int IsEmptyQueue(ConnPQueue *queue);
int EnQueue(ConnPQueue *queue, ConnHandler ch, uint32_t priority);
int DeQueue(ConnPQueue *queue, ConnHandler *ch);
void DestroyQueue(ConnPQueue *queue);


#endif