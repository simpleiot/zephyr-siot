#include "snmp.h"
#include <stdlib.h>

bool snmp_begin(struct snmp *snmp, struct udp *udp)
{
	snmp->udp = udp;
	// Assuming udp_begin is a function that initializes the UDP connection
	return udp_begin(snmp->udp, snmp->port);
}

void snmp_loop(struct snmp *snmp)
{
	// Assuming udp_parse_packet is a function that checks for incoming packets
	if (udp_parse_packet(snmp->udp)) {
		struct snmp_message *message = malloc(sizeof(struct snmp_message));
		if (message) {
			// Assuming message_parse is a function that parses the incoming message
			message_parse(message, snmp->udp);

			// Assuming udp_remote_ip and udp_remote_port are functions to get sender
			// info
			struct ip_address remote_ip = udp_remote_ip(snmp->udp);
			uint16_t remote_port = udp_remote_port(snmp->udp);

			snmp->on_message(message, &remote_ip, remote_port);
			free(message);
		}
	}
}

bool snmp_send(struct snmp *snmp, struct snmp_message *message, const struct ip_address *ip,
	       uint16_t port)
{
	// Assuming udp_begin_packet, message_build, and udp_end_packet are appropriate functions
	udp_begin_packet(snmp->udp, ip, port);
	message_build(message, snmp->udp);
	return udp_end_packet(snmp->udp);
}

void snmp_on_message(struct snmp *snmp, snmp_event_handler event)
{
	snmp->on_message = event;
}

void snmp_agent_init(struct snmp_agent *agent)
{
	agent->base.port = SNMP_PORT;
	agent->base.udp = NULL;
	agent->base.on_message = NULL;
}

void snmp_manager_init(struct snmp_manager *manager)
{
	manager->base.port = TRAP_PORT;
	manager->base.udp = NULL;
	manager->base.on_message = NULL;
}
