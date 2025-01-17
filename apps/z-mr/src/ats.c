#include "ats.h"
#include "zpoint.h"

ats_chan_state z_ats_get_state_a(ats *a)
{
	if (!a->aon && !a->ona) {
		return ATS_OFF;
	} else if (a->aon && !a->ona) {
		return ATS_ON;
	} else if (a->ona) {
		return ATS_ACTIVE;
	}

	return ATS_ERROR;
}

ats_chan_state z_ats_get_state_b(ats *a)
{
	if (!a->bon && !a->onb) {
		return ATS_OFF;
	} else if (a->bon && !a->onb) {
		return ATS_ON;
	} else if (a->onb) {
		return ATS_ACTIVE;
	}

	return ATS_ERROR;
}
