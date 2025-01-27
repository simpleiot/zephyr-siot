#include "siot-string.h"
#include "zephyr/ztest_assert.h"
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(siot_string_tests, LOG_LEVEL_DBG);

ZTEST_SUITE(siot_string_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(siot_string_tests, ftoa)
{
	char buf[64];

	ftoa(53.283, buf, 3);
	zassert_str_equal(buf, "53.283");

	ftoa(53.283, buf, 1);
	zassert_str_equal(buf, "53.2");
}

ZTEST(siot_string_tests, ftoa_negative)
{
	char buf[64];

	ftoa(-523.2, buf, 1);
	zassert_str_equal(buf, "-523.2");
}
