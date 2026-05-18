#include <nvs.h>
#include <point.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>
#include <app_version.h>
#include <string.h>

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

/*
 * Points persisted to ZMS via lib/nvs.c. lib/nvs.c gives POINT_TYPE_BOOT_COUNT
 * special treatment: on every boot it reads the stored value, publishes it on
 * point_chan, then increments and writes it back. So bootCount climbing by one
 * across resets confirms ZMS is mounting and retaining data.
 */
static const struct nvs_point nvs_pts[] = {
	{1, &point_def_boot_count, "0"},
};

/*
 * Surface the boot count at INFO level. lib/nvs.c only logs it at DBG, so
 * without this a normal build wouldn't show it. The value published is the
 * count *before* this boot's increment (0 on the very first boot).
 */
static void boot_count_listener(const struct zbus_channel *chan)
{
	const point *src = zbus_chan_const_msg(chan);
	point p = *src;

	if (strcmp(p.type, POINT_TYPE_BOOT_COUNT) == 0) {
		LOG_INF("Boot count: %d", point_get_int(&p));
	}
}

ZBUS_CHAN_DECLARE(point_chan);
ZBUS_LISTENER_DEFINE(boot_count_lis, boot_count_listener);
ZBUS_CHAN_ADD_OBS(point_chan, boot_count_lis, 4);

int main(void)
{
	LOG_INF("siot-simple: %s %s", CONFIG_BOARD_TARGET, APP_VERSION_EXTENDED_STRING);

	nvs_init(nvs_pts, ARRAY_SIZE(nvs_pts));

	return 0;
}
