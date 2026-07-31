#include <timeconv.h>

#include <string.h>
#include <time.h>
#include <zephyr/sys/timeutil.h>

#define NS_PER_SEC 1000000000ULL

// civil_from_days converts a day count since 1970-01-01 into a calendar date.
// This is Howard Hinnant's algorithm, valid for any proleptic Gregorian date.
//
// It is implemented here rather than calling gmtime() so the conversion has no
// libc dependency and no static buffer, which matters when it runs from a
// zbus subscriber thread.
static void civil_from_days(int64_t z, int *year, unsigned *month, unsigned *day)
{
	z += 719468;
	const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
	const unsigned doe = (unsigned)(z - era * 146097);              // [0, 146096]
	const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
	const int64_t y = (int64_t)yoe + era * 400;
	const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);   // [0, 365]
	const unsigned mp = (5 * doy + 2) / 153;                        // [0, 11]
	const unsigned d = doy - (153 * mp + 2) / 5 + 1;                // [1, 31]
	const unsigned m = mp < 10 ? mp + 3 : mp - 9;                   // [1, 12]

	*year = (int)(y + (m <= 2 ? 1 : 0));
	*month = m;
	*day = d;
}

int timeconv_rfc3339_from_epoch_ns_utc(uint64_t epoch_ns, char *buf, size_t buf_len)
{
	if (buf == NULL || buf_len < RFC3339_MAX_LEN) {
		return -1;
	}

	uint64_t secs = epoch_ns / NS_PER_SEC;
	uint32_t nsec = (uint32_t)(epoch_ns % NS_PER_SEC);

	int64_t days = (int64_t)(secs / 86400);
	uint32_t rem = (uint32_t)(secs % 86400);

	int year;
	unsigned month, day;
	civil_from_days(days, &year, &month, &day);

	unsigned hour = rem / 3600;
	unsigned min = (rem % 3600) / 60;
	unsigned sec = rem % 60;

	// snprintf with %09u has caused trouble on some picolibc builds, so
	// write the fixed width fields directly.
	char *p = buf;

	*p++ = '0' + (year / 1000) % 10;
	*p++ = '0' + (year / 100) % 10;
	*p++ = '0' + (year / 10) % 10;
	*p++ = '0' + year % 10;
	*p++ = '-';
	*p++ = '0' + (month / 10) % 10;
	*p++ = '0' + month % 10;
	*p++ = '-';
	*p++ = '0' + (day / 10) % 10;
	*p++ = '0' + day % 10;
	*p++ = 'T';
	*p++ = '0' + (hour / 10) % 10;
	*p++ = '0' + hour % 10;
	*p++ = ':';
	*p++ = '0' + (min / 10) % 10;
	*p++ = '0' + min % 10;
	*p++ = ':';
	*p++ = '0' + (sec / 10) % 10;
	*p++ = '0' + sec % 10;
	*p++ = '.';

	for (int i = 8; i >= 0; i--) {
		p[i] = '0' + (nsec % 10);
		nsec /= 10;
	}
	p += 9;

	*p++ = 'Z';
	*p = '\0';

	return 0;
}

// is_digit avoids a ctype dependency and the locale machinery behind it.
static inline bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}

uint64_t timeconv_epoch_ns_from_rfc3339(const char *buf, size_t buf_len)
{
	if (buf == NULL || buf_len == 0) {
		return 0ULL;
	}

	char tmp[RFC3339_MAX_LEN + 8];
	size_t cpy = buf_len < sizeof(tmp) - 1 ? buf_len : sizeof(tmp) - 1;
	memcpy(tmp, buf, cpy);
	tmp[cpy] = '\0';

	// minimum is "YYYY-MM-DDTHH:MM:SSZ"
	if (strlen(tmp) < 20) {
		return 0ULL;
	}

	if (tmp[4] != '-' || tmp[7] != '-' || tmp[10] != 'T' || tmp[13] != ':' || tmp[16] != ':') {
		return 0ULL;
	}

	for (int i = 0; i < 19; i++) {
		if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) {
			continue;
		}
		if (!is_digit(tmp[i])) {
			return 0ULL;
		}
	}

	int year = (tmp[0] - '0') * 1000 + (tmp[1] - '0') * 100 + (tmp[2] - '0') * 10 +
		   (tmp[3] - '0');
	int mon = (tmp[5] - '0') * 10 + (tmp[6] - '0');
	int mday = (tmp[8] - '0') * 10 + (tmp[9] - '0');
	int hour = (tmp[11] - '0') * 10 + (tmp[12] - '0');
	int min = (tmp[14] - '0') * 10 + (tmp[15] - '0');
	int sec = (tmp[17] - '0') * 10 + (tmp[18] - '0');

	// Reject out of range fields rather than letting timegm normalize them.
	// Month 13 would otherwise become January of the following year, turning
	// a garbled timestamp into a plausible one, and this value is what
	// identifies a point echo.
	if (mon < 1 || mon > 12 || mday < 1 || mday > 31 || hour > 23 || min > 59 || sec > 60) {
		return 0ULL;
	}

	struct tm t = {0};
	t.tm_year = year - 1900;
	t.tm_mon = mon - 1;
	t.tm_mday = mday;
	t.tm_hour = hour;
	t.tm_min = min;
	t.tm_sec = sec;

	uint32_t nsec = 0;
	size_t i = 19;

	if (tmp[i] == '.') {
		i++;
		// Scale whatever precision was sent up to nanoseconds. More than
		// nine digits is truncated rather than rejected.
		uint32_t scale = 100000000U;
		while (is_digit(tmp[i])) {
			if (scale > 0) {
				nsec += (uint32_t)(tmp[i] - '0') * scale;
				scale /= 10;
			}
			i++;
		}
	}

	// Only "Z" is accepted, and nothing may follow it. SIOT always sends
	// UTC, which keeps this parser small.
	if (tmp[i] != 'Z' || tmp[i + 1] != '\0') {
		return 0ULL;
	}

	int64_t epoch = timeutil_timegm64(&t);
	if (epoch < 0) {
		return 0ULL;
	}

	return (uint64_t)epoch * NS_PER_SEC + nsec;
}
