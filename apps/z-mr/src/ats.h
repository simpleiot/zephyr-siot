#ifndef __ATS_H_
#define __ATS_H_

#include <stdbool.h>

typedef struct {
	bool aon;
	bool bon;
	bool ona;
	bool onb;
} ats;

#define AON 0
#define ONA 1
#define BON 2
#define ONB 3

#endif
