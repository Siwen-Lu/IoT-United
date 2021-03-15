#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

uint8_t eddy_stone_pattern[] = {0x02,0x01,0x04,0x03,0x03,0xaa,0xfe,0x11,0x16,0xaa,0xfe,0x20,0x00};

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		    struct net_buf_simple *buf)
{
    uint8_t id[13];
    memcpy(id,buf->data,13);

    if(memcmp(id,eddy_stone_pattern,13)==0){
        char bt_addr[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(addr, bt_addr, sizeof(bt_addr));

        printk("%s  ",bt_addr);

        printk("rssi: %d\n",rssi);

        uint8_t adv_data[12];

        memcpy(adv_data,&buf->data[13],12);

        uint16_t battery_voltage = (adv_data[0]<<8) + adv_data[1];
        uint8_t environ_temp_int = adv_data[2];
        uint8_t environ_temp_frac =adv_data[3];
        uint32_t uptime = (adv_data[8]<<24) + (adv_data[9]<<16)
                        + (adv_data[10]<<8) + adv_data[11];

        printk("battery voltage : %d\n",battery_voltage);
        printk("environ temp : %d.%d\n",environ_temp_int,environ_temp_frac);
        printk("uptime : %d\n",uptime);
        /*for(int x=0; x<12;x++){
            printk("%02x",adv_data[x]);
        }
        printk("\n");*/
    }
    
}

void main(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x0010,
		.window     = 0x0010,
	};
	int err;

	printk("Starting Scanner\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return;
	}
}
