#include "ats.h"
#include "zephyr/sys/util.h"
#include "zpoint.h"
#include <point.h>
#include <siot-string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/zbus/zbus.h>

#define STACKSIZE 1024
#define PRIORITY  7

#define LED_STATUS (6 + 8)
#define LED_BLE    (6)

LOG_MODULE_REGISTER(z_led, LOG_LEVEL_INF);

ZBUS_CHAN_DECLARE(point_chan);
ZBUS_CHAN_DECLARE(ticker_chan);

#define I2C0_NODE DT_NODELABEL(mcp23018_20)
static const struct i2c_dt_spec gpioExpander Z_GENERIC_SECTION(.rodata) = I2C_DT_SPEC_GET(I2C0_NODE);

const static struct device *i2c_dev Z_GENERIC_SECTION(.rodata) = DEVICE_DT_GET(DT_NODELABEL(i2c0));

void z_led_setup(const struct device *mcp_device)
{
	for (int i = 0; i < 6; i++) {
		int ret = gpio_pin_configure(mcp_device, i,
					     GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_ACTIVE_LOW);
		if (ret < 0) {
			LOG_ERR("Failed to configure GPIO pin %i: %d", i, ret);
		}

		ret = gpio_pin_set(mcp_device, i, 0);
		if (ret != 0) {
			LOG_ERR("ERROR writing to LED GPIO: %i", ret);
		}

		ret = gpio_pin_configure(mcp_device, i + 8,
					 GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_ACTIVE_LOW);
		if (ret < 0) {
			LOG_ERR("Failed to configure GPIO pin %i: %d", i + 8, ret);
		}

		ret = gpio_pin_set(mcp_device, i + 8, 0);
		if (ret != 0) {
			LOG_ERR("ERROR writing to LED GPIO: %i", ret);
		}
	}

	int ret = gpio_pin_configure(mcp_device, LED_STATUS,
				     GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_ACTIVE_LOW);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO pin %i: %d", LED_STATUS, ret);
	}

	ret = gpio_pin_configure(mcp_device, LED_BLE,
				 GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_ACTIVE_LOW);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO pin %i: %d", LED_BLE, ret);
	}

	gpio_pin_set(mcp_device, LED_STATUS, 0);
	gpio_pin_set(mcp_device, LED_BLE, 0);
}

// LED num starts at 0
void z_led_set(const struct device *mcp_device, bool is_b, int num, bool on)
{
	int i = num;
	if (i < 0 || i > 5) {
		LOG_ERR("Invalid LED number %i, only 6 channels", num);
		return;
	}

	if (is_b) {
		i += 8;
	}

	int ret = gpio_pin_set(mcp_device, i, on);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO pin %i: %d", i + 8, ret);
	}
}

void z_leds_test_pattern(const struct device *mcp_device)
{
	static int cur = 0;
	static int toggle = 0;

	// display pattern on LEDs
	gpio_pin_set(mcp_device, LED_STATUS, toggle);
	toggle = !toggle;
	gpio_pin_set(mcp_device, LED_BLE, toggle);

	// turn current led off
	z_led_set(mcp_device, false, cur, false);
	z_led_set(mcp_device, true, cur, false);

	// gpio_pin_set(mcp_device, cur, 0);
	// gpio_pin_set(mcp_device, cur + 8, 0);

	cur += 1;
	if (cur > 5) {
		cur = 0;
	}
	// turn next LED on
	z_led_set(mcp_device, false, cur, true);
	z_led_set(mcp_device, true, cur, true);
}

static ats_chan_state ats_chan_state_a[6] Z_GENERIC_SECTION(.rodata);
static ats_chan_state ats_chan_state_b[6] Z_GENERIC_SECTION(.rodata);

ZBUS_MSG_SUBSCRIBER_DEFINE(z_led_sub);
ZBUS_CHAN_ADD_OBS(point_chan, z_led_sub, 3);
ZBUS_CHAN_ADD_OBS(ticker_chan, z_led_sub, 4);

void z_leds_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("z leds thread");

	if (!device_is_ready(i2c_dev)) {
		LOG_ERR("I2C bus %s is not ready!", i2c_dev->name);
		return;
	}

	if (!device_is_ready(gpioExpander.bus)) {
		LOG_ERR("I2C bus %s is not ready!", gpioExpander.bus->name);
	} else {
		LOG_INF("I2C bus %s is ready", gpioExpander.bus->name);
	}

	const struct device *mcp_device = DEVICE_DT_GET(DT_NODELABEL(mcp23018_20));

	if (!device_is_ready(mcp_device)) {
		LOG_ERR("MCP23018 device not ready");
	}

	z_led_setup(mcp_device);

	bool testing = false;
	bool blink_on = false;

	point p;
	const struct zbus_channel *chan;

	while (!zbus_sub_wait_msg(&z_led_sub, &chan, &p, K_FOREVER)) {
		if (chan == &point_chan) {
			if (strcmp(p.type, POINT_TYPE_ATS_A) == 0) {
				int ats = atoi(p.key);
				if (ats >= ARRAY_SIZE(ats_chan_state_a)) {
					LOG_ERR("Point index exceeds state array size");
					continue;
				}
				ats_chan_state_a[ats] = point_get_int(&p);
			} else if (strcmp(p.type, POINT_TYPE_ATS_B) == 0) {
				int ats = atoi(p.key);
				if (ats >= ARRAY_SIZE(ats_chan_state_b)) {
					LOG_ERR("Point index exceeds state array size");
					continue;
				}
				ats_chan_state_b[ats] = point_get_int(&p);
			} else if (strcmp(p.type, POINT_TYPE_TEST_LEDS) == 0) {
				testing = point_get_int(&p);
			}
		} else if (chan == &ticker_chan) {
			if (testing) {
				z_leds_test_pattern(mcp_device);
				continue;
			}

			// display ATS state
			for (int i = 0; i < ARRAY_SIZE(ats_chan_state_a); i++) {
				ats_chan_state s = ats_chan_state_a[i];
				switch (s) {
				case ATS_OFF:
					z_led_set(mcp_device, false, i, false);
					break;
				case ATS_ON:
					z_led_set(mcp_device, false, i, true);
					break;
				case ATS_ACTIVE:
					if (blink_on) {
						z_led_set(mcp_device, false, i, true);
					} else {
						z_led_set(mcp_device, false, i, false);
					}
					break;
				case ATS_ERROR:
					// TODO: should fast blink here or something
					z_led_set(mcp_device, false, i, false);
					break;
				}

				s = ats_chan_state_b[i];
				switch (s) {
				case ATS_OFF:
					z_led_set(mcp_device, true, i, false);
					break;
				case ATS_ON:
					z_led_set(mcp_device, true, i, true);
					break;
				case ATS_ACTIVE:
					if (blink_on) {
						z_led_set(mcp_device, true, i, true);
					} else {
						z_led_set(mcp_device, true, i, false);
					}
					break;
				case ATS_ERROR:
					// TODO: should fast blink here or something
					z_led_set(mcp_device, true, i, false);
					break;
				}
			}
			blink_on = !blink_on;
		}
	}
}

K_THREAD_DEFINE(z_led, STACKSIZE, z_leds_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
