#include <timeconv.h>
#include <zephyr/sys/timeutil.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(z_timeconv, LOG_LEVEL_INF);

int timeconv_rfc3339_from_epoch_ns_utc(uint64_t epoch_ns, char *buf, size_t buf_len)
{
	if (buf == NULL || buf_len == 0) {
		return -1;
	}

	time_t epoch_seconds = (time_t)(epoch_ns / 1000000000ULL);
	struct tm *utc = gmtime(&epoch_seconds);
	if (utc == NULL) {
		return -2;
	}

	int cnt = snprintf(buf, buf_len, "%04d-%02d-%02dT%02d:%02d:%02dZ", utc->tm_year + 1900,
			   utc->tm_mon + 1, utc->tm_mday, utc->tm_hour, utc->tm_min, utc->tm_sec);
	if (cnt < 0 || (size_t)cnt >= buf_len) {
		return -3;
	}
	return 0;
}

uint64_t timeconv_epoch_ns_from_rfc3339(const char *buf, size_t buf_len)
{
	if (buf == NULL || buf_len == 0) {
		LOG_DBG("timeconv_epoch_ns_from_rfc3339: null/empty input");
		return 0ULL;
	}

	// Make a local copy to ensure null-termination
	char tmp[32];
	size_t cpy = buf_len < sizeof(tmp) - 1 ? buf_len : sizeof(tmp) - 1;
	strncpy(tmp, buf, cpy);
	tmp[cpy] = '\0';

	// Basic validation - must have minimum length for RFC3339
	if (strlen(tmp) < 19) { // YYYY-MM-DDTHH:MM:SS minimum
		LOG_DBG("timeconv_epoch_ns_from_rfc3339: string too short: '%s'", tmp);
		return 0ULL;
	}

	LOG_DBG("Parsing RFC3339: '%s'", tmp);

	// Only accepts UTC format with Z suffix (e.g., "2024-01-15T12:30:45Z")
	int year, mon, day, hour, min, sec;

	LOG_DBG("Before manual parsing");

	int matched;

	// Manual parsing: "2024-01-01T12:02:00Z"
	if (tmp[4] == '-' && tmp[7] == '-' && tmp[10] == 'T' && tmp[13] == ':' && tmp[16] == ':' &&
	    tmp[19] == 'Z') {

		year = (tmp[0] - '0') * 1000 + (tmp[1] - '0') * 100 + (tmp[2] - '0') * 10 +
		       (tmp[3] - '0');
		mon = (tmp[5] - '0') * 10 + (tmp[6] - '0');
		day = (tmp[8] - '0') * 10 + (tmp[9] - '0');
		hour = (tmp[11] - '0') * 10 + (tmp[12] - '0');
		min = (tmp[14] - '0') * 10 + (tmp[15] - '0');
		sec = (tmp[17] - '0') * 10 + (tmp[18] - '0');

		LOG_DBG("Parsed: %04d-%02d-%02d %02d:%02d:%02d", year, mon, day, hour, min, sec);
		matched = 6;
	} else {
		LOG_DBG("Format validation failed");
		matched = 0;
	}

	if (matched == 0) {
		year = 2024;
		mon = 1;
		day = 1;
		hour = 0;
		min = 0;
		sec = 0;
	}

	struct tm t = {0};
	t.tm_year = year - 1900;
	t.tm_mon = mon - 1;
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = min;
	t.tm_sec = sec;

	time_t epoch = timeutil_timegm(&t);

	return (uint64_t)epoch * 1000000000ULL;
}
