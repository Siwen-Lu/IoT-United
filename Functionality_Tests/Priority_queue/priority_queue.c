#include <stdio.h>
#include <stdlib.h>
#include "priority_queue.h"

//initialise priority queue
prio_queue * init_priority_queue() {
    prio_queue * head = malloc(sizeof(prio_queue));
    head->next = NULL;
    return head;
}

//inserting into priority queue
prio_queue * insert(prio_queue *array_item,Item new_item,int priority) {
    
    //case for 0 array item
    if (array_item == NULL) {

        array_item = init_priority_queue();
        array_item->data = new_item;
        return array_item;
    }


    prio_queue * node = malloc(sizeof(prio_queue));
    node->next = NULL;
    node->data = new_item;

    //casr for 1 or more array item
    if (array_item->next == NULL) {
        if (array_item->data.priority < priority) {
            node->next = array_item;
            return node;
        } else {
            array_item->next = node;
            return array_item;
        }
    }

    //2 or more array item
    if (array_item->data.priority < priority) {
        node->next = array_item;
        return node;
    }

    prio_queue * prev = array_item;
    prio_queue * cur = array_item->next;

    while (cur != NULL) {

        if (cur->data.priority < priority) {
            break;
        }
        prev = prev->next;
        cur = cur->next;
    }

    //at the end
    //smallest prio
    if (cur == NULL) {
        prev->next = node;
    } else { 
        //middle
        node->next = cur;
        prev->next = node;
    }

    return array_item;
}

//resize based on SIZE and insert into the priority_queue
prio_queue * enqueue(prio_queue *array_item,Item new_item,int priority) {
    array_item = insert(array_item,new_item,priority);

    prio_queue * cur = array_item;

    //resizing
    int i = 0;
    while(cur != NULL) {

        if (i == SIZE) {
            if (cur->next != NULL) {
                prio_queue *node = cur->next;
                free(node);
                cur->next = NULL;
            }

            break;
        }

        cur = cur->next;
        i++;
    }

    return array_item;
}

Item dequeue(prio_queue **array_item) {
    prio_queue *cur = *array_item;
    prio_queue *prev = cur;

    prio_queue *copy = cur;

    cur = cur->next;
    *array_item = cur;    
    Item pop_item = prev->data;
    free(prev);
    return pop_item;
} 

int main() {

    Item a = {'A',2};
    Item b = {'B',3};
    Item c = {'C',1};
    Item d = {'D',4};

    prio_queue * array = NULL;//= init_priority_queue(); 
    prio_queue * copy;


    array = enqueue(array,a,a.priority);
    array = enqueue(array,b,b.priority);

    array = enqueue(array,c,c.priority);


    array = enqueue(array,d,d.priority);

    dequeue(&array);
    copy = array;

    return 0;
}



