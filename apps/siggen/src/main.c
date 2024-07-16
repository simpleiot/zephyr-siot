
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/dac.h>

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

// HTTP_RESOURCE_DEFINE(ws_resource, test_http_service, "/", &ws_resource_detail);

// static uint16_t https_service_port = 80;
// HTTPS_SERVICE_DEFINE(siot_https_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR,
// 		     &https_service_port, 1, 10, NULL,
// 		     sec_tag_list_verify_none, sizeof(sec_tag_list_verify_none));

#define DAC_DEVICE_NODE DT_NODELABEL(dac1)

// The DAC works on the L432KC, but the user shell does not

void gen_triangle(const struct device *dac_dev, int chan)
{
	const int min = 0;
	const int max = 1241; // 1V
	bool up = true;
	int value = 0;
	const int delay = 110;

	LOG_DBG("Delay is %i: ", delay);

	while (1) {
		if (up) {
			value++;
			if (value >= max) {
				up = false;
			}
		} else {
			value--;
			if (value <= min) {
				up = true;
			}
		}
		int ret = dac_write_value(dac_dev, chan, value);
		if (ret != 0) {
			LOG_ERR("Error writing to DAC: %i", ret);
		}
		k_msleep(delay);
	}
}

int main(void)
{
	LOG_INF("SIOT Zephyr Signal Generator! %s", CONFIG_BOARD_TARGET);
	// http_server_start();

	const struct device *dac_dev = DEVICE_DT_GET(DAC_DEVICE_NODE);

	if (!device_is_ready(dac_dev)) {
		LOG_WRN("DAC device not ready\n");
	}

	const int chan = 1;

	struct dac_channel_cfg dac_cfg = {.channel_id = chan, .resolution = 12};

	if (dac_channel_setup(dac_dev, &dac_cfg) != 0) {
		LOG_ERR("DAC channel setup failed\n");
	}

	gen_triangle(dac_dev, chan);

	return 0;
}
