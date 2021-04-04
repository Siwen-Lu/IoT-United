#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#define FLAGS	0
#endif

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define ADV_UPDATE_THREAD_STACK_SIZE 500
#define ADV_UPDATE_THREAD_PRIORITY 5

#define HTS221_THREAD_STACK_SIZE 500
#define HTS221_THREAD_PRIORITY 5

void adv_update(void);
void hts221_update(void);
void hts221_process(const struct device *dev);

K_THREAD_DEFINE(adv_update_thread, ADV_UPDATE_THREAD_STACK_SIZE,
                adv_update, NULL, NULL, NULL,
                ADV_UPDATE_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(hts221_thread, HTS221_THREAD_STACK_SIZE,
                hts221_update, NULL, NULL, NULL,
                HTS221_THREAD_PRIORITY, 0, 0);

uint16_t battery_voltage = 4096;
uint16_t environ_temp = 27;
uint32_t uptime = 0;

const struct device *hts221 ;
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
	bt_id_get(&addr, &count);
	bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

	printk("Beacon started, advertising as %s\n", addr_s);
}

void adv_update(void){
    while(1){
        k_sleep(K_SECONDS(0.5));
        uptime = (uint32_t) k_uptime_get();
        uptime/= 100;
        printk("%d\n",uptime);
        printk("%x\n",environ_temp);
        struct bt_data adv[] = {
            BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
            BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
            BT_DATA_BYTES(BT_DATA_SVC_DATA16,
                0xaa, 0xfe, /* Eddystone UUID */
                0x20, /* Eddystone-TLM frame type */
                0x00, /* TLM version */
                0X10,
                0X11,/* Battery voltage */
                (uint8_t) (environ_temp >> 8),
                (uint8_t) (environ_temp & 0xff),/* Beacon temperature */
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
    }
}

void main(void)
{
	int err;

    hts221 = device_get_binding("HTS221");
	if (hts221 == NULL) {
		printk("Could not get HTS221 device\n");
		return;
	}

	printk("Starting Beacon Demo\n");
	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}

// void main(void)
// {
// 	const struct device *dev;
// 	bool led_is_on = true;
// 	int ret;

// 	dev = device_get_binding(LED0);
// 	if (dev == NULL) {
// 		return;
// 	}

// 	ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
// 	if (ret < 0) {
// 		return;
// 	}

// 	while (1) {
// 		gpio_pin_set(dev, PIN, (int)led_is_on);
// 		led_is_on = !led_is_on;
// 		k_msleep(SLEEP_TIME_MS);
// 	}
// }

void hts221_update(void){
    while(1){
        hts221_process(hts221);
        k_sleep(K_SECONDS(2));
    }
}

void hts221_process(const struct device *dev)
{
	struct sensor_value temp, hum;
	if (sensor_sample_fetch(dev) < 0) {
		printk("Sensor sample update error\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
		printk("Cannot read HTS221 temperature channel\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
		printk("Cannot read HTS221 humidity channel\n");
		return;
	}
	/* display temperature */
	printk("Temperature:%.1f C\n", sensor_value_to_double(&temp));
    double temp_value = sensor_value_to_double(&temp);
    environ_temp = (((int)temp_value) << 8)+ (int) ((temp_value - ((int)temp_value)) * 100);

	/* display humidity */
	printk("Relative Humidity:%.1f%%\n",
	       sensor_value_to_double(&hum));
}