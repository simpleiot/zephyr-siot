#include "snmp_message.h"
#include <stdlib.h>
#include <string.h>

snmp_message_t *snmp_message_create(snmp_version_t version, const char *community, pdu_type_t type)
{
	snmp_message_t *message = (snmp_message_t *)malloc(sizeof(snmp_message_t));
	if (message) {
		message->version = version;
		message->community = community ? strdup(community) : NULL;
		message->type = type;
		message->pdu.request_id = rand();
	}
	return message;
}

void snmp_message_destroy(snmp_message_t *message)
{
	if (message) {
		free((void *)message->community);
		free(message);
	}
}

int snmp_message_get_size(snmp_message_t *message, bool refresh)
{
	// Implement size calculation logic here
	return 0; // Placeholder
}

void snmp_message_set_request_id(snmp_message_t *message, int32_t request_id)
{
	if (message) {
		message->pdu.request_id = request_id;
	}
}

int32_t snmp_message_get_request_id(snmp_message_t *message)
{
	return message ? message->pdu.request_id : 0;
}

void snmp_message_set_error(snmp_message_t *message, uint8_t status, uint8_t index)
{
	if (message && message->type != PDU_GET_BULK_REQUEST) {
		message->pdu.error.status = status;
		message->pdu.error.index = index;
	}
}

uint8_t snmp_message_get_error_status(snmp_message_t *message)
{
	return (message && message->type != PDU_GET_BULK_REQUEST) ? message->pdu.error.status : 0;
}

uint8_t snmp_message_get_error_index(snmp_message_t *message)
{
	return (message && message->type != PDU_GET_BULK_REQUEST) ? message->pdu.error.index : 0;
}

void snmp_message_set_non_repeaters(snmp_message_t *message, uint8_t non_repeaters)
{
	if (message && message->type == PDU_GET_BULK_REQUEST) {
		message->pdu.bulk.non_repeaters = non_repeaters;
	}
}

void snmp_message_set_max_repetitions(snmp_message_t *message, uint8_t max_repetitions)
{
	if (message && message->type == PDU_GET_BULK_REQUEST) {
		message->pdu.bulk.max_repetitions = max_repetitions;
	}
}
