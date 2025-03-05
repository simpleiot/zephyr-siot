#include "zpoint.h"

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

#define EEPROM_FIELD_SIZE      30
#define EEPROM_IPN_OFFSET      0
#define EEPROM_VERSION_OFFSET  30
#define EEPROM_ID_OFFSET       34
#define EEPROM_MFG_DATE_OFFSET 64

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(eeprom, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);

ZBUS_MSG_SUBSCRIBER_DEFINE(eeprom_sub);

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

	size_t max_eeprom_write = MIN(sizeof(p.data), EEPROM_FIELD_SIZE);

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
