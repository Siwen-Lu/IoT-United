#include "mesh_cmd_handler.h"
#include <logging/log.h>
LOG_MODULE_DECLARE(command);

BUILD_ASSERT(BT_MESH_MODEL_BUF_LEN(BT_MESH_CHAT_CLI_OP_MESSAGE,
		BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE) <=
 BT_MESH_RX_SDU_MAX,
	"The message must fit inside an application SDU.");
BUILD_ASSERT(BT_MESH_MODEL_BUF_LEN(BT_MESH_CHAT_CLI_OP_MESSAGE,
		BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE) <=
 BT_MESH_TX_SDU_MAX,
	"The message must fit inside an application SDU.");

static void encode_presence(struct net_buf_simple *buf,
	enum bt_mesh_chat_cli_presence presence)
{
	bt_mesh_model_msg_init(buf, BT_MESH_CHAT_CLI_OP_PRESENCE);
	net_buf_simple_add_u8(buf, presence);
}

static const uint8_t *extract_msg(struct net_buf_simple *buf)
{
	buf->data[buf->len - 1] = '\0';
	return net_buf_simple_pull_mem(buf, buf->len);
}

static void handle_message(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->user_data;
	const uint8_t *msg;

	msg = extract_msg(buf);

	if (chat->handlers->message) {
		chat->handlers->message(chat, ctx, msg);
	}
}

static void send_message_reply(struct bt_mesh_chat_cli *chat,
	struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg,
		BT_MESH_CHAT_CLI_OP_MESSAGE_REPLY,
		BT_MESH_CHAT_CLI_MSG_LEN_MESSAGE_REPLY);
	bt_mesh_model_msg_init(&msg, BT_MESH_CHAT_CLI_OP_MESSAGE_REPLY);

	(void)bt_mesh_model_send(chat->model, ctx, &msg, NULL, NULL);
}

static void handle_private_message(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->user_data;
	const uint8_t *msg;

	msg = extract_msg(buf);

	if (chat->handlers->private_message) {
		chat->handlers->private_message(chat, ctx, msg);
	}

	send_message_reply(chat, ctx);
}
/* .. include_endpoint_chat_cli_rst_1 */

static void handle_message_reply(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->user_data;

	if (chat->handlers->message_reply) {
		chat->handlers->message_reply(chat, ctx);
	}
}

static void handle_presence(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->user_data;
	enum bt_mesh_chat_cli_presence presence;

	presence = net_buf_simple_pull_u8(buf);

	if (chat->handlers->presence) {
		chat->handlers->presence(chat, ctx, presence);
	}
}

static void handle_presence_get(struct bt_mesh_model *model,
	struct bt_mesh_msg_ctx *ctx,
	struct net_buf_simple *buf)
{
	struct bt_mesh_chat_cli *chat = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg,
		BT_MESH_CHAT_CLI_OP_PRESENCE,
		BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE);

	encode_presence(&msg, chat->presence);

	(void) bt_mesh_model_send(chat->model, ctx, &msg, NULL, NULL);
}

const struct bt_mesh_model_op _bt_mesh_chat_cli_op[] = {
{
	BT_MESH_CHAT_CLI_OP_MESSAGE,
	BT_MESH_CHAT_CLI_MSG_MINLEN_MESSAGE,
	handle_message
},
{
	BT_MESH_CHAT_CLI_OP_PRIVATE_MESSAGE,
	BT_MESH_CHAT_CLI_MSG_MINLEN_MESSAGE,
	handle_private_message
},
{
	BT_MESH_CHAT_CLI_OP_MESSAGE_REPLY,
	BT_MESH_CHAT_CLI_MSG_LEN_MESSAGE_REPLY,
	handle_message_reply
},
{
	BT_MESH_CHAT_CLI_OP_PRESENCE,
	BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE,
	handle_presence
},
{
	BT_MESH_CHAT_CLI_OP_PRESENCE_GET,
	BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE_GET,
	handle_presence_get
},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_chat_cli_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_chat_cli *chat = model->user_data;

	/* Continue publishing current presence. */
	encode_presence(model->pub->msg, chat->presence);

	return 0;
}

#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_chat_cli_settings_set(struct bt_mesh_model *model,
	const char *name,
	size_t len_rd,
	settings_read_cb read_cb,
	void *cb_arg)
{
	struct bt_mesh_chat_cli *chat = model->user_data;

	if (name) {
		return -ENOENT;
	}

	ssize_t bytes = read_cb(cb_arg,
		&chat->presence,
		sizeof(chat->presence));
	if (bytes < 0) {
		return bytes;
	}

	if (bytes != 0 && bytes != sizeof(chat->presence)) {
		return -EINVAL;
	}

	return 0;
}
#endif

static int bt_mesh_chat_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_chat_cli *chat = model->user_data;

	chat->model = model;

	net_buf_simple_init_with_data(&chat->pub_msg,
		chat->buf,
		sizeof(chat->buf));
	chat->pub.msg = &chat->pub_msg;
	chat->pub.update = bt_mesh_chat_cli_update_handler;

	return 0;
}

static int bt_mesh_chat_cli_start(struct bt_mesh_model *model)
{
	struct bt_mesh_chat_cli *chat = model->user_data;

	if (chat->handlers->start) {
		chat->handlers->start(chat);
	}

	return 0;
}

static void bt_mesh_chat_cli_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_chat_cli *chat = model->user_data;

	chat->presence = BT_MESH_CHAT_CLI_PRESENCE_AVAILABLE;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void) bt_mesh_model_data_store(model, true, NULL, NULL, 0);
	}
}

const struct bt_mesh_model_cb _bt_mesh_chat_cli_cb = {
	.init = bt_mesh_chat_cli_init,
	.start = bt_mesh_chat_cli_start,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = bt_mesh_chat_cli_settings_set,
#endif
	.reset = bt_mesh_chat_cli_reset,
};

int bt_mesh_chat_cli_presence_set(struct bt_mesh_chat_cli *chat,
	enum bt_mesh_chat_cli_presence presence)
{
	if (presence != chat->presence) {
		chat->presence = presence;

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			(void) bt_mesh_model_data_store(chat->model,
				true,
				NULL,
				&presence,
				sizeof(chat->presence));
		}
	}

	encode_presence(chat->model->pub->msg, chat->presence);

	return bt_mesh_model_publish(chat->model);
}

int bt_mesh_chat_cli_presence_get(struct bt_mesh_chat_cli *chat,
	uint16_t addr)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = chat->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf,
		BT_MESH_CHAT_CLI_OP_PRESENCE_GET,
		BT_MESH_CHAT_CLI_MSG_LEN_PRESENCE_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_CHAT_CLI_OP_PRESENCE_GET);

	return bt_mesh_model_send(chat->model, &ctx, &buf, NULL, NULL);
}

int bt_mesh_chat_cli_message_send(struct bt_mesh_chat_cli *chat,
	const uint8_t *msg)
{
	struct net_buf_simple *buf = chat->model->pub->msg;

	bt_mesh_model_msg_init(buf, BT_MESH_CHAT_CLI_OP_MESSAGE);

	net_buf_simple_add_mem(buf,
		msg,
		strnlen(msg,
			CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH));
	net_buf_simple_add_u8(buf, '\0');

	return bt_mesh_model_publish(chat->model);
}

int bt_mesh_chat_cli_private_message_send(struct bt_mesh_chat_cli *chat,
	uint16_t addr,
	const uint8_t *msg)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = chat->model->keys[0],
		.send_ttl = 0,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf,
		BT_MESH_CHAT_CLI_OP_PRIVATE_MESSAGE,
		BT_MESH_CHAT_CLI_MSG_MAXLEN_MESSAGE);
	bt_mesh_model_msg_init(&buf, BT_MESH_CHAT_CLI_OP_PRIVATE_MESSAGE);

	net_buf_simple_add_mem(&buf,
		msg,
		strnlen(msg,
			CONFIG_BT_MESH_CHAT_CLI_MESSAGE_LENGTH));
	net_buf_simple_add_u8(&buf, '\0');

	return bt_mesh_model_send(chat->model, &ctx, &buf, NULL, NULL);
}

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
			printk(
				"<you> are %s",
				presence_string[presence]);
		}
	}
	else {
		if (address_is_unicast(ctx->recv_dst)) {
			printk(
				"<0x%04X> is %s",
				ctx->addr,
				presence_string[presence]);
		}
		else if (presence_cache_entry_check_and_update(ctx->addr,
			presence)) {
			printk(
				"<0x%04X> is now %s",
				ctx->addr,
				presence_string[presence]);
		}
	}
}
struct k_work_q command_workqueue_work_q;
// receive broadcast from middle station and get RSSI
static void handle_chat_message(struct bt_mesh_chat_cli *chat,
	struct bt_mesh_msg_ctx *ctx,
	const uint8_t *msg)
{
	/* Don't print own messages. */
	if (address_is_local(chat->model, ctx->addr)) {
		return;
	}
	
	/* TODO */

}

static void handle_chat_private_message(struct bt_mesh_chat_cli *chat,
	struct bt_mesh_msg_ctx *ctx,
	const uint8_t *msg)
{
	/* Don't print own messages. */
	if (address_is_local(chat->model, ctx->addr)) {
		return;
	}

	printk( "<0x%04X>: *you* %s", ctx->addr, msg);
	
	//to do command parsing
}

static void handle_chat_message_reply(struct bt_mesh_chat_cli *chat,
	struct bt_mesh_msg_ctx *ctx)
{
	printk( "<0x%04X> received the message", ctx->addr);
}

static const struct bt_mesh_chat_cli_handlers chat_handlers = {
	.start = handle_chat_start,
	.presence = handle_chat_presence,
	.message = handle_chat_message,
	.private_message = handle_chat_private_message,
	.message_reply = handle_chat_message_reply,
};

struct bt_mesh_chat_cli chat = {
	.handlers = &chat_handlers,
};

static void print_client_status(void)
{
	if (!bt_mesh_is_provisioned()) {
		printk(
			"The mesh node is not provisioned. Please provision the mesh node before using the chat.");
	}
	else {
		printk(
			"The mesh node is provisioned. The client address is 0x%04x.",
			bt_mesh_model_elem(chat.model)->addr);
	}

	printk(
		"Current presence: %s",
		presence_string[chat.presence]);
}