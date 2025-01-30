#ifndef __ZPOINT_H_
#define __ZPOINT_H_

#define POINT_TYPE_ATS_A "atsA"
#define POINT_TYPE_ATS_B "atsB"
#define POINT_TYPE_SNMP_SERVER "snmpServer"
#define POINT_TYPE_TEST_LEDS   "testLEDs"

// Fan mode can be off, auto, manual
// auto uses temperature to control the fan
#define POINT_TYPE_FAN_MODE "fanMode"    
// Fan speed setting is specified in % (0-100)
#define POINT_TYPE_FAN_SET_SPEED "fanSetSpeed"
#define POINT_TYPE_FAN_SPEED "fanSpeed"

typedef enum {
  ATS_OFF,
  ATS_ON,
  ATS_ACTIVE,
  ATS_ERROR,
} ats_chan_state;

#endif // __ZPOINT_H_


