#include <point.h>

#include <zephyr/kernel.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <stdint.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(nvs_store, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);

static struct nvs_fs fs;

// NVS Keys
#define NVS_KEY_BOOT_CNT    1
#define NVS_KEY_DESCRIPTION 2
#define NVS_KEY_STATIC_IP   3
#define NVS_KEY_IP_ADDR     4
#define NVS_KEY_GATEWAY     5
#define NVS_KEY_NETMASK     6

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

int nvs_init()
{
	LOG_DBG("nvs_init");
	struct flash_pages_info info;
	int rc = 0;

	char buf[30];

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

	uint32_t uint32_buf;

	rc = nvs_read(&fs, NVS_KEY_BOOT_CNT, &uint32_buf, sizeof(uint32_buf));
	if (rc > 0) { /* item was found, show it */
		uint32_buf++;
		LOG_INF("Boot count: %d\n", uint32_buf);
		(void)nvs_write(&fs, NVS_KEY_BOOT_CNT, &uint32_buf, sizeof(uint32_buf));
	} else { /* item was not found, add it */
		LOG_INF("No boot counter found, adding it at id %d\n", NVS_KEY_BOOT_CNT);
		uint32_buf = 0;
		(void)nvs_write(&fs, NVS_KEY_BOOT_CNT, &uint32_buf, sizeof(uint32_buf));
	}

	point p;
	point_set_type_key(&p, POINT_TYPE_BOOT_COUNT, "0");
	point_put_int(&p, uint32_buf);
	zbus_chan_pub(&point_chan, &p, K_MSEC(500));

	rc = nvs_read(&fs, NVS_KEY_DESCRIPTION, buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Error reading device id: %i", rc);
		nvs_write(&fs, NVS_KEY_DESCRIPTION, "", sizeof(""));
	}

	point_set_type_key(&p, POINT_TYPE_DESCRIPTION, "0");
	point_put_string(&p, buf);
	zbus_chan_pub(&point_chan, &p, K_FOREVER);

	nvs_read(&fs, NVS_KEY_STATIC_IP, &uint32_buf, sizeof(uint32_buf));
	if (rc < 0) {
		LOG_ERR("Error reading static IP: %i", rc);
		uint32_buf = 0;
		nvs_write(&fs, NVS_KEY_STATIC_IP, &uint32_buf, sizeof(uint32_buf));
	}

	point_set_type_key(&p, POINT_TYPE_STATICIP, "0");
	point_put_int(&p, uint32_buf);
	zbus_chan_pub(&point_chan, &p, K_FOREVER);

	nvs_read(&fs, NVS_KEY_IP_ADDR, buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Error reading IP address: %i", rc);
		nvs_write(&fs, NVS_KEY_IP_ADDR, "", sizeof(""));
	}

	point_set_type_key(&p, POINT_TYPE_ADDRESS, "0");
	point_put_string(&p, buf);
	zbus_chan_pub(&point_chan, &p, K_FOREVER);

	nvs_read(&fs, NVS_KEY_NETMASK, &buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Error reading subnet mask: %i", rc);
		nvs_write(&fs, NVS_KEY_NETMASK, "", sizeof(""));
	}

	point_set_type_key(&p, POINT_TYPE_NETMASK, "0");
	point_put_string(&p, buf);
	zbus_chan_pub(&point_chan, &p, K_FOREVER);

	nvs_read(&fs, NVS_KEY_GATEWAY, &buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Error reading gateway: %i", rc);
		nvs_write(&fs, NVS_KEY_GATEWAY, "", sizeof(""));
	}

	point_set_type_key(&p, POINT_TYPE_GATEWAY, "0");
	point_put_string(&p, buf);
	zbus_chan_pub(&point_chan, &p, K_FOREVER);

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

void nvs_store_handle_point(point *p)
{
	int nvs_id = point_type_key_to_nvs_id(p->type, p->key);

	if (nvs_id < 0) {
		return;
	}

	int len = point_data_len(p);

	if (len <= 0) {
		LOG_DBG("Warning, received point with data len: %i", len);
	}

	LOG_DBG_POINT("Writing point to NVS", p);

	ssize_t cnt = nvs_write(&fs, nvs_id, p->data, len);
	// 0 indicates value is already written and nothing to do
	if (cnt != 0 && (cnt < 0 || cnt != len)) {
		LOG_ERR("Error writing setting: %s, len: %i, written: %zu", p->type, len, cnt);
		return;
	}
}

// we cannot send these points in the store thread or the app will deadlock, so
// do in sys init
SYS_INIT(nvs_init, APPLICATION, 99);

ZBUS_MSG_SUBSCRIBER_DEFINE(state_sub);

void nvs_store_thread(void *arg1, void *arg2, void *arg3)
{
	// we dynamically add the observer after we send the NVS state or else the application
	// will lock up -- assume due to zbus loop deadlock
	int ret = zbus_chan_add_obs(&point_chan, &state_sub, K_SECONDS(5));
	if (ret != 0) {
		LOG_DBG("Error adding observer: %i", ret);
	}

	const struct zbus_channel *chan;
	point p;

	while (!zbus_sub_wait_msg(&state_sub, &chan, &p, K_FOREVER)) {
		if (chan == &point_chan) {
			nvs_store_handle_point(&p);
		}
	}
}

K_THREAD_DEFINE(nvs_store, STACKSIZE, nvs_store_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
