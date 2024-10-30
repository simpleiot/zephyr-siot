#ifndef ASN1_H
#define ASN1_H

#include "oid.h"
#include <stdint.h>
#include <zephyr/types.h>

#define PARSE_PACKET 0
#define DUMP_PACKET  1

/*
 * Definitions for Abstract Syntax Notation One, ASN.1
 * As defined in ISO/IS 8824 and ISO/IS 8825
 *
 *
 */
/***********************************************************
	Copyright 1988, 1989 by Carnegie Mellon University

		      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/
/*
 * Portions of this file are copyrighted by:
 * Copyright (c) 2016 VMware, Inc. All rights reserved.
 * Use is subject to license terms specified in the COPYING file
 */

#ifndef MAX_NAME_LEN             /* conflicts with some libraries */
#define MAX_NAME_LEN MAX_OID_LEN /* obsolete. use MAX_OID_LEN */
#endif

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
/*
 * If x is an array, x and &(x)[0] have different types. If x is a pointer,
 * x and &(x)[0] have the same type. Trigger a build error if x is a pointer
 * by making the compiler evaluate sizeof(int[-1]).
 */
#define OID_LENGTH(x)                                                                              \
	(sizeof(x) / sizeof((x)[0]) +                                                              \
	 sizeof(int[-__builtin_types_compatible_p(typeof(x), typeof(&(x)[0]))]))
#else
#define OID_LENGTH(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef HAVE_ASN_BOOLEAN
#define ASN_BOOLEAN 0x01U
#endif
#ifndef HAVE_ASN_INTEGER
#define ASN_INTEGER 0x02U
#endif
#define ASN_BIT_STR   0x03U
#define ASN_OCTET_STR 0x04U
#define ASN_NULL      0x05U
#ifndef HAVE_ASN_OBJECT_ID
#define ASN_OBJECT_ID 0x06U
#endif
#ifndef HAVE_ASN_SEQUENCE
#define ASN_SEQUENCE 0x10U
#endif
#ifndef HAVE_ASN_SET
#define ASN_SET 0x11U
#endif

#define ASN_UNIVERSAL   0x00U
#define ASN_APPLICATION 0x40U
#define ASN_CONTEXT     0x80U
#define ASN_PRIVATE     0xC0U

#define ASN_PRIMITIVE   0x00U
#define ASN_CONSTRUCTOR 0x20U

#define ASN_LONG_LEN     0x80U
#define ASN_EXTENSION_ID 0x1FU
#define ASN_BIT8         0x80U

#define IS_CONSTRUCTOR(byte)  ((byte) & ASN_CONSTRUCTOR)
#define IS_EXTENSION_ID(byte) (((byte) & ASN_EXTENSION_ID) == ASN_EXTENSION_ID)

struct counter64 {
	uint32_t high;
	uint32_t low;
};

//#ifdef NETSNMP_WITH_OPAQUE_SPECIAL_TYPES
typedef struct counter64 integer64;
typedef struct counter64 unsigned64;

/*
 * The BER inside an OPAQUE is an context specific with a value of 48 (0x30)
 * plus the "normal" tag. For a Counter64, the tag is 0x46 (i.e., an
 * applications specific tag with value 6). So the value for a 64 bit
 * counter is 0x46 + 0x30, or 0x76 (118 base 10). However, values
 * greater than 30 can not be encoded in one octet. So the first octet
 * has the class, in this case context specific (ASN_CONTEXT), and
 * the special value (i.e., 31) to indicate that the real value follows
 * in one or more octets. The high order bit of each following octet
 * indicates if the value is encoded in additional octets. A high order
 * bit of zero, indicates the last. For this "hack", only one octet
 * will be used for the value.
 */

/*
 * first octet of the tag
 */
#define ASN_OPAQUE_TAG1 (ASN_CONTEXT | ASN_EXTENSION_ID)
/*
 * base value for the second octet of the tag - the
 * second octet was the value for the tag
 */
#define ASN_OPAQUE_TAG2 0x30U

#define ASN_OPAQUE_TAG2U 0x2fU /* second octet of tag for union */

/*
 * All the ASN.1 types for SNMP "should have been" defined in this file,
 * but they were not. (They are defined in snmp_impl.h)  Thus, the tag for
 * Opaque and Counter64 is defined, again, here with a different names.
 */
#define ASN_APP_OPAQUE    (ASN_APPLICATION | 4)
#define ASN_APP_COUNTER64 (ASN_APPLICATION | 6)
#define ASN_APP_FLOAT     (ASN_APPLICATION | 8)
#define ASN_APP_DOUBLE    (ASN_APPLICATION | 9)
#define ASN_APP_I64       (ASN_APPLICATION | 10)
#define ASN_APP_U64       (ASN_APPLICATION | 11)
#define ASN_APP_UNION     (ASN_PRIVATE | 1) /* or ASN_PRIV_UNION ? */

/*
 * value for Counter64
 */
#define ASN_OPAQUE_COUNTER64            (ASN_OPAQUE_TAG2 + ASN_APP_COUNTER64)
/*
 * max size of BER encoding of Counter64
 */
#define ASN_OPAQUE_COUNTER64_MX_BER_LEN 12

/*
 * value for Float
 */
#define ASN_OPAQUE_FLOAT         (ASN_OPAQUE_TAG2 + ASN_APP_FLOAT)
/*
 * size of BER encoding of Float
 */
#define ASN_OPAQUE_FLOAT_BER_LEN 7

/*
 * value for Double
 */
#define ASN_OPAQUE_DOUBLE         (ASN_OPAQUE_TAG2 + ASN_APP_DOUBLE)
/*
 * size of BER encoding of Double
 */
#define ASN_OPAQUE_DOUBLE_BER_LEN 11

/*
 * value for Integer64
 */
#define ASN_OPAQUE_I64            (ASN_OPAQUE_TAG2 + ASN_APP_I64)
/*
 * max size of BER encoding of Integer64
 */
#define ASN_OPAQUE_I64_MX_BER_LEN 11

/*
 * value for Unsigned64
 */
#define ASN_OPAQUE_U64            (ASN_OPAQUE_TAG2 + ASN_APP_U64)
/*
 * max size of BER encoding of Unsigned64
 */
#define ASN_OPAQUE_U64_MX_BER_LEN 12

//#endif /* NETSNMP_WITH_OPAQUE_SPECIAL_TYPES */

#define ASN_PRIV_INCL_RANGE        (ASN_PRIVATE | 2)
#define ASN_PRIV_EXCL_RANGE        (ASN_PRIVATE | 3)
#define ASN_PRIV_DELEGATED         (ASN_PRIVATE | 5)
#define ASN_PRIV_IMPLIED_OCTET_STR (ASN_PRIVATE | ASN_OCTET_STR) /* 4 */
#define ASN_PRIV_IMPLIED_OBJECT_ID (ASN_PRIVATE | ASN_OBJECT_ID) /* 6 */
#define ASN_PRIV_RETRY             (ASN_PRIVATE | 7)             /* 199 */
#define ASN_PRIV_STOP              (ASN_PRIVATE | 8)             /* 200 */
#define IS_DELEGATED(x)            ((x) == ASN_PRIV_DELEGATED)


int asn_check_packet(uint8_t *, size_t);

uint8_t *asn_parse_int(uint8_t *, size_t *, uint8_t *, long *, size_t);

uint8_t *asn_build_int(uint8_t *, size_t *, uint8_t, const long *, size_t);

uint8_t *asn_parse_unsigned_int(uint8_t *, size_t *, uint8_t *, uint32_t *, size_t);

uint8_t *asn_build_unsigned_int(uint8_t *, size_t *, uint8_t, const uint32_t *, size_t);

uint8_t *asn_parse_string(uint8_t *, size_t *, uint8_t *, uint8_t *, size_t *);

uint8_t *asn_build_string(uint8_t *, size_t *, uint8_t, const uint8_t *, size_t);

uint8_t *asn_parse_header(uint8_t *, size_t *, uint8_t *);

uint8_t *asn_parse_sequence(uint8_t *, size_t *, uint8_t *,
			   uint8_t expected_type, /* must be this type */
			   const char *estr);    /* error message prefix */

uint8_t *asn_build_header(uint8_t *, size_t *, uint8_t, size_t);

uint8_t *asn_build_sequence(uint8_t *, size_t *, uint8_t, size_t);

uint8_t *asn_parse_length(uint8_t *, uint32_t *);

uint8_t *asn_build_length(uint8_t *, size_t *, size_t);

uint8_t *asn_parse_objid(uint8_t *, size_t *, uint8_t *, oid *, size_t *);

uint8_t *asn_build_objid(uint8_t *, size_t *, uint8_t, const oid *, size_t);

uint8_t *asn_parse_null(uint8_t *, size_t *, uint8_t *);

uint8_t *asn_build_null(uint8_t *, size_t *, uint8_t);

uint8_t *asn_parse_bitstring(uint8_t *, size_t *, uint8_t *, uint8_t *, size_t *);

uint8_t *asn_build_bitstring(uint8_t *, size_t *, uint8_t, const uint8_t *, size_t);

uint8_t *asn_parse_unsigned_int64(uint8_t *, size_t *, uint8_t *, struct counter64 *, size_t);

uint8_t *asn_build_unsigned_int64(uint8_t *, size_t *, uint8_t, const struct counter64 *, size_t);

uint8_t *asn_parse_signed_int64(uint8_t *, size_t *, uint8_t *, struct counter64 *, size_t);

uint8_t *asn_build_signed_int64(uint8_t *, size_t *, uint8_t, const struct counter64 *, size_t);

uint8_t *asn_build_float(uint8_t *, size_t *, uint8_t, const float *, size_t);

uint8_t *asn_parse_float(uint8_t *, size_t *, uint8_t *, float *, size_t);

uint8_t *asn_build_double(uint8_t *, size_t *, uint8_t, const double *, size_t);

uint8_t *asn_parse_double(uint8_t *, size_t *, uint8_t *, double *, size_t);

#ifdef NETSNMP_USE_REVERSE_ASNENCODING

/*
 * Re-allocator function for below.
 */


int asn_realloc(uint8_t **, size_t *);

/*
 * Re-allocating reverse ASN.1 encoder functions.  Synopsis:
 *
 * uint8_t *buf = (uint8_t*)malloc(100);
 * uint8_t type = (ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER);
 * size_t buf_len = 100, offset = 0;
 * long data = 12345;
 * int allow_realloc = 1;
 *
 * if (asn_realloc_rbuild_int(&buf, &buf_len, &offset, allow_realloc,
 * type, &data, sizeof(long)) == 0) {
 * error;
 * }
 *
 * NOTE WELL: after calling one of these functions with allow_realloc
 * non-zero, buf might have moved, buf_len might have grown and
 * offset will have increased by the size of the encoded data.
 * You should **NEVER** do something like this:
 *
 * uint8_t *buf = (uint8_t *)malloc(100), *ptr;
 * uint8_t type = (ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER);
 * size_t buf_len = 100, offset = 0;
 * long data1 = 1234, data2 = 5678;
 * int rc = 0, allow_realloc = 1;
 *
 * rc  = asn_realloc_rbuild_int(&buf, &buf_len, &offset, allow_realloc,
 * type, &data1, sizeof(long));
 * ptr = buf[buf_len - offset];   / * points at encoding of data1 * /
 * if (rc == 0) {
 * error;
 * }
 * rc  = asn_realloc_rbuild_int(&buf, &buf_len, &offset, allow_realloc,
 * type, &data2, sizeof(long));
 * make use of ptr here;
 *
 *
 * ptr is **INVALID** at this point.  In general, you should store the
 * offset value and compute pointers when you need them:
 *
 *
 *
 * uint8_t *buf = (uint8_t *)malloc(100), *ptr;
 * uint8_t type = (ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER);
 * size_t buf_len = 100, offset = 0, ptr_offset;
 * long data1 = 1234, data2 = 5678;
 * int rc = 0, allow_realloc = 1;
 *
 * rc  = asn_realloc_rbuild_int(&buf, &buf_len, &offset, allow_realloc,
 * type, &data1, sizeof(long));
 * ptr_offset = offset;
 * if (rc == 0) {
 * error;
 * }
 * rc  = asn_realloc_rbuild_int(&buf, &buf_len, &offset, allow_realloc,
 * type, &data2, sizeof(long));
 * ptr = buf + buf_len - ptr_offset
 * make use of ptr here;
 *
 *
 *
 * Here, you can see that ptr will be a valid pointer even if the block of
 * memory has been moved, as it may well have been.  Plenty of examples of
 * usage all over asn1.c, snmp_api.c, snmpusm.c.
 *
 * The other thing you should **NEVER** do is to pass a pointer to a buffer
 * on the stack as the first argument when allow_realloc is non-zero, unless
 * you really know what you are doing and your machine/compiler allows you to
 * free non-heap memory.  There are rumours that such things exist, but many
 * consider them no more than the wild tales of a fool.
 *
 * Of course, you can pass allow_realloc as zero, to indicate that you do not
 * wish the packet buffer to be reallocated for some reason; perhaps because
 * it is on the stack.  This may be useful to emulate the functionality of
 * the old API:
 *
 * uint8_t my_static_buffer[100], *cp = NULL;
 * size_t my_static_buffer_len = 100;
 * float my_pi = (float)22/(float)7;
 *
 * cp = asn_rbuild_float(my_static_buffer, &my_static_buffer_len,
 * ASN_OPAQUE_FLOAT, &my_pi, sizeof(float));
 * if (cp == NULL) {
 * error;
 * }
 *
 *
 * IS EQUIVALENT TO:
 *
 *
 * uint8_t my_static_buffer[100];
 * size_t my_static_buffer_len = 100, my_offset = 0;
 * float my_pi = (float)22/(float)7;
 * int rc = 0;
 *
 * rc = asn_realloc_rbuild_float(&my_static_buffer, &my_static_buffer_len,
 * &my_offset, 0,
 * ASN_OPAQUE_FLOAT, &my_pi, sizeof(float));
 * if (rc == 0) {
 * error;
 * }
 *
 *
 */


int asn_realloc_rbuild_int(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
			   uint8_t type, const long *data, size_t data_size);


int asn_realloc_rbuild_string(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
			      uint8_t type, const uint8_t *data, size_t data_size);


int asn_realloc_rbuild_unsigned_int(uint8_t **pkt, size_t *pkt_len, size_t *offset,
				    int allow_realloc, uint8_t type, const uint32_t *data,
				    size_t data_size);


int asn_realloc_rbuild_header(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
			      uint8_t type, size_t data_size);


int asn_realloc_rbuild_sequence(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
				uint8_t type, size_t data_size);


int asn_realloc_rbuild_length(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
			      size_t data_size);


int asn_realloc_rbuild_objid(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
			     uint8_t type, const oid *, size_t);


int asn_realloc_rbuild_null(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
			    uint8_t type);


int asn_realloc_rbuild_bitstring(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
				 uint8_t type, const uint8_t *data, size_t data_size);


int asn_realloc_rbuild_unsigned_int64(uint8_t **pkt, size_t *pkt_len, size_t *offset,
				      int allow_realloc, uint8_t type, struct counter64 const *data,
				      size_t);


int asn_realloc_rbuild_signed_int64(uint8_t **pkt, size_t *pkt_len, size_t *offset,
				    int allow_realloc, uint8_t type, const struct counter64 *data,
				    size_t);


int asn_realloc_rbuild_float(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
			     uint8_t type, const float *data, size_t data_size);


int asn_realloc_rbuild_double(uint8_t **pkt, size_t *pkt_len, size_t *offset, int allow_realloc,
			      uint8_t type, const double *data, size_t data_size);

#endif
#endif /* ASN1_H */
