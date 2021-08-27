#ifndef __CMD_THREAD__
#define __CMD_THREAD__
#include <zephyr.h>

#define CMD_STACK_SIZE 1024
#define CMD_PRIORITY 5
extern struct k_work_q cmd_work_q;
void init_cmd_thread();
#endif