#include "ble_comms.h"

struct bt_conn *conn_connecting;
uint8_t conn_count;
struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
struct bt_gatt_discover_params discover_params;
struct bt_gatt_read_params read_params;

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	/* connect only to devices in close proximity */
	if (rssi < -40) {
		return;
	}
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (conn_connecting) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	//printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

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

void start_scan(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
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


// uint8_t read_func(struct bt_conn *conn, uint8_t err,
// 				    struct bt_gatt_read_params *params,
// 				    const void *data, uint16_t length)
// {
// 	uint8_t value = 0;
// 	memcpy(&value,data, length);
// 	char addr_str[BT_ADDR_LE_STR_LEN];
// 	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str,sizeof(addr_str));
// 	printk("%s :",addr_str);
// 	printk("%d\n",value);
// 	return BT_GATT_ITER_STOP;
// }
// uint8_t discover_func(struct bt_conn *conn,
// 			     const struct bt_gatt_attr *attr,
// 			     struct bt_gatt_discover_params *params)
// {
// 	int err;

// 	if (!attr) {
// 		printk("Discover complete\n");
// 		(void)memset(params, 0, sizeof(*params));
// 		return BT_GATT_ITER_STOP;
// 	}

// 	printk("[ATTRIBUTE] handle %u\n", attr->handle);

// 	if (!bt_uuid_cmp(discover_params.uuid,
// 				BT_UUID_BAS_BATTERY_LEVEL)) {
// 		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
// 		discover_params.uuid = &uuid.uuid;
// 		discover_params.start_handle = attr->handle + 2;
// 		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		
// 		read_params.func = read_func;
// 		read_params.handle_count = 1;
// 		read_params.single.handle = attr->handle + 1;
// 		read_params.single.offset = 0;

// 		bt_gatt_read(conn,&read_params);


// 		err = bt_gatt_discover(conn, &discover_params);
// 		if (err) {
// 			printk("Discover failed (err %d)\n", err);
// 		}
// 	}
// 	return BT_GATT_ITER_STOP;
// }

static void discover_all_completed(struct bt_gatt_dm *dm, void *ctx)
{
	char uuid_str[37];

	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);

	size_t attr_count = bt_gatt_dm_attr_cnt(dm);

	bt_uuid_to_str(gatt_service->uuid, uuid_str, sizeof(uuid_str));
	printk("Found service %s\n", uuid_str);
	printk("Attribute count: %d\n", attr_count);

	bt_gatt_dm_data_print(dm);
	bt_gatt_dm_data_release(dm);

	bt_gatt_dm_continue(dm, NULL);
}

static void discover_all_service_not_found(struct bt_conn *conn, void *ctx)
{
	printk("No more services\n");
}

static void discover_all_error_found(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
}

static struct bt_gatt_dm_cb discover_all_cb = {
	.completed = discover_all_completed,
	.service_not_found = discover_all_service_not_found,
	.error_found = discover_all_error_found,
};


static void connected(struct bt_conn *conn, uint8_t err)
{
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

	err = bt_gatt_dm_start(conn, NULL, &discover_all_cb, NULL);
	if (err) {
		printk("Failed to start discovery (err %d)\n", err);
	}
	printk("Failed to start discovery (err %d)\n", err);
	printk("Connected (%u): %s\n", conn_count, addr);
	// if (conn == conn_connecting) {
	// 	memcpy(&uuid, BT_UUID_BAS_BATTERY_LEVEL, sizeof(uuid));
	// 	discover_params.uuid = &uuid.uuid;
	// 	discover_params.func = discover_func;
	// 	discover_params.start_handle = 0x0001;
	// 	discover_params.end_handle = 0xffff;
	// 	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	// 	err = bt_gatt_discover(conn_connecting, &discover_params);
	// 	if (err) {
	// 		printk("Discover failed(err %d)\n", err);
	// 		return;
	// 	}
	// }

	conn_connecting = NULL;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_unref(conn);

	if (conn_count == CONFIG_BT_MAX_CONN) {
		start_scan();
	}
	conn_count--;

	printk("Disconnected (%u): %s (reason 0x%02x)\n", conn_count, addr, reason);
}

// static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
// {
// 	printk("LE conn  param req: int (0x%04x, 0x%04x) lat %d to %d\n",
// 	       param->interval_min, param->interval_max, param->latency,
// 	       param->timeout);

// 	return true;
// }

// static void le_param_updated(struct bt_conn *conn, uint16_t interval,
// 			     uint16_t latency, uint16_t timeout)
// {
// 	printk("LE conn param updated: int 0x%04x lat %d to %d\n", interval,
// 	       latency, timeout);
// }


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