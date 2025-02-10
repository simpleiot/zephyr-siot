#include "zephyr/kernel.h"
#include <point.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/init.h>

ZBUS_CHAN_DEFINE(point_chan, point, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(ticker_chan, uint8_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

void ticker_callback(struct k_timer *timer_id)
{
	uint8_t dummy = 0;
	zbus_chan_pub(&ticker_chan, &dummy, K_MSEC(500));
}

K_TIMER_DEFINE(ticker, ticker_callback, NULL);

int bus_init()
{
	k_timer_start(&ticker, K_MSEC(500), K_MSEC(500));

	point p;

	point_set_type_key(&p, POINT_TYPE_BOARD, "0");
	point_put_string(&p, CONFIG_BOARD_TARGET);

	zbus_chan_pub(&point_chan, &p, K_FOREVER);

	return 0;
}

SYS_INIT(bus_init, APPLICATION, 99);
