#include <zephyr/ztest.h>

ZTEST(test_test, test_functionality)
{
	zassert_equal(9, 10, "Expected result is 10");
}

ZTEST_SUIT(test_test_suit, test_test, NULL, NULL, NULL, NULL);
