#ifndef FAN_H
#define FAN_H

/*
I2C address of the fan controller is 0x2e
I2C read i2c@40005400 0x2e 0x20
*/
#define TEMP_INPUT_SENSOR_ADDRESS 0x48
#define TEMP_OUTPUT_SENSOR_ADDRESS 0x49

// Temp sensor delta threshold to turn on fans
#define FAN_TEMP_THRESHOLD 10
#define FAN_TEMP_MAX       20
// Change which fan is running periodically
#define FAN_CHANGE_SECONDS 60

typedef enum fan_id {
	FAN_NONE = 0,
	FAN_1,
	FAN_2,
} fan_id_t;

typedef enum temp_sensor {
	TEMP_NONE = 0,
	TEMP_INPUT,
	TEMP_OUTPUT,
} temp_sensor_t;

#endif // FAN_H
