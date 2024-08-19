
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

int main(void)
{
	LOG_INF("SIOT Zephyr Application! %s", CONFIG_BOARD_TARGET);

	return 0;
}
