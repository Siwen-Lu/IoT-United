#ifndef __MESH_LIGHT_HANDLER_H__
#define __MESH_LIGHT_HANDLER_H__
#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
struct led_ctx {
	struct bt_mesh_onoff_srv srv;
	struct k_work_delayable work;
	uint32_t remaining;
	bool value;
};

extern const struct bt_mesh_onoff_srv_handlers onoff_handlers;

extern struct led_ctx led_ctx[3];
void led_transition_start(struct led_ctx *led);
void led_status(struct led_ctx *led, struct bt_mesh_onoff_status *status);
#endif

