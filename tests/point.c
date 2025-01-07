#include "zephyr/ztest_assert.h"
#include <point.h>

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(point_tests);

ZTEST_SUITE(point_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(point_tests, test_test)
{
	zassert_equal(1, 1);
}

ZTEST(point_tests, encode)
{
	point_js test_point = {.type = POINT_TYPE_DESCRIPTION, .data = "test description"};
	char buf[128];

	int ret = point_json_encode(&test_point, buf, sizeof(buf));
	zassert_ok(ret);

	LOG_INF("Encoded data: %s", buf);
}
