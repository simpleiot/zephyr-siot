
#include <ctype.h>
#include <zephyr/kernel.h>
#include <string.h>

#include "html.h"

char *z_strdup(const char *str)
{
	size_t len = strlen(str) + 1;
	char *new_str = k_malloc(len);
	if (new_str) {
		memcpy(new_str, str, len);
	}
	return new_str;
}

// Function to URL-decode form data (since form data is URL-encoded by default)
void url_decode(char *src, char *dst)
{
	char a, b;
	while (*src) {
		if ((*src == '%') && ((a = src[1]) && (b = src[2])) &&
		    (isxdigit(a) && isxdigit(b))) {
			if (a >= 'a') {
				a -= 'a' - 'A';
			}
			if (a >= 'A') {
				a -= ('A' - 10);
			} else {
				a -= '0';
			}
			if (b >= 'a') {
				b -= 'a' - 'A';
			}
			if (b >= 'A') {
				b -= ('A' - 10);
			} else {
				b -= '0';
			}
			*dst++ = 16 * a + b;
			src += 3;
		} else if (*src == '+') {
			*dst++ = ' '; // '+' is decoded as space
			src++;
		} else {
			*dst++ = *src++;
		}
	}
	*dst = '\0';
}

// Function to parse form data (key=value pairs separated by '&')
void html_parse_form_data(const char *body, html_form_callback callback)
{
	char *key_value, *key, *value, *decoded_value;
	char *form_data = z_strdup(body); // Duplicate the string as we'll modify it
	char *saveptr1, *saveptr2;

	key_value = strtok_r(form_data, "&", &saveptr1);

	bool ipstatic_found = false;

	while (key_value != NULL) {
		key = strtok_r(key_value, "=", &saveptr2); // Extract the key
		value = strtok_r(NULL, "=", &saveptr2);    // Extract the value

		if (key && value) {
			decoded_value = (char *)k_malloc(strlen(value) +
							 1); // Allocate space for decoded value
			url_decode(value,
				   decoded_value); // Decode the value (since it's URL-encoded)

			if (strcmp(key, "ipstatic") == 0) {
				ipstatic_found = true;
			}

			callback(key, decoded_value);

			k_free(decoded_value); // Free the decoded value after use
		}

		key_value = strtok_r(NULL, "&", &saveptr1); // Get the next key-value pair
	}

	if (!ipstatic_found) {
		callback("ipstatic", "off");
	}

	k_free(form_data); // Free the duplicated form data string
}
