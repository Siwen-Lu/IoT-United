#include <stdio.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>

#include "chat_cli.h"
#include "model_handler.h"

#include <logging/log.h>
#include <sys/reboot.h>

LOG_MODULE_DECLARE(chat);

/******************************************************************************/
/*************************** Health server setup ******************************/
/******************************************************************************/
/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;
#define MQTT_PUB_STACK_SIZE 2048
#define MQTT_PUB_PRIORITY 5

K_THREAD_STACK_DEFINE(mqtt_pub_stack_area, MQTT_PUB_STACK_SIZE);

struct k_work_q mqtt_pub_work_q;

void mqtt_publish_cb(struct k_work *item);

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

/******************************************************************************/
/***************************** Chat model setup *******************************/
/******************************************************************************/
struct presence_cache {
	uint16_t addr;
	enum bt_mesh_chat_cli_presence presence;
};

/* Cache of Presence values of other chat clients. */
static struct presence_cache presence_cache[
			    CONFIG_BT_MESH_CHAT_SAMPLE_PRESENCE_CACHE_SIZE];

static const uint8_t *presence_string[] = {
	[BT_MESH_CHAT_CLI_PRESENCE_AVAILABLE] = "available",
	[BT_MESH_CHAT_CLI_PRESENCE_AWAY] = "away",
	[BT_MESH_CHAT_CLI_PRESENCE_DO_NOT_DISTURB] = "dnd",
	[BT_MESH_CHAT_CLI_PRESENCE_INACTIVE] = "inactive",
};

/**
 * Returns true if the specified address is an address of the local element.
 */
static bool address_is_local(struct bt_mesh_model *mod, uint16_t addr)
{
	return bt_mesh_model_elem(mod)->addr == addr;
}

/**
 * Returns true if the provided address is unicast address.
 */
static bool address_is_unicast(uint16_t addr)
{
	return (addr > 0) && (addr <= 0x7FFF);
}

/**
 * Returns true if the node is new or the presence status is different from
 * the one stored in the cache.
 */
static bool presence_cache_entry_check_and_update(uint16_t addr,
	enum bt_mesh_chat_cli_presence presence)
{
	static size_t presence_cache_head;
	size_t i;

	/* Find address in cache. */
	for (i = 0; i < ARRAY_SIZE(presence_cache); i++) {
		if (presence_cache[i].addr == addr) {
			if (presence_cache[i].presence == presence) {
				return false;
			}

			/* Break since the node in the cache. */
			break;
		}
	}

	/* Not in cache. */
	if (i == ARRAY_SIZE(presence_cache)) {
		for (i = 0; i < ARRAY_SIZE(presence_cache); i++) {
			if (!presence_cache[i].addr) {
				break;
			}
		}

		/* Cache is full. */
		if (i == ARRAY_SIZE(presence_cache)) {
			i = presence_cache_head;
			presence_cache_head = (presence_cache_head + 1)
			% CONFIG_BT_MESH_CHAT_SAMPLE_PRESENCE_CACHE_SIZE;
		}
	}

	/* Update cache. */
	presence_cache[i].addr = addr;
	presence_cache[i].presence = presence;

	return true;
}

static void print_client_status(void);

static void handle_chat_start(struct bt_mesh_chat_cli *chat)
{
	print_client_status();
}

static void handle_chat_presence(struct bt_mesh_chat_cli *chat,
	struct bt_mesh_msg_ctx *ctx,
	enum bt_mesh_chat_cli_presence presence)
{
	if (address_is_local(chat->model, ctx->addr)) {
		if (address_is_unicast(ctx->recv_dst)) {
			printk("<you> are %s\n",
				presence_string[presence]);
		}
	}
	else {
		if (address_is_unicast(ctx->recv_dst)) {
			printk("<0x%04X> is %s\n",
				ctx->addr,
				presence_string[presence]);
		}
		else if (presence_cache_entry_check_and_update(ctx->addr,
			presence)) {
			printk("<0x%04X> is now %s\n",
				ctx->addr,
				presence_string[presence]);
		}
	}
}

static void handle_chat_message(struct bt_mesh_chat_cli *chat,
	struct bt_mesh_msg_ctx *ctx,
	const uint8_t *msg)
{
	/* Don't print own messages. */
	if (address_is_local(chat->model, ctx->addr)) {
		return;
	}

	printk("<0x%04X>: %s\n", ctx->addr, msg);
}

typedef struct mqtt_pub_msg
{
	struct k_work work;
	uint16_t node_address;
	uint16_t middleAddr1;
	uint16_t middleAddr2;
	uint16_t middleAddr3;
	int8_t middleRSSI1;
	int8_t middleRSSI2;
	int8_t middleRSSI3;
}mqtt_pub_msg;

static void handle_chat_private_message(struct bt_mesh_chat_cli *chat,
	struct bt_mesh_msg_ctx *ctx,
	const uint8_t *msg)
{

	/* Don't print own messages. */
	if (address_is_local(chat->model, ctx->addr)) {
		return;
	}
	
	mqtt_pub_msg *mqtt_buf = k_malloc(sizeof(mqtt_pub_msg));
	
	memcpy(&mqtt_buf->middleAddr1, &msg[0], 2);
	memcpy(&mqtt_buf->middleRSSI1, &msg[2], 1);
	memcpy(&mqtt_buf->middleAddr2, &msg[3], 2);
	memcpy(&mqtt_buf->middleRSSI2, &msg[5], 1);
	memcpy(&mqtt_buf->middleAddr3, &msg[6], 2);
	memcpy(&mqtt_buf->middleRSSI3, &msg[8], 1);
	
	mqtt_buf->node_address = ctx->addr;
	
	k_work_init(&mqtt_buf->work, mqtt_publish_cb);
	k_work_submit_to_queue(&mqtt_pub_work_q, &mqtt_buf->work);

}

static void handle_chat_message_reply(struct bt_mesh_chat_cli *chat,
	struct bt_mesh_msg_ctx *ctx)
{
	printk("<0x%04X> received the message\n", ctx->addr);
}


static const struct bt_mesh_chat_cli_handlers chat_handlers = {
	.start = handle_chat_start,
	.presence = handle_chat_presence,
	.message = handle_chat_message,
	.private_message = handle_chat_private_message,
	.message_reply = handle_chat_message_reply,
};

/* .. include_startingpoint_model_handler_rst_1 */
static struct bt_mesh_chat_cli chat = {
	.handlers = &chat_handlers,
};

void mqtt_publish_cb(struct k_work *item)
{
	mqtt_pub_msg *buffer = CONTAINER_OF(item, mqtt_pub_msg, work);
	
	char str[64];
	
	sprintf(str, "%d  %d:%d  %d:%d  %d:%d", buffer->node_address, buffer->middleAddr1, buffer->middleRSSI1, buffer->middleAddr2, buffer->middleRSSI2, buffer->middleAddr3, buffer->middleRSSI3);	
	
	printk("%s\n", str);
	
	bt_mesh_chat_cli_private_message_send(&chat, 0x1000, str);
	
	k_free(buffer);
}


static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(
		1,
	BT_MESH_MODEL_LIST(
		BT_MESH_MODEL_CFG_SRV,
		BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
	BT_MESH_MODEL_LIST(BT_MESH_MODEL_CHAT_CLI(&chat))),
};
/* .. include_endpoint_model_handler_rst_1 */

static void print_client_status(void)
{
	if (!bt_mesh_is_provisioned()) {
		printk("The mesh node is not provisioned. Please provision the mesh node before using the chat.\n");
	}
	else {
		printk("The mesh node is provisioned. The client address is 0x%04x.\n", bt_mesh_model_elem(chat.model)->addr);
	}

	printk("Current presence: %s\n", presence_string[chat.presence]);
}

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

void rssiBoardcasting(struct k_timer *dummy)
{
	if (chat.model->keys[0] == 0xffff)
		return;
	
	uint8_t msg[4] = { 0x00, 0x00, 0x00, 0x01 };
	
	struct bt_mesh_msg_ctx ctx = {
		.addr = 0xC001,
		.app_idx = chat.model->keys[0],
		.send_ttl = 0,
		.send_rel = false,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf,
		BT_MESH_CHAT_CLI_OP_MESSAGE,
		BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE);
	bt_mesh_model_msg_init(&buf, BT_MESH_CHAT_CLI_OP_MESSAGE);

	net_buf_simple_add_mem(&buf,
		msg,
		strnlen(msg,
			CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH));
	net_buf_simple_add_u8(&buf, '\0');

	int err = bt_mesh_model_send(chat.model, &ctx, &buf, NULL, NULL);
	if (err) {
		printk("failed to send message: %d\n", err);
		sys_reboot(SYS_REBOOT_COLD);
	}
}

K_TIMER_DEFINE(rssi_timer, rssiBoardcasting, NULL);

/******************************************************************************/
/******************************** Public API **********************************/
/******************************************************************************/


const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);

	printk("Starting BLE Mesh\n");
	
	k_timer_start(&rssi_timer, K_SECONDS(5), K_SECONDS(1));

	k_work_queue_start(&mqtt_pub_work_q,
		mqtt_pub_stack_area,
		K_THREAD_STACK_SIZEOF(mqtt_pub_stack_area),
		MQTT_PUB_PRIORITY,
		NULL);
	return &comp;
}