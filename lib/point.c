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

static const struct json_obj_descr point_js_descr[] = {
	JSON_OBJ_DESCR_PRIM(point_js, time, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(point_js, type, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(point_js, key, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(point_js, data_type, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(point_js, data, JSON_TOK_STRING)};

int point_json_encode(point_js *p, char *buf, size_t len)
{
	return json_obj_encode_buf(point_js_descr, ARRAY_SIZE(point_js_descr), &p, buf, len);
}

int point_json_decode(char *json, size_t json_len, point_js *p)
{
	return json_obj_parse(json, json_len, point_js_descr, ARRAY_SIZE(point_js_descr), &p);
}
