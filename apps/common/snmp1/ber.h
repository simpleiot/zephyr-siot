#ifndef BER_H
#define BER_H

#include <stdint.h>
#include <stdbool.h>

#define SNMP_CAPACITY 6

// Forward declarations
struct ber;

typedef enum {
    BER_BOOLEAN,
    BER_INTEGER,
    BER_OCTET_STRING,
    BER_NULL,
    BER_OBJECT_IDENTIFIER,
    BER_SEQUENCE,
    BER_IP_ADDRESS,
    BER_COUNTER32,
    BER_GAUGE32,
    BER_TIMETICKS,
    BER_OPAQUE,
    BER_COUNTER64,
    BER_NO_SUCH_OBJECT,
    BER_NO_SUCH_INSTANCE,
    BER_END_OF_MIB_VIEW
} ber_type;

typedef struct {
    uint8_t class;
    uint8_t form;
    unsigned int tag;
} ber_type_info;

typedef struct {
    unsigned int length;
    unsigned int size;
} ber_length;

struct ber {
    ber_type_info type;
    ber_length length;
    unsigned int size;
    void* value;
};

// Function prototypes
struct ber* ber_create(ber_type type);
void ber_destroy(struct ber* ber);
unsigned int ber_get_size(struct ber* ber);
int ber_encode(struct ber* ber, uint8_t* buffer, unsigned int buffer_size);
int ber_decode(struct ber* ber, const uint8_t* buffer, unsigned int buffer_size);

// Specific BER types
struct ber* ber_create_boolean(bool value);
struct ber* ber_create_integer(int32_t value);
struct ber* ber_create_octet_string(const char* value, unsigned int length);
struct ber* ber_create_null(void);
struct ber* ber_create_object_identifier(const char* oid);
struct ber* ber_create_sequence(void);
struct ber* ber_create_ip_address(uint32_t ip);
struct ber* ber_create_counter32(uint32_t value);
struct ber* ber_create_gauge32(uint32_t value);
struct ber* ber_create_timeticks(uint32_t value);
struct ber* ber_create_opaque(struct ber* embedded);
struct ber* ber_create_counter64(uint64_t value);
struct ber* ber_create_no_such_object(void);
struct ber* ber_create_no_such_instance(void);
struct ber* ber_create_end_of_mib_view(void);

#endif /* BER_H */
