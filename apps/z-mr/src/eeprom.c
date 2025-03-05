#include "zephyr/sys/util.h"
#include "zpoint.h"
#include "point.h"

#include "zephyr/kernel.h"
#include <point.h>

#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/eeprom.h>
#include <stdint.h>
#include <string.h>

// EEProm layout
// https://gitea.zonit.com/Zonit-Dev/product/src/branch/master/eeprom-format.md
// IPN (internal part number): 30 bytes
// ID: 30 bytes
// Mfg date: 30 bytes

#define EEPROM_STRING_LEN 30

#define EEPROM_IPN_OFFSET      0
#define EEPROM_VERSION_OFFSET  (EEPROM_IPN_OFFSET + EEPROM_STRING_LEN)
#define EEPROM_ID_OFFSET       (EEPROM_VERSION_OFFSET + sizeof(uint32_t))
#define EEPROM_MFG_DATE_OFFSET (EEPROM_ID_OFFSET + EEPROM_STRING_LEN)

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(eeprom, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);

ZBUS_MSG_SUBSCRIBER_DEFINE(eeprom_sub);

struct eeprom_point {
	const point_def *point_def;
	int offset;
};

const point_def point_def_ipn = {POINT_TYPE_IPN, POINT_DATA_TYPE_STRING};
const point_def point_def_version = {POINT_TYPE_VERSION, POINT_DATA_TYPE_INT};
const point_def point_def_id = {POINT_TYPE_ID, POINT_DATA_TYPE_STRING};
const point_def point_def_mfg_date = {POINT_TYPE_MFG_DATE, POINT_DATA_TYPE_STRING};

// Note, order must not be changed as EEPROM format is fixed.
// Stuff can only be added, not removed.
// Offset must be populated by eeprom_points_init()
struct eeprom_point eeprom_points[] = {
	{&point_def_ipn, 0},
	{&point_def_version, 0},
	{&point_def_id, 0},
	{&point_def_mfg_date, 0},
};

static void eeprom_points_init_offset()
{
	for (int i = 1; i < ARRAY_SIZE(eeprom_points); i++) {
		size_t size = 0;
		switch (eeprom_points[i - 1].point_def->data_type) {
		case POINT_DATA_TYPE_STRING:
			size = EEPROM_STRING_LEN;
			break;
		case POINT_DATA_TYPE_INT:
			size = sizeof(uint32_t);
			break;
		default:
			LOG_ERR("EEPROM invalid data type: %i",
				eeprom_points[i - 1].point_def->data_type);
			k_panic();
		}
		eeprom_points[i].offset = eeprom_points[i - 1].offset + size;
	}
}

void eeprom_error()
{
	LOG_ERR("Critical eeprom error, stopping thread");
	// TODO: add error reporting in this thread
	k_sleep(K_FOREVER);
}

int eeprom_write_string(const struct device *dev, int offset, char *str, size_t max)
{
	int len = strnlen(str, max - 1);
	// make sure null terminated
	str[len] = 0;
	len++;

	return eeprom_write(dev, offset, str, len);
}

int eeprom_read_string(const struct device *dev, int offset, char *str, size_t size)
{
	int rc = eeprom_read(dev, offset, str, size);

	if (rc != 0) {
		return rc;
	}

	// check for null string termination
	for (int i = 0; i < size; i++) {
		if (str[i] == 0) {
			return 0;
		}
	}

	// did not find a null termination, put one in
	str[size - 1] = 0;

	return 0;
}

int eeprom_write_uint32(const struct device *dev, int offset, uint32_t v)
{
	return eeprom_write(dev, offset, &v, sizeof(v));
}

int eeprom_read_uint32(const struct device *dev, int offset, uint32_t *v)
{
	return eeprom_read(dev, offset, v, sizeof(*v));
}

void eeprom_send_points(const struct device *dev)
{
	int ret;
	point p;
	for (int i = 0; i < ARRAY_SIZE(eeprom_points); i++) {
		point_set_type_key(&p, eeprom_points[i].point_def->type, "0");
		p.data_type = eeprom_points[i].point_def->data_type;
		switch (eeprom_points[i].point_def->data_type) {
		case POINT_DATA_TYPE_STRING:
			ret = eeprom_read_string(dev, eeprom_points[i].offset, p.data,
						 sizeof(p.data));
			if (ret != 0) {
				LOG_ERR("Error reading EEPROM %s: %i",
					eeprom_points[i].point_def->type, ret);
				continue;
			}
			zbus_chan_pub(&point_chan, &p, K_MSEC(500));
			break;
		case POINT_DATA_TYPE_INT:
			ret = eeprom_read_uint32(dev, eeprom_points[i].offset, (uint32_t *)p.data);
			if (ret != 0) {
				LOG_ERR("Error reading EEPROM %s: %i",
					eeprom_points[i].point_def->type, ret);
			}
			zbus_chan_pub(&point_chan, &p, K_MSEC(500));
			break;
		default:
			LOG_ERR("unknown eeprom data type %s: %i", eeprom_points[i].point_def->type,
				eeprom_points[i].point_def->data_type);
		}
	}
}

void eeprom_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(m24512));

	if (!device_is_ready(dev)) {
		LOG_ERR("EEPROM device not ready\n");
		eeprom_error();
	}

	int size = eeprom_get_size(dev);
	LOG_DBG("EEPROM size: %i", size);

	const struct zbus_channel *chan;
	point p;

	eeprom_points_init_offset();
	eeprom_send_points(dev);

	// we dynamically add the observer after we send the eeprom points
	// this allows the above points to be sent before we start storing them
	int ret = zbus_chan_add_obs(&point_chan, &eeprom_sub, K_SECONDS(5));
	if (ret != 0) {
		LOG_DBG("Error adding observer: %i", ret);
	}

	size_t max_eeprom_write = MIN(sizeof(p.data), EEPROM_STRING_LEN);

	while (!zbus_sub_wait_msg(&eeprom_sub, &chan, &p, K_FOREVER)) {
		if (chan == &point_chan) {
			if (strcmp(p.type, POINT_TYPE_IPN) == 0) {
				int rc = eeprom_write_string(dev, EEPROM_IPN_OFFSET, p.data,
							     max_eeprom_write);

				if (rc != 0) {
					LOG_ERR("Error writing IPN to eeprom: %i", rc);
				}
			} else if (strcmp(p.type, POINT_TYPE_ID) == 0) {
				int rc = eeprom_write_string(dev, EEPROM_IPN_OFFSET, p.data,
							     max_eeprom_write);

				if (rc != 0) {
					LOG_ERR("Error writing id to eeprom: %i", rc);
				}
			} else if (strcmp(p.type, POINT_TYPE_MFG_DATE) == 0) {
				int rc = eeprom_write_string(dev, EEPROM_MFG_DATE_OFFSET, p.data,
							     max_eeprom_write);

				if (rc != 0) {
					LOG_ERR("Error writing mfg date to eeprom: %i", rc);
				}
			} else if (strcmp(p.type, POINT_TYPE_VERSION) == 0) {
				int rc = eeprom_write_uint32(dev, EEPROM_VERSION_OFFSET,
							     point_get_int(&p));

				if (rc != 0) {
					LOG_ERR("Error writing mfg date to eeprom: %i", rc);
				}
			}
		}
	}
}

K_THREAD_DEFINE(eeprom, STACKSIZE, eeprom_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
