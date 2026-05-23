#ifndef __TIMECONV_H_
#define __TIMECONV_H_

#include <stdint.h>
#include <stddef.h>

// Format RFC3339 from epoch nanoseconds using UTC offset (+00:00)
// Returns 0 on success, <0 on error
int timeconv_rfc3339_from_epoch_ns_utc(uint64_t epoch_ns, char *buf, size_t buf_len);

// Parse RFC3339 string to epoch nanoseconds.
// Only accepts UTC format with Z suffix (e.g., "2024-01-15T12:30:45Z")
// Returns epoch ns, or 0 on parse failure (consistent with existing usage)
uint64_t timeconv_epoch_ns_from_rfc3339(const char *buf, size_t buf_len);

#endif // __TIMECONV_H_
