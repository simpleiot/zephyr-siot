#include "point.h"

#include <string.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>

LOG_MODULE_REGISTER(z_point, LOG_LEVEL_DBG);

void point_set_type(point *p, char *t)
{
	strncpy(p->type, t, sizeof(p->type));
}

void point_set_key(point *p, char *k)
{
	strncpy(p->key, k, sizeof(p->key));
}

int point_get_int(point *p)
{
	return *((int *)(p->data));
}

float point_get_float(point *p)
{
	return *((float *)(p->data));
}

void point_get_string(point *p, char *dest, int len)
{
	strncpy(dest, p->data, len);
}

void point_put_int(point *p, int v)
{
	p->data_type = POINT_DATA_TYPE_INT;
	*((int *)(p->data)) = v;
}

void point_put_float(point *p, float v)
{
	p->data_type = POINT_DATA_TYPE_FLOAT;
	*((float *)(p->data)) = v;
}

void point_put_string(point *p, char *v)
{
	p->data_type = POINT_DATA_TYPE_STRING;
	strncpy(p->data, v, sizeof(p->data));
}

int point_data_len(point *p)
{
	switch (p->data_type) {
	case POINT_DATA_TYPE_INT:
	case POINT_DATA_TYPE_FLOAT:
		return 4;
	case POINT_DATA_TYPE_STRING:
		return strnlen(p->data, sizeof(p->data) - 1) + 1;
	}

	return 0;
}

// point_description generates a human readable description of the point
// useful for logging or debugging.
// you must pass in a buf that gets populated with the description
int point_description(point *p, char *buf, int len)
{
	int offset = 0;
	int remaining = len - 1; // leave space for null term

	if (remaining <= 0) {
		LOG_ERR("Buffer is too small, recommend 40 characters");
		return -1;
	}

	int cnt = snprintf(buf + offset, remaining, "%s", p->type);
	offset += cnt;
	remaining -= cnt;

	if (strlen(p->key) > 0) {
		cnt = snprintf(buf + offset, remaining, ".%s: ", p->key);
		offset += cnt;
		remaining -= cnt;
	} else {
		cnt = snprintf(buf + offset, remaining, ": ");
		offset += cnt;
		remaining -= cnt;
	}

	switch (p->data_type) {
	case POINT_DATA_TYPE_INT:
		cnt = snprintf(buf + offset, remaining, "%i", point_get_int(p));
		offset += cnt;
		remaining -= cnt;
		break;
	case POINT_DATA_TYPE_FLOAT:
		cnt = snprintf(buf + offset, remaining, "%f", (double)point_get_float(p));
		offset += cnt;
		remaining -= cnt;
		break;
	case POINT_DATA_TYPE_STRING:
		cnt = snprintf(buf + offset, remaining, "%s", p->data);
		offset += cnt;
		remaining -= cnt;
		break;
	default:
		LOG_ERR("Invalid point data type: %i", p->data_type);
	}

	return offset;
}

// When transmitting points over web APIs using JSON, we encode
// then using all text fields. The JSON encoder cannot encode fixed
// length char fields, so we have use pointers for now.
struct point_js {
	char *time;
	char *type;
	char *key;
	char *data_type;
	struct json_obj_token data;
};

static const struct json_obj_descr point_js_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct point_js, time, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, type, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, key, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, data_type, JSON_TOK_STRING),
	// We use float for the data field as it does not put quotes around
	// the value in the string, and we have to manually populate it anyway.
	JSON_OBJ_DESCR_PRIM(struct point_js, data, JSON_TOK_FLOAT)};

// all of the point_js fields MUST be filled in or the encoder will crash
int point_json_encode(point *p, char *buf, size_t len)
{
	struct point_js p_js = {
		.time = "",
		.type = p->type,
		.key = p->key,
		.data_type = "",
	};

	char data_buf[20];

	switch (p->data_type) {
	case POINT_DATA_TYPE_FLOAT:
		p_js.data_type = POINT_DATA_TYPE_FLOAT_S;
		snprintf(data_buf, sizeof(data_buf), "%f", (double)point_get_float(p));
		p_js.data.start = data_buf;
		p_js.data.length = strlen(data_buf);
		break;
	case POINT_DATA_TYPE_INT:
		p_js.data_type = POINT_DATA_TYPE_INT_S;
		snprintf(data_buf, sizeof(data_buf), "%i", point_get_int(p));
		p_js.data.start = data_buf;
		p_js.data.length = strlen(data_buf);
		break;
	case POINT_DATA_TYPE_STRING:
		p_js.data_type = POINT_DATA_TYPE_STRING_S;
		snprintf(data_buf, sizeof(data_buf), "\"%s\"", p->data);
		p_js.data.start = data_buf;
		p_js.data.length = strlen(data_buf);
		break;
	default:
		p_js.data.start = NULL;
		p_js.data.length = 0;
	}

	/* Calculate the encoded length. (could be smaller) */
	ssize_t enc_len = json_calc_encoded_len(point_js_descr, ARRAY_SIZE(point_js_descr), &p_js);
	if (enc_len > len) {
		return -ENOMEM;
	}

	return json_obj_encode_buf(point_js_descr, ARRAY_SIZE(point_js_descr), &p_js, buf, len);
}

int point_json_decode(char *json, size_t json_len, point *p)
{
	return json_obj_parse(json, json_len, point_js_descr, ARRAY_SIZE(point_js_descr), &p);
}
