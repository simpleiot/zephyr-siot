#ifndef __TIMECONV_H_
#define __TIMECONV_H_

#include <stdint.h>
#include <stddef.h>

// Buffer size needed for an RFC3339 timestamp produced by
// timeconv_rfc3339_from_epoch_ns_utc: "YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ" plus
// the null terminator.
#define RFC3339_MAX_LEN 31

// Format epoch nanoseconds as RFC3339 UTC with a fixed nine digit fractional
// second, e.g. "2026-07-31T12:00:00.123456789Z".
//
// The fraction is always nine digits, never trimmed. A variable width encoding
// is not canonical: it makes byte identical round trips depend on both ends
// agreeing on a trimming rule, and the resulting strings do not sort.
//
// Returns 0 on success, <0 on error.
int timeconv_rfc3339_from_epoch_ns_utc(uint64_t epoch_ns, char *buf, size_t buf_len);

// Parse an RFC3339 UTC timestamp to epoch nanoseconds. Accepts
// "YYYY-MM-DDTHH:MM:SSZ" with an optional fractional second of any length,
// e.g. "2026-07-31T12:00:00Z" or "2026-07-31T12:00:00.123456789Z".
//
// Only the "Z" zone is accepted. SIOT always sends UTC, which keeps this
// parser small.
//
// Returns epoch ns, or 0 if the input cannot be parsed. Callers treat 0 as
// "no timestamp" rather than as the Unix epoch.
uint64_t timeconv_epoch_ns_from_rfc3339(const char *buf, size_t buf_len);

#endif // __TIMECONV_H_
