#include <point.h>
#include <zephyr/zbus/zbus.h>

ZBUS_CHAN_DEFINE(point_chan, point, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(ticker_chan, uint8_t, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

void ticker_callback(struct k_timer *timer_id)
{
	uint8_t dummy = 0;
	zbus_chan_pub(&ticker_chan, &dummy, K_MSEC(500));
}

K_TIMER_DEFINE(ticker, ticker_callback, NULL);

int timer_init()
{
	k_timer_start(&ticker, K_MSEC(500), K_MSEC(500));

	return 0;
}

SYS_INIT(timer_init, APPLICATION, 99);
