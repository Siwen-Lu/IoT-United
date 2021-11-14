#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/dk_prov.h>
#include <dk_buttons_and_leds.h>

#include "model_handler.h"
#include "button_func.h"
#include "speaker_func.h"

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_mesh_init(bt_mesh_dk_prov_init(), model_handler_init());
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	/* This will be a no-op if settings_load() loaded provisioning info */
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

	printk("Mesh initialized\n");
}

void main(void)
{
	int err;
	
	button_init();
	speaker_init();
	dk_leds_init();

	printk("Initializing...\n");
	dk_set_led_on(DK_LED3);

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	
	while (1)
	{
		if (bt_mesh_is_provisioned())
		{
			dk_set_led_off(DK_LED3);
			dk_set_led_on(DK_LED2);
			break;
		}
		k_sleep(K_SECONDS(1));
	}
	
}
