
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

int main(void)
{
	LOG_INF("SIOT Zephyr Application! %s", CONFIG_BOARD_TARGET);

	while (1) {
		LOG_INF("Main loop");
		k_msleep(1000);
	}

	return 0;
}
