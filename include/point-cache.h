#ifndef __POINT_CACHE_H_
#define __POINT_CACHE_H_

#include <point.h>
#include <stdbool.h>

// The point cache holds the most recent value of every point published on
// zbus point_chan. A subscriber thread keeps it current, so any subsystem
// that needs the whole current state can read it without tracking points
// itself.
//
// Applications currently keep private caches of their own (see the web
// server in apps/siot-net). Those are meant to migrate onto this one so the
// tree has a single cache rather than several that can drift apart.
//
// All functions take the internal lock, so callers never handle it.

// Merge a point into the cache. Normally called by the cache's own
// subscriber thread; exposed for tests and for callers that bypass zbus.
// Returns 0 on success, -ENOMEM if the cache is full.
int point_cache_merge(point *p);

// Number of occupied slots.
int point_cache_count(void);

// Call cb for each cached point. The lock is held for the duration, so cb
// must not block or publish to point_chan. Iteration stops early if cb
// returns non-zero, and that value is returned.
int point_cache_foreach(int (*cb)(point *p, void *user_data), void *user_data);

// Encode the whole cache as a JSON array. This is what an HTTP handler
// serving the current state needs.
// Returns 0 on success, <0 on error.
int point_cache_json_encode(char *buf, size_t buf_len);

#ifdef CONFIG_SIOT_POINT_SHELL
// Whether points are currently streamed to the shell as they arrive.
bool point_cache_streaming(void);
#endif

#endif // __POINT_CACHE_H_
