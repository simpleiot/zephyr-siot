
#include <point.h>

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

#define STACKSIZE 1024
#define PRIORITY  7

LOG_MODULE_REGISTER(siot_metrics, LOG_LEVEL_INF);

ZBUS_CHAN_DECLARE(point_chan);

void siot_metrics_thread(void *arg1, void *arg2, void *arg3)
{
	while (1) {
		k_msleep(1000);
		k_thread_runtime_stats_t stats;
		int rc = k_thread_runtime_stats_all_get(&stats);

		if (rc != 0) {
			LOG_ERR("Error getting thread stats: %i", rc);
			continue;
		}

		float cpu_usage = (double)stats.total_cycles * 100 / stats.execution_cycles;
		LOG_DBG("cpu usage: %0.2f%%", (double)cpu_usage);

		point p = {.key = ""};

		point_set_type(&p, POINT_TYPE_METRIC_SYS_CPU_PERCENT);
		point_put_float(&p, cpu_usage);
		zbus_chan_pub(&point_chan, &p, K_MSEC(500));
	}
}

K_THREAD_DEFINE(siot_metrics, STACKSIZE, siot_metrics_thread, NULL, NULL, NULL, PRIORITY,
		K_ESSENTIAL, 0);
