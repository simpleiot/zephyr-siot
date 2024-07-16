
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static uint16_t http_service_port = 80;
HTTP_SERVICE_DEFINE(siot_http_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &http_service_port, 1, 10,
		    NULL);

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

int main(void)
{
	LOG_INF("SIOT Zephyr Application! %s", CONFIG_BOARD_TARGET);
	http_server_start();

	return 0;
}
