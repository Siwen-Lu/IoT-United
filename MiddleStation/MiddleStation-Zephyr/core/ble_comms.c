#include "main.h"

struct bt_conn *thingy;
int is_connected;

struct k_mutex discovering;
struct k_condvar wait_discovering_complete;

struct bt_conn *conn_connecting;
uint8_t conn_count;
struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
struct bt_gatt_discover_params discover_params;
struct bt_gatt_read_params read_params;

uint8_t thingy_pattern[21] = {0x02,0x01,0x06,0x11,0x06,0x42,0x00,0x74,0xa9,0xff,0x52,0x10,0x9b,0x33,0x49,0x35,0x9b,0x00,0x01,0x68,0xef};

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int key = irq_lock();
	uint8_t id[21];
    memcpy(id,ad->data,21);

    if(memcmp(id,thingy_pattern,21)==0){
		char addr_str[BT_ADDR_LE_STR_LEN];
		int err;

		if (conn_connecting) {
			printk("wait for disconnecting\n");
			return;
		}

		/* We're only interested in connectable events */
		if (type != BT_GAP_ADV_TYPE_ADV_IND &&
			type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
			return;
		}

		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

		printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

		if (bt_le_scan_stop()) {
			return;
		}

		err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT, &conn_connecting);
		if (err) {
			printk("Create conn to %s failed (%d)\n", addr_str, err);
			start_scan();
		}
	}
	irq_unlock(key);
}

void start_scan(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_PASSIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_GAP_SCAN_SLOW_INTERVAL_1,
		.window     = BT_GAP_SCAN_SLOW_INTERVAL_1,
	};
	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void discover_all_completed(struct bt_gatt_dm *dm, void *ctx)
{
	char uuid_str[37];

	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);

	size_t attr_count = bt_gatt_dm_attr_cnt(dm);

	bt_uuid_to_str(gatt_service->uuid, uuid_str, sizeof(uuid_str));

	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str,sizeof(addr_str));
	printk("%s :",addr_str);



	printk("Found service %s\n", uuid_str);
	printk("Attribute count: %d\n", attr_count);

	//bt_gatt_dm_data_print(dm);
	printk("print done \n");
	bt_gatt_dm_data_release(dm);
	printk("release done \n");

	bt_gatt_dm_continue(dm, NULL);
}

static void discover_all_service_not_found(struct bt_conn *conn, void *ctx)
{
	printk("No more services\n");
	k_mutex_lock(&discovering,K_FOREVER);
	k_condvar_signal(&wait_discovering_complete);
	k_mutex_unlock(&discovering);
	//bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void discover_all_error_found(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
	k_mutex_lock(&discovering,K_FOREVER);
	k_condvar_signal(&wait_discovering_complete);
	k_mutex_unlock(&discovering);
}

struct bt_gatt_dm_cb discover_all_cb = {
	.completed = discover_all_completed,
	.service_not_found = discover_all_service_not_found,
	.error_found = discover_all_error_found,
};


static void connected(struct bt_conn *conn, uint8_t err)
{
	int key = irq_lock();
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(conn_connecting);
		conn_connecting = NULL;

		start_scan();
		return;
	}
	conn_count++;
	if (conn_count < CONFIG_BT_MAX_CONN) {
		start_scan();
	}

	printk("Connected (%u): %s\n", conn_count, addr);

	thingy = conn;
	is_connected = 1;

	conn_connecting = NULL;
	irq_unlock(key);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int key = irq_lock();
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_unref(conn);
	is_connected = 0;
	if (conn_count == CONFIG_BT_MAX_CONN) {
		start_scan();
	}

	conn_count--;

	printk("Disconnected (%u): %s (reason 0x%02x)\n", conn_count, addr, reason);
	irq_unlock(key);
}

struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void ble_comms_start(){
    int err;
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	start_scan();

}