#include "config.h"
#include "zephyr/kernel.h"

#include <zephyr/fs/nvs.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(z_config, LOG_LEVEL_DBG);

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

void z_mr_config_init(z_mr_config *c)
{
	c->bootcount = 0;
	c->static_ip = false;
	c->device_id[0] = 0;
	c->ip_addr[0] = 0;
	c->subnet_mask[0] = 0;
	c->gateway[0] = 0;
}

void z_config_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("z config thread");
	// nvs_init();
	while (1) {
		k_msleep(1000);
	}
}

K_THREAD_DEFINE(z_config, STACKSIZE, z_config_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
