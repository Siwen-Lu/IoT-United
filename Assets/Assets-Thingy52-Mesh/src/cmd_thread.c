#include "cmd_thread.h"

K_THREAD_STACK_DEFINE(cmd_stack_area, CMD_STACK_SIZE);
struct k_work_q cmd_work_q;

void init_cmd_thread()
{
	k_work_queue_start(&cmd_work_q, cmd_stack_area, K_THREAD_STACK_SIZEOF(cmd_stack_area), CMD_PRIORITY, NULL);
}
