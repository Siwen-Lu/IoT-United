#include "conn_queue.h"
void InitQueue(ConnPQueue *queue){
	queue->Front = NULL;
	queue->Rear = NULL;
}

int IsEmptyQueue(ConnPQueue *queue) {
    if (queue->Front == NULL) {
        return true;
    }
    else {
        return false;
    }
}

int IsFullQueue(ConnPQueue *queue) {
	if(IsEmptyQueue(queue)){
		return false;
	}
	int i = 1;
	ConnPQNode *curr = queue->Front;
	while(curr->Next != NULL){
		curr = curr->Next;
		i++;
	}
	if(i >= MAX_CONN_QUEUE_SIZE){
		return true;
	}
	return false;
}

int EnQueue(ConnPQueue *queue, ConnHandler ch, uint32_t priority) {

	//printk("entering queue\n");

    ConnPQNode *p = (ConnPQNode *)k_calloc(1,sizeof(ConnPQNode));

    if (p != NULL) {

		//memcpy(&p->conn,&ch,sizeof(ConnHandler));
		p->priority = priority;

		if(IsEmptyQueue(queue)){
			//printk("queue is empty\n");
			p->Next = NULL;
			queue->Front = p;
			queue->Rear = p;
			return true;
		}

		ConnPQNode *curr = queue->Front;

		if(IsFullQueue(queue)){
			//printk("CONN Queue is full, need to drop last one\n");
			while(curr->Next != queue->Rear){
				curr = curr->Next;
			}
			ConnPQNode *last = queue->Rear;
			queue->Rear = curr;
			queue->Rear->Next = NULL;
			k_free(last);
			last = NULL;
		}

		curr = queue->Front;
		ConnPQNode *prev = NULL;
		while(curr->Next != NULL){
			if(priority <= curr->priority){
				prev = curr;
				curr = curr->Next;
			}else{
				break;
			}
		}

		if(curr == queue->Front){
			//printk("insert before the first node\n");
			p->Next = curr;
			queue->Front = p;
		}else if(curr == queue->Rear && priority <= curr->priority){
			//printk("insert after the last node\n");
			p->Next = NULL;
			curr->Next = p;
			queue->Rear = p;
		}else{
			//printk("insert in the middle\n");
			prev->Next = p;
			p->Next = curr;
		}

		return true;
    }else{
		return false;
	}
}

int DeQueue(ConnPQueue *queue, ConnHandler *ch){
    if (IsEmptyQueue(queue)!=1) {

		ConnPQNode *p = queue->Front;
		//memcpy(ch,&p->conn,sizeof(ConnHandler));
		queue->Front = queue->Front->Next;
		k_free(p);
		p = NULL;

		return true;
    }else{
		return false;
	}
}

void DestroyQueue(ConnPQueue *queue) {
    while (queue->Front != NULL) {
        queue->Rear = queue->Front->Next;
        k_free(queue->Front);
        queue->Front = queue->Rear;
    }
}