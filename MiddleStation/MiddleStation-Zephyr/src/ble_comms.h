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

#define BAT_SERV_UUID BT_UUID_DECLARE_16(0x180F)
#define BAT_LVL_CHAR_UUID BT_UUID_DECLARE_16(0x2A19)

#define SOUND_SERV_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xEF680500, 0x9B35, 0x4933, 0x9B10, 0x52FFA9740042))
#define SPEAKER_CHAR_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xEF680502, 0x9B35, 0x4933, 0x9B10, 0x52FFA9740042))

void ble_comms_start();
void start_scan(void);
void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad);

#endif