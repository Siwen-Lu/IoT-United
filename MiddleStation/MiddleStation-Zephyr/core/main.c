#include "main.h"

void main(void)
{
	is_connected = 0;
	k_mutex_init(&discovering);
	k_condvar_init(&wait_discovering_complete);
	ble_comms_start();

	while(1){
		if(is_connected){
			char addr_str[BT_ADDR_LE_STR_LEN];
			bt_addr_le_to_str(bt_conn_get_dst(thingy), addr_str,sizeof(addr_str));
			printk("connected %s\n",addr_str);

			k_mutex_lock(&discovering, K_FOREVER);
			int err = bt_gatt_dm_start(thingy, NULL, &discover_all_cb, NULL);
			if (err) {
				printk("Failed to start discovery (err %d)\n", err);
				bt_conn_disconnect(thingy, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				thingy = NULL;
				is_connected = 0;
			}else{
				k_condvar_wait(&wait_discovering_complete,&discovering,K_FOREVER);
				bt_conn_disconnect(thingy, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				thingy = NULL;
				is_connected = 0;
			}
			k_mutex_unlock(&discovering);
		}
	}
}