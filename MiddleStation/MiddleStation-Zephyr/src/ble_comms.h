#ifndef _BLE_COMMS_H_
#define _BLE_COMMS_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>

#include "conn_queue.h"
#include "device_record.h"

void ble_comms_start();
void start_scan(void);
void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad);

#endif