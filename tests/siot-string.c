#include "siot-string.h"
#include "zephyr/ztest_assert.h"
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(siot_string_tests, LOG_LEVEL_DBG);

ZTEST_SUITE(siot_string_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(siot_string_tests, ftoa)
{
	char buf[64];

	ftoa(53.243, buf, 3);
	zassert_str_equal(buf, "53.243");

	ftoa(53.243, buf, 1);
	zassert_str_equal(buf, "53.2");
}

ZTEST(siot_string_tests, ftoa_negative)
{
	char buf[64];

	ftoa(-523.2, buf, 1);
	zassert_str_equal(buf, "-523.2");
}

ZTEST(siot_string_tests, ftoa_round)
{
	char buf[64];

	ftoa(1.58, buf, 1);
	zassert_str_equal(buf, "1.6");

	ftoa(1.54, buf, 1);
	zassert_str_equal(buf, "1.5");

	ftoa(1.54, buf, 0);
	zassert_str_equal(buf, "2");

	ftoa(-1.58, buf, 0);
	zassert_str_equal(buf, "-1.6");
}
