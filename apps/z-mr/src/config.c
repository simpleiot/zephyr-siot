#include "config.h"
#include "zephyr/kernel.h"

#include <zephyr/fs/nvs.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/zbus/zbus.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(z_config, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(z_config_chan);

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

static z_mr_config config = Z_MR_CONFIG_INIT;

void nvs_dump_settings(z_mr_config *config)
{
	LOG_INF("Device ID: %s", config->device_id);

	LOG_INF("Static IP enabled: %s", config->static_ip ? "yes" : "no");

	LOG_INF("IP Address: %s", config->ip_addr);

	LOG_INF("Subnet mask: %s", config->subnet_mask);

	LOG_INF("Gateway: %s", config->gateway);
}

static char buf[30];

int nvs_init()
{
	struct flash_pages_info info;
	int rc = 0;

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

	rc = nvs_read(&fs, NVS_KEY_BOOT_CNT, &config.bootcount, sizeof(config.bootcount));
	if (rc > 0) { /* item was found, show it */
		LOG_INF("Boot count: %d\n", config.bootcount);
		config.bootcount++;
		(void)nvs_write(&fs, NVS_KEY_BOOT_CNT, &config.bootcount, sizeof(config.bootcount));
	} else { /* item was not found, add it */
		LOG_INF("No boot counter found, adding it at id %d\n", NVS_KEY_BOOT_CNT);
		(void)nvs_write(&fs, NVS_KEY_BOOT_CNT, &config.bootcount, sizeof(config.bootcount));
	}

	rc = nvs_read(&fs, NVS_KEY_DEVICE_ID, &config.device_id, sizeof(config.device_id));
	if (rc < 0) {
		LOG_ERR("Error reading device id: %i", rc);
		nvs_write(&fs, NVS_KEY_DEVICE_ID, "", sizeof(""));
	}

	nvs_read(&fs, NVS_KEY_STATIC_IP, &buf, sizeof(buf));
	if (rc > 0) {
		if (strcmp("true", buf)) {
			config.static_ip = true;
		} else {
			config.static_ip = false;
		}
	}

	nvs_read(&fs, NVS_KEY_IP_ADDR, &config.ip_addr, sizeof(config.ip_addr));
	if (rc < 0) {
		LOG_ERR("Error reading IP address: %i", rc);
		nvs_write(&fs, NVS_KEY_IP_ADDR, "", sizeof(""));
	}

	nvs_read(&fs, NVS_KEY_SUBNET_MASK, &config.subnet_mask, sizeof(config.subnet_mask));
	if (rc < 0) {
		LOG_ERR("Error reading subnet mask: %i", rc);
		nvs_write(&fs, NVS_KEY_SUBNET_MASK, "", sizeof(""));
	}

	nvs_read(&fs, NVS_KEY_GATEWAY, &config.gateway, sizeof(config.gateway));
	if (rc < 0) {
		LOG_ERR("Error reading gateway: %i", rc);
		nvs_write(&fs, NVS_KEY_GATEWAY, "", sizeof(""));
	}

	nvs_dump_settings(&config);

	zbus_chan_pub(&z_config_chan, &config, K_MSEC(500));

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
	nvs_init();

	while (1) {
		k_msleep(1000);
	}
}

K_THREAD_DEFINE(z_config, STACKSIZE, z_config_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
