#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

/* nRF Libraries */
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>

static const struct device *leds = DEVICE_DT_GET(DT_NODELABEL(npm1300_leds));

int main(void)
{
	LOG_INF("SIOT nRF Simple");

	LOG_INF("HTTPS Sample. Board: %s", CONFIG_BOARD);

	/* Init modem lib */
	int err = nrf_modem_lib_init();
	if (err < 0) {
		LOG_ERR("Failed to init modem lib. (err: %i)", err);
		return err;
	}

	/* Init lte_lc*/
	err = lte_lc_init();
	if (err < 0) {
		LOG_ERR("Failed to init. Err: %i", err);
		return err;
	}

	/* Power saving is turned on */
	lte_lc_psm_req(true);

	/* Connect */
	err = lte_lc_connect();
	if (err < 0) {
		LOG_ERR("Failed to connect. Err: %i", err);
		return err;
	}

	while (1) {
		led_on(leds, 2U);
		k_sleep(K_MSEC(500));
		led_off(leds, 2U);
		k_sleep(K_MSEC(500));
	}

	return 0;
}
