#include "zephyr/kernel.h"
#include <point.h>
#include <zephyr/zbus/zbus.h>
#include "app_version.h"

#define TICKER_STACKSIZE 1024
#define TICKER_PRIORITY  7

ZBUS_CHAN_DEFINE(point_chan, point, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(ticker_chan, uint8_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

// The ticker publishes from a thread rather than from the timer expiry
// function. A k_timer callback runs in interrupt context, where the only legal
// timeout is K_NO_WAIT. With CONFIG_ZBUS_MSG_SUBSCRIBER enabled every publish
// allocates a net_buf from the shared message subscriber pool, including
// publishes to channels that have no message subscribers at all, as this one
// does. K_NO_WAIT against a momentarily empty pool returns NULL and zbus
// asserts on that, so a burst of traffic on another channel could bring the
// system down from the tick. Waiting for a buffer is only possible in a thread.
//
// k_timer_status_sync() keeps the cadence anchored to the kernel timer, so the
// period does not drift by however long each publish takes.
K_TIMER_DEFINE(ticker, NULL, NULL);

static void ticker_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_timer_start(&ticker, K_MSEC(500), K_MSEC(500));

	while (1) {
		k_timer_status_sync(&ticker);

		uint8_t dummy = 0;
		zbus_chan_pub(&ticker_chan, &dummy, K_MSEC(500));
	}
}

K_THREAD_DEFINE(siot_ticker, TICKER_STACKSIZE, ticker_thread, NULL, NULL, NULL, TICKER_PRIORITY,
		K_ESSENTIAL, 0);

int bus_init()
{
	point p = {};

	point_set_type_key(&p, POINT_TYPE_BOARD, "0");
	// CONFIG_BOARD rather than CONFIG_BOARD_TARGET: the target adds the SoC
	// and core qualifier, as in esp32_devkitc/esp32/procpu, which overruns
	// the 20 byte point data field and is reported truncated mid-name. The
	// board name alone identifies the hardware and fits.
	point_put_string(&p, CONFIG_BOARD);
	zbus_chan_pub(&point_chan, &p, K_MSEC(500));

	bool dev = false;
	if (strstr(APP_VERSION_EXTENDED_STRING, "dev") > 0) {
		dev = true;
	}

	point_set_type_key(&p, POINT_TYPE_VERSION_FW, "0");
	if (dev) {
		point_put_string(&p, APP_VERSION_EXTENDED_STRING);
	} else {
		point_put_string(&p, APP_VERSION_STRING);
	}
	zbus_chan_pub(&point_chan, &p, K_MSEC(500));

	return 0;
}

SYS_INIT(bus_init, APPLICATION, 99);
