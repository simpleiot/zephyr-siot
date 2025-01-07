#include <zephyr/ztest.h>

ZTEST_SUITE(test_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_tests, simple_test)
{
	zassert_equal(9, 10, "Expected result is 10");
}
