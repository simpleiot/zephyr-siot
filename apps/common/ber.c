// Note that some of the more complex BER types (like sequences) are not fully implemented and may
// require additional work to handle all possible cases. The OID encoding is also simplified and may
// not handle all possible OID formats correctly.

#include "ber.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ber *ber_create(ber_type type)
{
	struct ber *ber = malloc(sizeof(struct ber));
	if (ber == NULL) {
		return NULL;
	}

	ber->type.class = 0;
	ber->type.form = 0;
	ber->type.tag = type;
	ber->length.length = 0;
	ber->length.size = 0;
	ber->size = 0;
	ber->value = NULL;

	return ber;
}

void ber_destroy(struct ber *ber)
{
	if (ber) {
		free(ber->value);
		free(ber);
	}
}

unsigned int ber_get_size(struct ber *ber)
{
	if (ber == NULL) {
		return 0;
	}
	return ber->size;
}

int ber_encode(struct ber *ber, uint8_t *buffer, unsigned int buffer_size)
{
	if (ber == NULL || buffer == NULL || buffer_size < ber->size) {
		return -1;
	}

	// Encode type
	*buffer++ = ber->type.tag;

	// Encode length
	if (ber->length.length < 128) {
		*buffer++ = ber->length.length;
	} else {
		uint8_t length_bytes = 0;
		unsigned int temp = ber->length.length;
		while (temp > 0) {
			temp >>= 8;
			length_bytes++;
		}
		*buffer++ = 0x80 | length_bytes;
		for (int i = length_bytes - 1; i >= 0; i--) {
			*buffer++ = (ber->length.length >> (i * 8)) & 0xFF;
		}
	}

	// Encode value
	memcpy(buffer, ber->value, ber->length.length);

	return ber->size;
}

int ber_decode(struct ber *ber, const uint8_t *buffer, unsigned int buffer_size)
{
	if (ber == NULL || buffer == NULL || buffer_size < 2) {
		return -1;
	}

	// Decode type
	ber->type.tag = *buffer++;

	// Decode length
	if (*buffer < 128) {
		ber->length.length = *buffer++;
	} else {
		uint8_t length_bytes = *buffer++ & 0x7F;
		ber->length.length = 0;
		for (int i = 0; i < length_bytes; i++) {
			ber->length.length = (ber->length.length << 8) | *buffer++;
		}
	}

	// Decode value
	if (buffer_size < (buffer - (const uint8_t *)ber) + ber->length.length) {
		return -1;
	}
	ber->value = malloc(ber->length.length);
	if (ber->value == NULL) {
		return -1;
	}
	memcpy(ber->value, buffer, ber->length.length);

	ber->size = (buffer - (const uint8_t *)ber) + ber->length.length;
	return ber->size;
}

struct ber *ber_create_boolean(bool value)
{
	struct ber *ber = ber_create(BER_BOOLEAN);
	if (ber) {
		ber->length.length = 1;
		ber->value = malloc(1);
		if (ber->value) {
			*(uint8_t *)ber->value = value ? 0xFF : 0x00;
			ber->size = 3;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_integer(int32_t value)
{
	struct ber *ber = ber_create(BER_INTEGER);
	if (ber) {
		ber->length.length = 4;
		ber->value = malloc(4);
		if (ber->value) {
			*(int32_t *)ber->value = value;
			ber->size = 6;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_octet_string(const char *value, unsigned int length)
{
	struct ber *ber = ber_create(BER_OCTET_STRING);
	if (ber) {
		ber->length.length = length;
		ber->value = malloc(length);
		if (ber->value) {
			memcpy(ber->value, value, length);
			ber->size = 2 + length;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_null(void)
{
	struct ber *ber = ber_create(BER_NULL);
	if (ber) {
		ber->length.length = 0;
		ber->size = 2;
	}
	return ber;
}

struct ber *ber_create_object_identifier(const char *oid)
{
	struct ber *ber = ber_create(BER_OBJECT_IDENTIFIER);
	if (ber) {
		// Simple OID encoding (not fully compliant, but functional for basic OIDs)
		unsigned int length = strlen(oid);
		ber->value = malloc(length);
		if (ber->value) {
			char *token;
			char *rest = strdup(oid);
			unsigned int index = 0;
			while ((token = strtok_r(rest, ".", &rest))) {
				((uint8_t *)ber->value)[index++] = atoi(token);
			}
			free(rest);
			ber->length.length = index;
			ber->size = 2 + index;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_sequence(void)
{
	return ber_create(BER_SEQUENCE);
}

struct ber *ber_create_ip_address(uint32_t ip)
{
	struct ber *ber = ber_create(BER_IP_ADDRESS);
	if (ber) {
		ber->length.length = 4;
		ber->value = malloc(4);
		if (ber->value) {
			*(uint32_t *)ber->value = ip;
			ber->size = 6;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_counter32(uint32_t value)
{
	struct ber *ber = ber_create(BER_COUNTER32);
	if (ber) {
		ber->length.length = 4;
		ber->value = malloc(4);
		if (ber->value) {
			*(uint32_t *)ber->value = value;
			ber->size = 6;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_gauge32(uint32_t value)
{
	struct ber *ber = ber_create(BER_GAUGE32);
	if (ber) {
		ber->length.length = 4;
		ber->value = malloc(4);
		if (ber->value) {
			*(uint32_t *)ber->value = value;
			ber->size = 6;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_timeticks(uint32_t value)
{
	struct ber *ber = ber_create(BER_TIMETICKS);
	if (ber) {
		ber->length.length = 4;
		ber->value = malloc(4);
		if (ber->value) {
			*(uint32_t *)ber->value = value;
			ber->size = 6;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_opaque(struct ber *embedded)
{
	struct ber *ber = ber_create(BER_OPAQUE);
	if (ber && embedded) {
		unsigned int embedded_size = ber_get_size(embedded);
		ber->length.length = embedded_size;
		ber->value = malloc(embedded_size);
		if (ber->value) {
			ber_encode(embedded, ber->value, embedded_size);
			ber->size = 2 + embedded_size;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_counter64(uint64_t value)
{
	struct ber *ber = ber_create(BER_COUNTER64);
	if (ber) {
		ber->length.length = 8;
		ber->value = malloc(8);
		if (ber->value) {
			*(uint64_t *)ber->value = value;
			ber->size = 10;
		} else {
			ber_destroy(ber);
			ber = NULL;
		}
	}
	return ber;
}

struct ber *ber_create_no_such_object(void)
{
	return ber_create(BER_NO_SUCH_OBJECT);
}

struct ber *ber_create_no_such_instance(void)
{
	return ber_create(BER_NO_SUCH_INSTANCE);
}

struct ber *ber_create_end_of_mib_view(void)
{
	return ber_create(BER_END_OF_MIB_VIEW);
}
