
#include "ats.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "zephyr/kernel.h"
#include "zephyr/device.h"
#include <zephyr/input/input.h>
#include <zephyr/zbus/zbus.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(z_dc, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(z_ats_chan);

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
void z_dc_thread(void *arg1, void *arg2, void *arg3)
{
	LOG_INF("z DC thread");

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

			switch (ats_event) {
			case AON:
				msg = MSG_AON;
				astate.state[ats].aon = evt.value;
				break;

			case ONA:
				msg = MSG_ONA;
				astate.state[ats].ona = evt.value;
				break;

			case BON:
				msg = MSG_BON;
				astate.state[ats].bon = evt.value;
				break;

			case ONB:
				msg = MSG_ONB;
				astate.state[ats].onb = evt.value;
				break;
			}

			zbus_chan_pub(&z_ats_chan, &astate, K_MSEC(500));

			LOG_DBG("ATS #%i: %s: %i", ats + 1, msg, evt.value);
		}
	}
}

K_THREAD_DEFINE(z_dc, STACKSIZE, z_dc_thread, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL, 0);
