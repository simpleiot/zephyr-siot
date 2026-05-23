#ifndef __POINT_H_
#define __POINT_H_

#include "zephyr/kernel.h"
#include <stdint.h>
#include <zephyr/data/json.h>

// The point datatype is used to represent most configuration and sensor data in the system
// One point is 74 bytes long. We currently allocate 24K of flash to NVS storage, so that
// allows us to store ~300 points.
typedef struct {
	uint64_t time;
	char type[24];
	char key[20];
	uint8_t data_type;
	char data[21];
} point;

// TODO: find a way to initialize new points with key set to "0"

// Point Data Types should match those in SIOT (not merged to master yet)
// https://github.com/simpleiot/simpleiot/blob/feat/js-subject-point-changes/data/point.go

#define POINT_BUFFER_COUNT 150
#define RFC_3339_MAX_LEN   21

#define POINT_DATA_TYPE_UNKNOWN 0
#define POINT_DATA_TYPE_FLOAT   1
#define POINT_DATA_TYPE_INT     2
#define POINT_DATA_TYPE_STRING  3
#define POINT_DATA_TYPE_JSON    4
// always keep _END at the end of this list
#define POINT_DATA_TYPE_END     5

// We use 3 letter codes for data types in JSON packets so they are easier to read
#define POINT_DATA_TYPE_FLOAT_S  "FLT"
#define POINT_DATA_TYPE_INT_S    "INT"
#define POINT_DATA_TYPE_STRING_S "STR"
#define POINT_DATA_TYPE_JSON_S   "JSN"

// ==================================================
// JSON Buffer Sizing Constants
// JSON structure: {"t":"","k":"","dt":"","tm":"","d":""}
#define JSON_POINT_FIXED_OVERHEAD 31 // Base JSON structure characters
#define JSON_POINT_DATA_TYPE_SIZE 3  // "INT", "FLT", or "STR"
#define JSON_POINT_TIMESTAMP_SIZE 21 // RFC3339 timestamp
#define JSON_POINT_TYPE_MAX_SIZE  24 // Maximum type field length
#define JSON_POINT_KEY_MAX_SIZE   20 // Maximum key field length
#define JSON_POINT_DATA_MAX_SIZE  21 // Maximum data field length (for strings)
#define JSON_POINT_INT_MAX_SIZE   12 // Maximum integer string representation
#define JSON_POINT_FLOAT_MAX_SIZE 16 // Maximum float string representation

// Minimum size per point (fixed overhead + data type + timestamp)
#define JSON_POINT_MIN_SIZE                                                                        \
	(JSON_POINT_FIXED_OVERHEAD + JSON_POINT_DATA_TYPE_SIZE + JSON_POINT_TIMESTAMP_SIZE)

// Maximum size per point (all fields at maximum length)
#define JSON_POINT_MAX_SIZE                                                                        \
	(JSON_POINT_FIXED_OVERHEAD + JSON_POINT_DATA_TYPE_SIZE + JSON_POINT_TIMESTAMP_SIZE +       \
	 JSON_POINT_TYPE_MAX_SIZE + JSON_POINT_KEY_MAX_SIZE + JSON_POINT_DATA_MAX_SIZE)

// ==================================================
// Point types
// These defines should match those in the SIOT schema
// https://github.com/simpleiot/simpleiot/blob/master/data/schema.go

#define POINT_TYPE_DESCRIPTION            "description"
#define POINT_TYPE_STATICIP               "staticIP"
#define POINT_TYPE_ADDRESS                "address"
#define POINT_TYPE_NETMASK                "netmask"
#define POINT_TYPE_GATEWAY                "gateway"
#define POINT_TYPE_METRIC_SYS_CPU_PERCENT "metricSysCPUPercent"
#define POINT_TYPE_UPTIME                 "uptime"
#define POINT_TYPE_TEMPERATURE            "temp"
#define POINT_TYPE_BOARD                  "board"
#define POINT_TYPE_BOOT_COUNT             "bootCount"
#define POINT_TYPE_NTP                    "ntpIP"
#define POINT_TYPE_VERSION_FW             "versionFW"
#define POINT_TYPE_COUNT                  "count"
#define POINT_TYPE_COUNT_INC              "countInc"
#define POINT_TYPE_HEARTBEAT              "heartbeat"
#define POINT_TYPE_UI_VERSION             "uiVersion"

typedef struct {
	char *type;
	int data_type;
} point_def;

extern const point_def point_def_description;
extern const point_def point_def_staticip;
extern const point_def point_def_address;
extern const point_def point_def_netmask;
extern const point_def point_def_gateway;
extern const point_def point_def_metric_sys_cpu_percent;
extern const point_def point_def_uptime;
extern const point_def point_def_temperature;
extern const point_def point_def_board;
extern const point_def point_def_boot_count;
extern const point_def point_def_ntp;

extern uint64_t (*get_current_time_ns_cb)(void);

void point_init(point *p, const char *t, const char *k);

void point_set_get_time_callback(uint64_t (*cb)(void));

int point_get_int(const point *p);
float point_get_float(const point *p);
void point_get_string(const point *p, char *dest, int len);

void point_put_int(point *p, const int v);
void point_put_float(point *p, const float v);
void point_put_string(point *p, const char *v);

int point_data_len(const point *p);
int point_dump(const point *p, char *buf, size_t len);
int points_dump(point *pts, size_t pts_len, char *buf, size_t len);
int points_merge(point *pts, size_t pts_len, point *p);

// JSON structure for shell command parsing
struct point_js {
	char *t;                  // type
	char *k;                  // key
	char *dt;                 // datatype
	struct json_obj_token tm; // time
	struct json_obj_token d;  // data
};

int point_json_encode(point *p, char *buf, size_t len);
int point_json_decode(char *json, size_t json_len, point *p);
int points_json_encode(point *pts_in, int count, char *buf, size_t len);
int points_json_decode(char *json, size_t json_len, point *pts, size_t p_cnt);

// Shell command parsing
int point_js_to_point(struct point_js *p_js, point *p);

// Accessor functions for web_points (defined in web.c)
point *get_web_points(void);
int get_web_points_count(void);

#define LOG_DBG_POINT(msg, p)                                                                      \
	Z_LOG_EVAL(LOG_LEVEL_DBG, ({                                                               \
			   char buf[40];                                                           \
			   point_dump(p, buf, sizeof(buf));                                        \
			   LOG_DBG("%s: %s", msg, buf);                                            \
		   }),                                                                             \
		   ())

#define LOG_DBG_POINTS(msg, pts, len)                                                              \
	Z_LOG_EVAL(LOG_LEVEL_DBG, ({                                                               \
			   char buf[128];                                                          \
			   points_dump(pts, len, buf, sizeof(buf));                                \
			   LOG_DBG("%s: %s", msg, buf);                                            \
		   }),                                                                             \
		   ())

#endif // __POINT_H_
