#include "zephyr/ztest_assert.h"
#include <point.h>

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>

LOG_MODULE_REGISTER(point_tests);

ZTEST_SUITE(point_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(point_tests, test)
{
	zassert_equal(1, 1);
}

struct sensor2_value {
	int32_t x_value;
	int32_t y_value;
	int32_t z_value;
};

/**
 * @brief Main JSON payload
 *
 */
struct example_json_payload {
	uint32_t timestamp;
	int32_t sensor1_value;
	struct sensor2_value sensor2_value;
};

static const struct json_obj_descr sensor2_value_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct sensor2_value, x_value, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct sensor2_value, y_value, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct sensor2_value, z_value, JSON_TOK_NUMBER),
};

static const struct json_obj_descr example_json_payload_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct example_json_payload, timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct example_json_payload, sensor1_value, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct example_json_payload, sensor2_value, sensor2_value_descr),
};

ZTEST(point_tests, json_test)
{
	char buf[128];

	struct example_json_payload payload = {
		.timestamp = 1234,
		.sensor1_value = 112233,
		.sensor2_value =
			{
				.x_value = 1,
				.y_value = 2,
				.z_value = -9,
			},
	};

	/* Calculate the encoded length. (could be smaller) */
	ssize_t len = json_calc_encoded_len(example_json_payload_descr,
					    ARRAY_SIZE(example_json_payload_descr), &payload);

	LOG_INF("calc encoded len: %zu", len);

	/* Return error if buffer isn't correctly sized */
	zassert(sizeof(buf) > len, "buffer is not long enough");

	/* Encode */
	int ret = json_obj_encode_buf(example_json_payload_descr,
				      ARRAY_SIZE(example_json_payload_descr), &payload, buf,
				      sizeof(buf));

	zassert_ok(ret, "encode failed");

	/* Return the length if possible*/
	LOG_INF("JSON encoded buffer: %s", buf);
}

ZTEST(point_tests, encode)
{
	LOG_INF("Points encode tests");
	struct point_js test_point = {.time = "",
				      .type = POINT_TYPE_DESCRIPTION,
				      .key = "",
				      .data_type = "str",
				      .data = "test description"};

	char buf[128];

	int ret = point_json_encode(&test_point, buf, sizeof(buf));
	zassert_ok(ret);

	LOG_INF("Encoded data: %s", buf);
}
