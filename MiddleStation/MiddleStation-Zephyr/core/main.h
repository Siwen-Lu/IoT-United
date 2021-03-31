#ifndef _MAIN_H_
#define _MAIN_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include "ble_comms.h"

extern struct bt_conn *thingy;
extern int is_connected;
extern struct k_mutex discovering;
extern struct k_condvar wait_discovering_complete;
extern struct bt_gatt_dm_cb discover_all_cb;


typedef struct{
    bool dirty; // was it connected before?
    struct bt_conn *conn;
    int8_t rssi;
    uint8_t battery_lvl;
}conn_handler;

typedef struct{
    bt_addr_le_t addr;
    uint32_t last_conn_timestamp;
}device_records;

#endif