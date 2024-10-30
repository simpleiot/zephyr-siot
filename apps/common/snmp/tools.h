/**
 * @file library/tools.h
 * @defgroup util Memory Utility Routines
 * @ingroup library
 * @{
 *
 * Portions of this file are copyrighted by:
 * Copyright (c) 2016 VMware, Inc. All rights reserved.
 * Use is subject to license terms specified in the COPYING file
 * distributed with the Net-SNMP package.
 */

#ifndef _TOOLS_H
#define _TOOLS_H

#include <zephyr/types.h>

#define SNMP_MAXBUF        (1024 * 4)
#define SNMP_MAXBUF_MEDIUM 1024
#define SNMP_MAXBUF_SMALL  512

#define SNMP_MAXBUF_MESSAGE 1500

#define SNMP_MAXOID           64
#define SNMP_MAX_CMDLINE_OIDS 128

#define SNMP_FILEMODE_CLOSED 0600
#define SNMP_FILEMODE_OPEN   0644

#define BYTESIZE(bitsize) ((bitsize + 7) >> 3)
#define ROUNDUP8(x)       (((x + 7) >> 3) * 8)

#define SNMP_STRORNULL(x) (x ? x : "(null)")

/** @def SNMP_FREE(s)
    Frees a pointer only if it is !NULL and sets its value to NULL */
#define SNMP_FREE(s)                                                                               \
	do {                                                                                       \
		if (s) {                                                                           \
			free(s);                                                                   \
			s = NULL;                                                                  \
		}                                                                                  \
	} while (0)

/*
 * XXX Not optimal everywhere.
 */
/** @def SNMP_MALLOC_STRUCT(s)
    Mallocs memory of sizeof(struct s), zeros it and returns a pointer to it. */
#define SNMP_MALLOC_STRUCT(s) (struct s *)calloc(1, sizeof(struct s))

/** @def SNMP_MALLOC_TYPEDEF(t)
    Mallocs memory of sizeof(t), zeros it and returns a pointer to it. */
#define SNMP_MALLOC_TYPEDEF(td) (td *)calloc(1, sizeof(td))

/** @def SNMP_ZERO(s,l)
    Zeros l bytes of memory starting at s. */
#define SNMP_ZERO(s, l)                                                                            \
	do {                                                                                       \
		if (s)                                                                             \
			memset(s, 0, l);                                                           \
	} while (0)

/**
 * @def NETSNMP_REMOVE_CONST(t, e)
 *
 * Cast away constness without that gcc -Wcast-qual prints a compiler warning,
 * similar to const_cast<> in C++.
 *
 * @param[in] t A pointer type.
 * @param[in] e An expression of a type that can be assigned to the type (const t).
 */
#if defined(__GNUC__)
#define NETSNMP_REMOVE_CONST(t, e)                                                                 \
	(__extension__({                                                                           \
		const t tmp = (e);                                                                 \
		(t)(size_t) tmp;                                                                   \
	}))
#else
#define NETSNMP_REMOVE_CONST(t, e) ((t)(size_t)(e))
#endif

#define TOUPPER(c) (c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c)
#define TOLOWER(c) (c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c)

#define HEX2VAL(s) ((isalpha(s) ? (TOLOWER(s) - 'a' + 10) : (TOLOWER(s) - '0')) & 0xf)
#define VAL2HEX(s) ((s) + (((s) >= 10) ? ('a' - 10) : '0'))

/** @def SNMP_MAX(a, b)
    Computers the maximum of a and b. */
#define SNMP_MAX(a, b) ((a) > (b) ? (a) : (b))

/** @def SNMP_MIN(a, b)
    Computers the minimum of a and b. */
#define SNMP_MIN(a, b) ((a) > (b) ? (b) : (a))

/** @def SNMP_MACRO_VAL_TO_STR(s)
 *  Expands to string with value of the s.
 *  If s is macro, the resulting string is value of the macro.
 *  Example:
 *   \#define TEST 1234
 *   SNMP_MACRO_VAL_TO_STR(TEST) expands to "1234"
 *   SNMP_MACRO_VAL_TO_STR(TEST+1) expands to "1234+1"
 */
#define SNMP_MACRO_VAL_TO_STR(s)      SNMP_MACRO_VAL_TO_STR_PRIV(s)
#define SNMP_MACRO_VAL_TO_STR_PRIV(s) #s

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define NETSNMP_IGNORE_RESULT(e)                                                                   \
	do {                                                                                       \
		if (e) {                                                                           \
		}                                                                                  \
	} while (0)

/*
 * QUIT the FUNction:
 *      e       Error code variable
 *      l       Label to goto to cleanup and get out of the function.
 *
 * XXX  It would be nice if the label could be constructed by the
 *      preprocessor in context.  Limited to a single error return value.
 *      Temporary hack at best.
 */
#define QUITFUN(e, l)                                                                              \
	if ((e) != SNMPERR_SUCCESS) {                                                              \
		rval = SNMPERR_GENERR;                                                             \
		goto l;                                                                            \
	}

/**
 * Compute res = a + b.
 *
 * @pre a and b must be normalized 'struct timeval' values.
 *
 * @note res may be the same variable as one of the operands. In other
 *   words, &a == &res || &b == &res may hold.
 */
#define NETSNMP_TIMERADD(a, b, res)                                                                \
	do {                                                                                       \
		(res)->tv_sec = (a)->tv_sec + (b)->tv_sec;                                         \
		(res)->tv_usec = (a)->tv_usec + (b)->tv_usec;                                      \
		if ((res)->tv_usec >= 1000000L) {                                                  \
			(res)->tv_usec -= 1000000L;                                                \
			(res)->tv_sec++;                                                           \
		}                                                                                  \
	} while (0)

/**
 * Compute res = a - b.
 *
 * @pre a and b must be normalized 'struct timeval' values.
 *
 * @note res may be the same variable as one of the operands. In other
 *   words, &a == &res || &b == &res may hold.
 */
#define NETSNMP_TIMERSUB(a, b, res)                                                                \
	do {                                                                                       \
		(res)->tv_sec = (a)->tv_sec - (b)->tv_sec - 1;                                     \
		(res)->tv_usec = (a)->tv_usec - (b)->tv_usec + 1000000L;                           \
		if ((res)->tv_usec >= 1000000L) {                                                  \
			(res)->tv_usec -= 1000000L;                                                \
			(res)->tv_sec++;                                                           \
		}                                                                                  \
	} while (0)

#define ENGINETIME_MAX 2147483647 /* ((2^31)-1) */
#define ENGINEBOOT_MAX 2147483647 /* ((2^31)-1) */

struct timeval;

/*
 * Prototypes.
 */

int snmp_realloc(uint8_t **buf, size_t *buf_len);

void free_zero(void *buf, size_t size);

uint8_t *malloc_random(size_t *size);
uint8_t *malloc_zero(size_t size);
void *netsnmp_memdup(const void *from, size_t size);
void *netsnmp_memdup_nt(const void *from, size_t from_len, size_t *to_len);

void netsnmp_check_definedness(const void *packet, size_t length);

int netsnmp_binary_to_hex(uint8_t **dest, size_t *dest_len, int allow_realloc, const uint8_t *input,
			  size_t len);

int binary_to_hex(const uint8_t *input, size_t len, char **output);
/* preferred */
int netsnmp_hex_to_binary(uint8_t **buf, size_t *buf_len, size_t *offset, int allow_realloc,
			  const char *hex, const char *delim);
/* calls netsnmp_hex_to_binary w/delim of " " */
int snmp_hex_to_binary(uint8_t **buf, size_t *buf_len, size_t *offset, int allow_realloc,
		       const char *hex);
/* handles odd lengths */
int hex_to_binary2(const uint8_t *input, size_t len, char **output);

int snmp_decimal_to_binary(uint8_t **buf, size_t *buf_len, size_t *out_len, int allow_realloc,
			   const char *decimal);
int snmp_strcat(uint8_t **buf, size_t *buf_len, size_t *out_len, int allow_realloc,
		const uint8_t *s);
int snmp_cstrcat(uint8_t **buf, size_t *buf_len, size_t *out_len, int allow_realloc, const char *s)
{
	return snmp_strcat(buf, buf_len, out_len, allow_realloc, (const uint8_t *)s);
}
char *netsnmp_strdup_and_null(const uint8_t *from, size_t from_len);

void dump_chunk(const char *debugtoken, const char *title, const uint8_t *buf, int size);
char *dump_snmpEngineID(const uint8_t *buf, size_t *buflen);

/** A pointer to an opaque time marker value. */
typedef void *marker_t;
typedef const void *const_marker_t;

marker_t atime_newMarker(void);
void atime_setMarker(marker_t pm);
void netsnmp_get_monotonic_clock(struct timeval *tv);
void netsnmp_set_monotonic_marker(marker_t *pm);
long atime_diff(const_marker_t first, const_marker_t second);
long uatime_diff(const_marker_t first, const_marker_t second);  /* 1/1000th sec */
long uatime_hdiff(const_marker_t first, const_marker_t second); /* 1/100th sec */
int atime_ready(const_marker_t pm, int delta_ms);
int netsnmp_ready_monotonic(const_marker_t pm, int delta_ms);
int uatime_ready(const_marker_t pm, unsigned int delta_ms);

int marker_tticks(const_marker_t pm);
int timeval_tticks(const struct timeval *tv);
char *netsnmp_getenv(const char *name);
int netsnmp_setenv(const char *envname, const char *envval, int overwrite);

int netsnmp_addrstr_hton(char *ptr, size_t len);

int netsnmp_string_time_to_secs(const char *time_string);

#endif /* _TOOLS_H */
