#ifndef __POINT_H_
#define __POINT_H_

#include "zephyr/kernel.h"
#include <stdint.h>

// The point datatype is used to represent most configuration and sensor data in the system
// One point is 73 bytes long. We currently allocate 24K of flash to NVS storage, so that
// allows us to store ~300 points. 
typedef struct {
	uint64_t time;
	char type[24];
	char key[20];
	uint8_t data_type;
	char data[20];
} point;

// TODO: find a way to initialize new points with key set to "0"

// Point Data Types should match those in SIOT (not merged to master yet)
// https://github.com/simpleiot/simpleiot/blob/feat/js-subject-point-changes/data/point.go

#define POINT_DATA_TYPE_UNKNOWN 0
#define POINT_DATA_TYPE_FLOAT 1
#define POINT_DATA_TYPE_INT 2
#define POINT_DATA_TYPE_STRING 3
#define POINT_DATA_TYPE_JSON 4
// always keep _END at the end of this list
#define POINT_DATA_TYPE_END 5 

// We use 3 letter codes for data types in JSON packets so they are easier to read
#define POINT_DATA_TYPE_FLOAT_S "FLT"
#define POINT_DATA_TYPE_INT_S "INT"
#define POINT_DATA_TYPE_STRING_S "STR"
#define POINT_DATA_TYPE_JSON_S "JSN"

// ==================================================
// Point types
// These defines should match those in the SIOT schema
// https://github.com/simpleiot/simpleiot/blob/master/data/schema.go

#define POINT_TYPE_DESCRIPTION "description"
#define POINT_TYPE_STATICIP "staticIP"
#define POINT_TYPE_ADDRESS "address"
#define POINT_TYPE_NETMASK "netmask"
#define POINT_TYPE_GATEWAY "gateway"
#define POINT_TYPE_METRIC_SYS_CPU_PERCENT "metricSysCPUPercent"
#define POINT_TYPE_UPTIME "uptime"
#define POINT_TYPE_TEMPERATURE "temp"
#define POINT_TYPE_BOARD "board"
#define POINT_TYPE_BOOT_COUNT "bootCount"

typedef struct {
	char * type;
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

void point_set_type(point *p, const char *t);
void point_set_key(point *p, const char *k);
void point_set_type_key(point *p, const char *t, const char *k);

int point_get_int(point *p);
float point_get_float(point *p);
void point_get_string(point *p, char *dest, int len);

void point_put_int(point *p, const int v);
void point_put_float(point *p, const float v);
void point_put_string(point *p, const char *v);

int point_data_len(point *p);
int point_dump(point *p, char *buf, size_t len);
int points_dump(point *pts, size_t pts_len, char *buf, size_t len);
int points_merge(point *pts, size_t pts_len, point *p);

int point_json_encode(point *p, char *buf, size_t len);
int point_json_decode(char *json, size_t json_len, point *p);
int points_json_encode(point *pts_in, int count, char *buf, size_t len);
int points_json_decode(char *json, size_t json_len, point *pts, size_t p_cnt);

#define LOG_DBG_POINT(msg, p) \
    Z_LOG_EVAL(LOG_LEVEL_DBG, \
    ({ \
        char buf[40]; \
        point_dump(p, buf, sizeof(buf)); \
        LOG_DBG("%s: %s", msg, buf); \
    }), \
    ())


#define LOG_DBG_POINTS(msg, pts, len) \
    Z_LOG_EVAL(LOG_LEVEL_DBG, \
    ({ \
        char buf[128]; \
        points_dump(pts, len, buf, sizeof(buf)); \
        LOG_DBG("%s: %s", msg, buf); \
    }), \
    ())



#endif // __POINT_H_
