
#include "zephyr/arch/xtensa/thread.h"
#include "zephyr/kernel.h"
#include "zephyr/kernel/thread.h"
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

// ********************************
// NVS Config storage

// NVS Keys
#define NVS_KEY_BOOT_CNT 1

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

	rc = nvs_read(&fs, NVS_KEY_BOOT_CNT, &reboot_counter, sizeof(reboot_counter));
	if (rc > 0) { /* item was found, show it */
		LOG_INF("Id: %d, boot_counter: %d\n", NVS_KEY_BOOT_CNT, reboot_counter);
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
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
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
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
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
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
		},
	.cb = board_handler,
	.data_buffer = recv_buffer,
	.data_buffer_len = sizeof(recv_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(board_resource, siot_http_service, "/board", &board_resource_detail);

// ********************************
// post handler

static uint8_t post_buf[256];

static int post_handler(struct http_client_ctx *client, enum http_data_status status,
			uint8_t *buffer, size_t len, void *user_data)
{
	static uint8_t post_payload_buf[32];
	static size_t cursor;

	LOG_DBG("LED handler status %d, size %zu", status, len);

	if (status == HTTP_SERVER_DATA_ABORTED) {
		cursor = 0;
		return 0;
	}

	if (len + cursor > sizeof(post_payload_buf)) {
		cursor = 0;
		return -ENOMEM;
	}

	/* Copy payload to our buffer. Note that even for a small payload, it may arrive split into
	 * chunks (e.g. if the header size was such that the whole HTTP request exceeds the size of
	 * the client buffer).
	 */
	memcpy(post_payload_buf + cursor, buffer, len);
	cursor += len;

	if (status == HTTP_SERVER_DATA_FINAL) {
		LOG_DBG("CLIFF: Post payload: %s", post_payload_buf);
		// parse_led_post(post_payload_buf, cursor);
		cursor = 0;
	}

	return 0;
}

static struct http_resource_detail_dynamic post_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_POST),
		},
	.cb = post_handler,
	.data_buffer = post_buf,
	.data_buffer_len = sizeof(post_buf),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(post_resource, siot_http_service, "/", &post_resource_detail);

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
