#include "zephyr/kernel.h"
#include <point.h>
#include <zephyr/zbus/zbus.h>
#include "app_version.h"

ZBUS_CHAN_DEFINE(point_chan, point, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(ticker_chan, uint8_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

void ticker_callback(struct k_timer *timer_id)
{
	uint8_t dummy = 0;
	// NOTE: anything other than K_NO_WAIT here will cause
	// Zephyr to crash
	zbus_chan_pub(&ticker_chan, &dummy, K_NO_WAIT);
}

K_TIMER_DEFINE(ticker, ticker_callback, NULL);

int bus_init()
{
	k_timer_start(&ticker, K_MSEC(500), K_MSEC(500));

	point p;

	point_set_type_key(&p, POINT_TYPE_BOARD, "0");
	point_put_string(&p, CONFIG_BOARD_TARGET);
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
