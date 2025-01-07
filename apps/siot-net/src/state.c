#include "state.h"
#include "point.h"

#include <zephyr/kernel.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/zbus/zbus.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(siot_state, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(siot_point_chan);

// ********************************
// NVS Config storage

// NVS Keys
#define NVS_KEY_BOOT_CNT    1
#define NVS_KEY_DESCRIPTION 2
#define NVS_KEY_STATIC_IP   3
#define NVS_KEY_IP_ADDR     4
#define NVS_KEY_GATEWAY     5
#define NVS_KEY_NETMASK     6

static struct nvs_fs fs;

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

// store saved settings in points here
static point state_points[10];

void nvs_dump_settings(point *points, size_t len)
{
	// TODO: extract settings from state
	// LOG_INF("Device ID: %s", config->device_id);

	// LOG_INF("Static IP enabled: %s", config->static_ip ? "yes" : "no");

	// LOG_INF("IP Address: %s", config->ip_addr);

	// LOG_INF("Subnet mask: %s", config->subnet_mask);

	// LOG_INF("Gateway: %s", config->gateway);
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

	// rc = nvs_read(&fs, NVS_KEY_BOOT_CNT, &config.bootcount, sizeof(config.bootcount));
	// if (rc > 0) { /* item was found, show it */
	// 	LOG_INF("Boot count: %d\n", config.bootcount);
	// 	config.bootcount++;
	// 	(void)nvs_write(&fs, NVS_KEY_BOOT_CNT, &config.bootcount, sizeof(config.bootcount));
	// } else { /* item was not found, add it */
	// 	LOG_INF("No boot counter found, adding it at id %d\n", NVS_KEY_BOOT_CNT);
	// 	(void)nvs_write(&fs, NVS_KEY_BOOT_CNT, &config.bootcount, sizeof(config.bootcount));
	// }

	// rc = nvs_read(&fs, NVS_KEY_DESCRIPTION, &config.device_id, sizeof(config.device_id));
	// if (rc < 0) {
	// 	LOG_ERR("Error reading device id: %i", rc);
	// 	nvs_write(&fs, NVS_KEY_DESCRIPTION, "", sizeof(""));
	// }

	// nvs_read(&fs, NVS_KEY_STATIC_IP, &buf, sizeof(buf));
	// if (rc > 0) {
	// 	if (strcmp("true", buf)) {
	// 		config.static_ip = true;
	// 	} else {
	// 		config.static_ip = false;
	// 	}
	// }

	// nvs_read(&fs, NVS_KEY_IP_ADDR, &config.ip_addr, sizeof(config.ip_addr));
	// if (rc < 0) {
	// 	LOG_ERR("Error reading IP address: %i", rc);
	// 	nvs_write(&fs, NVS_KEY_IP_ADDR, "", sizeof(""));
	// }

	// nvs_read(&fs, NVS_KEY_NETMASK, &config.subnet_mask, sizeof(config.subnet_mask));
	// if (rc < 0) {
	// 	LOG_ERR("Error reading subnet mask: %i", rc);
	// 	nvs_write(&fs, NVS_KEY_NETMASK, "", sizeof(""));
	// }

	// nvs_read(&fs, NVS_KEY_GATEWAY, &config.gateway, sizeof(config.gateway));
	// if (rc < 0) {
	// 	LOG_ERR("Error reading gateway: %i", rc);
	// 	nvs_write(&fs, NVS_KEY_GATEWAY, "", sizeof(""));
	// }

	// nvs_dump_settings(&config);

	// zbus_chan_pub(&z_config_chan, &config, K_MSEC(500));

	return 0;
}

int point_type_key_to_nvs_id(char *type, char *key)
{
	if (strcmp(type, POINT_TYPE_DESCRIPTION) == 0) {
		return NVS_KEY_DESCRIPTION;
	} else if (strcmp(type, POINT_TYPE_STATICIP) == 0) {
		return NVS_KEY_STATIC_IP;
	} else if (strcmp(type, POINT_TYPE_ADDRESS) == 0) {
		return NVS_KEY_IP_ADDR;
	} else if (strcmp(type, POINT_TYPE_NETMASK) == 0) {
		return NVS_KEY_NETMASK;
	} else if (strcmp(type, POINT_TYPE_GATEWAY) == 0) {
		return NVS_KEY_GATEWAY;
	} else {
		return -1;
	}
}

void z_config_handle_point(point *p)
{

	int nvs_id = point_type_key_to_nvs_id(p->type, p->key);

	if (nvs_id < 0) {
		LOG_ERR("Unhandled setting: %s:%s", p->type, p->key);
		return;
	}

	int len = point_data_len(p);

	if (len <= 0) {
		LOG_DBG("Warning, received point with data len: %i", len);
	}

	ssize_t cnt = nvs_write(&fs, nvs_id, p->data, len);
	// 0 indicates value is already written and nothing to do
	if (cnt != 0 && (cnt < 0 || cnt != len)) {
		LOG_ERR("Error writing setting: %s, len: %i, written: %zu", p->type, len, cnt);
		return;
	}
}

ZBUS_MSG_SUBSCRIBER_DEFINE(z_config_sub);

// For some reason the following does not work, so added a ZBUS_OBS_DECLARE in main.c
// ZBUS_CHAN_ADD_OBS(z_config_chan, z_config_sub, 3);

void z_config_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("z config thread");
	nvs_init();

	const struct zbus_channel *chan;
	point p;

	while (!zbus_sub_wait_msg(&z_config_sub, &chan, &p, K_FOREVER)) {
		if (chan == &siot_point_chan) {
			char desc[40];
			point_description(&p, desc, sizeof(desc));
			LOG_DBG("CLIFF: config read point: %s", desc);
			z_config_handle_point(&p);
		}
	}
}

K_THREAD_DEFINE(z_config, STACKSIZE, z_config_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
