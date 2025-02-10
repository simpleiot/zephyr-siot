/*
 * Copyright (c) 2023 Zonit Structured Solutions, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fan.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(z_fan, LOG_LEVEL_DBG);

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 10

// #define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

// static const struct device *gpio_dev;
// static struct gpio_callback gpio_cb;
// static bool polled_mode = false;
static bool alarm Z_GENERIC_SECTION(.rodata) = false;
const static struct device *const i2c_dev Z_GENERIC_SECTION(.rodata) = DEVICE_DT_GET(DT_NODELABEL(i2c0));

void fan_alarm_asserted(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	alarm = true;
}

void fan_dump_regs(void)
{
	uint8_t buf[0x40];
	i2c_burst_read(i2c_dev, 0x2e, 0x20, buf, sizeof(buf));
	for (int i = 0; i < 0x40; i++) {
		LOG_INF("%02x ", buf[i]);
		if ((i & 0x0f) == 0x0f) {
			LOG_INF("\n");
		}
	}
	LOG_INF("\n");
}

/*
Highest RPM: tach = 13,000
Lowest RPM: tach = 40,000 (could go slower but may experience stability issues)
*/
#define FAN_TACH_PERIOD_MIN 13000
#define FAN_TACH_PERIOD_MAX 40000
void fan_set_tach_target_period(uint16_t tach1, uint16_t tach2)
{
	uint8_t buf[0x10];
	LOG_DBG("Setting fan tach target period to %d, %d\n", tach1, tach2);
	buf[0] = tach1 & 0xff;
	buf[1] = tach1 >> 8;
	i2c_burst_write(i2c_dev, 0x2e, 0x3c, buf, 2);
	buf[0] = tach2 & 0xff;
	buf[1] = tach2 >> 8;
	i2c_burst_write(i2c_dev, 0x2e, 0x4c, buf, 2);
}

void fan_get_tach_period(uint16_t *tach1, uint16_t *tach2)
{
	uint8_t buf[0x10];
	i2c_burst_read(i2c_dev, 0x2e, 0x3e, buf, 2);
	*tach1 = ((int16_t)buf[0] << 8) | (buf[1]);
	i2c_burst_read(i2c_dev, 0x2e, 0x4e, buf, 2);
	*tach2 = ((int16_t)buf[0] << 8) | (buf[1]);
}

void fan_get_status(void)
{
	uint8_t buf[0x10];
	i2c_burst_read(i2c_dev, 0x2e, 0x24, buf, 4);
	if (buf[1] & 0x01) {
		LOG_ERR("Fan 1 stalled\n");
	}
	if (buf[1] & 0x02) {
		LOG_ERR("Fan 2 stalled\n");
	}
	if (buf[2] & 0x01) {
		LOG_ERR("Fan 1 cannot spin\n");
	}
	if (buf[2] & 0x02) {
		LOG_ERR("Fan 2 cannot spin\n");
	}
	if (buf[3] & 0x01) {
		LOG_ERR("Fan 1 drive fail\n");
	}
	if (buf[3] & 0x02) {
		LOG_ERR("Fan 2 drive fail\n");
	}
}

float fan_get_temp(temp_sensor_t sensor)
{
	uint8_t buf[0x10];
	/*
	I2C address of input air temp sensor is 0x48
	I2C address of output air temp sensor is 0x49
	*/
	switch (sensor) {
	case TEMP_INPUT:
		i2c_burst_read(i2c_dev, TEMP_INPUT_SENSOR_ADDRESS, 0x00, buf, 2);
		break;
	case TEMP_OUTPUT:
		i2c_burst_read(i2c_dev, TEMP_OUTPUT_SENSOR_ADDRESS, 0x00, buf, 2);
		break;
	default:
		LOG_ERR("Invalid temperature sensor\n");
		return 0;
	}
	uint16_t val = ((int16_t)buf[0] << 1) | (buf[1] >> 7);
	// Convert to 2's complement, since temperature can be negative
	if (val > 0x7FF) {
		val |= 0xF000;
	}
	// Convert to float temperature value (Celsius)
	float temp_c = val * 0.5;
	return temp_c;
}

bool fan_init(void)
{
	uint8_t buf[0x10];

	LOG_INF("Initializing fan controller...");
	// Set the fan speed target to 0xffff, which turns the fan off
	fan_set_tach_target_period(0xffff, 0xffff);
	// Enable the fan speed control algorithm
	buf[0] = 0xbb;
	i2c_burst_write(i2c_dev, 0x2e, 0x32, buf, 1);
	i2c_burst_write(i2c_dev, 0x2e, 0x42, buf, 1);
	// Set minimum drive
	buf[0] = 0x00;
	i2c_burst_write(i2c_dev, 0x2e, 0x38, buf, 1);
	i2c_burst_write(i2c_dev, 0x2e, 0x48, buf, 1);
	// Set PWM outputs to push-pull configuration
	buf[0] = 0x03;
	i2c_burst_write(i2c_dev, 0x2e, 0x2b, buf, 1);
	// Set up PWM drive frequency to 19.531 kHz
	buf[0] = 0x05;
	i2c_burst_write(i2c_dev, 0x2e, 0x2d, buf, 1);
	// Divide the PWM frequency by 2
	buf[0] = 0x02;
	i2c_burst_write(i2c_dev, 0x2e, 0x31, buf, 1);
	i2c_burst_write(i2c_dev, 0x2e, 0x41, buf, 1);
	// Enable fan alerts (interrupt enable)
	buf[0] = 0x03;
	i2c_burst_write(i2c_dev, 0x2e, 0x29, buf, 1);
	// Read fan product ID and revision registers
	i2c_burst_read(i2c_dev, 0x2e, 0xfd, buf, 3);
	LOG_DBG("Fan controller product ID: %d\n", buf[0]);
	LOG_DBG("Fan controller manufacturer ID: %d\n", buf[1]);
	LOG_DBG("Fan controller revision: %d\n", buf[2]);
	// gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpioe));
	// gpio_pin_configure(gpio_dev, 2, (GPIO_INPUT | GPIO_PULL_UP));
	// gpio_init_callback(&gpio_cb, fan_alarm_asserted, (1 << 13));
	// gpio_add_callback(gpio_dev, &gpio_cb);
	// int ret = gpio_pin_interrupt_configure(gpio_dev, 2, GPIO_INT_EDGE_FALLING);
	// if (ret != 0) {
	// 	LOG_ERR("Failed to configure fan interrupt, using polled mode");
	// 	polled_mode = true;
	// }
	return true;
}

void fan_thread(void *arg1, void *arg2, void *arg3)
{
	// uint16_t tach1 = 0;
	// uint16_t tach2 = 0;
	// uint16_t tach_target = 0xffff;
	// float temp_delta;
	// bool fan1_on = false;
	// bool fan2_on = false;
	// uint16_t fan_change_counter = 0;

	LOG_DBG("Fan thread starting\n");
	fan_init();
	while (1) {
		// if (!alarm && polled_mode) {
		// 	if (gpio_pin_get(gpio_dev, 13) == 0) {
		// 		alarm = true;
		// 	}
		// }
		// if (alarm) {
		// 	fan_get_status();
		// 	if (gpio_pin_get(gpio_dev, 13) == 1) {
		// 		alarm = false;
		// 		LOG_DBG("Fan alarm cleared\n");
		// 	}
		// }

		// fan_get_tach_period(&tach1, &tach2);
		// temp_delta = fan_get_temp(TEMP_INPUT) - fan_get_temp(TEMP_OUTPUT);
		// LOG_DBG("TACH: %d, %d, tem_deltap=%.1f\n", tach1, tach2, (double)temp_delta);
		// if (temp_delta > FAN_TEMP_THRESHOLD) {
		// 	// Only turn on fans if the temperature delta is above the
		// 	// FAN_TEMP_THRESHOLD
		// 	float duty = (temp_delta - FAN_TEMP_THRESHOLD) /
		// 		     (FAN_TEMP_MAX - FAN_TEMP_THRESHOLD);
		// 	uint16_t new_tach_target =
		// 		FAN_TACH_PERIOD_MAX -
		// 		(FAN_TACH_PERIOD_MAX - FAN_TACH_PERIOD_MIN) * duty;
		// 	if (tach_target != new_tach_target) {
		// 		tach_target = new_tach_target;
		// 		if (duty < 0.9f) {
		// 			// Only run one fan at a time
		// 			fan_set_tach_target_period(fan1_on ? tach_target : 0xffff,
		// 						   fan2_on ? tach_target : 0xffff);
		// 		} else {
		// 			// Run both fans
		// 			fan_set_tach_target_period(FAN_TACH_PERIOD_MIN,
		// 						   FAN_TACH_PERIOD_MIN);
		// 		}
		// 	}
		// } else {
		// 	// Fans off
		// 	tach_target = 0xffff;
		// }
		// if (++fan_change_counter >= FAN_CHANGE_SECONDS) {
		// 	// Change which fan is running
		// 	if (fan1_on) {
		// 		fan1_on = false;
		// 		fan2_on = true;
		// 	} else {
		// 		fan1_on = true;
		// 		fan2_on = false;
		// 	}
		// 	fan_set_tach_target_period(fan1_on ? tach_target : 0xffff,
		// 				   fan2_on ? tach_target : 0xffff);
		// 	fan_change_counter = 0;
		// }
		k_msleep(1000);
	}
}

K_THREAD_DEFINE(z_fan, STACKSIZE, fan_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
