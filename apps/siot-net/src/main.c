
#include "zephyr/kernel.h"
#include "zephyr/kernel/thread.h"
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <sys/types.h>

#include "html.h"

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

// ********************************
// NVS Config storage

// NVS Keys
#define NVS_KEY_BOOT_CNT    1
#define NVS_KEY_DEVICE_ID   2
#define NVS_KEY_STATIC_IP   3
#define NVS_KEY_IP_ADDR     4
#define NVS_KEY_GATEWAY     5
#define NVS_KEY_SUBNET_MASK 6

static struct nvs_fs fs;

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

void nvs_dump_settings()
{
	char buf[32];

	int rc = nvs_read(&fs, NVS_KEY_DEVICE_ID, &buf, sizeof(buf));
	if (rc > 0) {
		LOG_INF("Device ID: %s", buf);
	}

	nvs_read(&fs, NVS_KEY_STATIC_IP, &buf, sizeof(buf));
	if (rc > 0) {
		LOG_INF("Static IP: %s", buf);
	}

	nvs_read(&fs, NVS_KEY_IP_ADDR, &buf, sizeof(buf));
	if (rc > 0) {
		LOG_INF("IP Address: %s", buf);
	}

	nvs_read(&fs, NVS_KEY_SUBNET_MASK, &buf, sizeof(buf));
	if (rc > 0) {
		LOG_INF("Subnet mask: %s", buf);
	}

	nvs_read(&fs, NVS_KEY_GATEWAY, &buf, sizeof(buf));
	if (rc > 0) {
		LOG_INF("Gateway: %s", buf);
	}
}
int nvs_init()
{
	struct flash_pages_info info;
	int rc = 0;
	uint32_t reboot_counter = 0U;

	/* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	3 sectors
	 *	starting at NVS_PARTITION_OFFSET
	 */
	fs.flash_device = NVS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
		LOG_ERR("Flash device %s is not ready\n", fs.flash_device->name);
		return -1;
	}
	fs.offset = NVS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		LOG_ERR("Unable to get page info\n");
		return -1;
	}
	fs.sector_size = info.size;
	fs.sector_count = 3U;

	rc = nvs_mount(&fs);
	if (rc) {
		LOG_ERR("Flash Init failed\n");
		return -1;
	}

	rc = nvs_read(&fs, NVS_KEY_BOOT_CNT, &reboot_counter, sizeof(reboot_counter));
	if (rc > 0) { /* item was found, show it */
		LOG_INF("Boot count: %d\n", reboot_counter);
		reboot_counter++;
		(void)nvs_write(&fs, NVS_KEY_BOOT_CNT, &reboot_counter, sizeof(reboot_counter));
	} else { /* item was not found, add it */
		LOG_INF("No boot counter found, adding it at id %d\n", NVS_KEY_BOOT_CNT);
		(void)nvs_write(&fs, NVS_KEY_BOOT_CNT, &reboot_counter, sizeof(reboot_counter));
	}

	nvs_dump_settings();

	return 0;
}

// ********************************
// HTTP service

static uint8_t recv_buffer[1024];

static uint16_t http_service_port = 80;
HTTP_SERVICE_DEFINE(siot_http_service, "0.0.0.0", &http_service_port, 1, 10, NULL);

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

// ********************************
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
	uint32_t reboot_counter = 0U;

	if (status != HTTP_SERVER_DATA_FINAL) {
		return 0;
	}

	int rc = nvs_read(&fs, NVS_KEY_BOOT_CNT, &reboot_counter, sizeof(reboot_counter));
	if (rc > 0) { /* item was found, show it */
		sprintf(recv_buffer, "%i", reboot_counter);
	} else {
		strcpy(recv_buffer, "error");
	}

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
// CPU usage handler

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

	int rc = nvs_read(&fs, NVS_KEY_DEVICE_ID, &recv_buffer, sizeof(recv_buffer));
	if (rc > 0) {
		recv_buffer[rc] = 0;
	} else {
		recv_buffer[0] = 0;
	}

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

	int rc = nvs_read(&fs, NVS_KEY_STATIC_IP, &recv_buffer, sizeof(recv_buffer));
	if (rc > 0) {
		recv_buffer[rc] = 0;
	} else {
		recv_buffer[0] = 0;
	}

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

	int rc = nvs_read(&fs, NVS_KEY_IP_ADDR, &recv_buffer, sizeof(recv_buffer));
	if (rc > 0) {
		recv_buffer[rc] = 0;
	} else {
		recv_buffer[0] = 0;
	}

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

	int rc = nvs_read(&fs, NVS_KEY_SUBNET_MASK, &recv_buffer, sizeof(recv_buffer));
	if (rc > 0) {
		recv_buffer[rc] = 0;
	} else {
		recv_buffer[0] = 0;
	}

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

	int rc = nvs_read(&fs, NVS_KEY_GATEWAY, &recv_buffer, sizeof(recv_buffer));
	if (rc > 0) {
		recv_buffer[rc] = 0;
	} else {
		recv_buffer[0] = 0;
	}

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
	// LOG_DBG("setting: %s:%s", key, value);

	uint16_t nvs_id;

	if (strcmp(key, "did") == 0) {
		nvs_id = NVS_KEY_DEVICE_ID;
	} else if (strcmp(key, "ipstatic") == 0) {
		nvs_id = NVS_KEY_STATIC_IP;
	} else if (strcmp(key, "ipaddr") == 0) {
		nvs_id = NVS_KEY_IP_ADDR;
	} else if (strcmp(key, "subnet-mask") == 0) {
		nvs_id = NVS_KEY_SUBNET_MASK;
	} else if (strcmp(key, "gateway") == 0) {
		nvs_id = NVS_KEY_GATEWAY;
	} else {
		LOG_ERR("Unhandled setting: %s", key);
		return;
	}

	ssize_t cnt = nvs_write(&fs, nvs_id, value, strlen(value) + 1);
	// 0 indicates value is already written and nothing to do
	if (cnt != 0 && (cnt < 0 || cnt != strlen(value) + 1)) {
		LOG_ERR("Error writing setting: %s, len: %zu, written: %zu", key, strlen(value) + 1,
			cnt);

		return;
	}
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

		nvs_dump_settings();

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

// ********************************
// main

int main(void)
{
	LOG_INF("SIOT Zephyr Application! %s", CONFIG_BOARD_TARGET);
	nvs_init();

	http_server_start();

	return 0;
}
