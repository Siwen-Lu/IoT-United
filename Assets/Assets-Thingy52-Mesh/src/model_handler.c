#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
#include "mesh_light_handler.h"
#include "mesh_cmd_handler.h"
#include "rssi_thread.h"
#include "cmd_thread.h"
#include "power/reboot.h"

extern struct led_ctx led_ctx[3];
extern struct bt_mesh_chat_cli chat;

extern rssi_buffer nearest_three[3];

static void led_work(struct k_work *work)
{
	struct led_ctx *led = CONTAINER_OF(work, struct led_ctx, work.work);
	int led_idx = led - &led_ctx[0];

	if (led->remaining) {
		led_transition_start(led);
	}
	else {
		dk_set_led(led_idx, led->value);

		/* Publish the new value at the end of the transition */
		struct bt_mesh_onoff_status status;

		led_status(led, &status);
		bt_mesh_onoff_srv_pub(&led->srv, NULL, &status);
	}
}
static struct k_work_delayable attention_blink_work;
static bool attention;
static void attention_blink(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
		BIT(0) | BIT(1),
		BIT(1) | BIT(2),
		BIT(2) | BIT(3),
		BIT(3) | BIT(0),
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	}
	else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static void attention_on(struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(struct bt_mesh_model *mod)
{
	/* Will stop rescheduling blink timer */
	attention = false;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(
		1,
	BT_MESH_MODEL_LIST(
			BT_MESH_MODEL_CFG_SRV,
		BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
		BT_MESH_MODEL_LIST(BT_MESH_MODEL_CHAT_CLI(&chat))),
	BT_MESH_ELEM(
		2,
	BT_MESH_MODEL_LIST(BT_MESH_MODEL_ONOFF_SRV(&led_ctx[0].srv)),
	BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};


void uploadSelfLocation(struct k_timer *dummy)
{
	if (chat.model->keys[0] == 0xffff)
		return;
	rssi_buffer rbuf[3];
	int i = getRecords(&rbuf);
	if (rbuf[i].rssi <= -120 || i == -1)
		return;
	
	uint8_t msg[9] = {0};
	int x = 0;
	for (int n = 0; n < 9; n+=3)
	{
		memcpy(&msg[n], &rbuf[x].address, 2);
		memcpy(&msg[n + 2], &rbuf[x].rssi, 1);
		x++;
	}
	struct bt_mesh_msg_ctx ctx = {
		.addr = rbuf[i].address,
		.app_idx = chat.model->keys[0],
		.send_ttl = 0,
		.send_rel = false,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf,
		BT_MESH_CHAT_CLI_OP_PRIVATE_MESSAGE,
		BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE);
	bt_mesh_model_msg_init(&buf, BT_MESH_CHAT_CLI_OP_PRIVATE_MESSAGE);

	net_buf_simple_add_mem(&buf, msg, 9);
	net_buf_simple_add_u8(&buf, '\0');
	
	int err = bt_mesh_model_send(chat.model, &ctx, &buf, NULL, NULL);
	if (err) {
		printk("failed to send message: %d\n", err);
		sys_reboot(SYS_REBOOT_COLD);
	}
	init_buffer();
}

K_TIMER_DEFINE(rssi_timer, uploadSelfLocation, NULL);

const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);

	for (int i = 0; i < ARRAY_SIZE(led_ctx); ++i) {
		k_work_init_delayable(&led_ctx[i].work, led_work);
	}
	
	init_cmd_thread();
	init_rssi_thread();
	
	k_timer_start(&rssi_timer, K_SECONDS(10), K_SECONDS(5));
	
	return &comp;
}
