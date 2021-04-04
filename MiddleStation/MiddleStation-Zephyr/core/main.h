#ifndef _MAIN_H_
#define _MAIN_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <power/reboot.h>

#include "ble_comms.h"
#include "conn_queue.h"

extern struct k_mutex discovering;
extern struct k_condvar wait_discovering_complete;
extern struct bt_gatt_dm_cb discover_all_cb;
extern ConnPQueue ConnPQ;

typedef struct{
    bt_addr_le_t addr;
    uint32_t last_conn_timestamp;
}device_records;

#endif