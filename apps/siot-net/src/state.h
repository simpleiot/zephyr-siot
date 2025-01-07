#ifndef __STATE_H_
#define __STATE_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct siot_state_t {
  uint32_t bootcount;
  char device_id[20];
  bool static_ip;
  char ip_addr[20];
  char subnet_mask[20];
  char gateway[20];
} z_mr_config;

#define SIOT_STATE_INIT { \
    .bootcount = 0, \
    .device_id = {0}, \
    .static_ip = false, \
    .ip_addr = {0}, \
    .subnet_mask = {0}, \
    .gateway = {0} \
}

#endif // __STATE_H_
