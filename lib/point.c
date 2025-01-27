#include <point.h>
#include <siot-string.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(z_point, LOG_LEVEL_DBG);

const point_def point_def_description = {POINT_TYPE_DESCRIPTION, POINT_DATA_TYPE_STRING};
const point_def point_def_staticip = {POINT_TYPE_STATICIP, POINT_DATA_TYPE_INT};
const point_def point_def_address = {POINT_TYPE_ADDRESS, POINT_DATA_TYPE_STRING};
const point_def point_def_netmask = {POINT_TYPE_NETMASK, POINT_DATA_TYPE_STRING};
const point_def point_def_gateway = {POINT_TYPE_GATEWAY, POINT_DATA_TYPE_STRING};
const point_def point_def_metric_sys_cpu_percent = {POINT_TYPE_METRIC_SYS_CPU_PERCENT,
						    POINT_DATA_TYPE_FLOAT};
const point_def point_def_uptime = {POINT_TYPE_UPTIME, POINT_DATA_TYPE_INT};
const point_def point_def_temperature = {POINT_TYPE_TEMPERATURE, POINT_DATA_TYPE_FLOAT};
const point_def point_def_board = {POINT_TYPE_BOARD, POINT_DATA_TYPE_STRING};
const point_def point_def_boot_count = {POINT_TYPE_BOOT_COUNT, POINT_DATA_TYPE_INT};

void point_set_type(point *p, const char *t)
{
	strncpy(p->type, t, sizeof(p->type));
}

void point_set_key(point *p, const char *k)
{
	strncpy(p->key, k, sizeof(p->key));
}

void point_set_type_key(point *p, const char *t, const char *k)
{
	strncpy(p->type, t, sizeof(p->type));
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

void point_put_int(point *p, const int v)
{
	p->data_type = POINT_DATA_TYPE_INT;
	*((int *)(p->data)) = v;
}

void point_put_float(point *p, const float v)
{
	p->data_type = POINT_DATA_TYPE_FLOAT;
	*((float *)(p->data)) = v;
}

void point_put_string(point *p, const char *v)
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

// point_dump generates a human readable description of the point
// useful for logging or debugging.
// you must pass in a buf that gets populated with the description
// returns amount of space used in buffer
int point_dump(point *p, char *buf, size_t len)
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
		cnt = snprintf(buf + offset, remaining, "INT: %i", point_get_int(p));
		offset += cnt;
		remaining -= cnt;
		break;
	case POINT_DATA_TYPE_FLOAT:
		cnt = snprintf(buf + offset, remaining, "INT: %f", (double)point_get_float(p));
		offset += cnt;
		remaining -= cnt;
		break;
	case POINT_DATA_TYPE_STRING:
		cnt = snprintf(buf + offset, remaining, "STR: %s", p->data);
		offset += cnt;
		remaining -= cnt;
		break;
	case POINT_DATA_TYPE_UNKNOWN:
		cnt = snprintf(buf + offset, remaining, "unknown point type");
		offset += cnt;
		remaining -= cnt;
		break;
	default:
		cnt = snprintf(buf + offset, remaining, "invalid point type");
		offset += cnt;
		remaining -= cnt;
		break;
	}

	return offset;
}

// points_dump takes an array of points and dumps descriptions into buf
// all strings in pts must be initialized to null strings
int points_dump(point *pts, size_t pts_len, char *buf, size_t buf_len)
{
	int offset = 0;
	int remaining = buf_len - 1; // leave space for null term
	int cnt;

	if (buf_len <= 0) {
		return -ENOMEM;
	}

	// null terminate string in case there are no points
	buf[0] = 0;

	for (int i = 0; i < pts_len; i++) {
		if (pts[i].type[0] != 0) {
			if (remaining < 6) {
				return offset;
			}
			strncpy(buf + offset, "\r\n\t- ", remaining);
			offset += 5;
			remaining -= 5;
			cnt = point_dump(&pts[i], buf + offset, remaining);
			offset += cnt;
			remaining -= cnt;
		}
	}

	return offset;
}

// When transmitting points over web APIs using JSON, we encode
// then using all text fields. The JSON encoder cannot encode fixed
// length char fields, so we have use pointers for now.
struct point_js {
	char *t;                 // type
	char *k;                 // key
	char *dt;                // datatype
	struct json_obj_token d; // data
};

#define POINT_JS_ARRAY_MAX 40

struct point_js_array {
	struct point_js points[POINT_JS_ARRAY_MAX];
	size_t len;
};

static const struct json_obj_descr point_js_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct point_js, t, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, k, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, dt, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, d, JSON_TOK_OPAQUE)};

static const struct json_obj_descr point_js_array_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct point_js_array, points, POINT_JS_ARRAY_MAX, len,
				 point_js_descr, ARRAY_SIZE(point_js_descr)),
};

// point_js has pointers to strings, so the buf is used to store these strings
// Note: this functions assumes the input point will be valid for the duration of
// of the p_js lifecycle, as we are populating points to strings in the original
// p.
void point_to_point_js(point *p, struct point_js *p_js, char *buf, size_t buf_len)
{
	p_js->t = p->type;
	p_js->k = p->key;

	switch (p->data_type) {
	case POINT_DATA_TYPE_FLOAT:
		p_js->dt = POINT_DATA_TYPE_FLOAT_S;
		// snprintf has caused problems in the past, so use a leaner custom version
		ftoa(point_get_float(p), buf, 4);
		p_js->d.start = buf;
		p_js->d.length = strlen(buf);
		break;
	case POINT_DATA_TYPE_INT:
		p_js->dt = POINT_DATA_TYPE_INT_S;
		itoa(point_get_int(p), buf, 10);
		p_js->d.start = buf;
		p_js->d.length = strlen(buf);
		break;
	case POINT_DATA_TYPE_STRING:
		p_js->dt = POINT_DATA_TYPE_STRING_S;
		strncpy(buf, p->data, buf_len);
		p_js->d.start = buf;
		p_js->d.length = strlen(buf);
		break;
	default:
		p_js->d.start = NULL;
		p_js->d.length = 0;
	}
}

void point_js_to_point(struct point_js *p_js, point *p)
{
	char buf[30];
	p->time = 0;

	if (p_js->t == NULL || p_js->k == NULL) {
		LOG_ERR("Refusing to decode point with null type or key");
	}

	strncpy(p->type, p_js->t, sizeof(p->type));
	strncpy(p->key, p_js->k, sizeof(p->key));

	if (strcmp(p_js->dt, POINT_DATA_TYPE_FLOAT_S) == 0) {
		p->data_type = POINT_DATA_TYPE_FLOAT;
		// null terminate string so we can scan it
		int cnt = MIN(p_js->d.length, sizeof(buf) - 1);
		memcpy(buf, p_js->d.start, cnt);
		buf[cnt] = 0;
		*(float *)p->data = atof(buf);
	} else if (strcmp(p_js->dt, POINT_DATA_TYPE_INT_S) == 0) {
		p->data_type = POINT_DATA_TYPE_INT;
		// null terminate string so we can scan it
		int cnt = MIN(p_js->d.length, sizeof(buf) - 1);
		memcpy(buf, p_js->d.start, cnt);
		buf[cnt] = 0;
		*(int *)p->data = atoi(buf);
	} else if (strcmp(p_js->dt, POINT_DATA_TYPE_STRING_S) == 0) {
		p->data_type = POINT_DATA_TYPE_STRING;
		int cnt = MIN(p_js->d.length, sizeof(p->data) - 1);
		memcpy(p->data, p_js->d.start, cnt);
		// make sure string is null terminated
		p->data[cnt] = 0;
	} else {
		p->data_type = POINT_DATA_TYPE_UNKNOWN;
		p->data[0] = 0;
	}
}

// all of the point_js fields MUST be filled in or the encoder will crash
int point_json_encode(point *p, char *buf, size_t len)
{
	struct point_js p_js;

	char data_buf[20];

	point_to_point_js(p, &p_js, data_buf, sizeof(data_buf));

	/* Calculate the encoded length. (could be smaller) */
	ssize_t enc_len = json_calc_encoded_len(point_js_descr, ARRAY_SIZE(point_js_descr), &p_js);
	if (enc_len > len) {
		return -ENOMEM;
	}

	return json_obj_encode_buf(point_js_descr, ARRAY_SIZE(point_js_descr), &p_js, buf, len);
}

int point_json_decode(char *json, size_t json_len, point *p)
{
	struct point_js p_js;
	int ret = json_obj_parse(json, json_len, point_js_descr, ARRAY_SIZE(point_js_descr), &p_js);
	if (ret < 0) {
		return ret;
	}

	if (p_js.t == NULL || p_js.k == NULL) {
		LOG_ERR("Invalid JSON, does not have type or key");
		return -200;
	}

	point_js_to_point(&p_js, p);
	return 0;
}

int points_json_encode(point *pts_in, int count, char *buf, size_t len)
{
	// buffers for data types and fields
	char data_buf[POINT_JS_ARRAY_MAX][20];
	struct point_js_array pts_out = {.len = 0};

	if (count > POINT_JS_ARRAY_MAX) {
		return -ENOMEM;
	}

	for (int i = 0; i < count; i++) {
		// make sure it is not an empty point
		if (pts_in[i].type[0] != 0) {
			point_to_point_js(&pts_in[i], &pts_out.points[pts_out.len],
					  data_buf[pts_out.len], sizeof(data_buf[pts_out.len]));
			pts_out.len++;
		}
	}

	return json_arr_encode_buf(point_js_array_descr, &pts_out, buf, len);
}

// returns the number of points decoded, or less than 0 for error
int points_json_decode(char *json, size_t json_len, point *pts, size_t p_cnt)
{
	struct point_js_array pts_js;

	int ret = json_arr_parse(json, json_len, point_js_array_descr, &pts_js);
	if (ret != 0) {
		return ret;
	}

	if (pts_js.len > p_cnt) {
		LOG_ERR("Points array decode, decoded more points than target array: %zu",
			pts_js.len);
	}

	int len = MIN(p_cnt, pts_js.len);
	int i;
	for (i = 0; i < len; i++) {
		point_js_to_point(&pts_js.points[i], &pts[i]);
	}

	return i;
}

// pts must be initialized and not have random data in the string fields
int points_merge(point *pts, size_t pts_len, point *p)
{
	// look for existing points
	int empty_i = -1;

	// make sure key is set to "0" if blank
	if (p->key[0] == 0) {
		strcpy(p->key, "0");
	}

	for (int i = 0; i < pts_len; i++) {
		if (pts[i].type[0] == 0) {
			if (empty_i < 0) {
				empty_i = i;
			}
			continue;
		} else if (pts[i].data_type == POINT_DATA_TYPE_UNKNOWN ||
			   pts[i].data_type >= POINT_DATA_TYPE_END) {
			LOG_ERR("not merging unknown point type: %s:%s, type:%i", pts[i].type,
				pts[i].key, pts[i].data_type);
			continue;
		} else if (strncmp(pts[i].type, p->type, sizeof(p->type)) == 0 &&
			   strncmp(pts[i].key, p->key, sizeof(p->key)) == 0) {
			// we have a match
			pts[i] = *p;
			return 0;
		}
	}

	// need to add a new point
	if (empty_i >= 0) {
		pts[empty_i] = *p;
		return 0;
	}

	return -ENOMEM;
}
