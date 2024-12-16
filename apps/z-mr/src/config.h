#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct z_mr_config_t {
  uint32_t bootcount;
  char device_id[20];
  bool static_ip;
  char ip_addr[20];
  char subnet_mask[20];
  char gateway[20];
} z_mr_config;

void z_mr_config_init(z_mr_config *c);

#endif  // __CONFIG_H_
