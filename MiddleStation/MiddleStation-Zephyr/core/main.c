#include "main.h"
//#include <random/rand32.h>

extern void conn_process(void);

K_THREAD_DEFINE(conn_process_thread, 1024,
                conn_process, NULL, NULL, NULL,
                3, 0, 0);

void main(void)
{
	k_mutex_init(&discovering);
	k_condvar_init(&wait_discovering_complete);

	k_sem_init(&gatt_read_sem, 0, 1);

	ble_comms_start();
}

void conn_process(){
	while(1){
		if(IsEmptyQueue(&ConnPQ)==0){
			ConnHandler curr_conn;

			//Enter critical section
			int irq_key = irq_lock();
			DeQueue(&ConnPQ, &curr_conn);
			irq_unlock(irq_key);

			char addr_str[BT_ADDR_LE_STR_LEN];
			bt_addr_le_to_str(bt_conn_get_dst(curr_conn.conn), addr_str,sizeof(addr_str));
			printk("Successfully handling connection from %s\n",addr_str);

			k_mutex_lock(&discovering, K_FOREVER);
			int err = bt_gatt_dm_start(curr_conn.conn, NULL, &discover_all_cb, NULL);
			if (err) {
				printk("Failed to start discovery (err %d)\n", err);
				bt_conn_disconnect(curr_conn.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				curr_conn.conn = NULL;
			}else{
				k_condvar_wait(&wait_discovering_complete,&discovering,K_FOREVER);
				bt_conn_disconnect(curr_conn.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				curr_conn.conn = NULL;
			}
			k_mutex_unlock(&discovering);
		}
	}
}