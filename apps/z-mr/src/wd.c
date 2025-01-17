
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#define STACKSIZE 512
#define PRIORITY  7

LOG_MODULE_REGISTER(z_wd, LOG_LEVEL_DBG);

void z_wd_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

	if (!device_is_ready(gpio_dev)) {
		LOG_ERR("gpio device not ready");
	}

	gpio_pin_configure(gpio_dev, 2, (GPIO_OUTPUT | GPIO_ACTIVE_LOW));

	while (1) {
		k_msleep(5000);
		gpio_pin_set(gpio_dev, 2, 1);
		k_msleep(100);
		gpio_pin_set(gpio_dev, 2, 0);
	}
}

K_THREAD_DEFINE(z_wd, STACKSIZE, z_wd_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
