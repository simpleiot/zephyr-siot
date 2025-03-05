#include <point.h>

#include <zephyr/data/json.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

#include <string.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(siot_web, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);

// ==================================================
// State that is mirrored from other subsystems. A lock must be used
// when accessing these variables as they can be accessed in the web
// callbacks

K_MUTEX_DEFINE(web_points_lock);
static point web_points[40] = {};

// ==================================================
// HTTP Service

static uint8_t recv_buffer[2048];

static uint16_t http_service_port = 80;
HTTP_SERVICE_DEFINE(siot_http_service, "0.0.0.0", &http_service_port, 1, 10, NULL);

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static uint8_t index_js_gz[] = {
#include "index.js.gz.inc"
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

struct http_resource_detail_static index_js_gz_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "application/javascript",
		},
	.static_data = index_js_gz,
	.static_data_len = sizeof(index_js_gz),
};

HTTP_RESOURCE_DEFINE(index_js_gz_resource, siot_http_service, "/index.js",
		     &index_js_gz_resource_detail);

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

	static uint8_t v1_payload_buf[256];
	static size_t cursor;

	if (client->method == HTTP_POST) {
		// Copy payload to our buffer. Note that even for a small payload, it
		// may arrive split into chunks (e.g. if the header size was such that
		// the whole HTTP request exceeds the size of the client buffer).
		if (request_ctx->data_len + cursor > sizeof(v1_payload_buf)) {
			cursor = 0;
			return -ENOMEM;
		}

		memcpy(v1_payload_buf + cursor, request_ctx->data, request_ctx->data_len);
		cursor += request_ctx->data_len;
	}

	if (status == HTTP_SERVER_DATA_ABORTED) {
		cursor = 0;
		return 0;
	}

	if (status == HTTP_SERVER_DATA_FINAL) {
		if (strcmp(client->url_buffer, "/v1/points") == 0) {
			if (client->method == HTTP_GET) {
				k_mutex_lock(&web_points_lock, K_FOREVER);
				int ret = points_json_encode(web_points, ARRAY_SIZE(web_points),
							     recv_buffer, sizeof(recv_buffer));
				k_mutex_unlock(&web_points_lock);
				if (ret != 0) {
					LOG_ERR("Error returning JSON points: %i", ret);
				}
			} else {
				// must be a post
				v1_payload_buf[cursor] = 0;
				// LOG_DBG("data: %s", v1_payload_buf);

				point pts[5] = {};
				int ret = points_json_decode(v1_payload_buf, cursor, pts,
							     ARRAY_SIZE(pts));

				if (ret < 0) {
					LOG_DBG("Post error decoding data: %i", ret);
					strcpy(recv_buffer, "{\"error\":\"error decoding data\"}");
				} else {
					LOG_DBG_POINTS("Received points", pts, ret);
					k_mutex_lock(&web_points_lock, K_FOREVER);
					for (int i = 0; i < ret; i++) {
						points_merge(web_points, ARRAY_SIZE(web_points),
							     &pts[i]);
						zbus_chan_pub(&point_chan, &pts[i], K_MSEC(500));
					}
					k_mutex_unlock(&web_points_lock);
				}
				strcpy(recv_buffer, "{\"error\":\"\"}");
			}
		} else {
			sprintf(recv_buffer, "%s", CONFIG_BOARD_TARGET);
		}

		resp->body = recv_buffer;
		resp->body_len = strlen(recv_buffer);
		resp->final_chunk = true;
		cursor = 0;
	}

	return 0;
}

struct http_resource_detail_dynamic v1_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
		},
	.cb = v1_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(points_resource, siot_http_service, "/v1/*", &v1_resource_detail);

ZBUS_MSG_SUBSCRIBER_DEFINE(web_sub);
ZBUS_CHAN_ADD_OBS(point_chan, web_sub, 3);

void web_thread(void *arg1, void *arg2, void *arg3)
{
	// Initialize points with current network settings
	struct net_if *iface = net_if_get_default();
	if (iface) {
		char ip_addr[NET_IPV4_ADDR_LEN];
		char netmask[NET_IPV4_ADDR_LEN];
		char gateway[NET_IPV4_ADDR_LEN];
		
		// Get IP address
		struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
		if (ipv4) {
			net_addr_ntop(AF_INET, &ipv4->unicast[0].address.in_addr, ip_addr, sizeof(ip_addr));
			net_addr_ntop(AF_INET, &ipv4->netmask, netmask, sizeof(netmask));
			net_addr_ntop(AF_INET, &ipv4->gw, gateway, sizeof(gateway));
			
			// Initialize points with current network settings
			k_mutex_lock(&web_points_lock, K_FOREVER);
			
			point p_addr = {0};
			point_set_type_key(&p_addr, POINT_TYPE_ADDRESS, "0");
			point_put_string(&p_addr, ip_addr);
			points_merge(web_points, ARRAY_SIZE(web_points), &p_addr);
			
			point p_netmask = {0};
			point_set_type_key(&p_netmask, POINT_TYPE_NETMASK, "0");
			point_put_string(&p_netmask, netmask);
			points_merge(web_points, ARRAY_SIZE(web_points), &p_netmask);
			
			point p_gateway = {0};
			point_set_type_key(&p_gateway, POINT_TYPE_GATEWAY, "0");
			point_put_string(&p_gateway, gateway);
			points_merge(web_points, ARRAY_SIZE(web_points), &p_gateway);
			
			k_mutex_unlock(&web_points_lock);
		}
	}

	LOG_INF("siot web thread");
	http_server_start();

	point p;

	const struct zbus_channel *chan;
	while (!zbus_sub_wait_msg(&web_sub, &chan, &p, K_FOREVER)) {
		if (chan == &point_chan) {

			k_mutex_lock(&web_points_lock, K_FOREVER);
			int ret = points_merge(web_points, ARRAY_SIZE(web_points), &p);
			k_mutex_unlock(&web_points_lock);
			if (ret != 0) {
				LOG_ERR("Error storing point in web point cache: %i", ret);
			}
		}
	}
}

K_THREAD_DEFINE(web, STACKSIZE, web_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
