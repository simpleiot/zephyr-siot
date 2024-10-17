#ifndef SNMP_MESSAGE_H
#define SNMP_MESSAGE_H

#include <stdint.h>
#include <stdbool.h>

// Enums and defines
typedef enum {
    SNMP_VERSION_1 = 0,
    SNMP_VERSION_2C = 1,
    SNMP_VERSION_3 = 3
} snmp_version_t;

typedef enum {
    PDU_GET_REQUEST = 0xA0,
    PDU_GET_NEXT_REQUEST = 0xA1,
    PDU_GET_RESPONSE = 0xA2,
    PDU_SET_REQUEST = 0xA3,
    PDU_TRAP = 0xA4,
    PDU_GET_BULK_REQUEST = 0xA5,
    PDU_INFORM_REQUEST = 0xA6,
    PDU_TRAP_V2 = 0xA7
} pdu_type_t;

// Structs
typedef struct {
    uint8_t status;
    uint8_t index;
} snmp_error_t;

typedef struct {
    uint8_t non_repeaters;
    uint8_t max_repetitions;
} snmp_bulk_t;

typedef struct {
    uint32_t request_id;
    union {
        snmp_error_t error;
        snmp_bulk_t bulk;
    };
} snmp_pdu_t;

typedef struct {
    snmp_version_t version;
    const char *community;
    pdu_type_t type;
    snmp_pdu_t pdu;
    // Add other necessary fields
} snmp_message_t;

// Function prototypes
snmp_message_t *snmp_message_create(snmp_version_t version, const char *community, pdu_type_t type);
void snmp_message_destroy(snmp_message_t *message);
int snmp_message_get_size(snmp_message_t *message, bool refresh);
void snmp_message_set_request_id(snmp_message_t *message, int32_t request_id);
int32_t snmp_message_get_request_id(snmp_message_t *message);
void snmp_message_set_error(snmp_message_t *message, uint8_t status, uint8_t index);
uint8_t snmp_message_get_error_status(snmp_message_t *message);
uint8_t snmp_message_get_error_index(snmp_message_t *message);
void snmp_message_set_non_repeaters(snmp_message_t *message, uint8_t non_repeaters);
void snmp_message_set_max_repetitions(snmp_message_t *message, uint8_t max_repetitions);

#endif /* SNMP_MESSAGE_H */
