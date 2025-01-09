#include <point.h>

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

ZBUS_CHAN_DECLARE(point_chan);

// ==================================================
// State that is mirrored from other subsystems. A lock must be used
// when accessing these variables as they can be accessed in the web
// callbacks

K_MUTEX_DEFINE(web_points_lock);
static point web_points[20] = {};

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
// v1 API handler

// static const struct json_obj_descr point_descr[] = {
// 	JSON_OBJ_DESCR_FIELD(struct point_js_t, type, JSON_TOK_STRING),
// 	JSON_OBJ_DESCR_FIELD(point_js, key, JSON_TOK_STRING),
// };

static int v1_handler(struct http_client_ctx *client, enum http_data_status status,
		      const struct http_request_ctx *request_ctx, struct http_response_ctx *resp,
		      void *user_data)
{
	LOG_DBG("v1 handler: %s", client->url_buffer);

	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}

	if (strcmp(client->url_buffer, "/v1/points") == 0) {
		char buf[64];
		points_dump(web_points, ARRAY_SIZE(web_points), buf, sizeof(buf));
		LOG_DBG("web points: %s", buf);

		int ret = points_json_encode(web_points, ARRAY_SIZE(web_points), recv_buffer,
					     sizeof(recv_buffer));
		if (ret != 0) {
			LOG_ERR("Error returning JSON points: %i", ret);
		}
	} else {
		sprintf(recv_buffer, "%s", CONFIG_BOARD_TARGET);
	}

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

ZBUS_MSG_SUBSCRIBER_DEFINE(web_sub);
ZBUS_CHAN_ADD_OBS(point_chan, web_sub, 3);

void web_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("siot web thread");
	http_server_start();

	point p;

	const struct zbus_channel *chan;
	while (!zbus_sub_wait_msg(&web_sub, &chan, &p, K_FOREVER)) {
		if (chan == &point_chan) {
			int ret = points_merge(web_points, ARRAY_SIZE(web_points), &p);
			if (ret != 0) {
				LOG_ERR("Error storing point in web point cache: %i", ret);
			}

			char buf[64];
			points_dump(web_points, ARRAY_SIZE(web_points), buf, sizeof(buf));
			LOG_DBG("web points: %s", buf);
		}
	}
}

K_THREAD_DEFINE(web, STACKSIZE, web_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
