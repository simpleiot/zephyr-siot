#ifndef __SIOT_STRING_H__
#define __SIOT_STRING_H__

void ftoa(float num, char *str, int precision);
char *itoa(int num, char *str, int base);
int atoi(const char *str);
float atof(const char *str);

#endif // __SIOT_STRING__
