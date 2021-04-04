#ifndef _DEV_REC_
#define _DEV_REC_

#include "ble_comms.h"

#define NUM_MAX_RECORDS 256 //MAX 256

typedef struct device_record{
    bt_addr_le_t addr;
    int64_t last_conn_timestamp;
    int8_t rssi;
    int64_t priority;
}DeviceRec;

typedef struct conn_record{
    DeviceRec DevRec[NUM_MAX_RECORDS];
    uint8_t NumofRec;
    uint8_t earliest_idx;
}ConnRec;

void insertRec(ConnRec *connrec, DeviceRec devrec);
uint8_t getEarliest(ConnRec *connrec); //return the index of the earliest conn
int searchAddr(ConnRec *connrec,const bt_addr_le_t *addr); //-1 if no record
int64_t getLastConnTimeStamp(ConnRec *connrec,const bt_addr_le_t *addr); //INT64_MIN if no record


#endif