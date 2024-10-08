#ifndef SNMP_H_
#define SNMP_H_

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
struct udp;
struct ip_address;
struct snmp_message;

// Port definitions
#define SNMP_PORT 161
#define TRAP_PORT 162

// Event handler function pointer type
typedef void (*snmp_event_handler)(const struct snmp_message* message, const struct ip_address* remote, uint16_t port);

// SNMP structure
struct snmp {
    uint16_t port;
    struct udp* udp;
    snmp_event_handler on_message;
};

// Function prototypes
bool snmp_begin(struct snmp* snmp, struct udp* udp);
void snmp_loop(struct snmp* snmp);
bool snmp_send(struct snmp* snmp, struct snmp_message* message, const struct ip_address* ip, uint16_t port);
void snmp_on_message(struct snmp* snmp, snmp_event_handler event);

// Agent structure
struct snmp_agent {
    struct snmp base;
};

// Manager structure
struct snmp_manager {
    struct snmp base;
};

// Agent and Manager initialization functions
void snmp_agent_init(struct snmp_agent* agent);
void snmp_manager_init(struct snmp_manager* manager);

#endif /* SNMP_H_ */
