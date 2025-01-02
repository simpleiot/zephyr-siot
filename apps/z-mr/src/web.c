#include "html.h"
#include "config.h"
#include "ats.h"
#include "point.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/data/json.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(z_web, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(z_temp_chan);
ZBUS_CHAN_DECLARE(z_ats_chan);
ZBUS_CHAN_DECLARE(z_config_chan);
ZBUS_CHAN_DECLARE(z_point_chan);

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

// ==================================================
// State that is mirrored from other subsystems. A lock must be used
// when accessing these variables as they can be accessed in the web
// callbacks

K_MUTEX_DEFINE(state_lock);

static z_mr_config config = Z_MR_CONFIG_INIT;
static ats_state astate = INIT_ATS_STATE();
static float temp;

// ==================================================
// HTTP Service

static uint8_t recv_buffer[1024];

static uint16_t http_service_port = 80;
HTTP_SERVICE_DEFINE(siot_http_service, "0.0.0.0", &http_service_port, 1, 10, NULL);

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

// ==================================================
// index.html resource

struct http_resource_detail_static index_html_gz_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/html",
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

HTTP_RESOURCE_DEFINE(index_html_gz_resource, siot_http_service, "/",
		     &index_html_gz_resource_detail);

// ********************************
// boot count handler

static int bootcount_handler(struct http_client_ctx *client, enum http_data_status status,
			     uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			     void *user_data)
{
	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}

	k_mutex_lock(&state_lock, K_FOREVER);
	sprintf(recv_buffer, "%i", config.bootcount);
	k_mutex_unlock(&state_lock);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic bootcount_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = bootcount_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(bootcount_resource, siot_http_service, "/bootcount",
		     &bootcount_resource_detail);

// ********************************
// temp handler

static int temp_handler(struct http_client_ctx *client, enum http_data_status status,
			uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			void *user_data)
{
	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}

	k_mutex_lock(&state_lock, K_FOREVER);
	sprintf(recv_buffer, "%.1f°C", (double)temp);
	k_mutex_unlock(&state_lock);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic temp_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = temp_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(temp_resource, siot_http_service, "/temp", &temp_resource_detail);

// ********************************
// CPU usage handler
// TODO: CPU usage should be generated somewhere else and put on ZBus so things other
// than web can use it.

static int cpu_usage_handler(struct http_client_ctx *client, enum http_data_status status,
			     uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			     void *user_data)
{
	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}

	k_thread_runtime_stats_t stats;
	int rc = k_thread_runtime_stats_all_get(&stats);

	if (rc == 0) { /* item was found, show it */
		sprintf(recv_buffer, "%0.2lf%%",
			((double)stats.total_cycles) * 100 / stats.execution_cycles);
	} else {
		strcpy(recv_buffer, "error");
	}

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic cpu_usage_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = cpu_usage_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(cpu_usage_resource, siot_http_service, "/cpu-usage",
		     &cpu_usage_resource_detail);

// ********************************
// Points handler

// static const struct json_obj_descr point_descr[] = {
// 	JSON_OBJ_DESCR_FIELD(struct point_js_t, type, JSON_TOK_STRING),
// 	JSON_OBJ_DESCR_FIELD(point_js, key, JSON_TOK_STRING),
// };

static int points_handler(struct http_client_ctx *client, enum http_data_status status,
			  uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			  void *user_data)
{
	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}
	sprintf(recv_buffer, "%s", CONFIG_BOARD_TARGET);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic points_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = points_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(points_resource, siot_http_service, "/v1/points", &points_resource_detail);

// ********************************
// Board handler

static int board_handler(struct http_client_ctx *client, enum http_data_status status,
			 uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			 void *user_data)
{
	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}
	sprintf(recv_buffer, "%s", CONFIG_BOARD_TARGET);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic board_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = board_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(board_resource, siot_http_service, "/board", &board_resource_detail);

// ********************************
// DID handler

static int did_handler(struct http_client_ctx *client, enum http_data_status status,
		       uint8_t *buffer, size_t len, struct http_response_ctx *resp, void *user_data)
{

	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}

	LOG_DBG("CLIFF: did: %s", config.device_id);

	k_mutex_lock(&state_lock, K_FOREVER);
	strcpy(recv_buffer, config.device_id);
	k_mutex_unlock(&state_lock);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic did_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = did_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(did_resource, siot_http_service, "/did", &did_resource_detail);

// ********************************
// ipstatic handler

static int ipstatic_handler(struct http_client_ctx *client, enum http_data_status status,
			    uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			    void *user_data)
{

	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}

	k_mutex_lock(&state_lock, K_FOREVER);
	if (config.static_ip) {
		strcpy(recv_buffer, "true");
	} else {
		strcpy(recv_buffer, "false");
	}
	k_mutex_unlock(&state_lock);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic ipstatic_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = ipstatic_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(ipstatic_resource, siot_http_service, "/ipstatic", &ipstatic_resource_detail);

// ********************************
// ipaddr handler

static int ipaddr_handler(struct http_client_ctx *client, enum http_data_status status,
			  uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			  void *user_data)
{
	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}

	k_mutex_lock(&state_lock, K_FOREVER);
	strcpy(recv_buffer, config.ip_addr);
	k_mutex_unlock(&state_lock);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic ipaddr_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = ipaddr_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(ipaddr_resource, siot_http_service, "/ipaddr", &ipaddr_resource_detail);

// ********************************
// subnet-mask handler

static int subnet_mask_handler(struct http_client_ctx *client, enum http_data_status status,
			       uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			       void *user_data)
{
	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}

	k_mutex_lock(&state_lock, K_FOREVER);
	strcpy(recv_buffer, config.subnet_mask);
	k_mutex_unlock(&state_lock);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic subnet_mask_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = subnet_mask_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(subnet_mask_resource, siot_http_service, "/subnet-mask",
		     &subnet_mask_resource_detail);

// ********************************
// gateway handler

static int gateway_handler(struct http_client_ctx *client, enum http_data_status status,
			   uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			   void *user_data)
{

	k_mutex_lock(&state_lock, K_FOREVER);
	strcpy(recv_buffer, config.gateway);
	k_mutex_unlock(&state_lock);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic gateway_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = gateway_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(gateway_resource, siot_http_service, "/gateway", &gateway_resource_detail);

// ********************************
// settings post handler

void settings_callback(char *key, char *value)
{
	LOG_DBG("setting: %s:%s", key, value);

	point p = {0};

	point_set_type(&p, key);
	point_put_string(&p, value);

	// check for values that are not strings
	if (strcmp(key, POINT_TYPE_STATICIP) == 0) {
		if (strcmp(value, "true") == 0) {
			point_put_int(&p, 1);
		} else {
			point_put_int(&p, 0);
		}
	}

	char desc[40];
	point_description(&p, desc, sizeof(desc));

	zbus_chan_pub(&z_point_chan, &p, K_MSEC(500));
}

static int settings_handler(struct http_client_ctx *client, enum http_data_status status,
			    uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			    void *user_data)
{
	if (status == HTTP_SERVER_DATA_ABORTED) {
		return 0;
	}

	static uint8_t settings_buffer[256];
	static size_t cursor;

	if (len + cursor > sizeof(settings_buffer)) {
		cursor = 0;
		return -ENOMEM;
	}

	memcpy(settings_buffer + cursor, buffer, len);
	cursor += len;

	if (status == HTTP_SERVER_DATA_FINAL) {
		if (cursor >= sizeof(settings_buffer)) {
			cursor = 0;
			return -ENOMEM;
		}

		// make sure data is null terminated
		settings_buffer[cursor] = 0;

		// LOG_HEXDUMP_DBG(settings_buffer, cursor, "settings data");
		cursor = 0;

		html_parse_form_data(settings_buffer, settings_callback);

		LOG_INF("Settings updated");

		// FIXME: nvs_dump_settings();

		strcpy(recv_buffer, "Settings saved");

		resp->body = recv_buffer;
		resp->body_len = strlen(recv_buffer);
		resp->final_chunk = true;
	}

	return 0;
}

static struct http_resource_detail_dynamic settings_post_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_POST),
		},
	.cb = settings_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(settings_post_resource, siot_http_service, "/settings",
		     &settings_post_resource_detail);

// ==================================================
// /devices resource

static uint8_t recv_buffer[1024];

char *on = "<span class=\"on\"></span>";
char *off = "<span class=\"off\"></span>";

#define RENDER_ON_OFF(state) state ? on : off

static int devices_handler(struct http_client_ctx *client, enum http_data_status status,
			   uint8_t *buffer, size_t len, struct http_response_ctx *resp,
			   void *user_data)
{

	if (status == HTTP_SERVER_DATA_FINAL) {
		char *end = recv_buffer;

		end += sprintf(end, "<ul>");
		k_mutex_lock(&state_lock, K_FOREVER);
		for (int i = 0; i < ARRAY_LENGTH(astate.state); i++) {
			end += sprintf(end, "<li><b>#%i</b>: AON:%s ONA:%s BON:%s ONB:%s</li>", i,
				       RENDER_ON_OFF(astate.state[i].aon),
				       RENDER_ON_OFF(astate.state[i].ona),
				       RENDER_ON_OFF(astate.state[i].bon),
				       RENDER_ON_OFF(astate.state[i].onb));
		}
		k_mutex_unlock(&state_lock);
		end += sprintf(end, "</ul>");

		resp->body = recv_buffer;
		resp->body_len = strlen(recv_buffer);
		resp->final_chunk = true;
	}

	return 0;
}

struct http_resource_detail_dynamic devices_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
		},
	.cb = devices_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(devices_resource, siot_http_service, "/devices", &devices_resource_detail);

ZBUS_SUBSCRIBER_DEFINE(z_web_sub, 8);
ZBUS_CHAN_ADD_OBS(z_temp_chan, z_web_sub, 1);
ZBUS_CHAN_ADD_OBS(z_ats_chan, z_web_sub, 2);
ZBUS_CHAN_ADD_OBS(z_config_chan, z_web_sub, 3);

void z_web_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("z web thread");
	http_server_start();

	// zbus_chan_add_obs(&z_temp_chan, &z_web_sub, K_MSEC(500));
	// zbus_chan_add_obs(&z_ats_chan, &z_web_sub, K_MSEC(500));
	// zbus_chan_add_obs(&z_config_chan, &z_web_sub, K_MSEC(500));

	while (1) {
		const struct zbus_channel *chan;
		while (!zbus_sub_wait(&z_web_sub, &chan, K_FOREVER)) {
			k_mutex_lock(&state_lock, K_FOREVER);
			if (chan == &z_temp_chan) {
				zbus_chan_read(&z_temp_chan, &temp, K_NO_WAIT);
			} else if (chan == &z_ats_chan) {
				zbus_chan_read(&z_ats_chan, &astate, K_NO_WAIT);
			} else if (chan == &z_config_chan) {
				zbus_chan_read(&z_config_chan, &config, K_NO_WAIT);
			}
			k_mutex_unlock(&state_lock);
		}
	}
}

K_THREAD_DEFINE(z_web, STACKSIZE, z_web_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
