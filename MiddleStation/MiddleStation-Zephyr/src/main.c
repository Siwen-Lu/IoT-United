#include "main.h"

// Using the Marco K_THREAD_DEFINE to define a Thread to handle the Bluetooth connections
extern void conn_process(void);
K_THREAD_DEFINE(conn_process_thread,
	1024,
	conn_process,
	NULL,
	NULL,
	NULL,
	10,
	0,
	0);

void main(void)
{
	k_mutex_init(&discovering);
	k_condvar_init(&wait_discovering_complete);
	k_sem_init(&gatt_read_sem, 0, 1);
	ble_comms_start();
}

void conn_process() {
	while (1) {
		if (IsEmptyQueue(&ConnPQ) == 0) {
//			ConnPQNode *pNode = (ConnPQNode *)k_calloc(1, sizeof(ConnPQNode));
//			DeQueue(&ConnPQ, pNode);
//			struct bt_conn *curr_conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &pNode->addr);
//			bt_conn_disconnect(curr_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
//			
//			bt_conn_unref(curr_conn);
//			curr_conn = NULL;
//			k_free(pNode);
//			pNode = NULL;
			ConnPQNode *pNode = (ConnPQNode *)k_calloc(1, sizeof(ConnPQNode));
			DeQueue(&ConnPQ, pNode);
			char addr_str[BT_ADDR_LE_STR_LEN];
			bt_addr_le_to_str(&pNode->addr, addr_str, sizeof(addr_str));
			printk("Handling connection from %s\n", addr_str);
			
			struct bt_conn *curr_conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &pNode->addr);

			if (curr_conn != NULL)
			{
				bt_conn_unref(curr_conn);
				
				printk("start bas disc\n");
				
				// start to discover bas service & read battery level
				k_mutex_lock(&discovering, K_MINUTES(1));
				int err = bt_gatt_dm_start(curr_conn, BAT_SERV_UUID, &discover_all_cb, NULL);
				if (err) {
					printk("Failed to start discovery (err %d)\n", err);
					bt_conn_disconnect(curr_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				}
				else {

					k_condvar_wait(&wait_discovering_complete, &discovering, K_SECONDS(30));

					//k_sleep(K_MSEC(500));
				}
				k_mutex_unlock(&discovering);
				
				
				printk("start sound disc\n");
				
				k_mutex_lock(&discovering, K_MINUTES(1));
				err = bt_gatt_dm_start(curr_conn, SOUND_SERV_UUID, &discover_all_cb, NULL);
				if (err) {
					printk("Failed to start discovery (err %d)\n", err);
					bt_conn_disconnect(curr_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				}
				else {

					k_condvar_wait(&wait_discovering_complete, &discovering, K_SECONDS(30));

					//k_sleep(K_MSEC(500));
				}
				k_mutex_unlock(&discovering);


			}
			k_free(pNode);
			pNode = NULL;
		}
		// to avoid blocking other threads
		k_sleep(K_MSEC(10));
	}
}