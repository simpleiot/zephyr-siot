#include <point.h>
#include <siot-string.h>
#include <timeconv.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(z_point, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(point_chan);

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

uint64_t now_ns;

uint64_t (*get_current_time_ns_cb)(void) = NULL;

void point_set_get_time_callback(uint64_t (*cb)(void))
{
	get_current_time_ns_cb = cb;
}

void point_init(point *p, const char *t, const char *k)
{
	strncpy(p->type, t, sizeof(p->type));
	strncpy(p->key, k, sizeof(p->key));
	if (get_current_time_ns_cb) {
		now_ns = get_current_time_ns_cb();
		p->time = now_ns;
	} else {
		p->time = 0ULL;
	}
}

int point_get_int(const point *p)
{
	return *((const int *)(p->data));
}

float point_get_float(const point *p)
{
	return *((const float *)(p->data));
}

void point_get_string(const point *p, char *dest, int len)
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

int point_data_len(const point *p)
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
int point_dump(const point *p, char *buf, size_t len)
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
		cnt = snprintf(buf + offset, remaining, "FLT: %f", (double)point_get_float(p));
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

	char temp_buf[RFC_3339_MAX_LEN];

	// print time if set
	if (p->time != 0) {
		if (timeconv_rfc3339_from_epoch_ns_utc(p->time, temp_buf, sizeof(temp_buf)) == 0) {
			cnt = snprintf(buf + offset, remaining, " time: %s", temp_buf);
		}
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


// struct point_js is now defined in point.h

#define POINT_JS_ARRAY_MAX POINT_BUFFER_COUNT

struct point_js_array {
	struct point_js points[POINT_JS_ARRAY_MAX];
	size_t len;
};

static const struct json_obj_descr point_js_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct point_js, t, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, k, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, dt, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, tm, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct point_js, d, JSON_TOK_OPAQUE)};

static const struct json_obj_descr point_js_array_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct point_js_array, points, POINT_JS_ARRAY_MAX, len,
				 point_js_descr, ARRAY_SIZE(point_js_descr)),
};

/* Escape special characters in a string for JSON encoding per RFC 8259.
 * Returns the number of characters written (excluding null terminator),
 * or -1 if the buffer is too small.
 */
static int json_escape_string(const char *src, char *dst, size_t dst_len)
{
	size_t di = 0;

	for (size_t si = 0; src[si] != '\0'; si++) {
		unsigned char c = (unsigned char)src[si];
		char esc_char = 0;

		// Check for characters that need simple escape sequences
		switch (c) {
		case '"':
			esc_char = '"';
			break;
		case '\\':
			esc_char = '\\';
			break;
		case '\n':
			esc_char = 'n';
			break;
		case '\r':
			esc_char = 'r';
			break;
		case '\t':
			esc_char = 't';
			break;
		case '\b':
			esc_char = 'b';
			break;
		case '\f':
			esc_char = 'f';
			break;
		}

		if (esc_char != 0) {
			// Simple escape sequence: \X
			if (di + 2 >= dst_len) {
				return -1;
			}
			dst[di++] = '\\';
			dst[di++] = esc_char;
		} else if (c < 0x20) {
			// Other control characters: \uXXXX
			if (di + 6 >= dst_len) {
				return -1;
			}
			dst[di++] = '\\';
			dst[di++] = 'u';
			dst[di++] = '0';
			dst[di++] = '0';
			dst[di++] = (c >> 4) < 10 ? '0' + (c >> 4) : 'a' + (c >> 4) - 10;
			dst[di++] = (c & 0xf) < 10 ? '0' + (c & 0xf) : 'a' + (c & 0xf) - 10;
		} else {
			// Regular character
			if (di + 1 >= dst_len) {
				return -1;
			}
			dst[di++] = c;
		}
	}
	dst[di] = '\0';
	return (int)di;
}


// point_js has pointers to strings, so the buf is used to store these strings
// Note: this functions assumes the input point will be valid for the duration of
// of the p_js lifecycle, as we are populating points to strings in the original
// p.
void point_to_point_js(point *p, struct point_js *p_js, char *d_buf, size_t d_buf_len, char *tm_buf,
		       size_t tm_buf_len)
{
	p_js->t = p->type;
	p_js->k = p->key;
	timeconv_rfc3339_from_epoch_ns_utc(p->time, tm_buf, tm_buf_len);
	p_js->tm.start = tm_buf;
	p_js->tm.length = strnlen(tm_buf, tm_buf_len);

	// Data type and value
	switch (p->data_type) {
	case POINT_DATA_TYPE_FLOAT:
		p_js->dt = POINT_DATA_TYPE_FLOAT_S;
		ftoa(point_get_float(p), d_buf, 4);
		p_js->d.start = d_buf;
		p_js->d.length = strnlen(d_buf, d_buf_len);
		break;
	case POINT_DATA_TYPE_INT:
		p_js->dt = POINT_DATA_TYPE_INT_S;
		itoa(point_get_int(p), d_buf, 10);
		p_js->d.start = d_buf;
		p_js->d.length = strnlen(d_buf, d_buf_len);
		break;
	case POINT_DATA_TYPE_STRING:
		p_js->dt = POINT_DATA_TYPE_STRING_S;
		// Ensure null termination for string data
		strncpy(d_buf, p->data, d_buf_len - 1);
		d_buf[d_buf_len - 1] = '\0';
		p_js->d.start = d_buf;
		p_js->d.length = strnlen(d_buf, d_buf_len);
		break;
	case POINT_DATA_TYPE_JSON:
		p_js->dt = POINT_DATA_TYPE_JSON_S;
		// JSON data needs quotes escaped for proper JSON encoding
		int esc_len = json_escape_string(p->data, d_buf, d_buf_len);
		if (esc_len < 0) {
			// Buffer too small, truncate
			strncpy(d_buf, p->data, d_buf_len - 1);
			d_buf[d_buf_len - 1] = '\0';
		}
		p_js->d.start = d_buf;
		p_js->d.length = strnlen(d_buf, d_buf_len);
		break;
	default:
		// For invalid data types, provide safe default values to prevent JSON encoding
		// failures
		p_js->dt = POINT_DATA_TYPE_STRING_S;
		strcpy(d_buf, "");
		p_js->d.start = d_buf;
		p_js->d.length = 0;
	}
}

int point_js_to_point(struct point_js *p_js, point *p)
{
	char buf[30];
	char time_buffer[RFC_3339_MAX_LEN];

	if (p_js->t == NULL || p_js->k == NULL) {
		LOG_ERR("Refusing to decode point with null type or key");
		return -1;
	}

	strncpy(p->type, p_js->t, sizeof(p->type));
	strncpy(p->key, p_js->k, sizeof(p->key));

	// Safely handle missing time (e.g., when invoked from CLI without a tm token)
	if (p_js->tm.start != NULL && p_js->tm.length > 0) {
		int cnt = MIN((int)sizeof(time_buffer) - 1, p_js->tm.length);
		memcpy(time_buffer, p_js->tm.start, cnt);
		time_buffer[cnt] = '\0';
		uint64_t parsed_time =
			timeconv_epoch_ns_from_rfc3339(time_buffer, sizeof(time_buffer));
		// Fall back to global time if parsing failed (returns 0)
		p->time = (parsed_time != 0) ? parsed_time : ((now_ns != 0) ? now_ns : 0ULL);
	} else {
		// Use global time provider instead of 0
		p->time = (now_ns != 0) ? now_ns : 0ULL;
	}

	if (strncmp(p_js->dt, POINT_DATA_TYPE_FLOAT_S, 3) == 0) {
		p->data_type = POINT_DATA_TYPE_FLOAT;
		// null terminate string so we can scan it
		int cnt = MIN(p_js->d.length, sizeof(buf) - 1);
		memcpy(buf, p_js->d.start, cnt);
		buf[cnt] = 0;
		*(float *)p->data = atof(buf);
	} else if (strncmp(p_js->dt, POINT_DATA_TYPE_INT_S, 3) == 0) {
		p->data_type = POINT_DATA_TYPE_INT;
		// null terminate string so we can scan it
		int cnt = MIN(p_js->d.length, sizeof(buf) - 1);
		memcpy(buf, p_js->d.start, cnt);
		buf[cnt] = 0;
		*(int *)p->data = atoi(buf);
	} else if (strncmp(p_js->dt, POINT_DATA_TYPE_STRING_S, 3) == 0) {
		p->data_type = POINT_DATA_TYPE_STRING;
		int cnt = MIN(p_js->d.length, sizeof(p->data) - 1);
		memcpy(p->data, p_js->d.start, cnt);
		// make sure string is null terminated
		p->data[cnt] = 0;
	} else if (strncmp(p_js->dt, POINT_DATA_TYPE_JSON_S, 3) == 0) {
		p->data_type = POINT_DATA_TYPE_JSON;
		int cnt = MIN(p_js->d.length, sizeof(p->data) - 1);
		memcpy(p->data, p_js->d.start, cnt);
		// make sure string is null terminated
		p->data[cnt] = 0;
	} else {
		p->data_type = POINT_DATA_TYPE_UNKNOWN;
		p->data[0] = 0;
		return -1;
	}

	return 0;
}

// all of the point_js fields MUST be filled in or the encoder will crash
int point_json_encode(point *p, char *buf, size_t len)
{
	struct point_js p_js = {};

	// Buffer needs extra space for JSON escaping (quotes become \")
	char data_buf[sizeof(((point *)0)->data) * 2];
	char time_buf[RFC_3339_MAX_LEN];
	point_to_point_js(p, &p_js, data_buf, sizeof(data_buf), time_buf, sizeof(time_buf));

	/* Calculate the encoded length. (could be smaller) */
	ssize_t enc_len = json_calc_encoded_len(point_js_descr, ARRAY_SIZE(point_js_descr), &p_js);
	if (enc_len > len) {
		return -ENOMEM;
	}

	return json_obj_encode_buf(point_js_descr, ARRAY_SIZE(point_js_descr), &p_js, buf, len);
}

int point_json_decode(char *json, size_t json_len, point *p)
{
	struct point_js p_js = {};
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

// Buffer needs extra space for JSON escaping (quotes become \")
// Place large buffers in external RAM on ESP32 with PSRAM, otherwise use regular BSS
#if defined(CONFIG_ESP_SPIRAM)
__attribute__((section(
	".ext_ram.bss"))) static char data_buf[POINT_JS_ARRAY_MAX][sizeof(((point *)0)->data) * 2];
__attribute__((section(".ext_ram.bss"))) static char time_buf[POINT_JS_ARRAY_MAX][RFC_3339_MAX_LEN];
#else
static char data_buf[POINT_JS_ARRAY_MAX][sizeof(((point *)0)->data) * 2];
static char time_buf[POINT_JS_ARRAY_MAX][RFC_3339_MAX_LEN];
#endif

static K_MUTEX_DEFINE(points_json_encode_mutex);

int points_json_encode(point *pts_in, int count, char *buf, size_t len)
{

	struct point_js_array pts_out = {.len = 0};

	if (count > POINT_JS_ARRAY_MAX) {
		return -ENOMEM;
	}

	k_mutex_lock(&points_json_encode_mutex, K_FOREVER);

	for (int i = 0; i < count; i++) {
		// make sure it is not an empty point
		if (pts_in[i].type[0] != 0) {
			// Skip points with invalid data types to prevent JSON encoding failures
			if (pts_in[i].data_type >= POINT_DATA_TYPE_END ||
			    pts_in[i].data_type == POINT_DATA_TYPE_UNKNOWN) {
				LOG_DBG("Skipping point with invalid data_type: %s:%s, type:%i",
					pts_in[i].type, pts_in[i].key, pts_in[i].data_type);
				continue;
			}
			point_to_point_js(&pts_in[i], &pts_out.points[pts_out.len],
					  data_buf[pts_out.len], sizeof(data_buf[pts_out.len]),
					  time_buf[pts_out.len], sizeof(time_buf[pts_out.len]));

			pts_out.len++;
		}
	}

	int ret = json_arr_encode_buf(point_js_array_descr, &pts_out, buf, len);
	k_mutex_unlock(&points_json_encode_mutex);
	return ret;
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

static int handle_sendpoint(const struct shell *shell, size_t argc, char **argv)
{
	/* Handle shell command: "p <type> <key> <INT|FLT|STR> <data>" */
	if (argc < 5) {
		shell_print(shell, "Usage: p <type> <key> <INT|FLT|STR> <data>");
		return -1;
	}

	struct point_js p_js = {};

	p_js.t = argv[1];
	p_js.k = argv[2];
	p_js.dt = argv[3];
	p_js.d.start = argv[4];
	p_js.d.length = strlen(argv[4]);

	point p;
	int ret = point_js_to_point(&p_js, &p);

	if (ret != 0) {
		shell_print(shell, "Invalid point");
		return -1;
	}

	zbus_chan_pub(&point_chan, &p, K_MSEC(500));

	return 0;
}

static int handle_show_points(const struct shell *shell, size_t argc, char **argv)
{
	point *points = get_web_points();
	int count = get_web_points_count();
	const char *filter_type = NULL;

	// Check if a filter type was provided
	if (argc > 2) {
		filter_type = argv[2];
	}

	if (filter_type == NULL || strcmp(filter_type, "all") == 0) {
		shell_print(shell, "Showing all points:");
	} else {
		shell_print(shell, "Showing points of type '%s':", filter_type);
	}

	// Note: We can't lock the mutex here as it's defined in web.c
	// The show will display a snapshot of the points at this moment
	for (int i = 0; i < count; i++) {
		if (points[i].type[0] != 0) {
			// Apply filter if specified
			if (filter_type != NULL && strcmp(filter_type, "all") != 0) {
				if (strcmp(points[i].type, filter_type) != 0) {
					continue;
				}
			}
			char buf[200];
			point_dump(&points[i], buf, sizeof(buf));
			shell_print(shell, "%s", buf);
		}
	}

	return 0;
}

static int handle_help(const struct shell *shell, size_t argc, char **argv)
{
	(void)argc;
	(void)argv;

	shell_print(shell, "Point Commands:");
	shell_print(shell, "  p send <type> <key> <INT|FLT|STR> <data>  - Send a point");
	shell_print(shell, "  p show [all|<type>]  - Show points (all or filtered by type)");
	shell_print(shell, "  p help  - Show this help message");
	shell_print(shell, "");
	shell_print(shell, "Data Types:");
	shell_print(shell, "  INT - Integer values");
	shell_print(shell, "  FLT - Float values");
	shell_print(shell, "  STR - String values");
	shell_print(shell, "");
	shell_print(shell, "Examples:");
	shell_print(shell, "  p send temp 0 FLT 25.5");
	shell_print(shell, "  p send description 0 STR \"My Device\"");
	shell_print(shell, "  p send count 0 INT 42");
	shell_print(shell, "  p show all");
	shell_print(shell, "  p show temp");

	return 0;
}

static int handle_point_command(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_print(shell, "Usage: p <command> [args...]");
		shell_print(shell, "Commands: send, show, help");
		shell_print(shell, "Use 'p help' for detailed usage information");
		return -1;
	}

	// Handle subcommands
	if (strcmp(argv[1], "send") == 0) {
		if (argc < 6) {
			shell_print(shell, "Usage: p send <type> <key> <INT|FLT|STR> <data>");
			return -1;
		}
		// Create a new argv array starting from the type argument
		char *send_argv[] = {argv[0], argv[2], argv[3], argv[4], argv[5]};
		return handle_sendpoint(shell, 5, send_argv);
	} else if (strcmp(argv[1], "show") == 0) {
		return handle_show_points(shell, argc, argv);
	} else if (strcmp(argv[1], "help") == 0) {
		return handle_help(shell, argc, argv);
	} else {
		shell_print(shell, "Unknown command: %s", argv[1]);
		shell_print(shell, "Use 'p help' for usage information");
		return -1;
	}
}

SHELL_CMD_REGISTER(p, NULL, "Point commands (send, show, help)", handle_point_command);
