#include "conn_queue.h"
void InitQueue(ConnPQueue *queue){
	queue->Front = NULL;
	queue->Rear = NULL;
}

int IsEmptyQueue(ConnPQueue *queue) {
	int key = irq_lock();
    if (queue->Front == NULL) {
	    irq_unlock(key);
        return true;
    }
    else {
	    irq_unlock(key);
        return false;
    }
}

int IsFullQueue(ConnPQueue *queue) {
	int key = irq_lock();
	if(IsEmptyQueue(queue)){
		return false;
	}
	int i = 1;
	ConnPQNode *curr = queue->Front;
	while(curr->Next != NULL){
		curr = curr->Next;
		i++;
	}
	irq_unlock(key);
	if(i >= MAX_CONN_QUEUE_SIZE){
		return true;
	}
	return false;
}

//can assume the queue never exceed the max length
int EnQueue(ConnPQueue *queue, const bt_addr_le_t *addr, int64_t priority) {
	
	int key = irq_lock();
	
    ConnPQNode *p = (ConnPQNode *)k_calloc(1,sizeof(ConnPQNode));

    if (p != NULL) {

	    bt_addr_le_copy(&p->addr, addr);
		p->priority = priority;

		if(IsEmptyQueue(queue)){
			p->Next = NULL;
			queue->Front = p;
			queue->Rear = p;
			return true;
		}
		ConnPQNode *curr = queue->Front;
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
			p->Next = curr;
			queue->Front = p;
		}else if(curr == queue->Rear && priority <= curr->priority){
			p->Next = NULL;
			curr->Next = p;
			queue->Rear = p;
		}else{
			prev->Next = p;
			p->Next = curr;
		}
	    irq_unlock(key);
		return true;
    }else{
	    irq_unlock(key);
		return false;
	}
}

int DeQueue(ConnPQueue *queue, ConnPQNode *node) {
	int key = irq_lock();
    if (IsEmptyQueue(queue)!=1) {
		ConnPQNode *p = queue->Front;
	    // Assign connection info to ConnPQNode *node
	    bt_addr_le_copy(&node->addr, &p->addr);
	    node->priority = p->priority;
	    node->Next = NULL;
	    // Update the PQ
		queue->Front = queue->Front->Next;
	    // free the extracted node
		k_free(p);
		p = NULL;
	    irq_unlock(key);
		return true;
    }else{
	    irq_unlock(key);
		return false;
	}
}
