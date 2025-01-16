#ifndef __ATS_H_
#define __ATS_H_

#include "zpoint.h"

#include <stdbool.h>

typedef struct {
	bool aon;
	bool bon;
	bool ona;
	bool onb;
} ats;

typedef struct {
	ats state[6];
} ats_state;

#define INIT_ATS_STATE() (ats_state) { .state = { [0 ... 5] = { .aon = false, .bon = false, .ona = false, .onb = false } } }

#define AON 0
#define ONA 1
#define BON 2
#define ONB 3

ats_chan_state z_ats_get_state_a(ats * a);
ats_chan_state z_ats_get_state_b(ats * a);

#endif
