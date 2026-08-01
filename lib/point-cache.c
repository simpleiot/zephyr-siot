#include <point-cache.h>
#include <point.h>
#include <timeconv.h>

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#ifdef CONFIG_SIOT_POINT_SHELL
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#endif

LOG_MODULE_REGISTER(z_point_cache, CONFIG_SIOT_LOG_LEVEL);

ZBUS_CHAN_DECLARE(point_chan);

BUILD_ASSERT(CONFIG_SIOT_POINT_CACHE_SIZE <= 47,
	     "SIOT_POINT_CACHE_SIZE must not exceed POINT_JS_ARRAY_MAX, or "
	     "points_json_encode refuses to encode the cache");

static K_MUTEX_DEFINE(cache_lock);
static point cache[CONFIG_SIOT_POINT_CACHE_SIZE];

int point_cache_merge(point *p)
{
	k_mutex_lock(&cache_lock, K_FOREVER);
	int ret = points_merge(cache, ARRAY_SIZE(cache), p);
	k_mutex_unlock(&cache_lock);
	return ret;
}

int point_cache_count(void)
{
	int count = 0;

	k_mutex_lock(&cache_lock, K_FOREVER);
	for (int i = 0; i < ARRAY_SIZE(cache); i++) {
		if (cache[i].type[0] != 0) {
			count++;
		}
	}
	k_mutex_unlock(&cache_lock);

	return count;
}

int point_cache_foreach(int (*cb)(point *p, void *user_data), void *user_data)
{
	int ret = 0;

	k_mutex_lock(&cache_lock, K_FOREVER);
	for (int i = 0; i < ARRAY_SIZE(cache); i++) {
		if (cache[i].type[0] == 0) {
			continue;
		}
		ret = cb(&cache[i], user_data);
		if (ret != 0) {
			break;
		}
	}
	k_mutex_unlock(&cache_lock);

	return ret;
}

int point_cache_json_encode(char *buf, size_t buf_len)
{
	k_mutex_lock(&cache_lock, K_FOREVER);
	int ret = points_json_encode(cache, ARRAY_SIZE(cache), buf, buf_len);
	k_mutex_unlock(&cache_lock);
	return ret;
}

// ==================================================
// Shell point streaming
//
// Points are emitted to the console as lines of ASCII so a host running
// Simple IoT can consume the same output a developer reads:
//
//   pt <type> <key> <INT|FLT|STR|JSN> <data> [<time>]
//
// This mirrors the `p` command's argument layout, so a report can be replayed
// as a command by changing the verb. The verbs differ deliberately: the host
// must never mistake an echoed command for a point report.

#ifdef CONFIG_SIOT_POINT_SHELL

static bool streaming;

bool point_cache_streaming(void)
{
	return streaming;
}

// quote_field renders a value for the Zephyr shell tokenizer, which is what
// parses it if the line is replayed as a `p` command. Values are emitted bare
// unless they contain a space, a quote, a backslash, or a control character.
// Returns the number of characters written, or -1 if the buffer is too small.
static int quote_field(const char *src, char *dst, size_t dst_len)
{
	bool needs_quote = (src[0] == '\0');

	for (size_t i = 0; src[i] != '\0'; i++) {
		char c = src[i];
		if (c == ' ' || c == '"' || c == '\\' || c == '\t' || c == '\r' || c == '\n' ||
		    (unsigned char)c < 0x20) {
			needs_quote = true;
			break;
		}
	}

	size_t di = 0;

	if (!needs_quote) {
		for (size_t i = 0; src[i] != '\0'; i++) {
			if (di + 1 >= dst_len) {
				return -1;
			}
			dst[di++] = src[i];
		}
		dst[di] = '\0';
		return (int)di;
	}

	if (di + 1 >= dst_len) {
		return -1;
	}
	dst[di++] = '"';

	for (size_t i = 0; src[i] != '\0'; i++) {
		char c = src[i];
		char esc = 0;

		switch (c) {
		case '"':
			esc = '"';
			break;
		case '\\':
			esc = '\\';
			break;
		case '\r':
			esc = 'r';
			break;
		case '\n':
			esc = 'n';
			break;
		case '\t':
			esc = 't';
			break;
		}

		if (esc != 0) {
			if (di + 2 >= dst_len) {
				return -1;
			}
			dst[di++] = '\\';
			dst[di++] = esc;
		} else {
			if (di + 1 >= dst_len) {
				return -1;
			}
			dst[di++] = c;
		}
	}

	if (di + 2 > dst_len - 1) {
		return -1;
	}
	dst[di++] = '"';
	dst[di] = '\0';

	return (int)di;
}

// point_emit writes one point to the console as a `pt ` line.
static void point_emit(point *p)
{
	const struct shell *sh = shell_backend_uart_get_ptr();

	if (sh == NULL) {
		return;
	}

	if (p->data_type == POINT_DATA_TYPE_UNKNOWN || p->data_type >= POINT_DATA_TYPE_END) {
		LOG_DBG("not emitting point with unknown data type: %s:%s", p->type, p->key);
		return;
	}

	const char *dt;

	switch (p->data_type) {
	case POINT_DATA_TYPE_FLOAT:
		dt = POINT_DATA_TYPE_FLOAT_S;
		break;
	case POINT_DATA_TYPE_INT:
		dt = POINT_DATA_TYPE_INT_S;
		break;
	case POINT_DATA_TYPE_STRING:
		dt = POINT_DATA_TYPE_STRING_S;
		break;
	case POINT_DATA_TYPE_JSON:
		dt = POINT_DATA_TYPE_JSON_S;
		break;
	default:
		return;
	}

	char raw[sizeof(p->data) + 1];
	if (point_data_to_string(p, raw, sizeof(raw)) != 0) {
		return;
	}

	// worst case is every character escaped, plus the surrounding quotes
	char data_q[sizeof(raw) * 2 + 3];
	char type_q[sizeof(p->type) * 2 + 3];
	char key_q[sizeof(p->key) * 2 + 3];

	if (quote_field(raw, data_q, sizeof(data_q)) < 0 ||
	    quote_field(p->type, type_q, sizeof(type_q)) < 0 ||
	    quote_field(p->key, key_q, sizeof(key_q)) < 0) {
		LOG_WRN("point does not fit in the emit buffer: %s", p->type);
		return;
	}

	// The timestamp is only present when the host supplied it. The MCU has
	// no clock, so it never invents one -- an absent time means the host
	// stamps the point when it arrives.
	//
	// Because the MCU has no clock, every timestamp it holds came from the
	// host, and the host always sends the current time. A value before
	// POINT_TIME_MIN is therefore not a real timestamp but an uninitialized
	// point struct, which is easy to introduce in a publisher and would
	// otherwise be emitted as a plausible looking date.
	if (p->time >= POINT_TIME_MIN) {
		char time_buf[RFC3339_MAX_LEN];

		if (timeconv_rfc3339_from_epoch_ns_utc(p->time, time_buf, sizeof(time_buf)) == 0) {
			shell_print(sh, "pt %s %s %s %s %s", type_q, key_q, dt, data_q, time_buf);
			return;
		}
	} else if (p->time != 0) {
		LOG_WRN("ignoring implausible timestamp on %s:%s -- publisher likely "
			"left the point struct uninitialized",
			p->type, p->key);
	}

	shell_print(sh, "pt %s %s %s %s", type_q, key_q, dt, data_q);
}

static int emit_cb(point *p, void *user_data)
{
	ARG_UNUSED(user_data);
	point_emit(p);
	return 0;
}

static int cmd_siot_dump(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	// One line per point rather than a single blob: a full cache would
	// exceed any sane line length. point_cache_json_encode covers the
	// whole-state-at-once case over HTTP.
	point_cache_foreach(emit_cb, NULL);

	return 0;
}

static int cmd_siot_stream(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_print(sh, "streaming is %s", streaming ? "on" : "off");
		return 0;
	}

	if (strcmp(argv[1], "on") == 0) {
		streaming = true;
	} else if (strcmp(argv[1], "off") == 0) {
		streaming = false;
	} else {
		shell_print(sh, "Usage: siot stream <on|off>");
		return -1;
	}

	return 0;
}

static int cmd_siot_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "points cached: %i of %i", point_cache_count(),
		    CONFIG_SIOT_POINT_CACHE_SIZE);
	shell_print(sh, "streaming: %s", streaming ? "on" : "off");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(siot_cmds,
			       SHELL_CMD(dump, NULL, "Emit every cached point", cmd_siot_dump),
			       SHELL_CMD(stream, NULL, "Stream points as they change: on|off",
					 cmd_siot_stream),
			       SHELL_CMD(status, NULL, "Show cache and streaming state",
					 cmd_siot_status),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(siot, &siot_cmds, "Simple IoT point cache", NULL);

#endif // CONFIG_SIOT_POINT_SHELL

// ==================================================
// Cache maintenance thread

ZBUS_MSG_SUBSCRIBER_DEFINE(point_cache_sub);
ZBUS_CHAN_ADD_OBS(point_chan, point_cache_sub, 4);

static void point_cache_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_INF("siot point cache thread");

	point p;
	const struct zbus_channel *chan;

	while (!zbus_sub_wait_msg(&point_cache_sub, &chan, &p, K_FOREVER)) {
		if (chan != &point_chan) {
			continue;
		}

		int ret = point_cache_merge(&p);
		if (ret != 0) {
			LOG_ERR("Error storing point in cache: %i", ret);
		}

#ifdef CONFIG_SIOT_POINT_SHELL
		if (streaming) {
			point_emit(&p);
		}
#endif
	}
}

K_THREAD_DEFINE(point_cache, CONFIG_SIOT_POINT_CACHE_THREAD_STACK, point_cache_thread, NULL, NULL,
		NULL, CONFIG_SIOT_POINT_CACHE_THREAD_PRIO, 0, 0);
