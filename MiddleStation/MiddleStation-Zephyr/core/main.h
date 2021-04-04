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
#include "device_record.h"

extern struct k_mutex discovering;
extern struct k_condvar wait_discovering_complete;

extern struct k_sem gatt_read_sem;





extern struct bt_gatt_dm_cb discover_all_cb;
extern struct bt_gatt_read_params gatt_read_params;
extern struct bt_gatt_write_params gatt_write_params;



extern ConnPQueue ConnPQ;
extern ConnRec ConnRC;

#endif