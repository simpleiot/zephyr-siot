#include "zephyr/sys/util.h"
#include "zephyr/ztest_assert.h"
#include <point.h>

#include <zephyr/data/json.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(point_tests, LOG_LEVEL_DBG);

point test_points[] = {
	{0, POINT_TYPE_METRIC_SYS_CPU_PERCENT, ""},
	{0, POINT_TYPE_TEMPERATURE, ""},
	{0, POINT_TYPE_DESCRIPTION, ""},
};

void *init_test_points(void)
{
	point_put_int(&test_points[0], -232);
	point_put_float(&test_points[1], -572.2);
	point_put_string(&test_points[2], "device #4");

	return NULL;
}

char test_point1_json[] = "{\"t\":\"temp\",\"k\":\"\","
			  "\"dt\":\"INT\",\"d\":\"-32\"}";

char test_point2_json[] = "{\"t\":\"temp\",\"k\":\"\","
			  "\"dt\":\"FLT\",\"d\":\"-32.2000\"}";

char test_point3_json[] = "{\"t\":\"description\",\"k\":\"\",\"dt\":\"STR\","
			  "\"d\":\"device #3\"}";

char test_point_all_json[] = "[{\"t\":\"metricSysCPUPercent\",\"k\":\"\",\"dt\":"
			     "\"INT\",\"d\":\"-232\"},{\"t\":\"temp\",\"k\":\"\","
			     "\"dt\":\"FLT\",\"d\":\"-572.2000\"},{\"t\":"
			     "\"description\",\"k\":\"\",\"dt\":\"STR\",\"d\":\"device #4\"}]";

ZTEST_SUITE(point_tests, NULL, init_test_points, NULL, NULL, NULL);

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

	LOG_DBG("EXP: %s", test_point1_json);
	LOG_DBG("GOT: %s", buf);

	zassert_str_equal(buf, test_point1_json, "did not get expected JSON data");

	LOG_DBG("Encoded data: %s", buf);
}

ZTEST(point_tests, encode_point_float)
{
	point tp = {.type = POINT_TYPE_TEMPERATURE};
	point_put_float(&tp, -32.2);

	char buf[128];

	int ret = point_json_encode(&tp, buf, sizeof(buf));
	zassert_ok(ret);

	LOG_DBG("Encoded data: %s", buf);

	zassert_str_equal(buf, test_point2_json, "did not get expected JSON data");
}

ZTEST(point_tests, encode_point_string)
{
	point tp = {.type = POINT_TYPE_DESCRIPTION};
	point_put_string(&tp, "device #3");

	char buf[128];

	int ret = point_json_encode(&tp, buf, sizeof(buf));
	zassert_ok(ret);

	zassert_str_equal(buf, test_point3_json, "did not get expected JSON data");

	LOG_DBG("Encoded data: %s", buf);
}

ZTEST(point_tests, encode_point_array)
{
	char buf[512];

	int ret = points_json_encode(test_points, ARRAY_SIZE(test_points), buf, sizeof(buf));
	zassert_ok(ret);

	zassert_str_equal(buf, test_point_all_json, "encoded string not correct");
}

ZTEST(point_tests, merge)
{
	point pts[5] = {0};

	int ret = points_merge(pts, ARRAY_SIZE(pts), &test_points[0]);
	zassert_ok(ret);

	point p = pts[0];
	point_put_int(&p, 55);

	ret = points_merge(pts, ARRAY_SIZE(pts), &p);
	zassert_ok(ret);

	zassert_not_equal(point_get_int(&p), point_get_int(&test_points[0]));

	zassert_equal(55, point_get_int(&pts[0]));

	ret = points_merge(pts, ARRAY_SIZE(pts), &test_points[1]);
	zassert_ok(ret);

	zassert_equal((float)-572.2, point_get_float(&pts[1]));
}

ZTEST(point_tests, decode_point_int)
{
	point p;

	// JSON decoding seems to destroy the JSON data, so make a copy of it first
	char buf[128];
	strncpy(buf, test_point1_json, sizeof(buf));

	int ret = point_json_decode(buf, sizeof(test_point1_json), &p);
	zassert(ret >= 0, "decode returned error");

	point_dump(&p, buf, sizeof(buf));

	zassert(point_get_int(&p) == -32, "point value is not -32");
	zassert_str_equal(p.type, POINT_TYPE_TEMPERATURE, "point type is not correct");
}

ZTEST(point_tests, decode_point_float)
{
	point p;

	// JSON decoding seems to destroy the JSON data, so make a copy of it first
	char buf[128];
	strncpy(buf, test_point2_json, sizeof(buf));

	int ret = point_json_decode(buf, sizeof(test_point2_json), &p);
	zassert(ret >= 0, "decode returned error");

	point_dump(&p, buf, sizeof(buf));
	LOG_DBG("Point: %s", buf);

	zassert(point_get_float(&p) == (float)-32.2, "point value is not -32.2");
	zassert_str_equal(p.type, POINT_TYPE_TEMPERATURE, "point type is not correct");
}

ZTEST(point_tests, decode_point_string)
{
	point p;

	// JSON decoding seems to destroy the JSON data, so make a copy of it first
	char buf[128];
	strncpy(buf, test_point3_json, sizeof(buf));

	int ret = point_json_decode(buf, sizeof(test_point3_json), &p);
	zassert(ret >= 0, "decode returned error");

	point_dump(&p, buf, sizeof(buf));
	LOG_DBG("Point: %s", buf);

	zassert_str_equal(p.data, "device #3");
	zassert_str_equal(p.type, POINT_TYPE_DESCRIPTION, "point type is not correct");
}

ZTEST(point_tests, decode_point_array)
{
	char buf[256];
	strncpy(buf, test_point_all_json, sizeof(buf));

	point pts[5];

	int ret = points_json_decode(buf, sizeof(test_point_all_json), pts, ARRAY_SIZE(pts));
	zassert(ret == 3, "did not decode 3 points");

	// spot check a few points
	zassert(point_get_int(&pts[0]) == -232, "point 0 not correct value");
	zassert(point_get_float(&pts[1]) == (float)-572.2, "point 1 not correct value");
	zassert_str_equal(pts[2].data, "device #4");
}

ZTEST(point_tests, decode_point_array_size_one)
{
	char buf[] = "[{\"time\":\"\",\"t\":\"staticIP\",\"k\":\"0\",\"dt\":\"INT\","
		     "\"d\":\"1\"}]";
	point pts[5];

	int ret = points_json_decode(buf, sizeof(test_point_all_json), pts, ARRAY_SIZE(pts));
	zassert(ret == 1, "did not decode 1 points");

	zassert(point_get_int(&pts[0]) == 1, "point 0 not correct value");
}

char test_point1_no_datatype_json[] =
	"{\"tm\":\"\",\"t\":\"temp\",\"k\":\"\",\"dt\":\"\",\"d\":\"-32\"}";

ZTEST(point_tests, decode_point_with_no_datatype)
{

	point p;

	// JSON decoding seems to destroy the JSON data, so make a copy of it first
	char buf[128];
	strncpy(buf, test_point1_no_datatype_json, sizeof(buf));

	int ret = point_json_decode(buf, sizeof(test_point1_no_datatype_json), &p);
	zassert(ret >= 0, "decode returned error");

	point_dump(&p, buf, sizeof(buf));
	LOG_DBG("Point: %s", buf);

	zassert_str_equal(p.type, POINT_TYPE_TEMPERATURE, "point type is not correct");
}

ZTEST(point_tests, merge_with_no_datatype)
{
	point pts[5] = {0};

	int ret = points_merge(pts, ARRAY_SIZE(pts), &test_points[0]);
	zassert_ok(ret);

	point p = test_points[0];
	point_put_int(&p, 55);
	// set the data_type to ""
	p.data_type = 0;

	ret = points_merge(pts, ARRAY_SIZE(pts), &p);
	zassert_ok(ret);

	zassert_not_equal(point_get_int(&p), point_get_int(&test_points[0]));

	zassert_equal(55, point_get_int(&pts[0]));

	char buf[64];
	points_dump(pts, ARRAY_SIZE(pts), buf, sizeof(buf));
	LOG_DBG("pts: %s", buf);
}
