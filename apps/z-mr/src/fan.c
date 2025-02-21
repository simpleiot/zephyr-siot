/*
 * Copyright (c) 2023 Zonit Structured Solutions, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/util.h"
#include "zpoint.h"

#include <point.h>
#include <siot-string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>
#include <zephyr/zbus/zbus.h>
#include <stdint.h>

LOG_MODULE_REGISTER(z_fan, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);
ZBUS_CHAN_DECLARE(ticker_chan);

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 10

#define FAN_COUNT 2

// ================================================
// FAN Control tuning
// The following settings need to be tuned for:
//   - fan model
//   - power filtering circuit
//   - input voltage level
//
// Run the fan in PWM mode and watch the power
// and tachometer stability over the PWM range
// to determine the valid PWM and RPM control
// ranges for the fan.

// Min and max fan speeds
#define FAN_SPEED_MIN_RPM 4500
#define FAN_SPEED_MAX_RPM 10500

// Min PWM duty cycle, currently 5%
// the below value is 8-bit count, so calculate
// by duty-cycle * 255/100
#define FAN_MIN_DUTY_CYCLE_COUNT 13

// ================================================

// Temp sensor delta threshold to turn on fans
#define FAN_TEMP_THRESHOLD 10
#define FAN_TEMP_MAX       20
// Change which fan is running periodically
#define FAN_CHANGE_SECONDS 60

#define EMC230X_I2C_ADDR 0x2e

#define EMC230X_REG_FAN_STATUS        0x24
#define EMC230X_REG_FAN_STALL_STATUS  0x25
#define EMC230X_REG_FAN_SPIN_STATUS   0x26
#define EMC230X_REG_DRIVE_FAIL_STATUS 0x27
#define EMC230X_REG_OUTPUT_CONFIG     0x2b
#define EMC230X_REG_PWM_BASE_FREQ     0x2d
#define EMC230X_REG_VENDOR            0xfe

#define EMC230X_FAN_MAX              0xff
#define EMC230X_FAN_MIN              0x00
#define EMC230X_FAN_MAX_STATE        10
#define EMC230X_TACH_REGS_UNUSE_BITS 3

// FAN CONFIG register (0x) -- currently these are all at defaults

// The following can be used to set the FAN CONFIG:RNG bits
// 0: x1, 500 RPM min
// 1: x2: 1000 RPM min
// 2: x4: 2000 RPM min
// 3: x8: 4000 RPM min
#define EMC230X_FAN_CFG_RNG    1
// #define EMC230X_TACH_RANGE_MIN 480
#define EMC230X_TACH_RANGE_MIN 960

// Number edges to sample when calculating RPM, FAN CONFIG:EDG
// 0: 3 edges (1 pole)
// 1: 5 edges (2 poles)
// 2: 7 edges (3 poles)
// 3: 9 edges (4 poles)
#define EMC230X_FAN_CFG_EDG 1

// PID update rate
// 0: 100ms
// 1: 200ms
// 2: 300ms
// 3: 400ms
// 4: 500ms
// 5: 800ms
// 6: 1200ms
// 7: 1600ms
#define EMC230X_FAN_CFG_UDT 3

#define EMC230X_FAN_CFG_VALUE                                                                      \
	(EMC230X_FAN_CFG_RNG << 5 | EMC230X_FAN_CFG_EDG << 3 | EMC230X_FAN_CFG_UDT)

/*
 * Factor by equations [2] and [3] from data sheet; valid for fans where the number of edges
 * equal (poles * 2 + 1).
 */
#define EMC230X_RPM_FACTOR 3932160

#define EMC230X_REG_FAN_DRIVE(n)       (0x30 + 0x10 * (n))
#define EMC230X_REG_PWM_DIVIDE(n)      (0x31 + 0x10 * (n))
#define EMC230X_REG_FAN_CFG(n)         (0x32 + 0x10 * (n))
#define EMC230X_REG_FAN_MIN_DRIVE(n)   (0x38 + 0x10 * (n))
#define EMC230X_REG_FAN_VALID(n)       (0x39 + 0x10 * (n))
#define EMC230X_REG_DRIVE_FAIL_BAND(n) (0x3a + 0x10 * (n))
#define EMC230X_REG_FAN_TARGET(n)      (0x3c + 0x10 * (n))
#define EMC230X_REG_FAN_TACH(n)        (0x3e + 0x10 * (n))

typedef enum fan_id {
	FAN_NONE = 0,
	FAN_1,
	FAN_2,
} fan_id_t;

typedef enum temp_sensor {
	TEMP_NONE = 0,
	TEMP_INPUT,
	TEMP_OUTPUT,
} temp_sensor_t;

// #define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

// static const struct device *gpio_dev;
// static struct gpio_callback gpio_cb;
// static bool polled_mode = false;
static bool alarm = false;
const static struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

void fan_alarm_asserted(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	alarm = true;
}

int fan_i2c_read_uint8(uint8_t start_addr, uint8_t *value)
{
	return i2c_burst_read(i2c_dev, EMC230X_I2C_ADDR, start_addr, value, 1);
}

int fan_i2c_read_uint16(uint8_t start_addr, uint16_t *value)
{
	return i2c_burst_read(i2c_dev, EMC230X_I2C_ADDR, start_addr, (uint8_t *)value, 2);
}

// for regs that are big endian ...
int fan_i2c_read_uint16_be(uint8_t start_addr, uint16_t *value)
{
	uint8_t buf[2];
	int ret = i2c_burst_read(i2c_dev, EMC230X_I2C_ADDR, start_addr, buf, sizeof(buf));
	*value = buf[0] << 8 | buf[1];
	return ret;
}

int fan_i2c_write_uint8(uint8_t addr, uint8_t value)
{
	return i2c_burst_write(i2c_dev, EMC230X_I2C_ADDR, addr, &value, 1);
}

int fan_i2c_write_uint16(uint8_t addr, uint16_t value)
{
	uint8_t *buf = (uint8_t *)(&value);
	return i2c_burst_write(i2c_dev, EMC230X_I2C_ADDR, addr, buf, 2);
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
	LOG_DBG("Setting fan tach target period to %d, %d\n", tach1, tach2);
	fan_i2c_write_uint16(0x3c, tach1);
	fan_i2c_write_uint16(0x4c, tach1);
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

bool fan_init_old(void)
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

// if in automatic mode, this function has no affect
int fan_set_drive(int index, uint8_t value)
{
	if (index >= 4) {
		LOG_ERR("invalid fan index: %i", index);
	}
	return fan_i2c_write_uint8(EMC230X_REG_FAN_DRIVE(index), value);
}

// returns RPM
int fan_read_tach(int index, int *rpm)
{
	uint16_t value;
	fan_i2c_read_uint16_be(EMC230X_REG_FAN_TACH(index), &value);
	// don't let divide by zero
	if (value == 0) {
		value = 1;
	}
	value = value >> EMC230X_TACH_REGS_UNUSE_BITS;
	*rpm = EMC230X_RPM_FACTOR * 2 / value;
	if (*rpm <= EMC230X_TACH_RANGE_MIN) {
		*rpm = 0;
	}

	// FIXME error checking
	return 0;
}

uint16_t fan_rpm_to_tach(int rpm)
{
	int value;
	if (rpm == 0) {
		value = 0xffff;
	} else {
		value = EMC230X_RPM_FACTOR * 2 / rpm;
		value = value << EMC230X_TACH_REGS_UNUSE_BITS;
		// check if we overflow 16 bits
		if (value > 0xffff) {
			value = 0xffff;
		}
	}

	return (uint16_t)value;
}

int fan_set_target(int index, int rpm)
{
	uint16_t value = fan_rpm_to_tach(rpm);

	fan_i2c_write_uint16(EMC230X_REG_FAN_TARGET(index), (uint16_t)value);
	// FIXME error checking
	return 0;
}

int fan_set_valid(int index, int rpm)
{
	uint16_t value = fan_rpm_to_tach(rpm);
	value = value >> 8;

	fan_i2c_write_uint8(EMC230X_REG_FAN_VALID(index), (uint8_t)value);
	// FIXME error checking
	return 0;
}

int fan_set_fail_band(int index, int rpm)
{
	uint16_t value = fan_rpm_to_tach(rpm);

	fan_i2c_write_uint16(EMC230X_REG_DRIVE_FAIL_BAND(index), (uint16_t)value);
	// FIXME error checking
	return 0;
}

int fan_init(bool enable_speed_control)
{
	uint8_t cfg_value = EMC230X_FAN_CFG_VALUE;

	if (enable_speed_control) {
		cfg_value |= (1 << 7);
	}
	// set to manual mode initially
	fan_i2c_write_uint8(EMC230X_REG_FAN_CFG(0), cfg_value);
	fan_i2c_write_uint8(EMC230X_REG_FAN_CFG(1), cfg_value);

	// set drive to push/pull
	fan_i2c_write_uint8(EMC230X_REG_OUTPUT_CONFIG, 0x3);

	// set PWM freq base to 19.53KHz
	fan_i2c_write_uint8(EMC230X_REG_PWM_BASE_FREQ, 0x5);

	// set PWM divide to 3
	fan_i2c_write_uint8(EMC230X_REG_PWM_DIVIDE(0), 0x2);
	fan_i2c_write_uint8(EMC230X_REG_PWM_DIVIDE(1), 0x2);

	// set speed to max
	fan_set_drive(0, 0xff);
	fan_set_drive(1, 0xff);

	fan_i2c_write_uint8(EMC230X_REG_FAN_MIN_DRIVE(0), FAN_MIN_DUTY_CYCLE_COUNT);
	fan_i2c_write_uint8(EMC230X_REG_FAN_MIN_DRIVE(1), FAN_MIN_DUTY_CYCLE_COUNT);

	// anything below 3000 RPM is flagged as a stall
	fan_set_valid(0, 3000);
	fan_set_valid(1, 3000);

	// at full drive, if the actual drive is 2000 less than commanded, flag a drive failure
	// TODO: this is not working yet
	fan_set_fail_band(0, 2000);
	fan_set_fail_band(1, 2000);

	return 0;
}

ZBUS_MSG_SUBSCRIBER_DEFINE(z_fan_sub);
ZBUS_CHAN_ADD_OBS(point_chan, z_fan_sub, 3);
ZBUS_CHAN_ADD_OBS(ticker_chan, z_fan_sub, 4);

enum fan_mode_enum {
	FAN_MODE_OFF,
	FAN_MODE_TEMP,
	FAN_MODE_TACH,
	FAN_MODE_PWM,
};

enum fan_status_enum {
	FAN_STATUS_NOT_SET,
	FAN_STATUS_OK,
	FAN_STATUS_STALL,
	FAN_STATUS_SPIN_FAIL,
	FAN_STATUS_DRIVE_FAIL,
};

#define FAN_COUNT 2

void fan_thread(void *arg1, void *arg2, void *arg3)
{

	point p;
	const struct zbus_channel *chan;

	int status_tick = 0;

	fan_init(false);

	int fan_mode = FAN_MODE_OFF;
	float fan_set_speed[2] = {FAN_COUNT};
	int fan_status[2] = {FAN_COUNT};
	// TODO: we should store the timestamp of the last valid temp
	// and run the fans at high speed if we are not getting temp values
	float temp = 0;

	while (!zbus_sub_wait_msg(&z_fan_sub, &chan, &p, K_FOREVER)) {
		if (chan == &point_chan) {
			if (strcmp(p.type, POINT_TYPE_FAN_MODE) == 0) {
				fan_mode = point_get_int(&p);
				LOG_DBG("Fan mode: %s", p.data);
				if (strcmp(p.data, POINT_VALUE_OFF) == 0) {
					fan_mode = FAN_MODE_OFF;
					fan_init(false);
				} else if (strcmp(p.data, POINT_VALUE_TEMP) == 0) {
					fan_mode = FAN_MODE_TEMP;
					fan_init(true);
				} else if (strcmp(p.data, POINT_VALUE_PWM) == 0) {
					fan_mode = FAN_MODE_PWM;
					fan_init(false);
				} else if (strcmp(p.data, POINT_VALUE_TACH) == 0) {
					fan_mode = FAN_MODE_TACH;
					fan_init(true);
				}
			} else if (strcmp(p.type, POINT_TYPE_FAN_SET_SPEED) == 0) {
				int index = atoi(p.key);
				if (index >= ARRAY_SIZE(fan_set_speed)) {
					LOG_ERR("Set fan speed, index out of bounds: %i", index);
					continue;
				}
				float speed = point_get_float(&p);
				LOG_DBG("Fan speed %i: %f", index, (double)speed);
				fan_set_speed[index] = point_get_float(&p);
			} else if (strcmp(p.type, POINT_TYPE_TEMPERATURE) == 0) {
				temp = point_get_float(&p);
			}
		} else if (chan == &ticker_chan) {
			status_tick++;
			if (status_tick >= 2) {
				for (int i = 0; i < FAN_COUNT; i++) {
					int rpm;
					fan_read_tach(i, &rpm);
					status_tick = 0;

					point p;
					char index[10];
					itoa(i, index, 10);
					point_set_type_key(&p, POINT_TYPE_FAN_SPEED, index);
					point_put_int(&p, rpm);
					zbus_chan_pub(&point_chan, &p, K_MSEC(500));

					uint8_t b;
					fan_i2c_read_uint8(EMC230X_REG_FAN_STALL_STATUS, &b);
					if (b & (1 << i)) {
						// we have a stall
						if (fan_status[i] != FAN_STATUS_STALL) {
							fan_status[i] = FAN_STATUS_STALL;
							point_set_type_key(
								&p, POINT_TYPE_FAN_STATUS, index);
							point_put_string(&p, POINT_VALUE_STALL);
							zbus_chan_pub(&point_chan, &p, K_MSEC(500));
						}
						// only report one problem
						continue;
					}

					fan_i2c_read_uint8(EMC230X_REG_FAN_SPIN_STATUS, &b);
					if (b & (1 << i)) {
						// we have a stall
						if (fan_status[i] != FAN_STATUS_SPIN_FAIL) {
							fan_status[i] = FAN_STATUS_SPIN_FAIL;
							point_set_type_key(
								&p, POINT_TYPE_FAN_STATUS, index);
							point_put_string(&p, POINT_VALUE_SPIN_FAIL);
							zbus_chan_pub(&point_chan, &p, K_MSEC(500));
						}
						// only report one problem
						continue;
					}

					fan_i2c_read_uint8(EMC230X_REG_DRIVE_FAIL_STATUS, &b);
					if (b & (1 << i)) {
						// we have a stall
						if (fan_status[i] != FAN_STATUS_DRIVE_FAIL) {
							fan_status[i] = FAN_STATUS_DRIVE_FAIL;
							point_set_type_key(
								&p, POINT_TYPE_FAN_STATUS, index);
							point_put_string(&p,
									 POINT_VALUE_DRIVE_FAIL);
							zbus_chan_pub(&point_chan, &p, K_MSEC(500));
						}
						// only report one problem
						continue;
					}

					// no errors found, make sure status is set to OK
					if (fan_status[i] != FAN_STATUS_OK) {
						fan_status[i] = FAN_STATUS_OK;
						point_set_type_key(&p, POINT_TYPE_FAN_STATUS,
								   index);
						point_put_string(&p, POINT_VALUE_OK);
						zbus_chan_pub(&point_chan, &p, K_MSEC(500));
					}
				}
			}

			// fan control
			switch (fan_mode) {
			case FAN_MODE_OFF:
				fan_set_drive(0, 0);
				fan_set_drive(1, 0);
				break;
			case FAN_MODE_PWM:
				for (int i = 0; i < ARRAY_SIZE(fan_set_speed); i++) {
					int v = fan_set_speed[i] * 255 / 100;
					if (v > 255) {
						v = 255;
					}

					if (v < 0) {
						v = 0;
					}

					fan_set_drive(i, (uint8_t)v);
				}
				break;
			case FAN_MODE_TACH:
				for (int i = 0; i < ARRAY_SIZE(fan_set_speed); i++) {
					fan_set_target(i, (int)fan_set_speed[i]);
				}
				break;
			case FAN_MODE_TEMP: {
				// the fan_set_speed contain start and max temp and we linearly
				// increase fan speed between these two temp points
				float temp_min = fan_set_speed[0];
				float temp_max = fan_set_speed[1];

				float speed;

				// check for invalid settings
				if (temp_max < temp_min || temp_min == temp_max || temp_min == 0 ||
				    temp_max == 0) {
					// run fans at full speed
					speed = FAN_SPEED_MAX_RPM;
				} else {
					speed = FAN_SPEED_MIN_RPM +
						(FAN_SPEED_MAX_RPM - FAN_SPEED_MIN_RPM) *
							(temp - temp_min) / (temp_max - temp_min);
				}

				if (speed < FAN_SPEED_MIN_RPM) {
					speed = 0;
				}

				for (int i = 0; i < FAN_COUNT; i++) {
					fan_set_target(i, (int)speed);
				}
			}

			break;
			}
		}
	}
}

K_THREAD_DEFINE(z_fan, STACKSIZE, fan_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
