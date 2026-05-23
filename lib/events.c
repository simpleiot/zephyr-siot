// Need some includes up here
#include <point.h>
#include <nvs.h>
#include <events.h>
#include <dispatcher.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#define STACKSIZE 2048
#define PRIORITY  7

#ifdef CONFIG_EVENTS_ENABLED

K_THREAD_STACK_DEFINE(events_stack, STACKSIZE);
static struct k_thread events_data;

#define NVS_EVENT_QUEUE_SIZE CONFIG_NVS_EVENT_QUEUE_SIZE
#define RAM_EVENT_QUEUE_SIZE CONFIG_RAM_EVENT_QUEUE_SIZE

LOG_MODULE_REGISTER(events, LOG_LEVEL_INF);

/* zbus channels - ticker_chan for timing */
ZBUS_CHAN_DECLARE(ticker_chan);

/* Ticker still uses direct zbus subscription */
ZBUS_MSG_SUBSCRIBER_DEFINE(events_sub);
ZBUS_CHAN_ADD_OBS(ticker_chan, events_sub, 4);

/* Message queue for points from dispatcher */
K_MSGQ_DEFINE(events_msgq, sizeof(point), CONFIG_DISPATCHER_QUEUE_DEPTH, 4);

K_MUTEX_DEFINE(events_lock);

static struct nvs_fs event_history_fs;

// Function declarations
int nvs_event_history_init(void);
int nvs_event_history_store(const point *p, int count);
int nvs_event_history_read(point *events, int *count, int *head, int *tail);

// NVS events are now stored in NVS, not external RAM
__attribute__((section(".ext_ram.bss"))) static point nvs_events[NVS_EVENT_QUEUE_SIZE];
__attribute__((section(".ext_ram.bss"))) static point ram_events[RAM_EVENT_QUEUE_SIZE];
__attribute__((section(
	".ext_ram.bss"))) static point all_events[NVS_EVENT_QUEUE_SIZE + RAM_EVENT_QUEUE_SIZE];

// Event history storage uses a different partition or offset
#define EVENT_HISTORY_NVS_PARTITION        storage_partition
#define EVENT_HISTORY_NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(EVENT_HISTORY_NVS_PARTITION)
#define EVENT_HISTORY_NVS_PARTITION_OFFSET                                                         \
	FIXED_PARTITION_OFFSET(EVENT_HISTORY_NVS_PARTITION) + 0x3000

// Event history storage IDs (separate from main NVS IDs)
#define EVENT_HISTORY_COUNT_ID      1000
#define EVENT_HISTORY_HEAD_ID       1001
#define EVENT_HISTORY_TAIL_ID       1002
#define EVENT_HISTORY_DATA_START_ID 1010

typedef struct {
	int head;
	int tail;
	int count;
} queue_info;

static queue_info nvs_event_queue_info = {0, 0, 0};
static queue_info ram_event_queue_info = {0, 0, 0};

// Application event filter callback
static event_filter_callback_t app_event_filter = NULL;

// Create the get and add functions for critical queue (NVS)
int get_all_events(uint8_t *recv_buffer, size_t buffer_size)
{

	int all_position = 0;

	k_mutex_lock(&events_lock, K_FOREVER);

	// Copy NVS events if any exist
	if (nvs_event_queue_info.count > 0) {
		int i = nvs_event_queue_info.tail;
		int count = 0;
		while (count < nvs_event_queue_info.count) {
			all_events[all_position] = nvs_events[i];
			all_position++;
			i = (i + 1) % NVS_EVENT_QUEUE_SIZE;
			count++;
		}
	}

	// Copy RAM events if any exist
	if (ram_event_queue_info.count > 0) {
		int i = ram_event_queue_info.tail;
		int count = 0;
		while (count < ram_event_queue_info.count) {
			all_events[all_position] = ram_events[i];
			all_position++;
			i = (i + 1) % RAM_EVENT_QUEUE_SIZE;
			count++;
		}
	}

	int ret = points_json_encode(all_events, all_position, recv_buffer, buffer_size);

	k_mutex_unlock(&events_lock);

	if (ret < 0) {
		return -1;
	}

	return 0;
}

static void add_nvs_event(point p)
{

	int rc = 0;

	k_mutex_lock(&events_lock, K_FOREVER);

	// Add new item at current head position
	nvs_events[nvs_event_queue_info.head] = p;

	// Increment head for next item
	nvs_event_queue_info.head++;

	if (nvs_event_queue_info.head >= NVS_EVENT_QUEUE_SIZE) {
		nvs_event_queue_info.head = 0;
		rc = nvs_write(&event_history_fs, EVENT_HISTORY_HEAD_ID, &nvs_event_queue_info.head,
			       sizeof(nvs_event_queue_info.head));
		if (rc < 0) {
			LOG_ERR("Failed to store events head: %d", rc);
			return;
		}
	}

	if (nvs_event_queue_info.count < NVS_EVENT_QUEUE_SIZE) {
		nvs_event_queue_info.count++;
		rc = nvs_write(&event_history_fs, EVENT_HISTORY_COUNT_ID,
			       &nvs_event_queue_info.count, sizeof(nvs_event_queue_info.count));
		if (rc < 0) {
			LOG_ERR("Failed to store events count: %d", rc);
			return;
		}
	}
	if (nvs_event_queue_info.count == NVS_EVENT_QUEUE_SIZE) {
		// if we just added our tenth item, or any subsequent 10th item
		nvs_event_queue_info.tail = (nvs_event_queue_info.tail + 1) % NVS_EVENT_QUEUE_SIZE;
		rc = nvs_write(&event_history_fs, EVENT_HISTORY_TAIL_ID, &nvs_event_queue_info.tail,
			       sizeof(nvs_event_queue_info.tail));
		if (rc < 0) {
			LOG_ERR("Failed to store events tail: %d", rc);
			return;
		}
	}

	// Store the updated events in NVS
	nvs_event_history_store(&p, nvs_event_queue_info.head);

	k_mutex_unlock(&events_lock);
}

static void add_ram_event(point p)
{

	k_mutex_lock(&events_lock, K_FOREVER);

	// Add new item at current head position
	ram_events[ram_event_queue_info.head] = p;

	// Increment head for next item
	ram_event_queue_info.head++;
	if (ram_event_queue_info.head >= RAM_EVENT_QUEUE_SIZE) {
		ram_event_queue_info.head = 0;
	}

	if (ram_event_queue_info.count < RAM_EVENT_QUEUE_SIZE) {
		ram_event_queue_info.count++;
	}

	if (ram_event_queue_info.count == RAM_EVENT_QUEUE_SIZE) {
		// Buffer is full, advance tail to overwrite oldest
		ram_event_queue_info.tail = (ram_event_queue_info.tail + 1) % RAM_EVENT_QUEUE_SIZE;
	}

	k_mutex_unlock(&events_lock);
}

/* External event configuration table - defined by application */
extern const event_config_t event_configs[];
extern const size_t event_configs_count;

// Void thread listening to point channel, ticker chan, acting accordingly
static void events_thread(void *arg1, void *arg2, void *arg3)
{
	/* Build subscription list from event_configs (unique types only) */
	const char *subs[32]; /* Max 32 unique types */
	int sub_count = 0;

	for (size_t i = 0; i < event_configs_count && sub_count < ARRAY_SIZE(subs) - 1; i++) {
		const char *type = event_configs[i].type;
		bool found = false;

		/* Check if already in list */
		for (int j = 0; j < sub_count; j++) {
			if (strcmp(subs[j], type) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			subs[sub_count++] = type;
		}
	}
	subs[sub_count] = NULL; /* NULL-terminate */

	/* Register with dispatcher */
	dispatcher_thread_register(&events_msgq, "events", subs);

	k_mutex_lock(&events_lock, K_FOREVER);

	// Initialize NVS events system
	int rc = nvs_event_history_init();
	if (rc != 0) {
		LOG_ERR("Failed to initialize NVS event history: %d", rc);
	} else {
		// Load existing events from NVS
		rc = nvs_event_history_read(nvs_events, &nvs_event_queue_info.count,
					    &nvs_event_queue_info.head, &nvs_event_queue_info.tail);
		if (rc != 0) {
			LOG_ERR("Failed to read NVS event history: %d", rc);
			// Initialize to empty state if read fails
			nvs_event_queue_info.count = 0;
			nvs_event_queue_info.head = 0;
			nvs_event_queue_info.tail = 0;
		}
		LOG_DBG("Loaded %d events from NVS event history", nvs_event_queue_info.count);
	}

	// Initialize all events arrays to ensure type[0] is 0 for empty points
	for (int i = 0; i < ARRAY_SIZE(ram_events); i++) {
		memset(&ram_events[i], 0, sizeof(point));
	}
	for (int i = 0; i < ARRAY_SIZE(all_events); i++) {
		memset(&all_events[i], 0, sizeof(point));
	}

	k_mutex_unlock(&events_lock);

	LOG_DBG("Arrays initialized");

	point p;
	int heartbeat_tick = 0;
	const struct zbus_channel *chan;
	uint8_t tick;

	while (!zbus_sub_wait_msg(&events_sub, &chan, &tick, K_FOREVER)) {
		/* Process all queued points from dispatcher (non-blocking) */
		while (k_msgq_get(&events_msgq, &p, K_NO_WAIT) == 0) {
			// Check if this point type should generate events
			bool point_matches = false;
			bool store_in_nvs = false;

			for (int i = 0; i < event_configs_count; i++) {
				if (strcmp(p.type, event_configs[i].type) == 0 &&
				    strcmp(p.key, event_configs[i].key) == 0) {
					point_matches = true;
					store_in_nvs = event_configs[i].store_in_nvs;
					break;
				}
			}

			if (point_matches) {
				// Check application filter if available
				bool should_add_event = true;
				if (app_event_filter != NULL) {
					should_add_event = app_event_filter(&p);
				}

				if (should_add_event) {
					if (store_in_nvs) {
						add_nvs_event(p);
					} else {
						add_ram_event(p);
					}
				}
			}
		}

		/* Ticker processing - happens on every zbus wakeup */
		if (chan == &ticker_chan) {
			heartbeat_tick++;

			if (heartbeat_tick >= 10) {
				point heartbeat_point;
				point_init(&heartbeat_point, POINT_TYPE_HEARTBEAT, "events");
				point_put_int(&heartbeat_point, 1);
				dispatcher_send_point(&heartbeat_point);
				heartbeat_tick = 0;
			}
		}
	}
}

void events_start_thread(void)
{
	k_thread_create(&events_data, events_stack, K_THREAD_STACK_SIZEOF(events_stack),
			events_thread, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
}

// Event array storage functions
int nvs_event_history_init(void)
{

	struct flash_pages_info info;
	int rc = 0;

	event_history_fs.flash_device = EVENT_HISTORY_NVS_PARTITION_DEVICE;
	if (!device_is_ready(event_history_fs.flash_device)) {
		LOG_ERR("Events flash device %s is not ready\n",
			event_history_fs.flash_device->name);
		return -1;
	}
	event_history_fs.offset = EVENT_HISTORY_NVS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(event_history_fs.flash_device, event_history_fs.offset,
					 &info);
	if (rc) {
		LOG_ERR("Unable to get page info for events\n");
		return -1;
	}
	event_history_fs.sector_size = info.size;
	event_history_fs.sector_count = 2U;

	rc = nvs_mount(&event_history_fs);
	if (rc) {
		LOG_ERR("Events Flash Init failed\n");
		return -1;
	}

	LOG_DBG("Events NVS initialized successfully");
	return 0;
}

int nvs_event_history_store(const point *p, int head)
{
	int rc;

	// we should just store the new item at the new head
	int data_id = EVENT_HISTORY_DATA_START_ID + head;
	rc = nvs_write(&event_history_fs, data_id, p, sizeof(*p));

	return rc;
}

int nvs_event_history_read(point *events, int *count, int *head, int *tail)
{
	int rc;

	// Initialize the events array to prevent partially initialized structures
	memset(events, 0, NVS_EVENT_QUEUE_SIZE * sizeof(point));

	// Read metadata
	rc = nvs_read(&event_history_fs, EVENT_HISTORY_COUNT_ID, count, sizeof(*count));
	if (rc < 0) {
		LOG_DBG("No events count found, initializing to 0");
		*count = 0;
		*head = 0;
		*tail = 0;
		return 0;
	}

	// Validate count
	if (*count < 0 || *count > NVS_EVENT_QUEUE_SIZE) {
		LOG_ERR("Invalid count read from NVS: %d", *count);
		*count = 0;
		*head = 0;
		*tail = 0;
		return 0;
	}

	rc = nvs_read(&event_history_fs, EVENT_HISTORY_HEAD_ID, head, sizeof(*head));
	if (rc < 0) {
		LOG_ERR("Failed to read events head: %d", rc);
		return rc;
	}

	rc = nvs_read(&event_history_fs, EVENT_HISTORY_TAIL_ID, tail, sizeof(*tail));
	if (rc < 0) {
		LOG_ERR("Failed to read events tail: %d", rc);
		return rc;
	}

	// Read event data
	for (int i = 0; i < *count; i++) {
		int data_id = EVENT_HISTORY_DATA_START_ID + i;
		rc = nvs_read(&event_history_fs, data_id, &events[i], sizeof(point));
		if (rc < 0) {
			LOG_ERR("Failed to read event %d: %d", i, rc);
			return rc;
		}
	}

	LOG_DBG("Read %d events (head=%d, tail=%d)", *count, *head, *tail);
	return 0;
}

int nvs_event_history_clear(void)
{
	int rc;

	int zero = 0;
	rc = nvs_write(&event_history_fs, EVENT_HISTORY_COUNT_ID, &zero, sizeof(zero));
	if (rc < 0) {
		LOG_ERR("Failed to clear events count: %d", rc);
		return rc;
	}

	rc = nvs_write(&event_history_fs, EVENT_HISTORY_HEAD_ID, &zero, sizeof(zero));
	if (rc < 0) {
		LOG_ERR("Failed to clear events head: %d", rc);
		return rc;
	}

	rc = nvs_write(&event_history_fs, EVENT_HISTORY_TAIL_ID, &zero, sizeof(zero));
	if (rc < 0) {
		LOG_ERR("Failed to clear events tail: %d", rc);
		return rc;
	}

	LOG_DBG("Events NVS cleared");
	return 0;
}

static int handle_events_help(const struct shell *shell, size_t argc, char **argv)
{

	shell_print(shell, "Event Commands:");
	shell_print(shell, "  events clear all - clear all or some stored events");
	shell_print(shell, "  events history - Show all stored points with timestamps");
	shell_print(shell, "  events help  - Show this help message");

	return 0;
}

static int handle_events_clear(const struct shell *shell, size_t argc, char **argv)
{

	nvs_event_history_clear();

	shell_print(shell, "NVS events cleared");

	for (int i = 0; i < ARRAY_SIZE(ram_events); i++) {
		memset(&ram_events[i], 0, sizeof(point));
	}
	for (int i = 0; i < ARRAY_SIZE(all_events); i++) {
		memset(&all_events[i], 0, sizeof(point));
	}

	ram_event_queue_info.head = 0;
	ram_event_queue_info.tail = 0;
	ram_event_queue_info.count = 0;

	nvs_event_queue_info.head = 0;
	nvs_event_queue_info.tail = 0;
	nvs_event_queue_info.count = 0;

	shell_print(shell, "RAM events cleared");

	return 0;
}

static int handle_events_history(const struct shell *shell, size_t argc, char **argv)
{

	char buf[200];

	if (nvs_event_queue_info.count > 0) {
		shell_print(shell, "NVS Events (Persisted Between Reboots)");
		int i = nvs_event_queue_info.tail;
		int count = 0;
		while (count < nvs_event_queue_info.count) {
			point_dump(&nvs_events[i], buf, sizeof(buf));
			shell_print(shell, "%s", buf);
			i = (i + 1) % NVS_EVENT_QUEUE_SIZE;
			count++;
		}
	} else {
		shell_print(shell, "No NVS Events (Persisted between reboots)");
	}

	if (ram_event_queue_info.count > 0) {
		shell_print(shell, "RAM Events (Reset Between Reboots)");
		int j = ram_event_queue_info.tail;
		int count2 = 0;
		while (count2 < ram_event_queue_info.count) {
			point_dump(&ram_events[j], buf, sizeof(buf));
			shell_print(shell, "%s", buf);
			j = (j + 1) % NVS_EVENT_QUEUE_SIZE;
			count2++;
		}
	} else {
		shell_print(shell, "No RAM Events (Reset between reboots)");
	}

	return 0;
}

static int handle_events_command(const struct shell *shell, size_t argc, char **argv)
{

	if (argc < 2) {
		shell_print(shell, "Usage: events <command> [args...]");
		shell_print(shell, "Commands: clear, history");
		shell_print(shell, "Use 'events help' for detailed usage information");
		return -1;
	}

	char *send_argv[] = {argv[0], argv[2], argv[3], argv[4], argv[5]};

	if (strcmp(argv[1], "history") == 0) {
		handle_events_history(shell, argc, send_argv);
	} else if (strcmp(argv[1], "clear") == 0) {
		handle_events_clear(shell, argc, send_argv);
	} else if (strcmp(argv[1], "help") == 0) {
		handle_events_help(shell, argc, send_argv);
	} else {
		shell_print(shell, "Unknown command: %s", argv[1]);
		shell_print(shell, "Use 'events help' for usage information");
		return -1;
	}

	return 0;
}

SHELL_CMD_REGISTER(events, NULL, "Events commands (clear, history)", handle_events_command);

// Set application event filter callback
void events_set_filter_callback(event_filter_callback_t callback)
{
	app_event_filter = callback;
	LOG_DBG("Event filter callback %s", callback ? "set" : "cleared");
}

#endif // CONFIG_EVENTS_ENABLED
