#include "device_record.h"

void insertRec(ConnRec *connrec, DeviceRec devrec){
    int idx = searchAddr(connrec,&devrec.addr);
    if(idx!=-1){
        connrec->DevRec[idx].last_conn_timestamp = devrec.last_conn_timestamp;
        connrec->DevRec[idx].priority = devrec.priority;
        connrec->DevRec[idx].rssi = devrec.rssi;
    }else{
        if(connrec->NumofRec < NUM_MAX_RECORDS){
            memcpy(&connrec->DevRec[connrec->NumofRec],&devrec,sizeof(DeviceRec));
            connrec->NumofRec++;
        }else{
            connrec->DevRec[connrec->earliest_idx] = devrec;
            memcpy(&connrec->DevRec[connrec->earliest_idx],&devrec,sizeof(DeviceRec));
        }
    }
    connrec->earliest_idx = getEarliest(connrec);
}

uint8_t getEarliest(ConnRec *connrec){
    uint8_t idx = 0;
    int64_t min = INT64_MAX;
    for(int i = 0; i < connrec->NumofRec; i++){
        if(connrec->DevRec[i].last_conn_timestamp < min){
            min = connrec->DevRec[i].last_conn_timestamp;
            idx = i;
        }
    }
    return idx;
}

int searchAddr(ConnRec *connrec,const bt_addr_le_t *addr){
    for(int i = 0; i < connrec->NumofRec; i++){
        if(bt_addr_le_cmp(&connrec->DevRec[i].addr, addr)==0){
            return i;
        }
    }
    return -1;
}

int64_t getLastConnTimeStamp(ConnRec *connrec,const bt_addr_le_t *addr){
    int idx = searchAddr(connrec, addr);
    if(idx == -1){
        return INT64_MIN;
    }else{
        return connrec->DevRec[idx].last_conn_timestamp;
    }
}