#ifndef __ZPOINT_H_
#define __ZPOINT_H_

#define POINT_TYPE_ATS_A "atsA"
#define POINT_TYPE_ATS_B "atsB"
#define POINT_TYPE_SNMP_SERVER "snmpServer"
#define POINT_TYPE_SWITCH "switch"
#define POINT_TYPE_TEST_LEDS   "testLEDs"

// Fan mode can be off, temp, tach, pwm
//   - temp: uses temperature to control the fan
//   - tach: uses fanSetSpeed as a tach set point which the controller tries to maintain
//   - pwm: users fanSetSpeed as a direct pwm set point 
#define POINT_TYPE_FAN_MODE "fanMode"    
#define POINT_TYPE_FAN_SET_SPEED "fanSetSpeed"
#define POINT_TYPE_FAN_SPEED "fanSpeed"

typedef enum {
  ATS_OFF,
  ATS_ON,
  ATS_ACTIVE,
  ATS_ERROR,
} ats_chan_state;

#endif // __ZPOINT_H_


