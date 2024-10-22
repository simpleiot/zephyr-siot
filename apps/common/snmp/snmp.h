#ifndef __SNMP_H_
#define __SNMP_H_

#include <zephyr/types.h>

int snmp_send(char *ipaddr, const void *messge, size_t len);

#endif // __SNMP_H_
