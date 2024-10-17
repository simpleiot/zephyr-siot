#include "snmp.h"
#include "snmp_message.h"
#include <stdlib.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>

bool snmp_begin(struct snmp *snmp)
{
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		return false;
	}

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(snmp->port),
		.sin_addr = INADDR_ANY,
	};

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(sock);
		return false;
	}

	snmp->sock = sock;
	return true;
}

void snmp_loop(struct snmp *snmp)
{
	char buf[1024];
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	int received = recvfrom(snmp->sock, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr,
				&client_addr_len);

	if (received > 0) {
		struct snmp_message *message = malloc(sizeof(struct snmp_message));
		if (message) {
			// Assuming message_parse is modified to accept a buffer
			message_parse(message, buf, received);

			struct in_addr remote_ip = client_addr.sin_addr;
			uint16_t remote_port = ntohs(client_addr.sin_port);

			snmp->on_message(message, (struct ip_address *)&remote_ip, remote_port);
			free(message);
		}
	}
}

bool snmp_send(struct snmp *snmp, struct snmp_message *message, const struct ip_address *ip,
	       uint16_t port)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = *(struct in_addr *)ip,
	};

	char buf[1024];
	int len = message_build(message, buf, sizeof(buf));

	if (len < 0) {
		return false;
	}

	int sent = sendto(snmp->sock, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
	return sent == len;
}

void snmp_on_message(struct snmp *snmp, snmp_event_handler event)
{
	snmp->on_message = event;
}

void snmp_agent_init(struct snmp_agent *agent)
{
	agent->base.port = SNMP_PORT;
	agent->base.sock = -1;
	agent->base.on_message = NULL;
}

void snmp_manager_init(struct snmp_manager *manager)
{
	manager->base.port = TRAP_PORT;
	manager->base.sock = -1;
	manager->base.on_message = NULL;
}

void snmp_close(struct snmp *snmp)
{
	if (snmp->sock >= 0) {
		close(snmp->sock);
		snmp->sock = -1;
	}
}
