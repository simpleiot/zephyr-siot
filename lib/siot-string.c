#include <siot-string.h>

void ftoa(float num, char *str, int precision)
{
	int i = 0;
	if (num < 0) {
		num = -num;
		str[i++] = '-';
	}

	int whole = (int)num;
	float fraction = num - whole;

	// Convert whole part
	if (whole == 0) {
		str[i++] = '0';
	} else {
		int temp = whole;
		while (temp > 0) {
			temp /= 10;
			i++;
		}
		int j = i - 1;
		temp = whole;
		while (temp > 0) {
			str[j--] = (temp % 10) + '0';
			temp /= 10;
		}
	}

	// Add decimal point
	if (precision > 0) {
		str[i++] = '.';

		// Convert fractional part
		while (precision > 0) {
			fraction *= 10;
			int digit = (int)fraction;
			str[i++] = digit + '0';
			fraction -= digit;
			precision--;
		}
	}

	str[i] = '\0';
}

void reverse(char *str, int length)
{
	int start = 0;
	int end = length - 1;
	while (start < end) {
		char temp = str[start];
		str[start] = str[end];
		str[end] = temp;
		start++;
		end--;
	}
}

char *itoa(int num, char *str, int base)
{
	int i = 0;
	int isNegative = 0;

	// Handle 0 explicitly
	if (num == 0) {
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	// Handle negative numbers for base 10
	if (num < 0 && base == 10) {
		isNegative = 1;
		num = -num;
	}

	// Process individual digits
	while (num != 0) {
		int rem = num % base;
		str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		num = num / base;
	}

	// Append negative sign for base 10
	if (isNegative) {
		str[i++] = '-';
	}

	str[i] = '\0';

	// Reverse the string
	reverse(str, i);

	return str;
}

int atoi(const char *str)
{
	int result = 0;
	int sign = 1;
	int i = 0;

	// Handle negative numbers
	if (str[0] == '-') {
		sign = -1;
		i++;
	}

	// Convert each digit
	while (str[i] != '\0') {
		if (str[i] >= '0' && str[i] <= '9') {
			result = result * 10 + (str[i] - '0');
		} else {
			break; // Stop at non-digit character
		}
		i++;
	}

	return sign * result;
}

float atof(const char *str)
{
	float result = 0.0f;
	float fraction = 0.1f;
	int sign = 1;
	int i = 0;
	int in_fraction = 0;

	// Handle negative numbers
	if (str[0] == '-') {
		sign = -1;
		i++;
	}

	// Convert digits
	while (str[i] != '\0') {
		if (str[i] >= '0' && str[i] <= '9') {
			if (!in_fraction) {
				result = result * 10.0f + (str[i] - '0');
			} else {
				result += (str[i] - '0') * fraction;
				fraction *= 0.1f;
			}
		} else if (str[i] == '.') {
			if (in_fraction) {
				break; // Multiple decimal points, invalid
			}
			in_fraction = 1;
		} else {
			break; // Stop at non-digit, non-decimal point character
		}
		i++;
	}

	return sign * result;
}
