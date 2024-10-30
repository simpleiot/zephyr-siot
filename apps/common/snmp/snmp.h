#ifndef __SNMP_H_
#define __SNMP_H_

#include <stdint.h>
#include <zephyr/types.h>

#include "asn1.h"

#define SNMP_MSG_TRAP2 (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x7) /* a7=167 */

typedef struct {
	int command;
	uint32_t reqid;
} snmp_pdu;

int snmp_trap(char *ipaddr, const void *messge, size_t len);

snmp_pdu *snmp_pdu_create(int command);

uint32_t snmp_get_next_requid(void);

#endif // __SNMP_H_
