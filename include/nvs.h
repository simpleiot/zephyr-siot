#ifndef __NVS_H_
#define __NVS_H_

#include <point.h>
#include <sys/types.h>

struct nvs_point {
  uint16_t nvs_id;
  const point_def *point_def;   
  char *key;
};

int nvs_init(const struct nvs_point *nvs_pts_in, size_t len);

#endif // __NVS_H_
