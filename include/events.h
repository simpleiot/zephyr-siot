// This file is going to need to expose the get functions for the web thread
// We can also probably store some structs in here

#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <point.h>
#include <stdbool.h>

// Forward declaration for event filter callback
typedef bool (*event_filter_callback_t)(const point *p);

// Event configuration structure - defined here, table populated by applications
typedef struct {
	const char *type;
	const char *key;
	bool store_in_nvs;
	float warning_threshold;  // For numeric types (0 = not used)
	float critical_threshold; // For numeric types (0 = not used)
	bool always_generate;     // Bypass filtering (for points that only get generated when an event has occurred)
} event_config_t;

// Application-specific event configuration table
// Should be placed at the top of main.c in application
extern const event_config_t event_configs[];
extern const size_t event_configs_count;

int get_all_events(uint8_t *recv_buffer, size_t buffer_size);

void events_start_thread(void);

// Set application-specific event filtering callback
void events_set_filter_callback(event_filter_callback_t callback);

#endif // __EVENTS_H__
