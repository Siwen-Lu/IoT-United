// #ifndef PRIORITY_QUEUE_H
// #define PRIORITY_QUEUE_H
// #endif

#define SIZE 64

typedef struct item {
    char val;
    int priority;
} Item;

typedef struct prio_queue {
    struct prio_queue * next;
    Item data;    
} prio_queue;


//resize based on SIZE and insert into the priority_queue
prio_queue *enqueue(prio_queue *array_item,Item new_item,int priority);

//delete Item and return the deleted Item
Item dequeue(prio_queue **array_item);

//initialise
prio_queue * init_priority_queue();