#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#define STACKSIZE 1024
#define PRIORITY  7

#define LED_STATUS (6 + 8)

LOG_MODULE_REGISTER(z_led, LOG_LEVEL_DBG);

#define I2C0_NODE DT_NODELABEL(mcp23018_20)
static const struct i2c_dt_spec gpioExpander = I2C_DT_SPEC_GET(I2C0_NODE);

const static struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

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

	gpio_pin_set(mcp_device, LED_STATUS, 0);
}

// LED num starts at 1
void z_led_set(const struct device *mcp_device, bool is_b, int num, bool on)
{
	int i = num - 1;
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

	int cur = 1;

	// display pattern on LEDs
	while (1) {
		k_msleep(200);
		gpio_pin_toggle(mcp_device, LED_STATUS);

		// turn current led off
		z_led_set(mcp_device, false, cur, false);
		z_led_set(mcp_device, true, cur, false);

		gpio_pin_set(mcp_device, cur, 0);
		gpio_pin_set(mcp_device, cur + 8, 0);
		cur += 1;
		if (cur > 6) {
			cur = 1;
		}
		// turn next LED on
		z_led_set(mcp_device, false, cur, true);
		z_led_set(mcp_device, true, cur, true);
	}
}

K_THREAD_DEFINE(z_led, STACKSIZE, z_leds_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
