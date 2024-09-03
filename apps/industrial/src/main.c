
#include "zephyr/device.h"
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

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

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static uint16_t http_service_port = 80;
HTTP_SERVICE_DEFINE(siot_http_service, "0.0.0.0", &http_service_port, 1, 10, NULL);

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

int toggle_pin(int pin)
{
	const struct device *gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0_dev)) {
		LOG_ERR("gpio0 not ready");
	}

	int ret = gpio_pin_configure(gpio0_dev, pin, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error %d: failed to configure GPIO\n", ret);
		return 0;
	}

	LOG_DBG("Toggle pin %d", pin);
	while (true) {
		// Set the pin high
		gpio_pin_toggle(gpio0_dev, pin);
		k_msleep(500);
	}
}

int main(void)
{
	int rc = 0;

	LOG_INF("Zonit Industrial Monitoring Application! %s", CONFIG_BOARD_TARGET);

	nvs_init();

	http_server_start();

	// toggle_pin(0);
}
