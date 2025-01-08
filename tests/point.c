#include "zephyr/sys/util.h"
#include "zephyr/ztest_assert.h"
#include <point.h>

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>

LOG_MODULE_REGISTER(point_tests, LOG_LEVEL_INF);

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

	LOG_DBG("calc encoded len: %zu", len);

	/* Return error if buffer isn't correctly sized */
	zassert(sizeof(buf) > len, "buffer is not long enough");

	/* Encode */
	int ret = json_obj_encode_buf(example_json_payload_descr,
				      ARRAY_SIZE(example_json_payload_descr), &payload, buf,
				      sizeof(buf));

	zassert_ok(ret, "encode failed");

	/* Return the length if possible*/
	LOG_DBG("JSON encoded buffer: %s", buf);
}

struct float_value {
	struct json_obj_token value;
	int value_int;
};

static const struct json_obj_descr float_value_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct float_value, value, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM(struct float_value, value_int, JSON_TOK_NUMBER),
};

ZTEST(point_tests, json_float)
{
	char buf[128];

	struct float_value fv = {.value_int = -92781};

	char float_buffer[20];

	snprintf(float_buffer, sizeof(float_buffer), "%f", -123.23);

	fv.value.start = float_buffer;
	fv.value.length = strlen(float_buffer);

	int ret = json_obj_encode_buf(float_value_descr, ARRAY_SIZE(float_value_descr), &fv, buf,
				      sizeof(buf));

	zassert_ok(ret, "encode failed");

	LOG_DBG("Encoded float falue: %s", buf);
}

ZTEST(point_tests, encode_point_int)
{
	point tp = {.type = POINT_TYPE_TEMPERATURE};
	point_put_int(&tp, -32);

	char buf[128];

	int ret = point_json_encode(&tp, buf, sizeof(buf));
	zassert_ok(ret);

	char *exp =
		"{\"time\":\"\",\"type\":\"temp\",\"key\":\"\",\"data_type\":\"INT\",\"data\":-32}";
	zassert_str_equal(buf, exp, "did not get expected JSON data");

	LOG_DBG("Encoded data: %s", buf);
}

ZTEST(point_tests, encode_point_float)
{
	point tp = {.type = POINT_TYPE_TEMPERATURE};
	point_put_float(&tp, -32.23);

	char buf[128];

	int ret = point_json_encode(&tp, buf, sizeof(buf));
	zassert_ok(ret);

	char *exp =
		"{\"time\":\"\",\"type\":\"temp\",\"key\":\"\",\"data_type\":\"FLT\",\"data\":-32."
		"230000}";
	zassert_str_equal(buf, exp, "did not get expected JSON data");

	LOG_DBG("Encoded data: %s", buf);
}

ZTEST(point_tests, encode_point_string)
{
	point tp = {.type = POINT_TYPE_DESCRIPTION};
	point_put_string(&tp, "device #3");

	char buf[128];

	int ret = point_json_encode(&tp, buf, sizeof(buf));
	zassert_ok(ret);

	char *exp = "{\"time\":\"\",\"type\":\"description\",\"key\":\"\",\"data_type\":\"STR\","
		    "\"data\":\"device #3\"}";
	zassert_str_equal(buf, exp, "did not get expected JSON data");

	LOG_DBG("Encoded data: %s", buf);
}

point test_points[] = {
	{0, POINT_TYPE_TEMPERATURE, ""},
	{0, POINT_TYPE_TEMPERATURE, ""},
	{0, POINT_TYPE_DESCRIPTION, ""},
};

ZTEST(point_tests, encode_point_array)
{
	point_put_int(&test_points[0], -232);
	point_put_float(&test_points[1], -572.2923);
	point_put_string(&test_points[2], "device #4");

	char buf[512];

	int ret = point_json_encode_points(test_points, ARRAY_SIZE(test_points), buf, sizeof(buf));
	zassert_ok(ret);

	char exp[] = "[{\"time\":\"\",\"type\":\"temp\",\"key\":\"\",\"data_type\":\"INT\","
		     "\"data\":-232},{\"time\":\"\",\"type\":\"temp\",\"key\":\"\",\"data_type\":"
		     "\"FLT\",\"data\":-572.292297},{\"time\":\"\",\"type\":\"description\","
		     "\"key\":\"\",\"data_type\":\"STR\",\"data\":\"device #4\"}]";

	zassert_str_equal(exp, buf, "encoded string not correct");

	LOG_DBG("Encoded data: %s", buf);
}
