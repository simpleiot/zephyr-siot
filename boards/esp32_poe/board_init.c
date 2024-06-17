/*
 * Copyright (c) 2022 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>

#define ESP32_POE_PHY_PWR_EN_PIN	12

static int board_esp32_ethernet_kit_init(void)
{
	const struct device *gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));

	if (!device_is_ready(gpio)) {
		return -ENODEV;
	}

	printf("CLIFF: enable phy power\n");

	/* Enable the Ethernet phy */
	int res = gpio_pin_configure(
		gpio, ESP32_POE_PHY_PWR_EN_PIN,
		GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);

	return res;
}

SYS_INIT(board_esp32_ethernet_kit_init, PRE_KERNEL_2, CONFIG_GPIO_INIT_PRIORITY);
