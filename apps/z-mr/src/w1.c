#include <point.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(z_w1, LOG_LEVEL_INF);

#define STACKSIZE 1024
#define PRIORITY  7

ZBUS_CHAN_DECLARE(point_chan);

/*
 * Get a device structure from a devicetree node with compatible
 * "maxim,ds18b20". (If there are multiple, just pick one.)
 */
static const struct device *get_ds18b20_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(maxim_ds18b20);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;
}

void z_w1_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = get_ds18b20_device();
	int res;

	if (dev == NULL) {
		LOG_ERR("Cannot get ds18b20 device");
		return;
	}

	while (true) {
		struct sensor_value temp;

		res = sensor_sample_fetch(dev);
		if (res != 0) {
			LOG_ERR("sample_fetch() failed: %d", res);
		}

		res = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		if (res != 0) {
			LOG_ERR("channel_get() failed: %d", res);
		}

		float v = sensor_value_to_float(&temp);
		point p;

		point_set_type_key(&p, POINT_TYPE_TEMPERATURE, "0");
		point_put_float(&p, v);

		zbus_chan_pub(&point_chan, &p, K_MSEC(500));

		LOG_DBG("Temp: %.3f", (double)v);
		k_sleep(K_MSEC(2000));
	
	}	
}

K_THREAD_DEFINE(z_w1, STACKSIZE, z_w1_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
