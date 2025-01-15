
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

#define STACKSIZE 512
#define PRIORITY  7

LOG_MODULE_REGISTER(z_dc, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);

// ==================================================
// Industrial states

ats_state astate = INIT_ATS_STATE();

// ==================================================
// Key event handler, and message queue for passing DC events to main loop
K_MSGQ_DEFINE(dc_msgq, sizeof(struct input_event), 10, 4);

static void keymap_callback(struct input_event *evt, void *user_data)
{
	// Handle the input event
	if (evt->type == INPUT_EV_KEY) {
		k_msgq_put(&dc_msgq, evt, K_MSEC(50));
	}
}

static const struct device *const keymap_dev = DEVICE_DT_GET(DT_NODELABEL(keymap));

INPUT_CALLBACK_DEFINE(keymap_dev, keymap_callback, NULL);

void send_initial_states(void)
{
	char *ats_states[] = {
		POINT_TYPE_ATS_AON,
		POINT_TYPE_ATS_ONA,
		POINT_TYPE_ATS_BON,
		POINT_TYPE_ATS_ONB,
	};

	for (int i = 0; i < 1; i++) {
		for (int j = 0; j < ARRAY_SIZE(ats_states); j++) {
			point p;
			char index[10];
			snprintf(index, sizeof(index), "%i", i);
			point_set_type_key(&p, ats_states[j], index);
			point_put_int(&p, 0);
			char buf[30];
			point_dump(&p, buf, sizeof(buf));
			LOG_DBG("CLIFF: dc point: %s", buf);
			// int ret = zbus_chan_pub(&point_chan, &p, K_MSEC(500));
			// if (ret != 0) {
			// 	LOG_ERR("Error sending initial ats state: %i", ret);
			// }
		}
	}
}

void z_dc_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("z DC thread");

	// FIXME: for some reason, the below function crashes, but sending the
	// same code inline below works fine.
	// send_initial_states();

	char *ats_states[] = {
		POINT_TYPE_ATS_AON,
		POINT_TYPE_ATS_ONA,
		POINT_TYPE_ATS_BON,
		POINT_TYPE_ATS_ONB,
	};

	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < ARRAY_SIZE(ats_states); j++) {
			point p;
			char index[10];
			snprintf(index, sizeof(index), "%i", i);
			point_set_type_key(&p, ats_states[j], index);
			point_put_int(&p, 0);
			char buf[30];
			point_dump(&p, buf, sizeof(buf));
			LOG_DBG("CLIFF: dc point: %s", buf);
			int ret = zbus_chan_pub(&point_chan, &p, K_MSEC(500));
			if (ret != 0) {
				LOG_ERR("Error sending initial ats state: %i", ret);
			}
		}
	}

	LOG_DBG("CLIFF: after send initial states()");

	struct input_event evt;

	static const char MSG_AON[] = "AON";
	static const char MSG_ONA[] = "ONA";
	static const char MSG_BON[] = "BON";
	static const char MSG_ONB[] = "ONB";

	while (1) {
		if (k_msgq_get(&dc_msgq, &evt, K_FOREVER) == 0) {
			// Process the input event
			int code_z = evt.code - 1;
			int ats = code_z / 4;
			int ats_event = code_z % 4;

			const char *msg = "unknown";
			const char *point_type = "unknown";

			switch (ats_event) {
			case AON:
				msg = MSG_AON;
				point_type = POINT_TYPE_ATS_AON;
				astate.state[ats].aon = evt.value;
				break;

			case ONA:
				msg = MSG_ONA;
				point_type = POINT_TYPE_ATS_ONA;
				astate.state[ats].ona = evt.value;
				break;

			case BON:
				msg = MSG_BON;
				point_type = POINT_TYPE_ATS_BON;
				astate.state[ats].bon = evt.value;
				break;

			case ONB:
				msg = MSG_ONB;
				point_type = POINT_TYPE_ATS_ONB;
				astate.state[ats].onb = evt.value;
				break;
			}

			point p;

			char index[10];
			sprintf(index, "%i", ats);

			point_set_type_key(&p, point_type, index);
			point_put_int(&p, evt.value);
			zbus_chan_pub(&point_chan, &p, K_MSEC(500));

			LOG_DBG("ATS #%i: %s: %i", ats + 1, msg, evt.value);
		}
	}
}

K_THREAD_DEFINE(z_dc, STACKSIZE, z_dc_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
