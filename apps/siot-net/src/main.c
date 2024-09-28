
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
#define NVS_KEY_BOOT_CNT  1
#define NVS_KEY_DEVICE_ID 2
#define NVS_KEY_STATIC_IP 3

static struct nvs_fs fs;

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

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

	char did[32];

	rc = nvs_read(&fs, NVS_KEY_DEVICE_ID, &did, sizeof(did));
	if (rc > 0) {
		LOG_INF("Device ID: %s", did);
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
			     uint8_t *buffer, size_t len, void *user_data)
{
	char *end = recv_buffer;
	static bool processed = false;
	int rc = 0;
	uint32_t reboot_counter = 0U;

	rc = nvs_read(&fs, NVS_KEY_BOOT_CNT, &reboot_counter, sizeof(reboot_counter));
	if (rc > 0) { /* item was found, show it */
		sprintf(recv_buffer, "%i", reboot_counter);
	} else {
		strcpy(recv_buffer, "error");
	}

	if (processed) {
		processed = false;
		return 0;
	}

	processed = true;
	return strlen(recv_buffer);
}

struct http_resource_detail_dynamic bootcount_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = bootcount_handler,
	.data_buffer = recv_buffer,
	.data_buffer_len = sizeof(recv_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(bootcount_resource, siot_http_service, "/bootcount",
		     &bootcount_resource_detail);

// ********************************
// CPU usage handler

static int cpu_usage_handler(struct http_client_ctx *client, enum http_data_status status,
			     uint8_t *buffer, size_t len, void *user_data)
{
	char *end = recv_buffer;
	static bool processed = false;
	int rc = 0;

	k_thread_runtime_stats_t stats;
	rc = k_thread_runtime_stats_all_get(&stats);

	if (rc == 0) { /* item was found, show it */
		sprintf(recv_buffer, "%0.2lf%%",
			((double)stats.total_cycles) * 100 / stats.execution_cycles);
	} else {
		strcpy(recv_buffer, "error");
	}

	if (processed) {
		processed = false;
		return 0;
	}

	processed = true;
	return strlen(recv_buffer);
}

struct http_resource_detail_dynamic cpu_usage_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = cpu_usage_handler,
	.data_buffer = recv_buffer,
	.data_buffer_len = sizeof(recv_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(cpu_usage_resource, siot_http_service, "/cpu-usage",
		     &cpu_usage_resource_detail);

// ********************************
// Board handler

static int board_handler(struct http_client_ctx *client, enum http_data_status status,
			 uint8_t *buffer, size_t len, void *user_data)
{
	char *end = recv_buffer;
	static bool processed = false;

	sprintf(recv_buffer, "%s", CONFIG_BOARD_TARGET);

	if (processed) {
		processed = false;
		return 0;
	}

	processed = true;
	return strlen(recv_buffer);
}

struct http_resource_detail_dynamic board_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = board_handler,
	.data_buffer = recv_buffer,
	.data_buffer_len = sizeof(recv_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(board_resource, siot_http_service, "/board", &board_resource_detail);

// ********************************
// DID handler

static int did_handler(struct http_client_ctx *client, enum http_data_status status,
		       uint8_t *buffer, size_t len, void *user_data)
{
	char *end = recv_buffer;
	static bool processed = false;

	int rc = nvs_read(&fs, NVS_KEY_DEVICE_ID, &recv_buffer, sizeof(recv_buffer));
	if (rc > 0) {
		recv_buffer[rc] = 0;
	} else {
		recv_buffer[0] = 0;
	}

	if (processed) {
		processed = false;
		return 0;
	}

	processed = true;
	return strlen(recv_buffer);
}

struct http_resource_detail_dynamic did_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = did_handler,
	.data_buffer = recv_buffer,
	.data_buffer_len = sizeof(recv_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(did_resource, siot_http_service, "/did", &did_resource_detail);

// ********************************
// settings post handler

void settings_callback(char *key, char *value)
{
	LOG_DBG("setting: %s:%s", key, value);

	uint16_t nvs_id;

	if (strcmp(key, "did") == 0) {
		nvs_id = NVS_KEY_DEVICE_ID;
	} else if (strcmp(key, "ipstatic") == 0) {
		nvs_id = NVS_KEY_STATIC_IP;
	} else {
		LOG_ERR("Unhandled setting: %s", key);
		return;
	}

	ssize_t cnt = nvs_write(&fs, nvs_id, value, strlen(value) + 1);
	// 0 indicates value is already written and nothing to do
	if (cnt != 0 && (cnt < 0 || cnt != strlen(value) + 1)) {
		LOG_ERR("Error writing setting: %s, len: %lu, written: %lu", key, strlen(value) + 1,
			cnt);

		return;
	}

	char buf[30];
	cnt = nvs_read(&fs, nvs_id, buf, sizeof(buf));
}

static int settings_post_handler(struct http_client_ctx *client, enum http_data_status status,
				 uint8_t *buffer, size_t len, void *user_data)
{
	if (status == HTTP_SERVER_DATA_ABORTED) {
		return 0;
	}

	// make sure data is null terminated
	buffer[len] = 0;

	html_parse_form_data(buffer, settings_callback);

	// LOG_HEXDUMP_DBG(buffer, len, "settings data");

	return 0;
}

static struct http_resource_detail_dynamic settings_post_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_POST),
		},
	.cb = settings_post_handler,
	.data_buffer = recv_buffer,
	.data_buffer_len = sizeof(recv_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(settings_post_resource, siot_http_service, "/settings",
		     &settings_post_resource_detail);

// ********************************
// main

int main(void)
{
	int rc = 0;

	LOG_INF("SIOT Zephyr Application! %s", CONFIG_BOARD_TARGET);
	nvs_init();

	http_server_start();

	return 0;
}
