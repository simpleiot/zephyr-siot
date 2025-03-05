#include <point.h>
#include <nvs.h>

#include <sys/cdefs.h>

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

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

static const struct nvs_point *nvs_pts;
static size_t nvs_pts_count = 0;

// returns -1 if not found
int point_type_key_to_nvs_id(const struct nvs_point *pts, size_t len, char *type, char *key)
{
	for (int i = 0; i < len; i++) {
		if (strcmp(type, pts[i].point_def->type) == 0 && strcmp(key, pts[i].key) == 0) {
			return pts[i].nvs_id;
		}
	}
	return -1;
}

// this needs to be called early on from your application
int nvs_init(const struct nvs_point *nvs_pts_in, size_t len)
{
	LOG_DBG("nvs_init");

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

	point p;
	uint32_t uint32_buf;
	float float_buf;
	char string_buf[sizeof(p.data)];

	// read persisted points from NVS and broadcast
	for (int i = 0; i < len; i++) {
		const struct nvs_point *npt = &nvs_pts_in[i];
		switch (npt->point_def->data_type) {
		case POINT_DATA_TYPE_FLOAT:
			float_buf = 0;
			rc = nvs_read(&fs, npt->nvs_id, &float_buf, sizeof(float_buf));
			if (rc < 0) {
				LOG_ERR("Error reading %s: %i, setting zero value",
					npt->point_def->type, rc);
				nvs_write(&fs, npt->nvs_id, 0, sizeof(float_buf));
			}
			point_put_float(&p, float_buf);
			break;

		case POINT_DATA_TYPE_INT:
			uint32_buf = 0;
			rc = nvs_read(&fs, npt->nvs_id, &uint32_buf, sizeof(uint32_buf));
			if (rc < 0) {
				LOG_ERR("Error reading %s: %i, setting zero value",
					npt->point_def->type, rc);
				nvs_write(&fs, npt->nvs_id, 0, sizeof(uint32_buf));
			}
			point_put_int(&p, uint32_buf);

			if (strcmp(POINT_TYPE_BOOT_COUNT, npt->point_def->type) == 0) {
				LOG_DBG("Boot count: %i", uint32_buf);
				uint32_buf++;
				nvs_write(&fs, npt->nvs_id, &uint32_buf, sizeof(uint32_buf));
			}
			break;

		case POINT_DATA_TYPE_STRING:
			string_buf[0] = 0;
			rc = nvs_read(&fs, npt->nvs_id, &string_buf, sizeof(string_buf));
			if (rc < 0) {
				LOG_ERR("Error reading %s: %i", npt->point_def->type, rc);
				nvs_write(&fs, npt->nvs_id, "", sizeof(""));
			}
			point_put_string(&p, string_buf);
			break;

		default:
			LOG_ERR("Unknown point data type %s: %i", npt->point_def->type,
				npt->point_def->data_type);
			continue;
		}

		point_set_type_key(&p, npt->point_def->type, npt->key);
		zbus_chan_pub(&point_chan, &p, K_MSEC(500));
	}

	// We set this late in the fuction because the main loop does not process
	// NVS points until this is set. This allows us to broadcast saved points
	// before we start saving new ones. Otherwise we would save points we just
	// broadcasted.
	nvs_pts = nvs_pts_in;
	nvs_pts_count = len;

	return 0;
}

void nvs_store_handle_point(point *p)
{
	int nvs_id = point_type_key_to_nvs_id(nvs_pts, nvs_pts_count, p->type, p->key);

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

ZBUS_MSG_SUBSCRIBER_DEFINE(state_sub);

void nvs_store_thread(void *arg1, void *arg2, void *arg3)
{
	// we dynamically add the observer after we send the NVS state
	// this allows the points to be sent before we start storing them
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
