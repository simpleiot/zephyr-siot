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
	int data_type;
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

void point_set_type(point *p, char *t);
void point_set_key(point *p, char *k);
void point_set_type_key(point *p, char *t, char *k);

int point_get_int(point *p);
float point_get_float(point *p);
void point_get_string(point *p, char *dest, int len);

void point_put_int(point *p, int v);
void point_put_float(point *p, float v);
void point_put_string(point *p, char *v);

int point_data_len(point *p);
int point_dump(point *p, char *buf, size_t len);
int points_dump(point *pts, size_t pts_len, char *buf, size_t len);
int points_merge(point *pts, size_t pts_len, point *p);

int point_json_encode(point *p, char *buf, size_t len);
int point_json_decode(char *json, size_t json_len, point *p);
int points_json_encode(point *pts_in, int count, char *buf, size_t len);

#endif // __POINT_H_
