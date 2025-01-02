#include "ats.h"

ats_led_state z_ats_get_led_state_a(ats *a)
{
	if (!a->aon && !a->ona) {
		return ATS_LED_OFF;
	} else if (a->aon && !a->ona) {
		return ATS_LED_ON;
	} else if (a->ona) {
		return ATS_LED_BLINK;
	}

	return ATS_LED_ERROR;
}

ats_led_state z_ats_get_led_state_b(ats *a)
{
	if (!a->bon && !a->onb) {
		return ATS_LED_OFF;
	} else if (a->bon && !a->onb) {
		return ATS_LED_ON;
	} else if (a->onb) {
		return ATS_LED_BLINK;
	}

	return ATS_LED_ERROR;
}
