#ifndef __ZPOINT_H_
#define __ZPOINT_H_

// These defines should match ZPoint.elm

#define POINT_TYPE_ATS_A       "atsA"
#define POINT_TYPE_ATS_B       "atsB"
#define POINT_TYPE_SNMP_SERVER "snmpServer"
#define POINT_TYPE_SWITCH      "switch"
#define POINT_TYPE_TEST_LEDS   "testLEDs"

// Fan mode can be off, temp, tach, pwm
//   - off: sets fan off
//   - temp: uses temperature to control the fan
//   - tach: uses fanSetSpeed as a tach set point which the controller tries to maintain
//   - pwm: users fanSetSpeed as a direct pwm set point
#define POINT_TYPE_FAN_MODE "fanMode"
#define POINT_VALUE_OFF     "off"
#define POINT_VALUE_TEMP    "temp"
#define POINT_VALUE_TACH    "tach"
#define POINT_VALUE_PWM     "pwm"

#define POINT_TYPE_FAN_SET_SPEED "fanSetSpeed"
#define POINT_TYPE_FAN_SPEED     "fanSpeed"

#define POINT_TYPE_FAN_STATUS  "fanStatus"
#define POINT_VALUE_OK         "ok"
#define POINT_VALUE_STALL      "stall"
#define POINT_VALUE_SPIN_FAIL  "spinFail"
#define POINT_VALUE_DRIVE_FAIL "driveFail"

#define POINT_TYPE_IPN        "ipn"
#define POINT_TYPE_ID         "id"
#define POINT_TYPE_MFG_DATE   "mfgDate"
#define POINT_TYPE_VERSION_HW "versionHW"
#define POINT_TYPE_VERSION_FW "versionFW"

#endif // __ZPOINT_H_
