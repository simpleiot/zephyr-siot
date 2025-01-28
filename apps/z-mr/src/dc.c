
#include "ats.h"
#include "point.h"
#include "zephyr/sys/util.h"
#include "zpoint.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "zephyr/kernel.h"
#include "zephyr/device.h"
#include <zephyr/input/input.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/gpio.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(z_dc, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);

// ==================================================
// Industrial states

ats_state astate = INIT_ATS_STATE();

K_TIMER_DEFINE(dc_timer, NULL, NULL);

const struct device *mcp_device1 = DEVICE_DT_GET(DT_NODELABEL(mcp23018_21));
const struct device *mcp_device2 = DEVICE_DT_GET(DT_NODELABEL(mcp23018_22));

struct ats_dc {
	const struct device *dev;
	int aon;
	int ona;
	int bon;
	int onb;
};

const char *ats_states[] = {
	POINT_TYPE_ATS_A,
	POINT_TYPE_ATS_B,
};

// configure all pins with pullups
void z_dc_gpio_setup(const struct device *mcp_device)
{
	for (int i = 0; i < 16; i++) {
		int ret = gpio_pin_configure(mcp_device, i,
					     GPIO_INPUT | GPIO_ACTIVE_LOW | GPIO_PULL_UP);
		if (ret < 0) {
			LOG_ERR("Failed to configure GPIO pin %i: %d", i, ret);
			return; // FIXME remove this
		}
	}
}

void z_dc_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("z DC thread");

	const struct ats_dc ats_dcs[] = {
		{mcp_device1, 8 + 1, 8 + 0, 1, 0}, {mcp_device1, 8 + 3, 8 + 2, 3, 2},
		{mcp_device1, 8 + 5, 8 + 4, 5, 4}, {mcp_device2, 8 + 1, 8 + 0, 1, 0},
		{mcp_device2, 8 + 3, 8 + 2, 3, 2}, {mcp_device2, 8 + 5, 8 + 4, 5, 4},
	};

	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < ARRAY_SIZE(ats_states); j++) {
			point p;
			char index[10];
			snprintf(index, sizeof(index), "%i", i);
			point_set_type_key(&p, ats_states[j], index);
			point_put_int(&p, 0);
			int ret = zbus_chan_pub(&point_chan, &p, K_MSEC(500));
			if (ret != 0) {
				LOG_ERR("Error sending initial ats state: %i", ret);
			}
		}
	}

	k_timer_start(&dc_timer, K_MSEC(200), K_MSEC(200));

	if (!device_is_ready(mcp_device1)) {
		LOG_ERR("MCP23018 device1 not ready");
	}

	if (!device_is_ready(mcp_device2)) {
		LOG_ERR("MCP23018 device2 not ready");
	}

	z_dc_gpio_setup(mcp_device1);
	z_dc_gpio_setup(mcp_device2);

	// struct input_event evt;

	// static const char MSG_AON[] = "AON";
	// static const char MSG_ONA[] = "ONA";
	// static const char MSG_BON[] = "BON";
	// static const char MSG_ONB[] = "ONB";
	point p;

	char index[10];

	while (1) {
		k_timer_status_sync(&dc_timer);

		for (int i = 0; i < ARRAY_SIZE(ats_dcs); i++) {
			bool a_changed = false;
			bool b_changed = false;
			const struct ats_dc *a = &ats_dcs[i];
			bool aon = gpio_pin_get(a->dev, a->aon);
			bool ona = gpio_pin_get(a->dev, a->ona);
			bool bon = gpio_pin_get(a->dev, a->bon);
			bool onb = gpio_pin_get(a->dev, a->onb);

			if (aon != astate.state[i].aon) {
				LOG_DBG("ATS #%i: AON: %i", i + 1, aon);
				astate.state[i].aon = aon;
				a_changed = true;
			}

			if (ona != astate.state[i].ona) {
				LOG_DBG("ATS #%i: ONA: %i", i + 1, ona);
				astate.state[i].ona = ona;
				a_changed = true;
			}

			if (bon != astate.state[i].bon) {
				LOG_DBG("ATS #%i: BON: %i", i + 1, bon);
				astate.state[i].bon = bon;
				b_changed = true;
			}

			if (onb != astate.state[i].onb) {
				LOG_DBG("ATS #%i: ONB: %i", i + 1, onb);
				astate.state[i].onb = onb;
				b_changed = true;
			}

			point p;
			char index[10];

			if (a_changed) {
				sprintf(index, "%i", i);
				point_set_type_key(&p, POINT_TYPE_ATS_A, index);
				point_put_int(&p, z_ats_get_state_a(&astate.state[i]));
				zbus_chan_pub(&point_chan, &p, K_MSEC(500));
			}

			if (b_changed) {
				sprintf(index, "%i", i);
				point_set_type_key(&p, POINT_TYPE_ATS_B, index);
				point_put_int(&p, z_ats_get_state_b(&astate.state[i]));
				zbus_chan_pub(&point_chan, &p, K_MSEC(500));
			}
		}
	}
}

K_THREAD_DEFINE(z_dc, STACKSIZE, z_dc_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
