#include "html.h"
#include "point.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/util.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(siot_web, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(siot_point_chan);

// ==================================================
// State that is mirrored from other subsystems. A lock must be used
// when accessing these variables as they can be accessed in the web
// callbacks

K_MUTEX_DEFINE(state_lock);

static point points[20];
static const int points_len = ARRAY_SIZE(points);

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
// v1 API handler

// static const struct json_obj_descr point_descr[] = {
// 	JSON_OBJ_DESCR_FIELD(struct point_js_t, type, JSON_TOK_STRING),
// 	JSON_OBJ_DESCR_FIELD(point_js, key, JSON_TOK_STRING),
// };

static int v1_handler(struct http_client_ctx *client, enum http_data_status status, uint8_t *buffer,
		      size_t len, struct http_response_ctx *resp, void *user_data)
{
	LOG_DBG("v1 handler: %s", client->url_buffer);

	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}
	sprintf(recv_buffer, "%s", CONFIG_BOARD_TARGET);

	resp->body = recv_buffer;
	resp->body_len = strlen(recv_buffer);
	resp->final_chunk = true;

	return 0;
}

struct http_resource_detail_dynamic v1_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = v1_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(points_resource, siot_http_service, "/v1/*", &v1_resource_detail);

// ********************************
// Board handler
// FIXME: move this to state

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

ZBUS_SUBSCRIBER_DEFINE(z_web_sub, 8);

ZBUS_CHAN_ADD_OBS(siot_point_chan, z_web_sub, 3);

void z_web_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("siot web thread");
	http_server_start();

	while (1) {
		const struct zbus_channel *chan;
		while (!zbus_sub_wait(&z_web_sub, &chan, K_FOREVER)) {
			k_mutex_lock(&state_lock, K_FOREVER);
			if (chan == &siot_point_chan) {
				// TODO: do something
			}
			k_mutex_unlock(&state_lock);
		}
	}
}

K_THREAD_DEFINE(z_web, STACKSIZE, z_web_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
