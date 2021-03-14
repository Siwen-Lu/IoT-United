#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

uint16_t battery_voltage = 4096;
uint16_t environ_temp = 27;
uint32_t uptime = 0;
uint8_t up_time_ready = 0;
struct k_timer adv_update_timer;
void adv_update_cb(struct k_timer *timer_id);
/*
 * Set Advertisement data. Based on the Eddystone specification:
 * https://github.com/google/eddystone/blob/master/protocol-specification.md
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
		      0xaa, 0xfe, /* Eddystone UUID */
		      0x20, /* Eddystone-TLM frame type */
		      0x00, /* TLM version */
		      0X10,
              0X11,/* Battery voltage */
              0X13,
              0X00,/* Beacon temperature */
              0X00,
              0X00,
              0X00,
              0X00,/* Advertising PDU count */
		      0X00,
              0X00,
              0X00,
              0X00) /* Time since power-on or reboot */
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_ready(int err)
{
	char addr_s[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr = {0};
	size_t count = 1;

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}


	/* For connectable advertising you would use
	 * bt_le_oob_get_local().  For non-connectable non-identity
	 * advertising an non-resolvable private address is used;
	 * there is no API to retrieve that.
	 */

	bt_id_get(&addr, &count);
	bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

	printk("Beacon started, advertising as %s\n", addr_s);
}

void main(void)
{
	int err;

	printk("Starting Beacon Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
    k_timer_init(&adv_update_timer,adv_update_cb,NULL);
    k_timer_start(&adv_update_timer,K_SECONDS(1),K_SECONDS(0.1));
    while(1){
        k_sleep(K_SECONDS(0.1));
        if(up_time_ready){
            printk("update\n");
            struct bt_data adv[] = {
                BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
                BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
                BT_DATA_BYTES(BT_DATA_SVC_DATA16,
                    0xaa, 0xfe, /* Eddystone UUID */
                    0x20, /* Eddystone-TLM frame type */
                    0x00, /* TLM version */
                    0X10,
                    0X11,/* Battery voltage */
                    0X13,
                    0X00,/* Beacon temperature */
                    0X00,
                    0X00,
                    0X00,
                    0X00,/* Advertising PDU count */
                    (uint8_t)((uptime & 0xff000000) >> 24),
                    (uint8_t)((uptime & 0x00ff0000) >> 16),
                    (uint8_t)((uptime & 0x0000ff00) >> 8),
                    (uint8_t)(uptime & 0x000000ff)
                    ) /* Time since power-on or reboot */
            };
            bt_le_adv_update_data(adv, ARRAY_SIZE(adv), sd, ARRAY_SIZE(sd));
            up_time_ready = 0;
        }
    }
}

void adv_update_cb(struct k_timer *timer_id){
    uptime = (uint32_t) k_uptime_get();
    uptime/= 100; //  0.1 second resolution
    printk("%d\n",uptime);
    up_time_ready = 1;
}