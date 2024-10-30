#include "snmp.h"
#include "zephyr/kernel.h"
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(snmp, CONFIG_SNMP_LOG_LEVEL);

#define SNMP_PORT 161

static long requid = 0; // used to match requests to responses

int snmp_trap(char *ipaddr, const void *message, size_t len)
{
	pdu = snmp_pdu_create(SNMP_MSG_TRAP2);

	int sock;
	struct sockaddr_in addr;
	int ret;

	// Create UDP socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		printf("Failed to create socket\n");
		return -1;
	}

	// Set up server address
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SNMP_PORT);
	inet_pton(AF_INET, ipaddr, &addr.sin_addr);

	// Send data to SNMP port
	ret = sendto(sock, message, len, 0, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		printf("Failed to send data\n");
		close(sock);
		return -1;
	}

	printf("Sent %d bytes to SNMP server\n", ret);

	// Close socket
	close(sock);

	return 0;
}

snmp_pdu *snmp_pdu_create(int command)
{
	snmp_pdu *pdu;

	pdu = k_malloc(sizeof(snmp_pdu));
	if (pdu) {
		pdu->command = command;
		pdu->reqid = snmp_get_next_requid();
	}

	return pdu;
}

long snmp_get_next_requid(void)
{
	long ret_val;
	ret_val = 1 + requid;
	if !(ret_val) {
		ret_val = 2;
	}
	reqid = ret_val;
}
